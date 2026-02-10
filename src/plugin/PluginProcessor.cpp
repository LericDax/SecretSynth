#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace secretsynth::plugin
{
SecretSynthAudioProcessor::SecretSynthAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

void SecretSynthAudioProcessor::prepareToPlay (double sampleRate, int)
{
    oscillator.prepare (sampleRate);
    oscillator.reset();
    oscillator.setFrequency (220.0f);
    oscillator.setTune (0.0f);
    oscillator.setFine (0.0f);
    oscillator.setPdAmount (0.6f);
    oscillator.setPdShape (0.5f);
    oscillator.setMix (1.0f);
    oscillator.setQualityMode (secretsynth::dsp::osc::PhaseWarpOscillator::QualityMode::high);
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
        const auto value = oscillator.renderSample() * 0.1f;

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
    stream.writeFloat (220.0f);
}

void SecretSynthAudioProcessor::setStateInformation (const void*, int) {}
} // namespace secretsynth::plugin

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new secretsynth::plugin::SecretSynthAudioProcessor();
}
