#include "PluginEditor.h"

namespace
{
constexpr int kEditorW = 520;
constexpr int kEditorH = 400;
constexpr int kPad = 8;
constexpr int kTitleH = 40;
constexpr int kTitleHitW = 150;
constexpr int kTextBoxW = 56;
constexpr int kTextBoxH = 18;
constexpr int kHintH = 32;

// What the knob does in each preset, shown under it so the one control explains itself.
const char* const kHints[fn0b::kPresetCount] = {
    "clean boost 0 to +18 dB, top end closing 20 kHz to 4 kHz",
    "cutoff 200 Hz to 8 kHz, resonance 0.2 to 0.9, drive 0 to +12 dB",
    "sample rate 1x to 16x, 16 to 4 bits, drive 0 to +12 dB",
    "delay 60 to 450 ms, repeats 15 to 60%, top end falling to 2.5 kHz",
    "room grows to full, octave shimmer blooms to 50%, tone falls to 6 kHz",
    "sweep 0.12 to 3.5 Hz, depth to full, feedback to 70%"
};
} // namespace

// ---------------------------------------------------------------- Look

void fN0BAudioProcessorEditor::PanelLnF::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                                           float pos, float startAngle, float endAngle,
                                                           juce::Slider&)
{
    using namespace fn0b;
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (1.f);
    const float d = juce::jmin (bounds.getWidth(), bounds.getHeight());
    if (d < 8.f)
        return;

    auto area = bounds.withSizeKeepingCentre (d, d);
    const auto c = area.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Shadow: two offset ellipses, not a DropShadow (which goes soft under the scale transform).
    const float flangeD = d * 0.86f;
    auto flange = area.withSizeKeepingCentre (flangeD, flangeD);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (flange.translated (0.f, d * 0.04f).expanded (d * 0.01f));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (flange.translated (0.f, d * 0.018f));

    g.setGradientFill (juce::ColourGradient (kMint.brighter (0.08f), c.x, flange.getY(),
                                              kMint.darker (0.18f), c.x, flange.getBottom(), false));
    g.fillEllipse (flange);

    const float tickW = juce::jmax (1.f, d * 0.018f);
    g.setColour (kAmber);
    constexpr int nTicks = 11;
    for (int i = 0; i < nTicks; ++i)
    {
        const float t = (float) i / (float) (nTicks - 1);
        const float a = startAngle + t * (endAngle - startAngle);
        const auto p0 = c.getPointOnCircumference (flangeD * 0.48f, a);
        const auto p1 = c.getPointOnCircumference (flangeD * 0.40f, a);
        g.drawLine (p0.x, p0.y, p1.x, p1.y, tickW);
    }

    const float bodyD = d * 0.68f;
    auto body = area.withSizeKeepingCentre (bodyD, bodyD);
    g.setColour (kInk);
    g.drawEllipse (body.expanded (d * 0.012f), juce::jmax (1.2f, d * 0.018f));
    g.setGradientFill (juce::ColourGradient (kMint.brighter (0.22f), c.x, body.getY(),
                                              kMint.darker (0.12f), c.x, body.getBottom(), false));
    g.fillEllipse (body);

    constexpr int nFlutes = 14;
    const float fluteW = juce::jmax (1.f, d * 0.014f);
    g.setColour (kAmber.withAlpha (0.85f));
    for (int i = 0; i < nFlutes; ++i)
    {
        const float a = angle + (float) i * juce::MathConstants<float>::twoPi / (float) nFlutes;
        const auto p0 = c.getPointOnCircumference (bodyD * 0.12f, a);
        const auto p1 = c.getPointOnCircumference (bodyD * 0.46f, a);
        g.drawLine (p0.x, p0.y, p1.x, p1.y, fluteW);
    }

    juce::Path pointer;
    pointer.startNewSubPath (c.getPointOnCircumference (bodyD * 0.06f, angle));
    pointer.lineTo (c.getPointOnCircumference (bodyD * 0.46f, angle));
    g.setColour (kAmber);
    g.strokePath (pointer, juce::PathStrokeType (juce::jmax (2.f, d * 0.045f),
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

juce::Font fN0BAudioProcessorEditor::PanelLnF::getLabelFont (juce::Label&)
{
    return fn0b::boldFont (11.f);
}

void fN0BAudioProcessorEditor::PanelLnF::drawTooltip (juce::Graphics& g, const juce::String& text,
                                                      int width, int height)
{
    using namespace fn0b;
    auto r = juce::Rectangle<float> (0.f, 0.f, (float) width, (float) height);
    g.setColour (kInk);
    g.fillRoundedRectangle (r, 3.f);
    g.setColour (kMint);
    g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);
    g.setColour (kLight);
    g.setFont (boldFont (12.f));
    g.drawFittedText (text, r.reduced (6.f, 4.f).toNearestInt(), juce::Justification::centredLeft, 4);
}

// ---------------------------------------------------------------- Preset bar

void fN0BAudioProcessorEditor::PresetBar::setText (const juce::String& t)
{
    if (text == t)
        return;
    text = t;
    repaint();
}

void fN0BAudioProcessorEditor::PresetBar::paint (juce::Graphics& g)
{
    using namespace fn0b;
    auto r = getLocalBounds().toFloat();
    g.setColour (kInk);
    g.fillRoundedRectangle (r, 4.f);
    g.setColour (kGrey);
    g.drawRoundedRectangle (r.reduced (0.5f), 4.f, 1.f);

    auto left = r.removeFromLeft (18.f);
    auto right = r.removeFromRight (18.f);
    auto chev = [] (juce::Rectangle<float> box, bool back)
    {
        juce::Path p;
        const float cx = box.getCentreX();
        const float cy = box.getCentreY();
        if (back)
            p.addTriangle (cx + 3.f, cy - 5.f, cx - 4.f, cy, cx + 3.f, cy + 5.f);
        else
            p.addTriangle (cx - 3.f, cy - 5.f, cx + 4.f, cy, cx - 3.f, cy + 5.f);
        return p;
    };
    g.setColour (kMint);
    g.fillPath (chev (left, true));
    g.fillPath (chev (right, false));
    g.setColour (kLight);
    g.setFont (boldFont (12.f));
    g.drawText (text, r.toNearestInt(), juce::Justification::centred, true);
}

void fN0BAudioProcessorEditor::PresetBar::mouseUp (const juce::MouseEvent& e)
{
    if (! e.mouseWasClicked())
        return;
    if (e.x < 20)
    {
        if (onStep)
            onStep (-1);
    }
    else if (e.x > getWidth() - 20)
    {
        if (onStep)
            onStep (1);
    }
    else if (onOpen)
        onOpen();
}

// ---------------------------------------------------------------- Editor

fN0BAudioProcessorEditor::fN0BAudioProcessorEditor (fN0BAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    using namespace fn0b;
    setLookAndFeel (&panelLnF);
    setOpaque (true);
    content.setOpaque (true);

    panelLnF.setColour (juce::PopupMenu::backgroundColourId, kInk);
    panelLnF.setColour (juce::PopupMenu::textColourId, kLight);
    panelLnF.setColour (juce::PopupMenu::highlightedBackgroundColourId, kMint);
    panelLnF.setColour (juce::PopupMenu::highlightedTextColourId, kInk);
    panelLnF.setColour (juce::TooltipWindow::backgroundColourId, kInk);
    panelLnF.setColour (juce::TooltipWindow::textColourId, kLight);
    panelLnF.setColour (juce::TooltipWindow::outlineColourId, kMint);

    logo = juce::ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    knob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    knob.setTextBoxStyle (juce::Slider::TextBoxBelow, false, kTextBoxW, kTextBoxH);
    knob.setColour (juce::Slider::textBoxTextColourId, kLight);
    knob.setColour (juce::Slider::textBoxBackgroundColourId, kInk);
    knob.setColour (juce::Slider::textBoxOutlineColourId, kGrey);
    knob.setTooltip ("The knob. Everything the preset does is on this one control.");
    if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (ParamID::knob)))
        knob.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    knobAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, ParamID::knob, knob);
    content.addAndMakeVisible (knob);

    knobHint.setJustificationType (juce::Justification::centred);
    knobHint.setColour (juce::Label::textColourId, kGrey);
    knobHint.setFont (boldFont (14.f));
    content.addAndMakeVisible (knobHint);

    presetBar.setTooltip ("Preset. Arrows step, click for the list.");
    presetBar.onStep = [this] (int d) { stepPreset (d); };
    presetBar.onOpen = [this] { openPresetMenu(); };
    content.addAndMakeVisible (presetBar);

    refreshBar();

    baseW = kEditorW;
    baseH = kEditorH;
    content.setSize (baseW, baseH);
    addAndMakeVisible (content);

    // Drag the corner to scale (40%..150%), aspect ratio locked.
    setResizable (true, true);
    setResizeLimits (baseW * 2 / 5, baseH * 2 / 5, baseW * 3 / 2, baseH * 3 / 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / (double) baseH);
    setSize (baseW, baseH);
}

fN0BAudioProcessorEditor::~fN0BAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void fN0BAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (fn0b::kChassis);  // covers the sub-pixel edge the scaled content may leave
}

void fN0BAudioProcessorEditor::resized()
{
    if (baseW == 0)
        return;  // called before the constructor finished
    const float s = (float) getWidth() / (float) baseW;
    content.setTransform (juce::AffineTransform::scale (s));
    content.setBounds (0, 0, baseW, baseH);
}

void fN0BAudioProcessorEditor::paintContent (juce::Graphics& g)
{
    using namespace fn0b;
    g.fillAll (kChassis);

    auto r = content.getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    if (logo.isValid())
    {
        auto logoR = title.removeFromLeft (kTitleH).reduced (2);
        g.drawImage (logo, logoR.toFloat(), juce::RectanglePlacement::centred);
        title.removeFromLeft (6);
    }
    auto nameCol = title.removeFromLeft (kTitleHitW - kTitleH - 6);
    g.setColour (kAmber);
    g.setFont (boldFont (16.f));
    g.drawText ("fN0B", nameCol.removeFromTop (22), juce::Justification::centredLeft, false);
    g.setColour (kLight);
    g.setFont (boldFont (10.f));
    g.drawText ("v" JucePlugin_VersionString, nameCol, juce::Justification::centredLeft, false);

    g.setColour (kChassis.brighter (0.08f));
    g.drawRoundedRectangle (content.getLocalBounds().toFloat().reduced (1.5f), 2.f, 2.f);

    // The knob well, so the one control has a frame.
    auto well = knob.getBounds().toFloat().expanded (18.f, 10.f);
    g.setColour (kPanel);
    g.fillRoundedRectangle (well, 6.f);
    g.setColour (kGrey);
    g.drawRoundedRectangle (well.reduced (0.5f), 6.f, 1.f);

    // Decorative curve: the classic one-knob "more" ramp.
    auto ramp = content.getLocalBounds().reduced (kPad).removeFromBottom (34);
    juce::Path p;
    const float x0 = (float) ramp.getX(), x1 = (float) ramp.getRight();
    const float yb = (float) ramp.getBottom(), yt = (float) ramp.getY();
    p.startNewSubPath (x0, yb);
    for (int i = 0; i <= 24; ++i)
    {
        const float t = (float) i / 24.f;
        p.lineTo (x0 + (x1 - x0) * t, yb - (yb - yt) * t * t);
    }
    g.setColour (kMint.withAlpha (0.35f));
    g.strokePath (p, juce::PathStrokeType (1.5f));
}

void fN0BAudioProcessorEditor::layoutContent()
{
    auto r = content.getLocalBounds().reduced (kPad);
    auto title = r.removeFromTop (kTitleH);
    title.removeFromLeft (kTitleHitW);
    presetBar.setBounds (title.removeFromRight (240).reduced (2, 6));

    r.removeFromTop (4);
    auto knobArea = r.removeFromTop (r.getHeight() - 40);
    const int d = juce::jmin (knobArea.getWidth(), knobArea.getHeight() - kTextBoxH - kHintH - 8);
    knob.setBounds (knobArea.withSizeKeepingCentre (d, d + kTextBoxH).translated (0, -8));
    knobHint.setBounds (knob.getBounds().withY (knob.getBottom() + 2).withHeight (kHintH));
}

void fN0BAudioProcessorEditor::refreshBar()
{
    const int i = proc.presetIndex();
    presetBar.setText (fn0b::kPresetNames[i]);
    knobHint.setText (kHints[i], juce::dontSendNotification);
}

void fN0BAudioProcessorEditor::stepPreset (int delta)
{
    const int i = (proc.presetIndex() + delta + fn0b::kPresetCount) % fn0b::kPresetCount;
    if (auto* p = proc.apvts.getParameter (ParamID::preset))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) i));
        p->endChangeGesture();
    }
    refreshBar();
}

void fN0BAudioProcessorEditor::openPresetMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&panelLnF);  // top-level windows would otherwise use the default look
    for (int i = 0; i < fn0b::kPresetCount; ++i)
        m.addItem (i + 1, fn0b::kPresetNames[i], true, i == proc.presetIndex());

    juce::Component::SafePointer<fN0BAudioProcessorEditor> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (presetBar),
                     [safe] (int result)
                     {
                         if (safe == nullptr || result <= 0)
                             return;
                         safe->stepPreset (result - 1 - safe->proc.presetIndex());
                     });
}
