#include "MultiModeFilter.h"

#include <algorithm>
#include <limits>

namespace secretsynth::dsp::filter
{
void MultiModeFilter::prepare (double newSampleRate) noexcept
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;
}

void MultiModeFilter::reset() noexcept
{
    ic1eq = 0.0f;
    ic2eq = 0.0f;
}

void MultiModeFilter::setMode (Mode newMode) noexcept
{
    mode = newMode;
}

void MultiModeFilter::setCutoffHz (float newCutoffHz) noexcept
{
    cutoffHz = std::max (minCutoffHz, newCutoffHz);
}

void MultiModeFilter::setResonance (float newResonance) noexcept
{
    resonance = std::clamp (newResonance, 0.0f, 0.99f);
}

void MultiModeFilter::setKeyTracking (float newKeyTracking) noexcept
{
    keyTracking = std::clamp (newKeyTracking, 0.0f, 1.0f);
}

void MultiModeFilter::setKeyTrackingReferenceHz (float newReferenceHz) noexcept
{
    keyTrackingReferenceHz = std::max (newReferenceHz, 1.0f);
}

float MultiModeFilter::processSample (float input, float keyFrequencyHz) noexcept
{
    constexpr float pi = 3.14159265358979323846f;

    const auto safeKeyFrequency = std::max (1.0f, keyFrequencyHz);
    const auto keyRatio = safeKeyFrequency / keyTrackingReferenceHz;
    const auto trackedCutoff = cutoffHz * std::pow (keyRatio, keyTracking);
    const auto nyquistLimitedCutoff = std::min (trackedCutoff, static_cast<float> (0.49 * sampleRate));
    const auto g = std::tan (pi * nyquistLimitedCutoff / static_cast<float> (sampleRate));
    const auto k = 2.0f - 1.99f * resonance;

    const auto a1 = 1.0f / (1.0f + g * (g + k));
    const auto a2 = g * a1;
    const auto a3 = g * a2;

    const auto v3 = input - ic2eq;
    const auto v1 = a1 * ic1eq + a2 * v3;
    const auto v2 = ic2eq + a2 * ic1eq + a3 * v3;

    ic1eq = flushDenormal (2.0f * v1 - ic1eq);
    ic2eq = flushDenormal (2.0f * v2 - ic2eq);

    switch (mode)
    {
        case Mode::lowPass:
            return flushDenormal (v2);
        case Mode::bandPass:
            return flushDenormal (v1);
        case Mode::highPass:
            return flushDenormal (input - k * v1 - v2);
    }

    return 0.0f;
}

float MultiModeFilter::flushDenormal (float value) noexcept
{
    if (! std::isfinite (value) || std::abs (value) < std::numeric_limits<float>::min())
        return 0.0f;

    return value;
}
} // namespace secretsynth::dsp::filter
