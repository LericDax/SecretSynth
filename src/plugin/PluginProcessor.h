#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../dsp/filter/MultiModeFilter.h"
#include "../dsp/osc/PhaseWarpOscillator.h"
#include "../dsp/mod/Modulation.h"
#include "parameters/StateSerialization.h"

namespace secretsynth::plugin
{
class SecretSynthAudioProcessor : public juce::AudioProcessor
{
public:
    SecretSynthAudioProcessor();
    ~SecretSynthAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    enum class FilterPosition
    {
        preOscMix,
        postOscMix
    };

    static float softLimit (float sample) noexcept;
    void applyStateToEngine();

    secretsynth::dsp::osc::PhaseWarpOscillator oscillator;
    secretsynth::dsp::filter::MultiModeFilter filter;
    secretsynth::dsp::mod::ModulationMatrix modulationMatrix;
    secretsynth::dsp::mod::ModulationEngine modulationEngine;

    FilterPosition filterPosition { FilterPosition::postOscMix };
    float oscillatorMixGain { 1.0f };
    int activeVoices { 0 };
    float lastKeyFrequencyHz { 220.0f };
    parameters::PluginState pluginState { parameters::makeDefaultState() };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SecretSynthAudioProcessor)
};
} // namespace secretsynth::plugin
