#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace secretsynth::dsp::mod
{
class AdsrEnvelope
{
public:
    struct Parameters
    {
        float attackSeconds { 0.01f };
        float decaySeconds { 0.1f };
        float sustainLevel { 0.8f };
        float releaseSeconds { 0.2f };
    };

    void setSampleRate (double newSampleRate) noexcept;
    void setParameters (const Parameters& newParameters) noexcept;
    void noteOn() noexcept;
    void noteOff() noexcept;
    void reset() noexcept;
    float processSample() noexcept;

    [[nodiscard]] float getCurrentValue() const noexcept { return currentValue; }

private:
    enum class Stage
    {
        idle,
        attack,
        decay,
        sustain,
        release
    };

    double sampleRate { 44100.0 };
    Parameters parameters {};
    Stage stage { Stage::idle };
    float currentValue { 0.0f };
    float releaseStartValue { 0.0f };
    std::uint32_t stageSamplesProcessed { 0 };
};

class Lfo
{
public:
    enum class Waveform
    {
        sine,
        triangle
    };

    enum class RateMode
    {
        hertz,
        tempoSync
    };

    enum class SyncDivision
    {
        whole,
        half,
        quarter,
        eighth,
        sixteenth,
        eighthTriplet
    };

    void setSampleRate (double newSampleRate) noexcept;
    void setWaveform (Waveform newWaveform) noexcept;
    void setRateHz (float newRateHz) noexcept;
    void setRateMode (RateMode newMode) noexcept;
    void setTempoBpm (float newTempoBpm) noexcept;
    void setSyncDivision (SyncDivision newDivision) noexcept;
    void reset() noexcept;

    float processSample() noexcept;

    [[nodiscard]] float getCurrentValue() const noexcept { return currentValue; }
    [[nodiscard]] float getCurrentFrequencyHz() const noexcept;

private:
    static float syncDivisionToCyclesPerBeat (SyncDivision division) noexcept;

    double sampleRate { 44100.0 };
    Waveform waveform { Waveform::sine };
    RateMode rateMode { RateMode::hertz };
    SyncDivision syncDivision { SyncDivision::quarter };
    float rateHz { 2.0f };
    float tempoBpm { 120.0f };
    float phase { 0.0f };
    float currentValue { 0.0f };
};

enum class Source
{
    ampEnv,
    modEnv,
    lfo1,
    lfo2,
    velocity,
    keyTrack,
    count
};

enum class Destination
{
    pitch,
    pdAmount,
    filterCutoff,
    amp,
    count
};

struct Route
{
    Source source { Source::lfo1 };
    Destination destination { Destination::pitch };
    float depth { 0.0f };
    bool bipolar { true };
};

class DestinationSmoother
{
public:
    void setSampleRate (double newSampleRate) noexcept;
    void setSmoothingTimeSeconds (float newTimeSeconds) noexcept;
    void reset (float initialValue = 0.0f) noexcept;
    float processSample (float targetValue) noexcept;

private:
    void updateCoefficient() noexcept;

    double sampleRate { 44100.0 };
    float smoothingTimeSeconds { 0.01f };
    float coefficient { 0.0f };
    float currentValue { 0.0f };
};

class ModulationMatrix
{
public:
    static constexpr std::uint32_t schemaVersion = 1;

    void setSampleRate (double newSampleRate) noexcept;
    void setDestinationSmoothingTimeSeconds (float timeSeconds) noexcept;

    void clearRoutes() noexcept;
    void addRoute (const Route& route);
    [[nodiscard]] const std::vector<Route>& getRoutes() const noexcept { return routes; }

    [[nodiscard]] std::array<float, static_cast<std::size_t> (Destination::count)> process (
        const std::array<float, static_cast<std::size_t> (Source::count)>& sourceValues) noexcept;

    [[nodiscard]] std::string serialize() const;
    bool deserialize (std::string_view text);

private:
    static std::size_t toIndex (Source source) noexcept { return static_cast<std::size_t> (source); }
    static std::size_t toIndex (Destination destination) noexcept { return static_cast<std::size_t> (destination); }

    std::vector<Route> routes;
    std::array<DestinationSmoother, static_cast<std::size_t> (Destination::count)> smoothers;
};

class ModulationEngine
{
public:
    void setSampleRate (double newSampleRate) noexcept;
    void reset() noexcept;

    AdsrEnvelope ampEnv;
    AdsrEnvelope modEnv;
    Lfo lfo1;
    Lfo lfo2;
};
} // namespace secretsynth::dsp::mod
