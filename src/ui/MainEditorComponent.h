#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace secretsynth::ui
{
class MainEditorComponent : public juce::Component
{
public:
    MainEditorComponent();

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label titleLabel;
};
} // namespace secretsynth::ui
