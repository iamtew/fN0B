#pragma once

#include "PluginProcessor.h"
#include "ui/Theme.h"

#include <functional>

// A small plugin: a preset bar, one big knob, and in/out trim sliders that also meter.
// The editor lays out at a fixed base size and scales the whole picture when the
// window is resized, the same trick the sibling plugins use.
class fN0BAudioProcessorEditor : public juce::AudioProcessorEditor,
                                 private juce::Timer
{
public:
    explicit fN0BAudioProcessorEditor (fN0BAudioProcessor&);
    ~fN0BAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Content : public juce::Component
    {
        explicit Content (fN0BAudioProcessorEditor& e) : ed (e) {}
        void paint (juce::Graphics& g) override { ed.paintContent (g); }
        void resized() override { ed.layoutContent(); }
        fN0BAudioProcessorEditor& ed;
    };

    void paintContent (juce::Graphics&);
    void layoutContent();
    void refreshBar();
    void timerCallback() override;
    void setupGainSlider (juce::Slider& s, const char* paramId, const juce::String& tooltipText);

    fN0BAudioProcessor& proc;

    // Rotary + linear look: vector paint, sharp at every editor scale (see knob-design.md).
    struct PanelLnF : public juce::LookAndFeel_V4
    {
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
        void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float sliderPos,
                               float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle,
                               juce::Slider&) override;
        void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
        juce::Font getLabelFont (juce::Label&) override;
    };

    // "<  name  >": the ends step through presets, the middle opens the menu.
    struct PresetBar : public juce::Component,
                       public juce::SettableTooltipClient
    {
        void paint (juce::Graphics& g) override;
        void mouseUp (const juce::MouseEvent& e) override;
        void setText (const juce::String& t);

        juce::String text;
        std::function<void (int)> onStep;
        std::function<void()> onOpen;
    };

    void stepPreset (int delta);
    void openPresetMenu();

    PanelLnF panelLnF;
    Content content { *this };
    int baseW = 0, baseH = 0;

    juce::Slider knob;
    juce::Label knobHint;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtt;

    juce::Slider inGain, outGain;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inGainAtt, outGainAtt;
    float inMeter = 0.f, outMeter = 0.f;

    PresetBar presetBar;
    juce::Image logo;
    juce::TooltipWindow tip { this, 400 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (fN0BAudioProcessorEditor)
};
