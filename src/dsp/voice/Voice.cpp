#include "Voice.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace secretsynth::dsp::voice
{
namespace
{
constexpr float a4Frequency = 440.0f;
constexpr int a4MidiNote = 69;
constexpr float semitonesPerOctave = 12.0f;
} // namespace

void Voice::reset() noexcept
{
    note = {};
    state = State::idle;
    keyHeld = false;
    currentPitchHz = 0.0f;
    targetPitchHz = 0.0f;
    glideProgress = 1.0f;
    glideDurationSamples = 0.0f;
    releaseSamplesRemaining = 0;
}

void Voice::prepare (double newSampleRate, int newBlockSize) noexcept
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;

    if (newBlockSize > 0)
        blockSize = newBlockSize;
}

void Voice::startNote (const NoteEvent& event, float startPitchHz, float glideTimeSeconds, GlideCurve curve, bool restartPhase) noexcept
{
    (void) restartPhase;

    note = event;
    state = State::active;
    keyHeld = true;
    targetPitchHz = midiNoteToFrequency (event.midiNote) * std::pow (2.0f, event.detuneCents / 1200.0f);

    const auto glideSamples = static_cast<float> (std::max (0.0f, glideTimeSeconds) * sampleRate);
    glideCurve = curve;

    if (glideSamples <= 1.0f || startPitchHz <= std::numeric_limits<float>::epsilon())
    {
        currentPitchHz = targetPitchHz;
        glideProgress = 1.0f;
        glideDurationSamples = 0.0f;
    }
    else
    {
        currentPitchHz = startPitchHz;
        glideProgress = 0.0f;
        glideDurationSamples = glideSamples;
    }

    releaseSamplesRemaining = 0;
}

void Voice::startRelease (float releaseTimeSeconds) noexcept
{
    keyHeld = false;

    if (state == State::idle)
        return;

    const auto clamped = std::max (0.0f, releaseTimeSeconds);
    releaseSamplesRemaining = static_cast<int> (std::round (clamped * static_cast<float> (sampleRate)));

    if (releaseSamplesRemaining <= 0)
    {
        forceIdle();
        return;
    }

    state = State::releasing;
}

void Voice::forceIdle() noexcept
{
    reset();
}

void Voice::advance (int numSamples) noexcept
{
    if (state == State::idle)
        return;

    if (glideProgress < 1.0f && glideDurationSamples > 0.0f)
    {
        glideProgress = std::min (1.0f, glideProgress + static_cast<float> (numSamples) / glideDurationSamples);

        if (glideCurve == GlideCurve::linear)
        {
            currentPitchHz += (targetPitchHz - currentPitchHz) * glideProgress;
        }
        else
        {
            const auto ratio = std::max (0.000001f, targetPitchHz / std::max (0.000001f, currentPitchHz));
            currentPitchHz *= std::pow (ratio, glideProgress);
        }

        if (glideProgress >= 1.0f)
            currentPitchHz = targetPitchHz;
    }
    else
    {
        currentPitchHz = targetPitchHz;
    }

    if (state == State::releasing)
    {
        releaseSamplesRemaining -= numSamples;
        if (releaseSamplesRemaining <= 0)
            forceIdle();
    }
}

float Voice::midiNoteToFrequency (int midiNote) noexcept
{
    return a4Frequency * std::pow (2.0f, (static_cast<float> (midiNote) - a4MidiNote) / semitonesPerOctave);
}
} // namespace secretsynth::dsp::voice
