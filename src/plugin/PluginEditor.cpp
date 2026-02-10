#include "PluginEditor.h"
#include "PluginProcessor.h"

namespace secretsynth::plugin
{
SecretSynthAudioProcessorEditor::SecretSynthAudioProcessorEditor (SecretSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), mainComponent (p)
{
    addAndMakeVisible (mainComponent);
    setResizable (true, true);
    setResizeLimits (900, 580, 1900, 1200);
    setSize (1200, 760);
}

void SecretSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (12, 15, 19));
}

void SecretSynthAudioProcessorEditor::resized()
{
    mainComponent.setBounds (getLocalBounds());
}
} // namespace secretsynth::plugin
