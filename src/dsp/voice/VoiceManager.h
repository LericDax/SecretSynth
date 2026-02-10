#pragma once

#include "Voice.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace secretsynth::dsp::voice
{
class VoiceManager
{
public:
    enum class Mode
    {
        mono,
        poly,
        legato,
        unison
    };

    struct Config
    {
        Mode mode { Mode::poly };
        std::size_t maxVoices { 8 };
        int unisonVoices { 2 };
        float unisonDetuneCents { 8.0f };
        float unisonSpread { 1.0f };
        float releaseTimeSeconds { 0.1f };
        float glideTimeSeconds { 0.0f };
        GlideCurve glideCurve { GlideCurve::linear };
    };

    VoiceManager();
    explicit VoiceManager (Config newConfig);

    void setConfig (const Config& newConfig);
    [[nodiscard]] const Config& getConfig() const noexcept { return config; }

    void prepare (double newSampleRate, int newBlockSize);
    void reset();

    void noteOn (int midiNote, float velocity);
    void noteOff (int midiNote);
    void allNotesOff();

    void advance (int numSamples);

    [[nodiscard]] const std::vector<Voice>& getVoices() const noexcept { return voices; }
    [[nodiscard]] std::vector<Voice>& getVoices() noexcept { return voices; }

private:
    struct HeldNote
    {
        int midiNote { -1 };
        float velocity { 0.0f };
        std::uint64_t eventIndex { 0 };
    };

    [[nodiscard]] std::size_t targetVoiceCount() const;
    [[nodiscard]] bool isMonophonicMode() const;
    [[nodiscard]] int requiredUnisonCount() const;

    void ensureVoiceCount();
    void startNoteOnVoice (Voice& voice, int midiNote, float velocity, std::uint64_t eventIndex, int unisonIndex, float detuneCents, float spreadPan, bool restartPhase);
    Voice* findVoiceForNote (int midiNote, int unisonIndex = -1);
    Voice* findStealVoice();
    void releaseVoicesForNote (int midiNote);

    void pushHeldNote (int midiNote, float velocity);
    void removeHeldNote (int midiNote);
    [[nodiscard]] HeldNote latestHeldNote() const;

    Config config {};
    std::vector<Voice> voices;
    std::vector<HeldNote> heldNotes;
    std::uint64_t eventCounter { 1 };
    double sampleRate { 44100.0 };
    int blockSize { 0 };
};
} // namespace secretsynth::dsp::voice
