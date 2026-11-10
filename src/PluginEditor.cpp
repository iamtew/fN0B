#include "PluginEditor.h"

namespace
{
constexpr int kEditorW = 520;
constexpr int kEditorH = 400;
constexpr int kPad = 8;
constexpr int kTitleH = 40;
constexpr int kTitleHitW = 110;
constexpr int kTextBoxW = 56;
constexpr int kTextBoxH = 18;
constexpr int kHintH = 44;
constexpr int kHintGap = 14;
constexpr int kGainColW = 48;
constexpr int kGainLabelH = 14;
constexpr int kWellExpandX = 18;
constexpr int kWellExpandY = 10;

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
    const float tickW = juce::jmax (1.f, d * 0.018f);

    g.setColour (kAmber);
    constexpr int nTicks = 11;
    for (int i = 0; i < nTicks; ++i)
    {
        const float t = (float) i / (float) (nTicks - 1);
        const float a = startAngle + t * (endAngle - startAngle);
        const auto p0 = c.getPointOnCircumference (d * 0.48f, a);
        const auto p1 = c.getPointOnCircumference (d * 0.40f, a);
        g.drawLine (p0.x, p0.y, p1.x, p1.y, tickW);
    }

    // Shadow: two offset ellipses, not a DropShadow (which goes soft under the scale transform).
    const float bodyD = d * 0.72f;
    auto body = area.withSizeKeepingCentre (bodyD, bodyD);
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillEllipse (body.translated (0.f, d * 0.04f).expanded (d * 0.01f));
    g.setColour (juce::Colours::black.withAlpha (0.4f));
    g.fillEllipse (body.translated (0.f, d * 0.018f));

    const auto flange = kMint.darker (0.40f);
    g.setGradientFill (juce::ColourGradient (flange.brighter (0.14f), c.x, body.getY(),
                                             flange.darker (0.22f), c.x, body.getBottom(), false));
    g.fillEllipse (body);
    g.setColour (kMint.darker (0.55f).withAlpha (0.5f));
    g.drawEllipse (body.reduced (0.6f), juce::jmax (1.f, d * 0.012f));

    const float capD = bodyD * 0.5f;
    auto cap = area.withSizeKeepingCentre (capD, capD);
    g.setColour (kMint);
    g.fillEllipse (cap);

    const float angle = startAngle + pos * (endAngle - startAngle);
    juce::Path pointer;
    pointer.startNewSubPath (c.getPointOnCircumference (bodyD * 0.32f, angle));
    pointer.lineTo (c.getPointOnCircumference (bodyD * 0.46f, angle));
    g.setColour (kAmber);
    g.strokePath (pointer, juce::PathStrokeType (juce::jmax (1.15f, d * 0.028f),
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));

    const float pip = juce::jmax (1.6f, d * 0.045f);
    const auto pipC = c.getPointOnCircumference (capD * 0.28f, angle);
    g.fillEllipse (pipC.x - pip * 0.5f, pipC.y - pip * 0.5f, pip, pip);
}

void fN0BAudioProcessorEditor::PanelLnF::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h,
                                                           float sliderPos, float, float,
                                                           juce::Slider::SliderStyle, juce::Slider& slider)
{
    using namespace fn0b;
    auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    if (bounds.getHeight() < 8.f)
        return;

    const float wellW = 10.f;
    auto well = bounds.withSizeKeepingCentre (wellW, juce::jmax (8.f, bounds.getHeight() - 4.f));

    g.setColour (kInk);
    g.fillRoundedRectangle (well, 3.f);
    g.setColour (kGrey);
    g.drawRoundedRectangle (well.reduced (0.5f), 3.f, 1.f);

    const float meter = juce::jlimit (0.f, 1.f, (float) slider.getProperties().getWithDefault ("meter", 0.0));
    if (meter > 0.001f)
    {
        auto fill = well.reduced (1.5f);
        auto bar = fill.removeFromBottom (fill.getHeight() * meter);
        g.setColour (meter > 0.95f ? kAmber : kMint.withAlpha (0.85f));
        g.fillRoundedRectangle (bar, 2.f);
    }

    constexpr int nMarks = 7;
    for (int i = 1; i < nMarks; ++i)
    {
        const float yy = well.getY() + well.getHeight() * ((float) i / (float) nMarks);
        g.setColour (i == 1 ? kAmber.withAlpha (0.85f) : kGrey.withAlpha (0.55f));
        g.drawLine (well.getX() - 3.f, yy, well.getRight() + 3.f, yy, 1.f);
    }

    const float th = 5.f, tw = 16.f;
    auto thumb = juce::Rectangle<float> (bounds.getCentreX() - tw * 0.5f, sliderPos - th * 0.5f, tw, th);
    g.setColour (kAmber);
    g.fillRoundedRectangle (thumb, 2.f);
}

juce::Font fN0BAudioProcessorEditor::PanelLnF::getLabelFont (juce::Label& label)
{
    return label.getFont();
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
    if (logo.isValid())
        logo = logo.rescaled (kTitleH * 2, kTitleH * 2, juce::Graphics::highResamplingQuality);

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

    setupGainSlider (inGain, ParamID::inputGain, "Input gain. Hits the signal before the preset.");
    setupGainSlider (outGain, ParamID::outputGain, "Output gain. Hits the signal after the preset.");
    inGainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, ParamID::inputGain, inGain);
    outGainAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        proc.apvts, ParamID::outputGain, outGain);

    knobHint.setJustificationType (juce::Justification::centred);
    knobHint.setColour (juce::Label::textColourId, kGrey);
    knobHint.setFont (boldFont (16.f));
    knobHint.setMinimumHorizontalScale (1.f);
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

    startTimerHz (30);
}

void fN0BAudioProcessorEditor::setupGainSlider (juce::Slider& s, const char* paramId, const juce::String& tooltipText)
{
    using namespace fn0b;
    s.setSliderStyle (juce::Slider::LinearVertical);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, kGainColW, kTextBoxH);
    s.setColour (juce::Slider::textBoxTextColourId, kLight);
    s.setColour (juce::Slider::textBoxBackgroundColourId, kInk);
    s.setColour (juce::Slider::textBoxOutlineColourId, kGrey);
    s.setTooltip (tooltipText);
    if (auto* prm = dynamic_cast<juce::RangedAudioParameter*> (proc.apvts.getParameter (paramId)))
        s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    content.addAndMakeVisible (s);
}

fN0BAudioProcessorEditor::~fN0BAudioProcessorEditor()
{
    stopTimer();
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
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
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
    auto well = knob.getBounds().toFloat().expanded ((float) kWellExpandX, (float) kWellExpandY);
    g.setColour (kPanel);
    g.fillRoundedRectangle (well, 6.f);
    g.setColour (kGrey);
    g.drawRoundedRectangle (well.reduced (0.5f), 6.f, 1.f);

    g.setColour (kGrey);
    g.setFont (boldFont (10.f));
    g.drawText ("IN", juce::Rectangle<int> (inGain.getX(), inGain.getY() - kGainLabelH,
                                            inGain.getWidth(), kGainLabelH),
                juce::Justification::centred, false);
    g.drawText ("OUT", juce::Rectangle<int> (outGain.getX(), outGain.getY() - kGainLabelH,
                                             outGain.getWidth(), kGainLabelH),
                juce::Justification::centred, false);

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
    auto inner = content.getLocalBounds().reduced (kPad);
    auto title = inner.removeFromTop (kTitleH);
    knobHint.setBounds (inner.removeFromBottom (kHintH));
    inner.removeFromBottom (kWellExpandY + kHintGap);

    auto left = inner.removeFromLeft (kGainColW);
    auto right = inner.removeFromRight (kGainColW);
    left.removeFromTop (kGainLabelH);
    right.removeFromTop (kGainLabelH);
    inGain.setBounds (left);
    outGain.setBounds (right);

    const int d = juce::jmin (inner.getWidth(), inner.getHeight() - kTextBoxH);
    knob.setBounds (inner.withSizeKeepingCentre (d, d + kTextBoxH));

    auto well = knob.getBounds().expanded (kWellExpandX, 0);
    presetBar.setBounds (well.getX(), title.getY() + 6, well.getWidth(), title.getHeight() - 12);
}

void fN0BAudioProcessorEditor::timerCallback()
{
    auto toMeter = [] (float peak, float& vis)
    {
        vis = juce::jmax (peak, vis * 0.72f);
        const float db = juce::Decibels::gainToDecibels (vis, -60.f);
        return juce::jlimit (0.f, 1.f, (db + 60.f) / 60.f);
    };

    inGain.getProperties().set ("meter", toMeter (proc.takePeakIn(), inMeter));
    outGain.getProperties().set ("meter", toMeter (proc.takePeakOut(), outMeter));
    inGain.repaint();
    outGain.repaint();
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
