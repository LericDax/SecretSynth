#include <chrono>
#include <iomanip>
#include <iostream>

#include "../src/dsp/osc/PhaseWarpOscillator.h"

int main()
{
    using Clock = std::chrono::high_resolution_clock;
    using Osc = secretsynth::dsp::osc::PhaseWarpOscillator;

    constexpr int sampleRate = 48000;
    constexpr int warmupSamples = 20000;
    constexpr int benchmarkSamples = sampleRate * 8;
    constexpr int voices = 32;

    Osc osc;
    osc.prepare (sampleRate);
    osc.setFrequency (220.0f);
    osc.setTune (7.0f);
    osc.setFine (-12.0f);
    osc.setPdAmount (0.7f);
    osc.setPdShape (0.35f);
    osc.setMix (1.0f);

    std::cout << "Quality mode CPU benchmark (single oscillator scaled per voice)\n";
    std::cout << "voices=" << voices << ", samples=" << benchmarkSamples << "\n\n";

    for (const auto [name, mode] : { std::pair { "low", Osc::QualityMode::low },
                                     std::pair { "medium", Osc::QualityMode::medium },
                                     std::pair { "high", Osc::QualityMode::high } })
    {
        osc.setQualityMode (mode);
        osc.reset();

        volatile float sink = 0.0f;
        for (int i = 0; i < warmupSamples; ++i)
            sink += osc.renderSample();

        const auto start = Clock::now();
        for (int i = 0; i < benchmarkSamples; ++i)
            sink += osc.renderSample();
        const auto end = Clock::now();

        const auto elapsedNs = std::chrono::duration_cast<std::chrono::nanoseconds> (end - start).count();
        const auto nsPerSample = static_cast<double> (elapsedNs) / benchmarkSamples;
        const auto nsPerVoiceSample = nsPerSample / static_cast<double> (voices);
        const auto cpuPerVoicePercent = (nsPerVoiceSample * sampleRate * 100.0) / 1'000'000'000.0;

        std::cout << std::left << std::setw (8) << name
                  << " total=" << std::setw (10) << elapsedNs << " ns"
                  << " | ns/sample=" << std::setw (10) << std::fixed << std::setprecision (2) << nsPerSample
                  << " | ns/voice-sample=" << std::setw (10) << nsPerVoiceSample
                  << " | est% CPU/voice @48k=" << cpuPerVoicePercent << "\n";
    }

    return 0;
}
