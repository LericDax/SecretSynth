#include "PhaseWarpOscillator.h"

namespace secretsynth::dsp::osc
{
void PhaseWarpOscillator::prepare (double newSampleRate) noexcept
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;
}

void PhaseWarpOscillator::reset (float newPhase) noexcept
{
    phase01 = wrap01 (newPhase);
}

void PhaseWarpOscillator::setFrequency (float newFrequencyHz) noexcept
{
    baseFrequencyHz = std::max (0.0f, newFrequencyHz);
}

void PhaseWarpOscillator::setTune (float semitones) noexcept
{
    tuneSemitones = std::clamp (semitones, -48.0f, 48.0f);
}

void PhaseWarpOscillator::setFine (float cents) noexcept
{
    fineCents = std::clamp (cents, -100.0f, 100.0f);
}

void PhaseWarpOscillator::setPdAmount (float amount) noexcept
{
    pdAmount = std::clamp (amount, 0.0f, 1.0f);
}

void PhaseWarpOscillator::setPdShape (float shape) noexcept
{
    pdShape = std::clamp (shape, 0.0f, 1.0f);
}

void PhaseWarpOscillator::setMix (float amount) noexcept
{
    mix = std::clamp (amount, 0.0f, 1.0f);
}

void PhaseWarpOscillator::setQualityMode (QualityMode mode) noexcept
{
    qualityMode = mode;
}

float PhaseWarpOscillator::getFrequencyHz() const noexcept
{
    return computeEffectiveFrequency();
}

float PhaseWarpOscillator::renderSample() noexcept
{
    const auto oversample = getOversampleFactor();
    const auto frequency = computeEffectiveFrequency();
    const auto phaseStep = frequency / static_cast<float> (sampleRate * oversample);

    auto accumulated = 0.0f;
    for (int i = 0; i < oversample; ++i)
    {
        const auto warpedLinear = warpPiecewiseLinear (phase01);
        const auto warpedCurved = warpCurved (phase01);
        const auto warped = warpedLinear + (warpedCurved - warpedLinear) * pdShape;

        const auto dry = std::sin (twoPi * phase01);
        const auto wet = std::sin (twoPi * warped);
        accumulated += dry + (wet - dry) * mix;

        phase01 = wrap01 (phase01 + phaseStep);
    }

    return accumulated / static_cast<float> (oversample);
}

float PhaseWarpOscillator::computeEffectiveFrequency() const noexcept
{
    const auto semitoneOffset = tuneSemitones + fineCents * 0.01f;
    const auto ratio = std::pow (2.0f, semitoneOffset / 12.0f);
    return baseFrequencyHz * ratio;
}

int PhaseWarpOscillator::getOversampleFactor() const noexcept
{
    switch (qualityMode)
    {
        case QualityMode::medium: return 2;
        case QualityMode::high: return 4;
        case QualityMode::low:
        default: return 1;
    }
}

float PhaseWarpOscillator::warpPiecewiseLinear (float phase) const noexcept
{
    const auto center = std::clamp (0.5f + (pdAmount - 0.5f) * 0.9f, 0.05f, 0.95f);

    if (phase < center)
        return 0.5f * (phase / center);

    return 0.5f + 0.5f * ((phase - center) / (1.0f - center));
}

float PhaseWarpOscillator::warpCurved (float phase) const noexcept
{
    const auto minShape = 0.2f;
    const auto maxShape = 5.0f;
    const auto curvature = minShape + (maxShape - minShape) * pdAmount;
    const auto skew = 0.5f + 0.5f * std::sin ((pdShape * 2.0f - 1.0f) * 1.57079632679f);
    const auto exponentA = 1.0f + (curvature - 1.0f) * skew;
    const auto exponentB = 1.0f + (curvature - 1.0f) * (1.0f - skew);

    if (phase < 0.5f)
        return 0.5f * std::pow (phase * 2.0f, exponentA);

    return 1.0f - 0.5f * std::pow ((1.0f - phase) * 2.0f, exponentB);
}

float PhaseWarpOscillator::wrap01 (float value) const noexcept
{
    value -= std::floor (value);
    if (value >= 1.0f)
        value -= 1.0f;
    return value;
}
} // namespace secretsynth::dsp::osc
