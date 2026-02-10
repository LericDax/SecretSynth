#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace secretsynth::plugin
{
namespace
{
juce::NormalisableRange<float> makeRange (const parameters::ParameterSpec& spec)
{
    if (spec.maximum > 1000.0f)
        return { spec.minimum, spec.maximum, 0.0f, 0.35f };

    return { spec.minimum, spec.maximum };
}
} // namespace

float SecretSynthAudioProcessor::softLimit (float sample) noexcept
{
    return std::tanh (sample);
}

SecretSynthAudioProcessor::SecretSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState (*this, nullptr, "SecretSynthParameters", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout SecretSynthAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> layout;
    layout.reserve (parameters::parameterSpecs.size());

    for (const auto& spec : parameters::parameterSpecs)
    {
        layout.push_back (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { spec.stableId.data(), 1 },
                                                                        juce::String (spec.stableId.data()),
                                                                        makeRange (spec),
                                                                        spec.defaultValue));
    }

    return { layout.begin(), layout.end() };
}

float SecretSynthAudioProcessor::getParameterValue (parameters::ParameterId id) const noexcept
{
    const auto& spec = parameters::getSpec (id);
    if (const auto* value = valueTreeState.getRawParameterValue (juce::String (spec.stableId.data())))
        return value->load();

    return spec.defaultValue;
}

void SecretSynthAudioProcessor::applyStateToEngine()
{
    oscillator.setFrequency (getParameterValue (parameters::ParameterId::oscillatorFrequency));
    oscillator.setPdAmount (getParameterValue (parameters::ParameterId::oscillatorPdAmount));
    oscillator.setPdShape (getParameterValue (parameters::ParameterId::oscillatorPdShape));
    oscillator.setTune (getParameterValue (parameters::ParameterId::oscillatorTune));
    oscillator.setFine (getParameterValue (parameters::ParameterId::oscillatorFine));
    oscillator.setMix (getParameterValue (parameters::ParameterId::oscillatorMix));

    filter.setCutoffHz (getParameterValue (parameters::ParameterId::filterCutoffHz));
    filter.setResonance (getParameterValue (parameters::ParameterId::filterResonance));

    modulationEngine.lfo1.setRateHz (getParameterValue (parameters::ParameterId::modLfo1RateHz));
    modulationEngine.lfo2.setRateHz (getParameterValue (parameters::ParameterId::modLfo2RateHz));

    modulationEngine.ampEnv.setParameters ({ getParameterValue (parameters::ParameterId::ampAttackSeconds),
                                             0.12f,
                                             0.9f,
                                             getParameterValue (parameters::ParameterId::ampReleaseSeconds) });

    oscillatorMixGain = getParameterValue (parameters::ParameterId::outputGain);
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
    oscillator.setQualityMode (secretsynth::dsp::osc::PhaseWarpOscillator::QualityMode::high);

    filter.prepare (sampleRate);
    filter.reset();
    filter.setMode (secretsynth::dsp::filter::MultiModeFilter::Mode::lowPass);
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

    applyStateToEngine();

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

        const auto basePdAmount = getParameterValue (parameters::ParameterId::oscillatorPdAmount);
        const auto baseCutoff = getParameterValue (parameters::ParameterId::filterCutoffHz);
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

        uiPdAmountMod.store (pdAmountMod, std::memory_order_relaxed);
        uiFilterCutoffMod.store (cutoffMod, std::memory_order_relaxed);
        uiAmpMod.store (ampMod, std::memory_order_relaxed);
    }
}

SecretSynthAudioProcessor::UiModulationState SecretSynthAudioProcessor::getUiModulationState() const noexcept
{
    return { uiPdAmountMod.load (std::memory_order_relaxed),
             uiFilterCutoffMod.load (std::memory_order_relaxed),
             uiAmpMod.load (std::memory_order_relaxed) };
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
    for (const auto& spec : parameters::parameterSpecs)
    {
        pluginState.values[static_cast<std::size_t> (spec.id)] = getParameterValue (spec.id);
    }

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
        if (line.startsWith ("state.version=") || line.startsWith ("osc.") || line.startsWith ("filter.") || line.startsWith ("output.") || line.startsWith ("mod.") || line.startsWith ("amp.") || line.startsWith ("performance."))
            parameterStateText << line << "\n";

        if (line.startsWith ("schema=") || line.startsWith ("routes=") || line.containsChar (','))
            modulationText << line << "\n";
    }

    pluginState = parameters::deserializeState (parameterStateText.toStdString());

    for (const auto& spec : parameters::parameterSpecs)
    {
        const auto value = parameters::clampToRange (spec.id, pluginState.values[static_cast<std::size_t> (spec.id)]);
        valueTreeState.getParameter (juce::String (spec.stableId.data()))->setValueNotifyingHost (
            valueTreeState.getParameterRange (juce::String (spec.stableId.data())).convertTo0to1 (value));
    }

    applyStateToEngine();

    if (! modulationText.isEmpty())
        modulationMatrix.deserialize (modulationText.toStdString());
}
} // namespace secretsynth::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new secretsynth::plugin::SecretSynthAudioProcessor();
}
