#include "PluginProcessor.h"
#include "PluginEditor.h"

#if defined(DYSMORPHIA_HAS_BINARY_DATA) && DYSMORPHIA_HAS_BINARY_DATA
  #include <BinaryData.h>
#endif

//==============================================================================
// DysmorphiaLookAndFeel
//==============================================================================
DysmorphiaLookAndFeel::DysmorphiaLookAndFeel()
{
    using namespace DysmorphiaColours;
    setColour (juce::Slider::thumbColourId,             sayaGreen);
    setColour (juce::Slider::rotarySliderFillColourId,  arterial);
    setColour (juce::Slider::rotarySliderOutlineColourId, fleshRed);
    setColour (juce::Label::textColourId,               boneWhite);

    // Text-box colours
    setColour (juce::Slider::textBoxTextColourId,
               boneWhite.withAlpha (0.7f));
    setColour (juce::Slider::textBoxBackgroundColourId,
               juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,
               juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId,
               sayaGreen.withAlpha (0.3f));
}

juce::Font DysmorphiaLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions ("Courier New", 10.0f,
                                          juce::Font::plain));
}

void DysmorphiaLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.setFont (getLabelFont (label));
    g.setColour (DysmorphiaColours::boneWhite.withAlpha (0.85f));
    g.drawText (label.getText(), label.getLocalBounds(),
                label.getJustificationType(), true);
}

void DysmorphiaLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                               int x, int y, int width, int height,
                                               float sliderPos,
                                               float startAngle, float endAngle,
                                               juce::Slider& /*slider*/)
{
    using namespace DysmorphiaColours;

    const float cx = static_cast<float>(x) + static_cast<float>(width)  * 0.5f;
    const float cy = static_cast<float>(y) + static_cast<float>(height) * 0.5f;
    const float r  = juce::jmin (width, height) * 0.40f;
    const float angle = startAngle + sliderPos * (endAngle - startAngle);

    //— Outer flesh ring ——————————————————————————————————————————————————
    {
        juce::Path ring;
        ring.addEllipse (cx - r - 4.f, cy - r - 4.f,
                         (r + 4.f) * 2.f, (r + 4.f) * 2.f);
        g.setColour (fleshRed.withAlpha (0.55f));
        g.strokePath (ring, juce::PathStrokeType (1.5f));
    }

    //— Knob body ——————————————————————————————————————————————————————————
    {
        juce::Path body;
        body.addEllipse (cx - r, cy - r, r * 2.f, r * 2.f);
        juce::ColourGradient grad (bloodDeep, cx, cy - r,
                                   voidBlack,  cx, cy + r, false);
        g.setGradientFill (grad);
        g.fillPath (body);

        g.setColour (arterial.withAlpha (0.35f));
        g.strokePath (body, juce::PathStrokeType (1.0f));
    }

    //— Value arc ——————————————————————————————————————————————————————————
    {
        juce::Path arc;
        arc.addArc (cx - r + 4.f, cy - r + 4.f,
                    (r - 4.f) * 2.f, (r - 4.f) * 2.f,
                    startAngle, angle, true);
        g.setColour (sayaGreen.withAlpha (0.9f));
        g.strokePath (arc, juce::PathStrokeType (2.5f,
                           juce::PathStrokeType::curved,
                           juce::PathStrokeType::rounded));
    }

    //— Remaining arc (track) ——————————————————————————————————————————————
    {
        juce::Path track;
        track.addArc (cx - r + 4.f, cy - r + 4.f,
                      (r - 4.f) * 2.f, (r - 4.f) * 2.f,
                      angle, endAngle, true);
        g.setColour (fleshRed.withAlpha (0.25f));
        g.strokePath (track, juce::PathStrokeType (1.0f));
    }

    //— Eye glow in centre ——————————————————————————————————————————————————
    {
        const float eyeR = r * 0.30f;
        juce::ColourGradient eyeGrad (sayaGreen.withAlpha (0.8f), cx, cy,
                                      sickGreen.withAlpha (0.0f),
                                      cx + eyeR, cy, false);
        g.setGradientFill (eyeGrad);
        g.fillEllipse (cx - eyeR, cy - eyeR, eyeR * 2.f, eyeR * 2.f);
    }

    //— Pointer line ————————————————————————————————————————————————————————
    {
        const float px = cx + (r - 6.f) * std::sin (angle);
        const float py = cy - (r - 6.f) * std::cos (angle);
        g.setColour (boneWhite.withAlpha (0.9f));
        g.drawLine (cx, cy, px, py, 1.8f);
        // Tip dot
        g.fillEllipse (px - 1.5f, py - 1.5f, 3.f, 3.f);
    }

    //— Tendon bumps around perimeter ——————————————————————————————————————
    const int numBumps = 10;
    for (int i = 0; i < numBumps; ++i)
    {
        const float a  = startAngle + static_cast<float>(i) /
                         static_cast<float>(numBumps) * (endAngle - startAngle);
        const float bx = cx + (r + 5.f) * std::sin (a);
        const float by = cy - (r + 5.f) * std::cos (a);
        g.setColour (tendon.withAlpha (0.45f));
        g.fillEllipse (bx - 1.5f, by - 1.5f, 3.f, 3.f);
    }
}

//==============================================================================
// SpectrumVisualizer
//==============================================================================
SpectrumVisualizer::SpectrumVisualizer (DysmorphiaAudioProcessor& p)
    : processor (p)
{
    startTimerHz (30);
}

void SpectrumVisualizer::resized() {}

void SpectrumVisualizer::timerCallback()
{
    for (int i = 0; i < 3; ++i)
    {
        inLevels[i]  = processor.spectrumData[i].load();
        outLevels[i] = processor.spectrumData[i + 3].load();
    }
    animPhase += 0.05f;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;
    ++frameCount;
    repaint();
}

void SpectrumVisualizer::drawFleshBand (juce::Graphics& g,
                                         float bx, float by, float bw, float bh,
                                         float level, juce::Colour col, bool isOut)
{
    using namespace DysmorphiaColours;

    const float clamped = juce::jlimit (0.f, 1.f, level * 5.f);
    const float fillH   = clamped * bh;
    const float fillY   = by + bh - fillH;

    // Dark track
    g.setColour (bloodDeep.withAlpha (0.7f));
    g.fillRect (bx, by, bw, bh);

    // Filled bar
    if (fillH > 0.5f)
    {
        juce::ColourGradient grad (col, bx, fillY,
                                   col.withAlpha (0.2f), bx, fillY + fillH,
                                   false);
        g.setGradientFill (grad);
        g.fillRect (bx, fillY, bw, fillH);

        // Animated vein lines inside bar
        for (int v = 0; v < 3; ++v)
        {
            const float vy   = fillY + fillH * (static_cast<float>(v + 1) / 4.f);
            const float wave = std::sin (animPhase * 2.3f +
                                         static_cast<float>(v) * 1.1f) * 2.0f;
            g.setColour (col.brighter (0.5f).withAlpha (0.4f));
            g.drawLine (bx, vy + wave, bx + bw, vy - wave * 0.5f, 0.7f);
        }
    }

    // Border
    g.setColour (isOut ? sayaGreen.withAlpha (0.5f)
                       : arterial.withAlpha (0.5f));
    g.drawRect (bx, by, bw, bh, 1.0f);
}

void SpectrumVisualizer::paint (juce::Graphics& g)
{
    using namespace DysmorphiaColours;

    const auto  bounds = getLocalBounds().toFloat();
    const float W      = bounds.getWidth();
    const float H      = bounds.getHeight();

    //— Background —————————————————————————————————————————————————————————
    g.setColour (voidBlack.withAlpha (0.88f));
    g.fillRoundedRectangle (bounds, 4.f);

    //— Breathing border ————————————————————————————————————————————————————
    const float breathe = 0.5f + 0.5f * std::sin (animPhase);
    g.setColour (arterial.withAlpha (0.25f + 0.15f * breathe));
    g.drawRoundedRectangle (bounds.reduced (1.f), 4.f, 1.5f);

    //— Title text inside visualiser ————————————————————————————————————————
    {
        g.setFont (juce::Font (juce::FontOptions (
            "Courier New", 7.5f, juce::Font::plain)));
        g.setColour (arterial.withAlpha (0.55f));
        g.drawText ("IN", 6, 2, 16, 10, juce::Justification::left,   false);
        g.setColour (sayaGreen.withAlpha (0.55f));
        g.drawText ("OUT", static_cast<int>(W) - 26, 2, 24, 10,
                    juce::Justification::right, false);
    }

    //— Animated tendon lines across top ————————————————————————————————————
    for (int t = 0; t < 4; ++t)
    {
        const float ty = 5.f + static_cast<float>(t) * 3.f;
        juce::Path td;
        td.startNewSubPath (0.f, ty);
        for (float px = 0.f; px <= W; px += 10.f)
        {
            const float wy = ty + std::sin (animPhase + px * 0.04f +
                                             static_cast<float>(t) * 0.8f) * 2.f;
            td.lineTo (px, wy);
        }
        g.setColour (tendon.withAlpha (0.12f));
        g.strokePath (td, juce::PathStrokeType (0.7f));
    }

    //— Band layout ————————————————————————————————————————————————————————
    // 3 groups (LOW / MID / HI), each group = 2 narrow bars (in | out)
    const float bandW    = 20.f;
    const float bandH    = H - 28.f;
    const float bandTop  = 16.f;
    const float pairGap  =  3.f;   // gap between in/out within group
    const float groupGap = 18.f;   // gap between groups

    const float totalW = 3.f * (bandW * 2.f + pairGap) + 2.f * groupGap;
    float gx = (W - totalW) * 0.5f;

    const juce::Colour inCols[]  = { arterial,   viscera,    fleshRed  };
    const juce::Colour outCols[] = { sayaGreen,  sickGreen,  eyeGlow   };
    const char* bandNames[]      = { "LOW",      "MID",      "HI"      };

    for (int band = 0; band < 3; ++band)
    {
        // In bar
        drawFleshBand (g, gx,               bandTop, bandW, bandH,
                       inLevels[band],  inCols[band],  false);
        // Out bar
        drawFleshBand (g, gx + bandW + pairGap, bandTop, bandW, bandH,
                       outLevels[band], outCols[band], true);

        // Band label
        g.setFont (juce::Font (juce::FontOptions (
            "Courier New", 8.f, juce::Font::plain)));
        g.setColour (boneWhite.withAlpha (0.55f));
        g.drawText (bandNames[band],
                    static_cast<int>(gx),
                    static_cast<int>(bandTop + bandH + 2),
                    static_cast<int>(bandW * 2.f + pairGap), 10,
                    juce::Justification::centred, false);

        gx += bandW * 2.f + pairGap + groupGap;
    }

    //— Occasional glitch scanline ——————————————————————————————————————————
    if ((frameCount % 90) < 3)
    {
        for (int y = 0; y < static_cast<int>(H); y += 3)
        {
            g.setColour (voidBlack.withAlpha (0.12f));
            g.fillRect (0.f, static_cast<float>(y), W, 1.f);
        }
    }
}

//==============================================================================
// DysmorphiaAudioProcessorEditor
//==============================================================================
DysmorphiaAudioProcessorEditor::DysmorphiaAudioProcessorEditor (
    DysmorphiaAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), specViz (p)
{
    setLookAndFeel (&lnf);

    //— Load background image ——————————————————————————————————————————————
#if defined(DYSMORPHIA_HAS_BINARY_DATA) && DYSMORPHIA_HAS_BINARY_DATA
    bgImage = juce::ImageFileFormat::loadFrom (
        BinaryData::saya_jpg,
        static_cast<size_t> (BinaryData::saya_jpgSize));
#else
    // Fallback: look for saya.jpg next to the plugin on disk
    bgImage = juce::ImageFileFormat::loadFrom (
        juce::File::getSpecialLocation (
            juce::File::currentApplicationFile)
                .getSiblingFile ("Resources/saya.jpg"));
#endif

    addAndMakeVisible (specViz);

    //— Sliders ————————————————————————————————————————————————————————————
    setupSlider (rateSlider,    rateLabel,    "RATE");
    setupSlider (depthSlider,   depthLabel,   "DEPTH");
    setupSlider (densitySlider, densityLabel, "DENSITY");
    setupSlider (spreadSlider,  spreadLabel,  "SPREAD");
    setupSlider (mixSlider,     mixLabel,     "MIX");

    setupSlider (chaosSlider,   chaosLabel,   "CHAOS");
    setupSlider (xoverSlider,   xoverLabel,   "X-OVER");
    setupSlider (specOffSlider, specOffLabel, "SPEC.OFF");
    setupSlider (widthSlider,   widthLabel,   "WIDTH");
    setupSlider (gainSlider,    gainLabel,    "GAIN");

    //— APVTS Attachments ——————————————————————————————————————————————————
    auto& apvts = audioProcessor.apvts;
    rateAtt    = std::make_unique<Attachment> (apvts, "rate",    rateSlider);
    depthAtt   = std::make_unique<Attachment> (apvts, "depth",   depthSlider);
    densityAtt = std::make_unique<Attachment> (apvts, "density", densitySlider);
    spreadAtt  = std::make_unique<Attachment> (apvts, "spread",  spreadSlider);
    mixAtt     = std::make_unique<Attachment> (apvts, "mix",     mixSlider);
    chaosAtt   = std::make_unique<Attachment> (apvts, "chaos",   chaosSlider);
    xoverAtt   = std::make_unique<Attachment> (apvts, "xover",   xoverSlider);
    specOffAtt = std::make_unique<Attachment> (apvts, "specOff", specOffSlider);
    widthAtt   = std::make_unique<Attachment> (apvts, "width",   widthSlider);
    gainAtt    = std::make_unique<Attachment> (apvts, "gain",    gainSlider);

    //— Title ——————————————————————————————————————————————————————————————
    titleLabel.setText  ("DYSMORPHIA", juce::dontSendNotification);
    vendorLabel.setText ("kyoa",       juce::dontSendNotification);

    titleLabel.setFont (juce::Font (juce::FontOptions (
        "Courier New", 24.f, juce::Font::bold)));
    vendorLabel.setFont (juce::Font (juce::FontOptions (
        "Courier New", 9.f,  juce::Font::plain)));

    titleLabel.setColour  (juce::Label::textColourId,
                           DysmorphiaColours::sayaGreen);
    vendorLabel.setColour (juce::Label::textColourId,
                           DysmorphiaColours::boneWhite.withAlpha (0.45f));

    titleLabel.setJustificationType  (juce::Justification::centred);
    vendorLabel.setJustificationType (juce::Justification::centred);

    addAndMakeVisible (titleLabel);
    addAndMakeVisible (vendorLabel);

    //— Section labels —————————————————————————————————————————————————————
    granularLabel.setText ("[ GRANULAR CLOUD ]", juce::dontSendNotification);
    spectralLabel.setText ("[ FREQ. SPLIT ]",    juce::dontSendNotification);
    chaosSecLabel.setText ("[ CHAOS / ENTROPY ]",juce::dontSendNotification);

    for (auto* lbl : { &granularLabel, &spectralLabel, &chaosSecLabel })
    {
        lbl->setFont (juce::Font (juce::FontOptions (
            "Courier New", 8.f, juce::Font::plain)));
        lbl->setColour (juce::Label::textColourId,
                        DysmorphiaColours::arterial.withAlpha (0.75f));
        lbl->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (lbl);
    }

    setSize (880, 560);
    startTimerHz (30);
}

DysmorphiaAudioProcessorEditor::~DysmorphiaAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void DysmorphiaAudioProcessorEditor::setupSlider (juce::Slider& s,
                                                   juce::Label&  lbl,
                                                   const juce::String& text)
{
    s.setSliderStyle      (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle     (juce::Slider::TextBoxBelow, false, 56, 14);
    s.setTextBoxIsEditable (true);

    addAndMakeVisible (s);

    lbl.setText (text, juce::dontSendNotification);
    lbl.setJustificationType (juce::Justification::centred);
    lbl.attachToComponent (&s, false);
    addAndMakeVisible (lbl);
}

//==============================================================================
void DysmorphiaAudioProcessorEditor::resized()
{
    const int W = getWidth();

    //— Top bar ————————————————————————————————————————————————————————————
    titleLabel .setBounds (W / 2 - 130, 7, 260, 30);
    vendorLabel.setBounds (W / 2 -  50, 37, 100, 13);

    //— Spectrum visualiser ————————————————————————————————————————————————
    specViz.setBounds (10, 53, W - 20, 82);

    //— Knob grid: 5 columns × 2 rows ——————————————————————————————————————
    const int knobW     = 74;
    const int knobH     = 78;
    const int labelH    = 15;
    const int colGap    = 22;
    const int row1Y     = 158;   // top of knob body (label sits above)
    const int row2Y     = 295;

    const int totalW = 5 * knobW + 4 * colGap;
    const int startX = (W - totalW) / 2;

    auto place = [&](juce::Slider& sl, int col, int rowY)
    {
        sl.setBounds (startX + col * (knobW + colGap),
                      rowY + labelH,
                      knobW, knobH);
    };

    // Row 1: granular cloud + mix
    place (rateSlider,    0, row1Y);
    place (depthSlider,   1, row1Y);
    place (densitySlider, 2, row1Y);
    place (spreadSlider,  3, row1Y);
    place (mixSlider,     4, row1Y);

    // Row 2: chaos / spectral / output
    place (chaosSlider,   0, row2Y);
    place (xoverSlider,   1, row2Y);
    place (specOffSlider, 2, row2Y);
    place (widthSlider,   3, row2Y);
    place (gainSlider,    4, row2Y);

    //— Section labels —————————————————————————————————————————————————————
    const int secLblH = 13;
    granularLabel.setBounds (startX,                         row1Y - secLblH - 2,
                             3 * (knobW + colGap) - colGap, secLblH);
    spectralLabel.setBounds (startX + 3 * (knobW + colGap), row1Y - secLblH - 2,
                             2 * (knobW + colGap) - colGap, secLblH);
    chaosSecLabel.setBounds (startX,                         row2Y - secLblH - 2,
                             totalW,                         secLblH);
}

//==============================================================================
void DysmorphiaAudioProcessorEditor::timerCallback()
{
    pulsePhase += 0.025f;
    if (pulsePhase > juce::MathConstants<float>::twoPi)
        pulsePhase -= juce::MathConstants<float>::twoPi;

    // Stochastic glitch trigger (roughly every 4-8 seconds at 30 fps)
    glitchTimer += 1.f / 30.f;
    if (!glitchActive && glitchTimer > 4.f)
    {
        glitchTimer = 0.f;
        if ((rng() % 4) == 0)
        {
            glitchActive = true;
            glitchFrame  = 0;
        }
    }
    if (glitchActive)
    {
        if (++glitchFrame > 7)
        {
            glitchActive = false;
            glitchFrame  = 0;
        }
    }

    repaint();
}

//==============================================================================
void DysmorphiaAudioProcessorEditor::drawSectionBorder (
    juce::Graphics& g, juce::Rectangle<int> r,
    juce::Colour col, float pulse) const
{
    const float alpha = 0.22f + 0.12f * pulse;
    g.setColour (col.withAlpha (alpha));

    const float rx = static_cast<float>(r.getX());
    const float ry = static_cast<float>(r.getY());
    const float rw = static_cast<float>(r.getWidth());
    const float rh = static_cast<float>(r.getHeight());

    juce::Path border;
    border.startNewSubPath (rx + 8.f, ry);
    border.lineTo (rx + rw - 8.f, ry);
    border.lineTo (rx + rw,       ry + 8.f);
    border.lineTo (rx + rw,       ry + rh - 8.f);
    border.lineTo (rx + rw - 8.f, ry + rh);
    border.lineTo (rx + 8.f,      ry + rh);
    border.lineTo (rx,            ry + rh - 8.f);
    border.lineTo (rx,            ry + 8.f);
    border.closeSubPath();

    g.strokePath (border, juce::PathStrokeType (1.2f));
}

void DysmorphiaAudioProcessorEditor::drawFleshVeins (
    juce::Graphics& g, juce::Rectangle<int> bounds,
    float phase, float alpha) const
{
    const float bx = static_cast<float>(bounds.getX());
    const float by = static_cast<float>(bounds.getY());
    const float bw = static_cast<float>(bounds.getWidth());
    const float bh = static_cast<float>(bounds.getHeight());

    std::mt19937 vRng (42u);
    for (int v = 0; v < 14; ++v)
    {
        const float t0 = static_cast<float>(vRng() % 1000) / 1000.f;
        const float t1 = static_cast<float>(vRng() % 1000) / 1000.f;
        const float sx = bx + t0 * bw;
        const float sy = by + t1 * bh;

        const float dx = static_cast<float>(static_cast<int>(vRng() % 240) - 120);
        const float dy = static_cast<float>(static_cast<int>(vRng() % 240) - 120);

        const float wave = std::sin (phase + static_cast<float>(v) * 0.53f) * 5.f;
        const float ex   = juce::jlimit (bx, bx + bw, sx + dx);
        const float ey   = juce::jlimit (by, by + bh, sy + dy);

        juce::Path vein;
        vein.startNewSubPath (sx, sy);
        vein.quadraticTo (sx + (ex - sx) * 0.5f + wave,
                          sy + (ey - sy) * 0.5f + wave,
                          ex, ey);

        g.setColour (DysmorphiaColours::arterial.withAlpha (alpha * 0.30f));
        g.strokePath (vein, juce::PathStrokeType (0.65f));
    }
}

void DysmorphiaAudioProcessorEditor::drawOrganicNoise (
    juce::Graphics& g, juce::Rectangle<int> bounds, float phase) const
{
    const int bh = bounds.getHeight();
    const int bx = bounds.getX();
    const int bw = bounds.getWidth();

    for (int y = 0; y < bh; y += 4)
    {
        const float v = std::sin (static_cast<float>(y) * 0.09f + phase * 1.7f)
                        * 0.5f + 0.5f;
        g.setColour (DysmorphiaColours::voidBlack.withAlpha (v * 0.035f));
        g.fillRect (bx, bounds.getY() + y, bw, 1);
    }
}

//==============================================================================
void DysmorphiaAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace DysmorphiaColours;

    const auto bounds = getLocalBounds();
    const float W = static_cast<float>(getWidth());
    const float H = static_cast<float>(getHeight());

    //— 1. Background image (saya.jpg) —————————————————————————————————————
    if (bgImage.isValid())
    {
        g.drawImage (bgImage, bounds.toFloat(),
                     juce::RectanglePlacement::stretchToFit);
        // Heavy void overlay so image becomes an eerie texture
        g.setColour (voidBlack.withAlpha (0.70f));
        g.fillRect  (bounds);
    }
    else
    {
        // Fallback: flesh gradient
        juce::ColourGradient fallback (bloodDeep, 0.f, 0.f,
                                       voidBlack,  W, H, false);
        g.setGradientFill (fallback);
        g.fillRect (bounds);
    }

    //— 2. Vein overlay ————————————————————————————————————————————————————
    drawFleshVeins (g, bounds, pulsePhase, 0.65f);

    //— 3. Scanline noise ——————————————————————————————————————————————————
    drawOrganicNoise (g, bounds, pulsePhase);

    //— 4. Title bar bg ————————————————————————————————————————————————————
    {
        juce::ColourGradient titleBg (bloodDeep.withAlpha (0.92f), 0.f,  0.f,
                                      voidBlack.withAlpha (0.92f), 0.f, 52.f,
                                      false);
        g.setGradientFill (titleBg);
        g.fillRect (0.f, 0.f, W, 52.f);

        const float lineAlpha = 0.45f + 0.3f * std::sin (pulsePhase);
        g.setColour (arterial.withAlpha (lineAlpha));
        g.drawLine (0.f, 52.f, W, 52.f, 2.0f);

        // Subtle eye motif watermark in title area
        const float eyeCX = W * 0.5f;
        const float eyeCY = 26.f;
        const float eyeRX = 40.f, eyeRY = 10.f;
        juce::Path eyePath;
        eyePath.addEllipse (eyeCX - eyeRX, eyeCY - eyeRY,
                            eyeRX * 2.f, eyeRY * 2.f);
        g.setColour (sayaGreen.withAlpha (0.06f));
        g.fillPath (eyePath);
    }

    //— 5. Section border panels ———————————————————————————————————————————
    const float ps = std::sin (pulsePhase);

    // Visualiser panel
    drawSectionBorder (g, { 10, 53, getWidth() - 20, 82 },
                       arterial, ps);

    // Row-1 left  (Granular Cloud: rate/depth/density/spread)
    {
        const int knobW  = 74, colGap = 22;
        const int totalW = 5 * knobW + 4 * colGap;
        const int startX = (getWidth() - totalW) / 2;
        drawSectionBorder (g,
            { startX - 6, 143,
              3 * (knobW + colGap) - colGap + 12, 145 },
            viscera, ps);
        drawSectionBorder (g,
            { startX + 3 * (knobW + colGap) - 6, 143,
              2 * (knobW + colGap) - colGap + 12, 145 },
            fleshRed, ps);
        // Row-2 panel (Chaos/Output)
        drawSectionBorder (g,
            { startX - 6, 280, totalW + 12, 145 },
            sayaGreen.withAlpha (0.5f), -ps);
    }

    //— 6. Glitch effect (analog-horror) ———————————————————————————————————
    if (glitchActive)
    {
        std::mt19937 gRng (static_cast<unsigned> (glitchFrame * 13 + 7));
        const int numBands = 3 + static_cast<int>(gRng() % 5);

        for (int i = 0; i < numBands; ++i)
        {
            const int gy     = static_cast<int>(gRng() % static_cast<unsigned>(getHeight()));
            const int gh     = 1 + static_cast<int>(gRng() % 7);
            const int gshift = static_cast<int>(gRng() % 16) - 8;

            g.setColour (sayaGreen.withAlpha (0.12f));
            g.fillRect (gshift, gy, getWidth(), gh);

            g.setColour (arterial.withAlpha (0.09f));
            g.fillRect (-gshift, gy + 1, getWidth(), gh);
        }

        // Occasional full horizontal smear
        if ((gRng() % 3) == 0)
        {
            const int smY = static_cast<int>(gRng() % static_cast<unsigned>(getHeight()));
            g.setColour (voidBlack.withAlpha (0.4f));
            g.fillRect (0, smY, getWidth(), 2);
        }
    }

    //— 7. Bottom status bar ———————————————————————————————————————————————
    {
        g.setColour (bloodDeep.withAlpha (0.88f));
        g.fillRect (0.f, H - 22.f, W, 22.f);

        // Pulsing separator line
        g.setColour (arterial.withAlpha (0.35f + 0.2f * ps));
        g.drawLine (0.f, H - 22.f, W, H - 22.f, 1.0f);

        g.setFont (juce::Font (juce::FontOptions (
            "Courier New", 8.f, juce::Font::plain)));

        // Alternate status text that slowly drifts
        const bool blink = (static_cast<int>(pulsePhase * 2.5f) % 2 == 0);
        const juce::String status = blink
            ? "... the world is beautiful ..."
            : "granular cloud  |  spectral split  |  entropy engine  |  kyoa";

        g.setColour (arterial.withAlpha (0.5f));
        g.drawText (status, 10, static_cast<int>(H) - 20,
                    getWidth() - 20, 18,
                    juce::Justification::centred, false);
    }
}
