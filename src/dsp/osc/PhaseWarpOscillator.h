#pragma once

#include <algorithm>
#include <cmath>

namespace secretsynth::dsp::osc
{
class PhaseWarpOscillator
{
public:
    enum class QualityMode
    {
        low,
        medium,
        high
    };

    void prepare (double newSampleRate) noexcept;
    void reset (float newPhase = 0.0f) noexcept;

    void setFrequency (float newFrequencyHz) noexcept;
    void setTune (float semitones) noexcept;
    void setFine (float cents) noexcept;
    void setPdAmount (float amount) noexcept;
    void setPdShape (float shape) noexcept;
    void setMix (float amount) noexcept;
    void setQualityMode (QualityMode mode) noexcept;

    [[nodiscard]] float getFrequencyHz() const noexcept;
    [[nodiscard]] float renderSample() noexcept;

private:
    static constexpr float twoPi = 6.28318530717958647692f;

    [[nodiscard]] float computeEffectiveFrequency() const noexcept;
    [[nodiscard]] int getOversampleFactor() const noexcept;
    [[nodiscard]] float warpPiecewiseLinear (float phase01) const noexcept;
    [[nodiscard]] float warpCurved (float phase01) const noexcept;
    [[nodiscard]] float wrap01 (float value) const noexcept;

    double sampleRate { 44100.0 };

    float baseFrequencyHz { 220.0f };
    float tuneSemitones { 0.0f };
    float fineCents { 0.0f };
    float pdAmount { 0.0f };
    float pdShape { 0.0f };
    float mix { 1.0f };

    float phase01 { 0.0f };
    QualityMode qualityMode { QualityMode::low };
};
} // namespace secretsynth::dsp::osc
