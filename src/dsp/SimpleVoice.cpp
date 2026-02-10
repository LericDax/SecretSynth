#include "SimpleVoice.h"

namespace secretsynth::dsp
{
void SimpleVoice::prepare (double newSampleRate) noexcept
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;
}

void SimpleVoice::setFrequency (float newFrequencyHz) noexcept
{
    const auto clampedFrequency = std::fmax (0.0f, newFrequencyHz);
    phaseIncrement = static_cast<float> (twoPi * clampedFrequency / static_cast<float> (sampleRate));
}

float SimpleVoice::renderSample() noexcept
{
    const auto sample = std::sin (phase);
    phase += phaseIncrement;

    if (phase >= twoPi)
        phase -= twoPi;

    return sample;
}

void SimpleVoice::reset() noexcept
{
    phase = 0.0f;
}
} // namespace secretsynth::dsp
