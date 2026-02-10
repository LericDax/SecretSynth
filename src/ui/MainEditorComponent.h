#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace secretsynth::plugin
{
class SecretSynthAudioProcessor;
}

namespace secretsynth::ui
{
class MainEditorComponent : public juce::Component
{
public:
    explicit MainEditorComponent (secretsynth::plugin::SecretSynthAudioProcessor&);
    ~MainEditorComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace secretsynth::ui
