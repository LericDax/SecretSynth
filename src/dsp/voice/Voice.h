#pragma once

#include <cstdint>

namespace secretsynth::dsp::voice
{
enum class GlideCurve
{
    linear,
    exponential
};

class Voice
{
public:
    enum class State
    {
        idle,
        active,
        releasing
    };

    struct NoteEvent
    {
        int midiNote { -1 };
        float velocity { 0.0f };
        std::uint64_t eventIndex { 0 };
        int unisonIndex { 0 };
        float detuneCents { 0.0f };
        float spreadPan { 0.0f };
    };

    Voice() = default;

    void reset() noexcept;
    void prepare (double newSampleRate, int newBlockSize) noexcept;

    void startNote (const NoteEvent& event, float startPitchHz, float glideTimeSeconds, GlideCurve curve, bool restartPhase) noexcept;
    void startRelease (float releaseTimeSeconds) noexcept;
    void forceIdle() noexcept;
    void advance (int numSamples) noexcept;

    [[nodiscard]] int getMidiNote() const noexcept { return note.midiNote; }
    [[nodiscard]] float getVelocity() const noexcept { return note.velocity; }
    [[nodiscard]] State getState() const noexcept { return state; }
    [[nodiscard]] bool isKeyHeld() const noexcept { return keyHeld; }
    [[nodiscard]] std::uint64_t getStartEventIndex() const noexcept { return note.eventIndex; }
    [[nodiscard]] float getCurrentPitchHz() const noexcept { return currentPitchHz; }
    [[nodiscard]] float getTargetPitchHz() const noexcept { return targetPitchHz; }
    [[nodiscard]] int getUnisonIndex() const noexcept { return note.unisonIndex; }
    [[nodiscard]] float getDetuneCents() const noexcept { return note.detuneCents; }
    [[nodiscard]] float getSpreadPan() const noexcept { return note.spreadPan; }

private:
    static float midiNoteToFrequency (int midiNote) noexcept;

    NoteEvent note {};
    State state { State::idle };
    double sampleRate { 44100.0 };
    int blockSize { 0 };
    bool keyHeld { false };

    float currentPitchHz { 0.0f };
    float targetPitchHz { 0.0f };
    float glideProgress { 1.0f };
    float glideDurationSamples { 0.0f };
    GlideCurve glideCurve { GlideCurve::linear };

    int releaseSamplesRemaining { 0 };
};
} // namespace secretsynth::dsp::voice
