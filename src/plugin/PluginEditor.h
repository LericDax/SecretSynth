#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "../ui/MainEditorComponent.h"

namespace secretsynth::plugin
{
class SecretSynthAudioProcessor;

class SecretSynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit SecretSynthAudioProcessorEditor (SecretSynthAudioProcessor&);
    ~SecretSynthAudioProcessorEditor() override = default;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    SecretSynthAudioProcessor& processor;
    secretsynth::ui::MainEditorComponent mainComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SecretSynthAudioProcessorEditor)
};
} // namespace secretsynth::plugin
