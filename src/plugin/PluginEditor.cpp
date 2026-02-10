#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace secretsynth::plugin
{
SecretSynthAudioProcessorEditor::SecretSynthAudioProcessorEditor (SecretSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    addAndMakeVisible (mainComponent);
    setSize (640, 420);
}

void SecretSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::darkslategrey);
}

void SecretSynthAudioProcessorEditor::resized()
{
    mainComponent.setBounds (getLocalBounds());
}
} // namespace secretsynth::plugin
