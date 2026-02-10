#include "MainEditorComponent.h"

namespace secretsynth::ui
{
MainEditorComponent::MainEditorComponent()
{
    titleLabel.setText ("SecretSynth", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);
}

void MainEditorComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.drawRect (getLocalBounds(), 1);
}

void MainEditorComponent::resized()
{
    titleLabel.setBounds (getLocalBounds().reduced (16));
}
} // namespace secretsynth::ui
