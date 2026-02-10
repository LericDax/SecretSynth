#pragma once

#include <cmath>

namespace secretsynth::dsp::filter
{
class MultiModeFilter
{
public:
    enum class Mode
    {
        lowPass,
        bandPass,
        highPass
    };

    void prepare (double newSampleRate) noexcept;
    void reset() noexcept;

    void setMode (Mode newMode) noexcept;
    void setCutoffHz (float newCutoffHz) noexcept;
    void setResonance (float newResonance) noexcept;
    void setKeyTracking (float newKeyTracking) noexcept;
    void setKeyTrackingReferenceHz (float newReferenceHz) noexcept;

    float processSample (float input, float keyFrequencyHz) noexcept;

private:
    static constexpr float minCutoffHz = 20.0f;

    [[nodiscard]] static float flushDenormal (float value) noexcept;

    double sampleRate { 44100.0 };
    Mode mode { Mode::lowPass };
    float cutoffHz { 1000.0f };
    float resonance { 0.1f };
    float keyTracking { 0.0f };
    float keyTrackingReferenceHz { 440.0f };

    float ic1eq { 0.0f };
    float ic2eq { 0.0f };
};
} // namespace secretsynth::dsp::filter
