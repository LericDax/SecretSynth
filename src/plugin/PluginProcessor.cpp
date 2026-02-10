#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace secretsynth::plugin
{
float SecretSynthAudioProcessor::softLimit (float sample) noexcept
{
    return std::tanh (sample);
}

SecretSynthAudioProcessor::SecretSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}


void SecretSynthAudioProcessor::applyStateToEngine()
{
    oscillator.setFrequency (pluginState.values[static_cast<std::size_t> (parameters::ParameterId::oscillatorFrequency)]);
    oscillator.setPdAmount (pluginState.values[static_cast<std::size_t> (parameters::ParameterId::oscillatorPdAmount)]);
    filter.setCutoffHz (pluginState.values[static_cast<std::size_t> (parameters::ParameterId::filterCutoffHz)]);
    oscillatorMixGain = pluginState.values[static_cast<std::size_t> (parameters::ParameterId::outputGain)];
}

void SecretSynthAudioProcessor::prepareToPlay (double sampleRate, int)
{
    modulationEngine.setSampleRate (sampleRate);
    modulationEngine.reset();

    modulationEngine.ampEnv.setParameters ({ 0.005f, 0.12f, 0.9f, 0.3f });
    modulationEngine.modEnv.setParameters ({ 0.02f, 0.3f, 0.0f, 0.4f });
    modulationEngine.ampEnv.noteOn();
    modulationEngine.modEnv.noteOn();

    modulationEngine.lfo1.setRateMode (secretsynth::dsp::mod::Lfo::RateMode::tempoSync);
    modulationEngine.lfo1.setSyncDivision (secretsynth::dsp::mod::Lfo::SyncDivision::eighth);
    modulationEngine.lfo1.setTempoBpm (120.0f);

    modulationEngine.lfo2.setRateMode (secretsynth::dsp::mod::Lfo::RateMode::hertz);
    modulationEngine.lfo2.setRateHz (5.0f);

    modulationMatrix.setSampleRate (sampleRate);
    modulationMatrix.setDestinationSmoothingTimeSeconds (0.015f);
    modulationMatrix.clearRoutes();
    modulationMatrix.addRoute ({ secretsynth::dsp::mod::Source::lfo1, secretsynth::dsp::mod::Destination::pdAmount, 0.25f, true });
    modulationMatrix.addRoute ({ secretsynth::dsp::mod::Source::modEnv, secretsynth::dsp::mod::Destination::filterCutoff, 0.8f, false });
    modulationMatrix.addRoute ({ secretsynth::dsp::mod::Source::ampEnv, secretsynth::dsp::mod::Destination::amp, 1.0f, false });

    oscillator.prepare (sampleRate);
    oscillator.reset();
    oscillator.setTune (0.0f);
    oscillator.setFine (0.0f);
    oscillator.setPdShape (0.5f);
    oscillator.setMix (1.0f);
    oscillator.setQualityMode (secretsynth::dsp::osc::PhaseWarpOscillator::QualityMode::high);

    filter.prepare (sampleRate);
    filter.reset();
    filter.setMode (secretsynth::dsp::filter::MultiModeFilter::Mode::lowPass);
    filter.setResonance (0.6f);
    filter.setKeyTracking (0.5f);
    filter.setKeyTrackingReferenceHz (440.0f);

    filterPosition = FilterPosition::postOscMix;
    activeVoices = 1;
    lastKeyFrequencyHz = 220.0f;

    applyStateToEngine();
}

void SecretSynthAudioProcessor::releaseResources() {}

bool SecretSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SecretSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto amp = modulationEngine.ampEnv.processSample();
        const auto mod = modulationEngine.modEnv.processSample();
        const auto lfo1 = 0.5f * (modulationEngine.lfo1.processSample() + 1.0f);
        const auto lfo2 = 0.5f * (modulationEngine.lfo2.processSample() + 1.0f);

        std::array<float, static_cast<std::size_t> (secretsynth::dsp::mod::Source::count)> sources {};
        sources[static_cast<std::size_t> (secretsynth::dsp::mod::Source::ampEnv)] = amp;
        sources[static_cast<std::size_t> (secretsynth::dsp::mod::Source::modEnv)] = mod;
        sources[static_cast<std::size_t> (secretsynth::dsp::mod::Source::lfo1)] = lfo1;
        sources[static_cast<std::size_t> (secretsynth::dsp::mod::Source::lfo2)] = lfo2;

        const auto destinations = modulationMatrix.process (sources);
        const auto pdAmountMod = destinations[static_cast<std::size_t> (secretsynth::dsp::mod::Destination::pdAmount)];
        const auto ampMod = destinations[static_cast<std::size_t> (secretsynth::dsp::mod::Destination::amp)];
        const auto cutoffMod = destinations[static_cast<std::size_t> (secretsynth::dsp::mod::Destination::filterCutoff)];
        activeVoices = (ampMod > 1.0e-4f ? 1 : 0);

        const auto basePdAmount = pluginState.values[static_cast<std::size_t> (parameters::ParameterId::oscillatorPdAmount)];
        const auto baseCutoff = pluginState.values[static_cast<std::size_t> (parameters::ParameterId::filterCutoffHz)];
        oscillator.setPdAmount (juce::jlimit (0.0f, 1.0f, basePdAmount + pdAmountMod));
        filter.setCutoffHz (juce::jlimit (20.0f, 20000.0f, baseCutoff + 8000.0f * cutoffMod));

        float oscillatorSample = oscillator.renderSample() * 0.1f;

        if (activeVoices <= 0)
        {
            oscillatorSample = 0.0f;
            filter.reset();
        }

        if (filterPosition == FilterPosition::preOscMix)
            oscillatorSample = filter.processSample (oscillatorSample, lastKeyFrequencyHz);

        const auto mixed = oscillatorSample * oscillatorMixGain;

        float filtered = mixed;
        if (filterPosition == FilterPosition::postOscMix)
            filtered = filter.processSample (mixed, lastKeyFrequencyHz);

        const auto amped = filtered * juce::jlimit (0.0f, 1.0f, ampMod);
        const auto value = (activeVoices > 0 ? softLimit (amped) : 0.0f);

        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.setSample (channel, sample, value);
    }
}

juce::AudioProcessorEditor* SecretSynthAudioProcessor::createEditor()
{
    return new SecretSynthAudioProcessorEditor (*this);
}

bool SecretSynthAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String SecretSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SecretSynthAudioProcessor::acceptsMidi() const
{
    return true;
}

bool SecretSynthAudioProcessor::producesMidi() const
{
    return false;
}

bool SecretSynthAudioProcessor::isMidiEffect() const
{
    return false;
}

double SecretSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SecretSynthAudioProcessor::getNumPrograms()
{
    return 1;
}

int SecretSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SecretSynthAudioProcessor::setCurrentProgram (int) {}

const juce::String SecretSynthAudioProcessor::getProgramName (int)
{
    return {};
}

void SecretSynthAudioProcessor::changeProgramName (int, const juce::String&) {}

void SecretSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, true);
    stream.writeString (parameters::serializeState (pluginState));
    stream.writeString (modulationMatrix.serialize());
}

void SecretSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto stateText = juce::String::fromUTF8 (static_cast<const char*> (data), sizeInBytes);
    const auto lines = juce::StringArray::fromLines (stateText);

    juce::String parameterStateText;
    juce::String modulationText;

    for (const auto& line : lines)
    {
        if (line.startsWith ("state.version=") || line.startsWith ("osc.") || line.startsWith ("filter.") || line.startsWith ("output."))
            parameterStateText << line << "\n";

        if (line.startsWith ("schema=") || line.startsWith ("routes=") || line.containsChar (','))
            modulationText << line << "\n";
    }

    pluginState = parameters::deserializeState (parameterStateText.toStdString());
    applyStateToEngine();

    if (! modulationText.isEmpty())
        modulationMatrix.deserialize (modulationText.toStdString());
}
} // namespace secretsynth::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new secretsynth::plugin::SecretSynthAudioProcessor();
}
