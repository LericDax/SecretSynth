#pragma once

#include <cmath>

namespace secretsynth::dsp
{
class SimpleVoice
{
public:
    void prepare (double newSampleRate) noexcept;
    void setFrequency (float newFrequencyHz) noexcept;
    float renderSample() noexcept;
    void reset() noexcept;

private:
    static constexpr float twoPi = 6.28318530717958647692f;

    double sampleRate { 44100.0 };
    float phaseIncrement { 0.0f };
    float phase { 0.0f };
};
} // namespace secretsynth::dsp
