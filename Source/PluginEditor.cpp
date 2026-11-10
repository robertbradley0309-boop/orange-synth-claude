#include "PluginEditor.h"

using namespace Orange;

static const juce::Colour kOrange   { 0xffff7a1a };
static const juce::Colour kOrangeLt { 0xffffa75c };
static const juce::Colour kBg       { 0xff17120e };
static const juce::Colour kPanel    { 0xff241b14 };

OrangeLookAndFeel::OrangeLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, kPanel);
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, kOrange.withAlpha (0.6f));
    setColour (juce::ComboBox::arrowColourId, kOrange);
    setColour (juce::PopupMenu::backgroundColourId, kPanel);
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, kOrange);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
    setColour (juce::TextButton::buttonColourId, kPanel);
    setColour (juce::TextButton::textColourOffId, kOrange);
    setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.85f));
    setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.85f));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
}

void OrangeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                          float startAngle, float endAngle, juce::Slider&)
{
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    g.setColour (kPanel.brighter (0.15f));
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    juce::Path track, value;
    const float arcR = radius - 2.0f;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.strokePath (track, juce::PathStrokeType (3.0f));
    g.setColour (kOrange);
    g.strokePath (value, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -radius + 5.0f, 3.0f, radius * 0.45f, 1.5f);
    g.setColour (kOrangeLt);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

OrangeSynthEditor::OrangeSynthEditor (OrangeSynthProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&laf);
    setSize (900, 560);

    for (int c = 0; c < kNumCategories; ++c)
        categoryBox.addItem (kCategories[c], c + 1);
    categoryBox.addItem ("All", kNumCategories + 1);
    addAndMakeVisible (categoryBox);
    addAndMakeVisible (presetBox);
    addAndMakeVisible (prevButton);
    addAndMakeVisible (nextButton);

    categoryBox.onChange = [this]
    {
        if (updating) return;
        rebuildPresetList();
        if (! listToGlobal.empty())
            selectPreset (listToGlobal.front());
    };
    presetBox.onChange = [this]
    {
        if (updating) return;
        const int i = presetBox.getSelectedId() - 1;
        if (i >= 0 && i < (int) listToGlobal.size())
            selectPreset (listToGlobal[(size_t) i]);
    };
    auto step = [this] (int dir)
    {
        const int n = presetBox.getNumItems();
        if (n == 0) return;
        int id = presetBox.getSelectedId() - 1 + dir;
        id = (id % n + n) % n;
        presetBox.setSelectedId (id + 1, juce::sendNotification);
    };
    prevButton.onClick = [step] { step (-1); };
    nextButton.onClick = [step] { step (1); };

    for (int i = 0; i < pCount; ++i)
    {
        const auto& d = kParams[i];
        Control c;
        c.index = i;
        c.label = std::make_unique<juce::Label>();
        c.label->setText (d.name, juce::dontSendNotification);
        c.label->setJustificationType (juce::Justification::centred);
        c.label->setFont (juce::FontOptions (12.0f));
        addAndMakeVisible (*c.label);

        if (d.kind == 1 || d.kind == 2)
        {
            c.combo = std::make_unique<juce::ComboBox>();
            if (d.kind == 1) { c.combo->addItem ("Saw", 1); c.combo->addItem ("Square", 2);
                               c.combo->addItem ("Triangle", 3); c.combo->addItem ("Sine", 4); }
            else             { c.combo->addItem ("Low-pass", 1); c.combo->addItem ("Band-pass", 2);
                               c.combo->addItem ("High-pass", 3); }
            addAndMakeVisible (*c.combo);
            c.cAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, d.id, *c.combo);
        }
        else
        {
            c.slider = std::make_unique<juce::Slider> (juce::Slider::RotaryHorizontalVerticalDrag,
                                                       juce::Slider::TextBoxBelow);
            c.slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 62, 15);
            c.slider->setDoubleClickReturnValue (true, d.defV);
            addAndMakeVisible (*c.slider);
            c.sAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, d.id, *c.slider);
        }
        controls.push_back (std::move (c));
    }

    int cat = 0;
    for (int c = 0; c < kNumCategories; ++c)
        if (juce::String (kCategories[c]) == kPresets[proc.currentProgram].category) cat = c;
    updating = true;
    categoryBox.setSelectedId (cat + 1, juce::dontSendNotification);
    updating = false;
    rebuildPresetList();
    showPreset (proc.currentProgram);   // display only: opening the editor must not reload the patch
    startTimerHz (5);
}

OrangeSynthEditor::~OrangeSynthEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void OrangeSynthEditor::timerCallback()
{
    // keep the boxes in sync if the host changes program (e.g. FL Studio's preset arrows)
    if (proc.currentProgram == shownProgram) return;
    const int idx = juce::jlimit (0, kNumPresets - 1, proc.currentProgram);
    for (int c = 0; c < kNumCategories; ++c)
        if (juce::String (kCategories[c]) == kPresets[idx].category && currentCategory() < kNumCategories
            && currentCategory() != c)
        {
            updating = true;
            categoryBox.setSelectedId (c + 1, juce::dontSendNotification);
            updating = false;
            rebuildPresetList();
        }
    showPreset (idx);
}

int OrangeSynthEditor::currentCategory() const
{
    return categoryBox.getSelectedId() - 1;
}

void OrangeSynthEditor::rebuildPresetList()
{
    const bool wasUpdating = updating;
    updating = true;
    presetBox.clear (juce::dontSendNotification);
    listToGlobal.clear();
    const int cat = currentCategory();
    int id = 1;
    for (int i = 0; i < kNumPresets; ++i)
    {
        if (cat < kNumCategories && juce::String (kPresets[i].category) != kCategories[cat])
            continue;
        const juce::String label = (cat >= kNumCategories ? juce::String (kPresets[i].category) + " - " : juce::String())
                                   + kPresets[i].name;
        presetBox.addItem (label, id++);
        listToGlobal.push_back (i);
    }
    updating = wasUpdating;
}

void OrangeSynthEditor::selectPreset (int globalIndex)
{
    proc.loadPreset (globalIndex);
    showPreset (globalIndex);
}

void OrangeSynthEditor::showPreset (int globalIndex)
{
    shownProgram = globalIndex;
    updating = true;
    for (size_t i = 0; i < listToGlobal.size(); ++i)
        if (listToGlobal[i] == globalIndex)
            presetBox.setSelectedId ((int) i + 1, juce::dontSendNotification);
    updating = false;
}

void OrangeSynthEditor::paint (juce::Graphics& g)
{
    g.fillAll (kBg);

    g.setColour (kOrange);
    g.setFont (juce::FontOptions (30.0f, juce::Font::bold));
    g.drawText ("ORANGE SYNTH", 20, 10, 320, 36, juce::Justification::centredLeft);
    g.setColour (kOrangeLt.withAlpha (0.8f));
    g.setFont (juce::FontOptions (13.0f));
    g.drawText ("by Robby B", 22, 42, 200, 18, juce::Justification::centredLeft);

    for (int i = 0; i < kNumGroups; ++i)
    {
        auto r = groupBounds[i];
        if (r.isEmpty()) continue;
        g.setColour (kPanel);
        g.fillRoundedRectangle (r.toFloat(), 8.0f);
        g.setColour (kOrange.withAlpha (0.35f));
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 8.0f, 1.0f);
        g.setColour (kOrange);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (kGroups[i], r.getX() + 10, r.getY() + 4, r.getWidth() - 20, 18, juce::Justification::centredLeft);
    }
}

void OrangeSynthEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    auto top = area.removeFromTop (52);
    top.removeFromLeft (300);
    prevButton.setBounds (top.removeFromRight (36).reduced (2, 10));
    nextButton.setBounds (top.removeFromRight (36).reduced (2, 10));
    top.removeFromRight (8);
    categoryBox.setBounds (top.removeFromLeft (130).reduced (0, 10));
    top.removeFromLeft (8);
    presetBox.setBounds (top.reduced (0, 10));
    area.removeFromTop (6);

    // Rows of panels: [OSC][FILTER] / [AMP][FLT ENV][MOD][MASTER]
    const int cellW = 72, cellH = 96, head = 26, pad = 8;
    auto place = [&] (int group, juce::Rectangle<int> r)
    {
        groupBounds[group] = r;
        int col = 0, row = 0;
        const int perRow = juce::jmax (1, (r.getWidth() - pad) / cellW);
        for (auto& c : controls)
        {
            if (kParams[c.index].group != group) continue;
            const int cx = r.getX() + pad + col * cellW;
            const int cy = r.getY() + head + row * (cellH + 4);
            if (c.combo)
            {
                c.label->setBounds (cx, cy + 16, cellW, 14);
                c.combo->setBounds (cx + 2, cy + 34, cellW - 4, 24);
            }
            else
            {
                c.label->setBounds (cx, cy, cellW, 14);
                c.slider->setBounds (cx, cy + 12, cellW, cellH - 12);
            }
            if (++col >= perRow) { col = 0; ++row; }
        }
    };

    const int rowH = 2 * (cellH + 4) + head + 4;
    auto row1 = area.removeFromTop (rowH);
    place (0, row1.removeFromLeft (area.getWidth() * 62 / 100));
    row1.removeFromLeft (8);
    place (1, row1);
    area.removeFromTop (8);

    auto row2 = area.removeFromTop (rowH);
    const int w4 = (area.getWidth() - 3 * 8) / 4;
    place (2, row2.removeFromLeft (w4)); row2.removeFromLeft (8);
    place (3, row2.removeFromLeft (w4)); row2.removeFromLeft (8);
    place (4, row2.removeFromLeft (w4)); row2.removeFromLeft (8);
    place (5, row2);
}
