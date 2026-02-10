#include "VoiceManager.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <tuple>

namespace secretsynth::dsp::voice
{
VoiceManager::VoiceManager()
{
    ensureVoiceCount();
}

VoiceManager::VoiceManager (Config newConfig)
    : config (newConfig)
{
    ensureVoiceCount();
}

void VoiceManager::setConfig (const Config& newConfig)
{
    config = newConfig;
    ensureVoiceCount();
}

void VoiceManager::prepare (double newSampleRate, int newBlockSize)
{
    if (newSampleRate > 0.0)
        sampleRate = newSampleRate;

    if (newBlockSize > 0)
        blockSize = newBlockSize;

    for (auto& voice : voices)
        voice.prepare (sampleRate, blockSize);
}

void VoiceManager::reset()
{
    heldNotes.clear();
    eventCounter = 1;
    for (auto& voice : voices)
        voice.reset();
}

void VoiceManager::noteOn (int midiNote, float velocity)
{
    pushHeldNote (midiNote, velocity);

    if (isMonophonicMode())
    {
        auto* monoVoice = voices.empty() ? nullptr : &voices.front();
        if (monoVoice == nullptr)
            return;

        const auto startPitch = (config.mode == Mode::legato && monoVoice->getState() != Voice::State::idle)
            ? monoVoice->getCurrentPitchHz()
            : 0.0f;

        const Voice::NoteEvent event { midiNote, velocity, eventCounter++, 0, 0.0f, 0.0f };
        monoVoice->startNote (event, startPitch, config.glideTimeSeconds, config.glideCurve, true);
        return;
    }

    const auto unisonCount = requiredUnisonCount();
    for (int unisonIndex = 0; unisonIndex < unisonCount; ++unisonIndex)
    {
        auto* target = findVoiceForNote (midiNote, unisonIndex);
        if (target == nullptr)
            target = findStealVoice();

        if (target == nullptr)
            continue;

        const auto center = static_cast<float> (unisonCount - 1) * 0.5f;
        const auto offset = static_cast<float> (unisonIndex) - center;
        const auto detune = (unisonCount > 1) ? offset * config.unisonDetuneCents : 0.0f;
        const auto spreadNorm = (unisonCount > 1) ? offset / center : 0.0f;
        const auto spreadPan = spreadNorm * config.unisonSpread;

        startNoteOnVoice (*target, midiNote, velocity, eventCounter++, unisonIndex, detune, spreadPan, true);
    }
}

void VoiceManager::noteOff (int midiNote)
{
    removeHeldNote (midiNote);

    if (isMonophonicMode())
    {
        auto* monoVoice = voices.empty() ? nullptr : &voices.front();
        if (monoVoice == nullptr || monoVoice->getState() == Voice::State::idle)
            return;

        const auto next = latestHeldNote();
        if (config.mode == Mode::legato && next.midiNote >= 0)
        {
            const Voice::NoteEvent event { next.midiNote, next.velocity, eventCounter++, 0, 0.0f, 0.0f };
            monoVoice->startNote (event, monoVoice->getCurrentPitchHz(), config.glideTimeSeconds, config.glideCurve, false);
            return;
        }

        monoVoice->startRelease (config.releaseTimeSeconds);
        return;
    }

    releaseVoicesForNote (midiNote);
}

void VoiceManager::allNotesOff()
{
    heldNotes.clear();
    for (auto& voice : voices)
        voice.startRelease (config.releaseTimeSeconds);
}

void VoiceManager::advance (int numSamples)
{
    for (auto& voice : voices)
        voice.advance (numSamples);
}

std::size_t VoiceManager::targetVoiceCount() const
{
    if (isMonophonicMode())
        return 1;

    return std::max<std::size_t> (1, config.maxVoices);
}

bool VoiceManager::isMonophonicMode() const
{
    return config.mode == Mode::mono || config.mode == Mode::legato;
}

int VoiceManager::requiredUnisonCount() const
{
    if (config.mode != Mode::unison)
        return 1;

    return std::max (1, config.unisonVoices);
}

void VoiceManager::ensureVoiceCount()
{
    voices.resize (targetVoiceCount());
    for (auto& voice : voices)
        voice.prepare (sampleRate, blockSize);
}

void VoiceManager::startNoteOnVoice (Voice& voice, int midiNote, float velocity, std::uint64_t newEventIndex, int unisonIndex, float detuneCents, float spreadPan, bool restartPhase)
{
    const auto startPitch = (config.glideTimeSeconds > 0.0f && voice.getState() != Voice::State::idle)
        ? voice.getCurrentPitchHz()
        : 0.0f;

    const Voice::NoteEvent event { midiNote, velocity, newEventIndex, unisonIndex, detuneCents, spreadPan };
    voice.startNote (event, startPitch, config.glideTimeSeconds, config.glideCurve, restartPhase);
}

Voice* VoiceManager::findVoiceForNote (int midiNote, int unisonIndex)
{
    auto pred = [midiNote, unisonIndex] (const auto& voice) {
        if (voice.getState() == Voice::State::idle)
            return false;

        if (voice.getMidiNote() != midiNote)
            return false;

        return unisonIndex < 0 || voice.getUnisonIndex() == unisonIndex;
    };

    const auto it = std::find_if (voices.begin(), voices.end(), pred);
    return it == voices.end() ? nullptr : &(*it);
}

Voice* VoiceManager::findStealVoice()
{
    auto best = voices.end();

    const auto score = [] (const Voice& voice) {
        int stateScore = 2;
        if (voice.getState() == Voice::State::idle)
            stateScore = 0;
        else if (voice.getState() == Voice::State::releasing)
            stateScore = 1;

        return std::tuple<int, std::uint64_t, int> { stateScore, voice.getStartEventIndex(), voice.getMidiNote() };
    };

    for (auto it = voices.begin(); it != voices.end(); ++it)
    {
        if (best == voices.end() || score (*it) < score (*best))
            best = it;
    }

    return best == voices.end() ? nullptr : &(*best);
}

void VoiceManager::releaseVoicesForNote (int midiNote)
{
    for (auto& voice : voices)
    {
        if (voice.getMidiNote() == midiNote && voice.isKeyHeld())
            voice.startRelease (config.releaseTimeSeconds);
    }
}

void VoiceManager::pushHeldNote (int midiNote, float velocity)
{
    removeHeldNote (midiNote);
    heldNotes.push_back ({ midiNote, velocity, eventCounter });
}

void VoiceManager::removeHeldNote (int midiNote)
{
    heldNotes.erase (std::remove_if (heldNotes.begin(), heldNotes.end(), [midiNote] (const HeldNote& note) {
        return note.midiNote == midiNote;
    }), heldNotes.end());
}

VoiceManager::HeldNote VoiceManager::latestHeldNote() const
{
    if (heldNotes.empty())
        return {};

    return heldNotes.back();
}
} // namespace secretsynth::dsp::voice
