#include "MainEditorComponent.h"

#include "../plugin/PluginProcessor.h"

#include <array>
#include <cmath>

namespace secretsynth::ui
{
namespace
{
class ModulatedSlider final : public juce::Slider, private juce::Timer
{
public:
    ModulatedSlider()
    {
        setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        startTimerHz (30);
    }

    void setModulationGetter (std::function<float()> getter)
    {
        modulationGetter = std::move (getter);
    }

    void paint (juce::Graphics& g) override
    {
        juce::Slider::paint (g);

        const auto amount = juce::jlimit (0.0f, 1.0f, std::abs (modulationAmount));
        if (amount <= 0.001f)
            return;

        const auto bounds = getLocalBounds().toFloat().reduced (6.0f);
        const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.46f;
        const auto centre = bounds.getCentre();

        juce::Path ring;
        ring.addCentredArc (centre.x,
                            centre.y,
                            radius,
                            radius,
                            0.0f,
                            juce::degreesToRadians (210.0f),
                            juce::degreesToRadians (210.0f + 300.0f * amount),
                            true);

        g.setColour (juce::Colours::deepskyblue.withAlpha (0.80f));
        g.strokePath (ring, juce::PathStrokeType (2.5f));
    }

private:
    void timerCallback() override
    {
        if (modulationGetter)
        {
            modulationAmount = modulationGetter();
            repaint();
        }
    }

    std::function<float()> modulationGetter;
    float modulationAmount { 0.0f };
};

class PdVisualizer final : public juce::Component, private juce::Timer
{
public:
    PdVisualizer (juce::Slider& amountSliderToUse, juce::Slider& shapeSliderToUse)
        : amountSlider (amountSliderToUse), shapeSlider (shapeSliderToUse)
    {
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour::fromRGB (14, 19, 28));
        g.setColour (juce::Colours::white.withAlpha (0.1f));
        g.drawRect (getLocalBounds());

        juce::Path waveform;
        const auto bounds = getLocalBounds().toFloat().reduced (10.0f, 12.0f);
        const auto width = bounds.getWidth();

        const auto pdAmount = static_cast<float> (amountSlider.getValue());
        const auto pdShape = static_cast<float> (shapeSlider.getValue());
        constexpr float twoPi = juce::MathConstants<float>::twoPi;

        for (int i = 0; i < static_cast<int> (width); ++i)
        {
            const auto xNorm = static_cast<float> (i) / juce::jmax (1.0f, width - 1.0f);
            const auto phase = xNorm * twoPi;
            const auto warp = std::sin (phase + pdShape * twoPi) * (0.5f * pdAmount);
            const auto y = std::sin (phase + warp);
            const auto drawX = bounds.getX() + static_cast<float> (i);
            const auto drawY = bounds.getCentreY() - y * bounds.getHeight() * 0.42f;

            if (i == 0)
                waveform.startNewSubPath (drawX, drawY);
            else
                waveform.lineTo (drawX, drawY);
        }

        g.setColour (juce::Colours::orange.withAlpha (0.9f));
        g.strokePath (waveform, juce::PathStrokeType (2.0f));

        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.setFont (juce::FontOptions (12.0f));
        g.drawText ("Phase Distortion preview", getLocalBounds().reduced (8).removeFromTop (18), juce::Justification::left);
    }

private:
    void timerCallback() override { repaint(); }

    juce::Slider& amountSlider;
    juce::Slider& shapeSlider;
};

class Section final : public juce::Component
{
public:
    explicit Section (juce::String titleText)
    {
        title.setText (std::move (titleText), juce::dontSendNotification);
        title.setJustificationType (juce::Justification::centredLeft);
        title.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (title);
    }

    void addControl (juce::Component& component, juce::String labelText)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText (std::move (labelText), juce::dontSendNotification);
        label->setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.8f));
        label->setJustificationType (juce::Justification::centred);

        labels.push_back (std::move (label));
        controls.push_back (&component);

        addAndMakeVisible (*controls.back());
        addAndMakeVisible (*labels.back());
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (juce::Colour::fromRGB (24, 29, 36));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 10.0f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 10.0f, 1.0f);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced (10);
        title.setBounds (bounds.removeFromTop (24));

        const auto columns = bounds.getWidth() > 300 ? 3 : 2;
        const auto rows = juce::jmax (1, static_cast<int> (std::ceil (controls.size() / static_cast<float> (columns))));
        const auto itemW = bounds.getWidth() / columns;
        const auto itemH = juce::jmax (80, bounds.getHeight() / rows);

        for (std::size_t i = 0; i < controls.size(); ++i)
        {
            const int row = static_cast<int> (i) / columns;
            const int col = static_cast<int> (i) % columns;
            auto cell = juce::Rectangle<int> (bounds.getX() + col * itemW, bounds.getY() + row * itemH, itemW, itemH).reduced (4);
            labels[i]->setBounds (cell.removeFromTop (20));
            controls[i]->setBounds (cell.reduced (4));
        }
    }

private:
    juce::Label title;
    std::vector<juce::Component*> controls;
    std::vector<std::unique_ptr<juce::Label>> labels;
};
} // namespace

class MainEditorComponent::Impl final : private juce::Timer
{
public:
    explicit Impl (MainEditorComponent& ownerToUse, plugin::SecretSynthAudioProcessor& processorToUse)
        : owner (ownerToUse),
          processor (processorToUse),
          valueTreeState (processor.getValueTreeState()),
          oscSection ("Osc"),
          modSection ("Mod"),
          filterSection ("Filter"),
          ampSection ("Amp"),
          perfSection ("Performance"),
          pdVisualizer (oscPdAmount, oscPdShape)
    {
        configureSlider (oscFrequency);
        configureSlider (oscPdAmount);
        configureSlider (oscPdShape);
        configureSlider (oscTune);
        configureSlider (oscFine);
        configureSlider (oscMix);
        configureSlider (modLfo1Rate);
        configureSlider (modLfo2Rate);
        configureSlider (filterCutoff);
        configureSlider (filterResonance);
        configureSlider (ampAttack);
        configureSlider (ampRelease);
        configureSlider (outputGain);
        configureSlider (performanceVoices, true);

        oscPdAmount.setModulationGetter ([this] { return processor.getUiModulationState().pdAmount; });
        filterCutoff.setModulationGetter ([this] { return processor.getUiModulationState().filterCutoff; });
        outputGain.setModulationGetter ([this] { return processor.getUiModulationState().amp; });

        addAttach (oscFrequencyAttachment, oscFrequency, "osc.frequency");
        addAttach (oscPdAmountAttachment, oscPdAmount, "osc.pd_amount");
        addAttach (oscPdShapeAttachment, oscPdShape, "osc.pd_shape");
        addAttach (oscTuneAttachment, oscTune, "osc.tune");
        addAttach (oscFineAttachment, oscFine, "osc.fine");
        addAttach (oscMixAttachment, oscMix, "osc.mix");
        addAttach (modLfo1RateAttachment, modLfo1Rate, "mod.lfo1_rate_hz");
        addAttach (modLfo2RateAttachment, modLfo2Rate, "mod.lfo2_rate_hz");
        addAttach (filterCutoffAttachment, filterCutoff, "filter.cutoff_hz");
        addAttach (filterResonanceAttachment, filterResonance, "filter.resonance");
        addAttach (ampAttackAttachment, ampAttack, "amp.attack_s");
        addAttach (ampReleaseAttachment, ampRelease, "amp.release_s");
        addAttach (outputGainAttachment, outputGain, "output.gain");
        addAttach (performanceVoicesAttachment, performanceVoices, "performance.voices");

        oscSection.addControl (oscFrequency, "Freq");
        oscSection.addControl (oscPdAmount, "PD Amt");
        oscSection.addControl (oscPdShape, "PD Shape");
        oscSection.addControl (oscTune, "Tune");
        oscSection.addControl (oscFine, "Fine");
        oscSection.addControl (oscMix, "Mix");

        modSection.addControl (modLfo1Rate, "LFO 1 Hz");
        modSection.addControl (modLfo2Rate, "LFO 2 Hz");

        filterSection.addControl (filterCutoff, "Cutoff");
        filterSection.addControl (filterResonance, "Resonance");

        ampSection.addControl (ampAttack, "Attack");
        ampSection.addControl (ampRelease, "Release");
        ampSection.addControl (outputGain, "Gain");

        perfSection.addControl (performanceVoices, "Voices");

        owner.addAndMakeVisible (oscSection);
        owner.addAndMakeVisible (modSection);
        owner.addAndMakeVisible (filterSection);
        owner.addAndMakeVisible (ampSection);
        owner.addAndMakeVisible (perfSection);
        owner.addAndMakeVisible (pdVisualizer);

        startTimerHz (30);
    }

    void resized()
    {
        auto area = owner.getLocalBounds().reduced (12);
        pdVisualizer.setBounds (area.removeFromTop (140));
        area.removeFromTop (8);

        juce::Grid grid;
        grid.autoFlow = juce::Grid::AutoFlow::row;
        grid.columnGap = juce::Grid::Px (8.0f);
        grid.rowGap = juce::Grid::Px (8.0f);

        const auto width = area.getWidth();
        const int columns = width > 1600 ? 5 : (width > 1180 ? 4 : (width > 900 ? 3 : 2));
        const int rows = 3;

        for (int i = 0; i < columns; ++i)
            grid.templateColumns.add (juce::Grid::TrackInfo (juce::Grid::Fr (1)));
        for (int i = 0; i < rows; ++i)
            grid.templateRows.add (juce::Grid::TrackInfo (juce::Grid::Fr (1)));

        std::array<juce::Component*, 5> sections { &oscSection, &modSection, &filterSection, &ampSection, &perfSection };
        for (auto* section : sections)
            grid.items.add (juce::GridItem (*section));

        grid.performLayout (area);
    }

private:
    void timerCallback() override
    {
        oscPdAmount.repaint();
        filterCutoff.repaint();
        outputGain.repaint();
    }

    static void configureSlider (juce::Slider& slider, bool integerStep = false)
    {
        if (integerStep)
            slider.setRange (1.0, 16.0, 1.0);

        slider.setTextValueSuffix (integerStep ? " voices" : "");
    }

    void addAttach (std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>& attachment,
                    juce::Slider& slider,
                    const char* parameterId)
    {
        attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (valueTreeState, parameterId, slider);
    }

    MainEditorComponent& owner;
    plugin::SecretSynthAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& valueTreeState;

    Section oscSection;
    Section modSection;
    Section filterSection;
    Section ampSection;
    Section perfSection;

    ModulatedSlider oscFrequency;
    ModulatedSlider oscPdAmount;
    ModulatedSlider oscPdShape;
    ModulatedSlider oscTune;
    ModulatedSlider oscFine;
    ModulatedSlider oscMix;
    ModulatedSlider modLfo1Rate;
    ModulatedSlider modLfo2Rate;
    ModulatedSlider filterCutoff;
    ModulatedSlider filterResonance;
    ModulatedSlider ampAttack;
    ModulatedSlider ampRelease;
    ModulatedSlider outputGain;
    ModulatedSlider performanceVoices;

    PdVisualizer pdVisualizer;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscFrequencyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscPdAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscPdShapeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscTuneAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscFineAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> oscMixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modLfo1RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modLfo2RateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResonanceAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ampReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> performanceVoicesAttachment;
};

MainEditorComponent::MainEditorComponent (secretsynth::plugin::SecretSynthAudioProcessor& processor)
    : impl (std::make_unique<Impl> (*this, processor))
{
}

MainEditorComponent::~MainEditorComponent() = default;

void MainEditorComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour::fromRGB (9, 11, 14));
}

void MainEditorComponent::resized()
{
    impl->resized();
}
} // namespace secretsynth::ui
