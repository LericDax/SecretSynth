#include <iostream>
#include <vector>

#include "../../src/dsp/voice/VoiceManager.h"

namespace
{
using secretsynth::dsp::voice::Voice;
using secretsynth::dsp::voice::VoiceManager;

bool testPolyAllocationAndReleaseTail()
{
    VoiceManager manager ({
        .mode = VoiceManager::Mode::poly,
        .maxVoices = 4,
        .releaseTimeSeconds = 0.01f,
    });

    manager.prepare (48000.0, 64);
    manager.noteOn (60, 0.7f);
    manager.noteOn (64, 0.8f);

    auto activeCount = 0;
    for (const auto& voice : manager.getVoices())
        if (voice.getState() == Voice::State::active)
            ++activeCount;

    if (activeCount != 2)
    {
        std::cerr << "Expected 2 active voices, got " << activeCount << '\n';
        return false;
    }

    manager.noteOff (60);
    auto releasingCount = 0;
    for (const auto& voice : manager.getVoices())
        if (voice.getState() == Voice::State::releasing)
            ++releasingCount;

    if (releasingCount != 1)
    {
        std::cerr << "Expected 1 releasing voice after note-off, got " << releasingCount << '\n';
        return false;
    }

    manager.advance (512);
    manager.advance (512);

    auto note60Exists = false;
    for (const auto& voice : manager.getVoices())
        if (voice.getMidiNote() == 60)
            note60Exists = true;

    if (note60Exists)
    {
        std::cerr << "Released note remained active too long\n";
        return false;
    }

    return true;
}

bool testNoStuckNotesOnRepeatedOffs()
{
    VoiceManager manager ({
        .mode = VoiceManager::Mode::poly,
        .maxVoices = 3,
        .releaseTimeSeconds = 0.0f,
    });

    manager.prepare (44100.0, 128);
    manager.noteOn (60, 1.0f);
    manager.noteOn (67, 1.0f);
    manager.noteOff (60);
    manager.noteOff (60);
    manager.noteOff (80);
    manager.noteOff (67);
    manager.allNotesOff();
    manager.advance (64);

    for (const auto& voice : manager.getVoices())
    {
        if (voice.getState() != Voice::State::idle)
        {
            std::cerr << "Voice remained non-idle after all note-offs\n";
            return false;
        }
    }

    return true;
}

bool testStealPriorityIsDeterministic()
{
    VoiceManager manager ({
        .mode = VoiceManager::Mode::poly,
        .maxVoices = 2,
        .releaseTimeSeconds = 0.2f,
    });

    manager.prepare (48000.0, 64);
    manager.noteOn (60, 1.0f); // oldest
    manager.noteOn (62, 1.0f);
    manager.noteOn (65, 1.0f); // should steal note 60 consistently

    auto has60 = false;
    auto has62 = false;
    auto has65 = false;

    for (const auto& voice : manager.getVoices())
    {
        has60 = has60 || voice.getMidiNote() == 60;
        has62 = has62 || voice.getMidiNote() == 62;
        has65 = has65 || voice.getMidiNote() == 65;
    }

    if (has60 || !has62 || !has65)
    {
        std::cerr << "Steal priority mismatch: expected notes 62 + 65\n";
        return false;
    }

    manager.noteOn (67, 1.0f);

    auto hasNew67 = false;
    has62 = false;
    has65 = false;
    for (const auto& voice : manager.getVoices())
    {
        has62 = has62 || voice.getMidiNote() == 62;
        has65 = has65 || voice.getMidiNote() == 65;
        hasNew67 = hasNew67 || voice.getMidiNote() == 67;
    }

    if (!has65 || !hasNew67 || has62)
    {
        std::cerr << "Second deterministic steal mismatch: expected notes 65 + 67\n";
        return false;
    }

    return true;
}

bool testLegatoAndUnisonAndPrepareUpdates()
{
    VoiceManager legato ({
        .mode = VoiceManager::Mode::legato,
        .maxVoices = 8,
        .glideTimeSeconds = 0.1f,
        .glideCurve = secretsynth::dsp::voice::GlideCurve::linear,
    });

    legato.prepare (48000.0, 64);
    legato.noteOn (60, 0.8f);
    legato.noteOn (67, 0.8f);

    const auto& monoVoice = legato.getVoices().front();
    if (monoVoice.getMidiNote() != 67)
    {
        std::cerr << "Legato mode failed to retarget mono voice\n";
        return false;
    }

    legato.noteOff (67);
    if (legato.getVoices().front().getMidiNote() != 60)
    {
        std::cerr << "Legato mode failed to return to held note\n";
        return false;
    }

    VoiceManager unison ({
        .mode = VoiceManager::Mode::unison,
        .maxVoices = 6,
        .unisonVoices = 3,
        .unisonDetuneCents = 12.0f,
        .unisonSpread = 1.0f,
    });

    unison.prepare (44100.0, 128);
    unison.noteOn (69, 1.0f);

    auto activeUnison = 0;
    std::vector<float> detunes;
    for (const auto& voice : unison.getVoices())
    {
        if (voice.getState() == Voice::State::active)
        {
            ++activeUnison;
            detunes.push_back (voice.getDetuneCents());
        }
    }

    if (activeUnison != 3)
    {
        std::cerr << "Expected 3 unison voices, got " << activeUnison << '\n';
        return false;
    }

    if (detunes.size() != 3 || detunes[0] != -12.0f || detunes[1] != 0.0f || detunes[2] != 12.0f)
    {
        std::cerr << "Unexpected unison detune distribution\n";
        return false;
    }

    unison.prepare (96000.0, 32);
    unison.advance (64);
    for (const auto& voice : unison.getVoices())
    {
        if (voice.getState() != Voice::State::idle && voice.getCurrentPitchHz() <= 0.0f)
        {
            std::cerr << "Invalid pitch after sample-rate/block-size change\n";
            return false;
        }
    }

    return true;
}
} // namespace

int main()
{
    if (! testPolyAllocationAndReleaseTail())
        return 1;

    if (! testNoStuckNotesOnRepeatedOffs())
        return 1;

    if (! testStealPriorityIsDeterministic())
        return 1;

    if (! testLegatoAndUnisonAndPrepareUpdates())
        return 1;

    std::cout << "VoiceManager tests passed\n";
    return 0;
}
