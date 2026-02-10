#include <array>
#include <cmath>
#include <iostream>

#include "../../src/dsp/osc/PhaseWarpOscillator.h"

namespace
{
using Osc = secretsynth::dsp::osc::PhaseWarpOscillator;

float estimateFrequency (Osc& osc, int sampleRate, int totalSamples)
{
    auto prev = osc.renderSample();
    auto crossings = 0;

    for (int i = 1; i < totalSamples; ++i)
    {
        const auto current = osc.renderSample();
        if (prev <= 0.0f && current > 0.0f)
            ++crossings;

        prev = current;
    }

    const auto seconds = static_cast<float> (totalSamples) / static_cast<float> (sampleRate);
    return static_cast<float> (crossings) / seconds;
}

bool testFrequencyStability()
{
    constexpr int sampleRate = 48000;
    constexpr int testSamples = sampleRate * 2;
    constexpr std::array<float, 4> frequencies { 110.0f, 220.0f, 440.0f, 880.0f };

    Osc osc;
    osc.prepare (sampleRate);
    osc.setMix (1.0f);
    osc.setPdAmount (0.65f);
    osc.setPdShape (0.5f);
    osc.setQualityMode (Osc::QualityMode::high);

    for (const auto frequency : frequencies)
    {
        osc.reset();
        osc.setFrequency (frequency);
        const auto measured = estimateFrequency (osc, sampleRate, testSamples);
        const auto error = std::abs (measured - frequency);
        if (error > 0.8f)
        {
            std::cerr << "Frequency stability failed for " << frequency << " Hz, measured " << measured << '\n';
            return false;
        }
    }

    return true;
}

bool testSlowPdModulationHasNoClicks()
{
    constexpr int sampleRate = 48000;
    constexpr int totalSamples = sampleRate * 2;

    Osc osc;
    osc.prepare (sampleRate);
    osc.reset();
    osc.setFrequency (330.0f);
    osc.setMix (1.0f);
    osc.setTune (0.0f);
    osc.setFine (0.0f);
    osc.setQualityMode (Osc::QualityMode::high);

    auto prev = osc.renderSample();
    auto maxDelta = 0.0f;

    for (int i = 1; i < totalSamples; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (totalSamples - 1);
        osc.setPdAmount (t);
        osc.setPdShape (0.5f + 0.5f * std::sin (6.28318530718f * t * 0.5f));

        const auto sample = osc.renderSample();
        maxDelta = std::max (maxDelta, std::abs (sample - prev));
        prev = sample;
    }

    if (maxDelta > 0.35f)
    {
        std::cerr << "Detected discontinuity-like jump during slow PD modulation, max delta = " << maxDelta << '\n';
        return false;
    }

    return true;
}

bool testBoundedOutputAtExtremeSettings()
{
    constexpr int sampleRate = 48000;

    Osc osc;
    osc.prepare (sampleRate);
    osc.reset();
    osc.setFrequency (12000.0f);
    osc.setTune (24.0f);
    osc.setFine (100.0f);
    osc.setPdAmount (1.0f);
    osc.setPdShape (1.0f);
    osc.setMix (1.0f);

    for (const auto mode : { Osc::QualityMode::low, Osc::QualityMode::medium, Osc::QualityMode::high })
    {
        osc.setQualityMode (mode);
        osc.reset();
        for (int i = 0; i < sampleRate; ++i)
        {
            const auto sample = osc.renderSample();
            if (! std::isfinite (sample) || std::abs (sample) > 1.05f)
            {
                std::cerr << "Bounded output test failed, sample " << sample << " at i=" << i << '\n';
                return false;
            }
        }
    }

    return true;
}
} // namespace

int main()
{
    if (! testFrequencyStability())
        return 1;

    if (! testSlowPdModulationHasNoClicks())
        return 1;

    if (! testBoundedOutputAtExtremeSettings())
        return 1;

    std::cout << "PhaseWarpOscillator regression tests passed\n";
    return 0;
}
