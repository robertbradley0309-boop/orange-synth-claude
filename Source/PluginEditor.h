#pragma once
#include <memory>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class OrangeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    OrangeLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
};

class OrangeSynthEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit OrangeSynthEditor (OrangeSynthProcessor&);
    ~OrangeSynthEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Control
    {
        int index = 0;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::ComboBox> combo;
        std::unique_ptr<juce::Label> label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sAtt;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> cAtt;
    };

    void timerCallback() override;
    void rebuildPresetList();
    void selectPreset (int globalIndex);   // loads the preset into the synth and shows it
    void showPreset (int globalIndex);     // updates the boxes only
    int  currentCategory() const;

    OrangeSynthProcessor& proc;
    OrangeLookAndFeel laf;

    juce::ComboBox categoryBox, presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    std::vector<Control> controls;
    std::vector<int> listToGlobal;
    juce::Rectangle<int> groupBounds[Orange::kNumGroups];
    bool updating = false;
    int shownProgram = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OrangeSynthEditor)
};
