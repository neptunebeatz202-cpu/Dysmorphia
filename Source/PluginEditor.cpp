#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

//==============================================================================
// Palette
const juce::Colour ChobitsLookAndFeel::bgDeep        = juce::Colour (0xff0e0520);
const juce::Colour ChobitsLookAndFeel::bgMid          = juce::Colour (0xff2d1b5e);
const juce::Colour ChobitsLookAndFeel::starGold        = juce::Colour (0xffffd700);
const juce::Colour ChobitsLookAndFeel::pinkAccent      = juce::Colour (0xffff6b9d);
const juce::Colour ChobitsLookAndFeel::silhouetteBlue  = juce::Colour (0xff3a4a7a);
const juce::Colour ChobitsLookAndFeel::hairGold        = juce::Colour (0xffe8c56d);
const juce::Colour ChobitsLookAndFeel::panelFill       = juce::Colour (0xd00d0620);

//==============================================================================
ChobitsLookAndFeel::ChobitsLookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId,    starGold);
    setColour (juce::Slider::rotarySliderOutlineColourId, bgMid);
    setColour (juce::Slider::thumbColourId,               pinkAccent);
    setColour (juce::Slider::textBoxTextColourId,         hairGold);
    setColour (juce::Slider::textBoxBackgroundColourId,   juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    setColour (juce::ComboBox::backgroundColourId,        panelFill);
    setColour (juce::ComboBox::outlineColourId,           pinkAccent);
    setColour (juce::ComboBox::textColourId,              starGold);
    setColour (juce::ComboBox::arrowColourId,             starGold);
    setColour (juce::PopupMenu::backgroundColourId,       bgDeep);
    setColour (juce::PopupMenu::textColourId,             starGold);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, bgMid);
    setColour (juce::PopupMenu::highlightedTextColourId,  pinkAccent);
    setColour (juce::TextButton::buttonColourId,          panelFill);
    setColour (juce::TextButton::buttonOnColourId,        pinkAccent.withAlpha(0.8f));
    setColour (juce::TextButton::textColourOffId,         hairGold);
    setColour (juce::TextButton::textColourOnId,          juce::Colours::white);
    setColour (juce::ToggleButton::tickColourId,          pinkAccent);
    setColour (juce::ToggleButton::tickDisabledColourId,  bgMid);
    setColour (juce::Label::textColourId,                 hairGold);
    setColour (juce::Label::backgroundColourId,           juce::Colours::transparentBlack);
}

//==============================================================================
void ChobitsLookAndFeel::drawRotarySlider (juce::Graphics& g,
    int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    float cx = x + width*0.5f, cy = y + height*0.5f;
    float r  = juce::jmin(width,height)*0.38f;
    float angle = startAngle + sliderPos*(endAngle-startAngle);

    // Backing
    g.setColour (panelFill.withAlpha(0.85f));
    g.fillEllipse (cx-r, cy-r, r*2.f, r*2.f);
    g.setColour (bgMid.withAlpha(0.9f));
    g.drawEllipse (cx-r-1, cy-r-1, (r+1)*2.f, (r+1)*2.f, 1.5f);

    // Filled arc
    {
        juce::Path arc;
        arc.addArc (cx-r, cy-r, r*2.f, r*2.f, startAngle, angle, true);
        g.setColour (starGold.withAlpha(0.9f));
        g.strokePath (arc, juce::PathStrokeType(3.f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }
    // Pink highlight
    {
        juce::Path hi;
        float span=0.20f;
        hi.addArc (cx-r*0.86f, cy-r*0.86f, r*1.72f, r*1.72f, angle-span, angle+span, true);
        g.setColour (pinkAccent.withAlpha(0.5f));
        g.strokePath (hi, juce::PathStrokeType(1.6f));
    }
    // Inner glow
    {
        juce::ColourGradient glow (hairGold.withAlpha(0.22f), cx, cy,
                                    bgDeep.withAlpha(0.f), cx+r*0.6f, cy+r*0.6f, true);
        g.setGradientFill (glow);
        g.fillEllipse (cx-r*0.58f, cy-r*0.58f, r*1.16f, r*1.16f);
    }
    // Thumb
    float tx = cx + r*0.78f*std::sin(angle);
    float ty = cy - r*0.78f*std::cos(angle);
    g.setColour (pinkAccent);
    g.fillEllipse (tx-4.f, ty-4.f, 8.f, 8.f);
    g.setColour (juce::Colours::white.withAlpha(0.85f));
    g.fillEllipse (tx-1.8f, ty-1.8f, 3.6f, 3.6f);
}

void ChobitsLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& btn,
    const juce::Colour&, bool highlighted, bool down)
{
    auto b = btn.getLocalBounds().toFloat().reduced(1.f);
    g.setColour (down ? pinkAccent.withAlpha(0.85f) : highlighted ? bgMid.brighter(0.15f) : panelFill);
    g.fillRoundedRectangle (b, 5.f);
    g.setColour (btn.getToggleState() ? pinkAccent : starGold.withAlpha(0.5f));
    g.drawRoundedRectangle (b, 5.f, 1.3f);
    if (btn.getToggleState())
    {
        juce::ColourGradient glow (pinkAccent.withAlpha(0.25f), b.getCentreX(), b.getCentreY(),
                                    pinkAccent.withAlpha(0.f), b.getRight(), b.getBottom(), true);
        g.setGradientFill (glow);
        g.fillRoundedRectangle (b, 5.f);
    }
}

void ChobitsLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& btn, bool, bool)
{
    g.setColour (btn.getToggleState() ? juce::Colours::white : hairGold);
    g.setFont (juce::Font("Georgia", 10.5f, juce::Font::bold));
    g.drawFittedText (btn.getButtonText(), btn.getLocalBounds(), juce::Justification::centred, 1);
}

void ChobitsLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h,
    bool, int, int, int, int, juce::ComboBox&)
{
    auto b = juce::Rectangle<float>(0,0,(float)w,(float)h);
    g.setColour (panelFill.withAlpha(0.88f));
    g.fillRoundedRectangle (b, 5.f);
    g.setColour (pinkAccent.withAlpha(0.7f));
    g.drawRoundedRectangle (b.reduced(0.5f), 5.f, 1.3f);
    juce::Path arrow;
    float ax=w-13.f, ay=h*0.5f-3.f;
    arrow.addTriangle (ax,ay, ax+7.f,ay, ax+3.5f,ay+5.5f);
    g.setColour (starGold);
    g.fillPath (arrow);
}

void ChobitsLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    g.fillAll (label.findColour(juce::Label::backgroundColourId));
    if (!label.isBeingEdited())
    {
        g.setColour (hairGold.withMultipliedAlpha(label.isEnabled() ? 1.f : 0.5f));
        g.setFont (label.getFont());
        g.drawFittedText (label.getText(),
                          getLabelBorderSize(label).subtractedFrom(label.getLocalBounds()),
                          label.getJustificationType(), 2,
                          label.getMinimumHorizontalScale());
    }
}

//==============================================================================
// Helpers
static void initKnob (juce::Slider& s)
{
    s.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 54, 13);
    s.setColour (juce::Slider::textBoxTextColourId,       ChobitsLookAndFeel::hairGold);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxOutlineColourId,    juce::Colours::transparentBlack);
}

static void initLabel (juce::Label& l, const juce::String& text)
{
    l.setText (text, juce::dontSendNotification);
    l.setFont (juce::Font("Georgia", 9.f, juce::Font::bold));
    l.setJustificationType (juce::Justification::centred);
    l.setColour (juce::Label::textColourId,       ChobitsLookAndFeel::hairGold);
    l.setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
}

//==============================================================================
PersoFormantAudioProcessorEditor::PersoFormantAudioProcessorEditor (PersoFormantAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel (&laf);
    bgImage = juce::ImageCache::getFromMemory (BinaryData::chobits_jpg,
                                               BinaryData::chobits_jpgSize);
    // Wider plugin so Chi sits in the middle with controls on both sides
    setSize (1000, 440);
    setResizable (false, false);

    // ---- Knobs ----
    for (auto* s : { &morphSlider, &lfoRateSlider, &lfoDepthSlider,
                     &envThreshSlider, &envDepthSlider,
                     &driveSlider, &wetGainSlider, &mixSlider,
                     &formantResSlider, &formantShiftSlider })
        initKnob (*s);

    for (auto* s : { &morphSlider, &lfoRateSlider, &lfoDepthSlider,
                     &envThreshSlider, &envDepthSlider,
                     &driveSlider, &wetGainSlider, &mixSlider,
                     &formantResSlider, &formantShiftSlider })
        addAndMakeVisible (*s);

    // ---- Labels ----
    initLabel (morphLabel,       "MORPH");
    initLabel (lfoRateLabel,     "LFO RATE");
    initLabel (lfoDepthLabel,    "DEPTH");
    initLabel (lfoSyncLabel,     "BPM SYNC");
    initLabel (envThreshLabel,   "THRESHOLD");
    initLabel (envDepthLabel,    "ENV→MORPH");
    initLabel (formantResLabel,  "RESONANCE");
    initLabel (formantShiftLabel,"SHIFT");
    initLabel (driveLabel,       "DRIVE");
    initLabel (wetGainLabel,     "WET GAIN");
    initLabel (mixLabel,         "MIX");
    initLabel (vowel1Label,      "VOWEL A");
    initLabel (vowel2Label,      "VOWEL B");
    initLabel (genderLabel,      "GENDER");

    for (auto* l : { &morphLabel, &lfoRateLabel, &lfoDepthLabel, &lfoSyncLabel,
                     &envThreshLabel, &envDepthLabel,
                     &formantResLabel, &formantShiftLabel,
                     &driveLabel, &wetGainLabel, &mixLabel,
                     &vowel1Label, &vowel2Label, &genderLabel })
        addAndMakeVisible (*l);

    // ---- Vowel combo boxes (12 presets) ----
    for (auto* box : { &vowel1Box, &vowel2Box })
    {
        for (int i = 0; i < 12; ++i)
            box->addItem (kVowelNames[i], i+1);
        box->setSelectedId (1);
        addAndMakeVisible (*box);
    }
    vowel2Box.setSelectedId (3);

    // Gender box
    genderBox.addItem (juce::CharPointer_UTF8("\xe2\x99\x82 Masc"),  1);
    genderBox.addItem (juce::CharPointer_UTF8("\xe2\x9c\xa6 Neut"),  2);
    genderBox.addItem (juce::CharPointer_UTF8("\xe2\x99\x80 Fem"),   3);
    genderBox.setSelectedId (2);
    addAndMakeVisible (genderBox);

    // LFO division combo
    for (int i=0; i<6; ++i)
        lfoDivBox.addItem (kSyncDivLabels[i], i+1);
    lfoDivBox.setSelectedId (4); // 1/4 note
    addAndMakeVisible (lfoDivBox);

    // Sync button
    lfoSyncButton.setButtonText ("BPM");
    lfoSyncButton.setClickingTogglesState (true);
    addAndMakeVisible (lfoSyncButton);

    // Title
    titleLabel.setText ("PersoFormant", juce::dontSendNotification);
    vendorLabel.setText ("by  Chobits",  juce::dontSendNotification);
    titleLabel .setFont (juce::Font("Georgia", 22.f, juce::Font::bold|juce::Font::italic));
    vendorLabel.setFont (juce::Font("Georgia", 10.f, juce::Font::italic));
    for (auto* l : { &titleLabel, &vendorLabel })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        addAndMakeVisible (*l);
    }
    titleLabel .setColour (juce::Label::textColourId, ChobitsLookAndFeel::starGold);
    vendorLabel.setColour (juce::Label::textColourId, ChobitsLookAndFeel::pinkAccent);

    // ---- APVTS Attachments ----
    morphAtt        = std::make_unique<SlAtt>(audioProcessor.apvts, "morph",             morphSlider);
    lfoRateAtt      = std::make_unique<SlAtt>(audioProcessor.apvts, "lfoRate",           lfoRateSlider);
    lfoDepthAtt     = std::make_unique<SlAtt>(audioProcessor.apvts, "lfoDepth",          lfoDepthSlider);
    envThreshAtt    = std::make_unique<SlAtt>(audioProcessor.apvts, "envThresh",         envThreshSlider);
    envDepthAtt     = std::make_unique<SlAtt>(audioProcessor.apvts, "envDepth",          envDepthSlider);
    formantResAtt   = std::make_unique<SlAtt>(audioProcessor.apvts, "formantRes",        formantResSlider);
    formantShiftAtt = std::make_unique<SlAtt>(audioProcessor.apvts, "formantShiftParam", formantShiftSlider);
    driveAtt        = std::make_unique<SlAtt>(audioProcessor.apvts, "drive",             driveSlider);
    wetGainAtt      = std::make_unique<SlAtt>(audioProcessor.apvts, "wetGain",           wetGainSlider);
    mixAtt          = std::make_unique<SlAtt>(audioProcessor.apvts, "mix",               mixSlider);
    vowel1Att       = std::make_unique<CbAtt>(audioProcessor.apvts, "vowel1",            vowel1Box);
    vowel2Att       = std::make_unique<CbAtt>(audioProcessor.apvts, "vowel2",            vowel2Box);
    genderAtt       = std::make_unique<CbAtt>(audioProcessor.apvts, "genderShift",       genderBox);
    lfoDivAtt       = std::make_unique<CbAtt>(audioProcessor.apvts, "lfoDiv",            lfoDivBox);
    lfoSyncAtt      = std::make_unique<BtAtt>(audioProcessor.apvts, "lfoSync",           lfoSyncButton);

    // Swap rate knob / div box when sync toggles
    lfoSyncButton.onClick = [this]{ updateSyncMode(); };
    updateSyncMode();

    // Star bursts on interaction
    auto burst = [this](juce::Slider& s){
        s.onValueChange = [this,&s]{
            spawnStars(4, (float)s.getBounds().getCentreX(), (float)s.getBounds().getCentreY());
        };
    };
    for (auto* s : { &morphSlider, &lfoRateSlider, &lfoDepthSlider,
                     &envThreshSlider, &envDepthSlider,
                     &driveSlider, &wetGainSlider, &mixSlider,
                     &formantResSlider, &formantShiftSlider })
        burst (*s);

    for (auto* box : { &vowel1Box, &vowel2Box })
        box->onChange = [this, box]{
            spawnStars (8, (float)box->getBounds().getCentreX(),
                            (float)box->getBounds().getCentreY());
        };

    startTimerHz (60);
}

PersoFormantAudioProcessorEditor::~PersoFormantAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

//==============================================================================
void PersoFormantAudioProcessorEditor::updateSyncMode()
{
    bool sync = lfoSyncButton.getToggleState();
    lfoRateSlider.setVisible (!sync);
    lfoRateLabel .setVisible (!sync);
    lfoDivBox    .setVisible ( sync);
}

//==============================================================================
void PersoFormantAudioProcessorEditor::timerCallback()
{
    animPhase += 0.018f;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;

    for (auto& p : particles)
    {
        p.x += std::cos(p.angle)*p.speed;
        p.y += std::sin(p.angle)*p.speed - 0.3f;
        p.life -= 0.020f;
        p.opacity = juce::jmax(0.f, p.life);
    }
    particles.erase (std::remove_if (particles.begin(), particles.end(),
        [](const StarParticle& s){ return s.life<=0.f; }), particles.end());

    float env  = audioProcessor.getEnvelopeLevel();
    bool  trig = env > 0.05f;
    if (trig && !lastEnvTrig && (int)particles.size() < 60)
        spawnStars (6, getWidth()*0.5f, getHeight()*0.4f);
    lastEnvTrig = trig;

    repaint();
}

//==============================================================================
void PersoFormantAudioProcessorEditor::spawnStars (int n, float cx, float cy)
{
    for (int i=0; i<n; ++i)
    {
        StarParticle p;
        p.x=cx; p.y=cy;
        p.size    = rng.nextFloat()*7.f + 3.f;
        p.speed   = rng.nextFloat()*2.f + 0.5f;
        p.angle   = rng.nextFloat()*juce::MathConstants<float>::twoPi;
        p.life    = 1.f;
        p.opacity = 1.f;
        particles.push_back (p);
    }
}

//==============================================================================
void PersoFormantAudioProcessorEditor::drawStarShape (juce::Graphics& g,
    float cx, float cy, float r, float opacity, juce::Colour col, int pts)
{
    juce::Path star;
    float inner = r*0.42f;
    for (int i=0; i<pts*2; ++i)
    {
        float a   = i*juce::MathConstants<float>::pi/pts - juce::MathConstants<float>::halfPi;
        float rad = (i%2==0) ? r : inner;
        float px  = cx + rad*std::cos(a);
        float py  = cy + rad*std::sin(a);
        if (i==0) star.startNewSubPath(px,py); else star.lineTo(px,py);
    }
    star.closeSubPath();
    g.setColour (col.withAlpha(opacity));
    g.fillPath (star);
    g.setColour (col.brighter(0.4f).withAlpha(opacity*0.4f));
    g.strokePath (star, juce::PathStrokeType(0.7f));
}

//==============================================================================
void PersoFormantAudioProcessorEditor::drawPanel (juce::Graphics& g,
    juce::Rectangle<float> b, const juce::String& title, juce::Colour accent)
{
    g.setColour (ChobitsLookAndFeel::panelFill.withAlpha(0.80f));
    g.fillRoundedRectangle (b, 8.f);
    g.setColour (accent.withAlpha(0.5f));
    g.drawRoundedRectangle (b.reduced(0.5f), 8.f, 1.5f);
    // Top accent bar
    g.setColour (accent.withAlpha(0.65f));
    g.fillRect (juce::Rectangle<float>(b.getX()+8.f, b.getY(), b.getWidth()-16.f, 3.f));
    // Title
    g.setFont (juce::Font("Georgia", 9.f, juce::Font::bold));
    g.setColour (accent.withAlpha(0.90f));
    g.drawText (title, (int)b.getX(), (int)b.getY()+4, (int)b.getWidth(), 13,
                juce::Justification::centred);
}

//==============================================================================
// Central morph visualiser — replaces the old tiny ring.
// Draws two overlapping vowel formant "clouds" that blend with the morph value,
// plus a rotating LFO indicator ring around them.
void PersoFormantAudioProcessorEditor::drawMorphVisualiser (juce::Graphics& g)
{
    // Centre of the visualiser (sits in the middle of the plugin between Chi's panels)
    const float cx = 500.f, cy = 245.f, R = 55.f;

    float morph   = audioProcessor.getSmoothedMorph();
    float lfoPhase= audioProcessor.getLfoPhase();
    float env     = audioProcessor.getEnvelopeLevel();

    // ---- Outer pulsing ring (env-reactive) ----
    float envPulse = 1.f + env * 0.35f;
    {
        juce::ColourGradient ringGrad (
            ChobitsLookAndFeel::pinkAccent.withAlpha(0.25f + env*0.4f), cx, cy,
            ChobitsLookAndFeel::starGold.withAlpha(0.05f), cx+R*envPulse, cy, true);
        g.setGradientFill (ringGrad);
        g.fillEllipse (cx-R*envPulse, cy-R*envPulse, R*2.f*envPulse, R*2.f*envPulse);
    }

    // ---- Vowel A blob (fades out as morph → 1) ----
    {
        float alpha = (1.f - morph) * 0.55f + 0.05f;
        juce::ColourGradient blob (
            ChobitsLookAndFeel::starGold.withAlpha(alpha), cx-R*0.35f, cy,
            ChobitsLookAndFeel::starGold.withAlpha(0.f),   cx-R*0.35f-R*0.7f, cy, true);
        g.setGradientFill (blob);
        g.fillEllipse (cx-R*1.05f, cy-R*0.7f, R*1.4f, R*1.4f);
    }

    // ---- Vowel B blob (fades in as morph → 1) ----
    {
        float alpha = morph * 0.55f + 0.05f;
        juce::ColourGradient blob (
            ChobitsLookAndFeel::pinkAccent.withAlpha(alpha), cx+R*0.35f, cy,
            ChobitsLookAndFeel::pinkAccent.withAlpha(0.f),   cx+R*0.35f+R*0.7f, cy, true);
        g.setGradientFill (blob);
        g.fillEllipse (cx-R*0.35f, cy-R*0.7f, R*1.4f, R*1.4f);
    }

    // ---- Three formant frequency arcs (concentric rings per band) ----
    // Use current smoothed morph to display which frequencies are active
    int v1 = juce::jlimit(0,11, (int)audioProcessor.apvts.getRawParameterValue("vowel1")->load());
    int v2 = juce::jlimit(0,11, (int)audioProcessor.apvts.getRawParameterValue("vowel2")->load());
    for (int k=0; k<3; ++k)
    {
        float f1 = kVowelFormants[v1].freq[k];
        float f2 = kVowelFormants[v2].freq[k];
        float f  = f1 + morph*(f2-f1);
        // Map 200-3200 Hz to ring radius 10-50
        float norm = juce::jlimit(0.f,1.f, (std::log(f)-std::log(200.f)) / (std::log(3200.f)-std::log(200.f)));
        float rk   = 10.f + norm*(R-12.f);
        float pulse = 1.f + 0.06f*std::sin(animPhase*2.f + k*2.1f);
        rk *= pulse;

        juce::Colour bandCol = (k==0) ? ChobitsLookAndFeel::pinkAccent
                             : (k==1) ? ChobitsLookAndFeel::starGold
                                      : ChobitsLookAndFeel::hairGold;
        float gDB = kVowelFormants[v1].gain[k] + morph*(kVowelFormants[v2].gain[k]-kVowelFormants[v1].gain[k]);
        float bandAlpha = juce::jmap(gDB, 0.f, 20.f, 0.15f, 0.75f);

        g.setColour (bandCol.withAlpha(bandAlpha));
        g.drawEllipse (cx-rk, cy-rk, rk*2.f, rk*2.f, 1.8f);

        // Small label at right edge of each ring
        g.setFont (juce::Font("Georgia", 7.f, juce::Font::plain));
        g.setColour (bandCol.withAlpha(0.7f));
        juce::String fStr = juce::String((int)f) + "Hz";
        g.drawText (fStr, (int)(cx+rk-16.f), (int)(cy-5.f), 32, 11,
                    juce::Justification::centred);
    }

    // ---- Dark backing disc so arcs read clearly ----
    g.setColour (ChobitsLookAndFeel::bgDeep.withAlpha(0.55f));
    g.fillEllipse (cx-R*0.55f, cy-R*0.55f, R*1.1f, R*1.1f);

    // ---- LFO rotating dot on outermost ring ----
    {
        float dotR = R + 7.f;
        float dotX = cx + dotR*std::cos(lfoPhase - juce::MathConstants<float>::halfPi);
        float dotY = cy + dotR*std::sin(lfoPhase - juce::MathConstants<float>::halfPi);

        // Trailing arc
        juce::Path trail;
        trail.addArc (cx-dotR, cy-dotR, dotR*2.f, dotR*2.f,
                      lfoPhase - juce::MathConstants<float>::halfPi - 1.2f,
                      lfoPhase - juce::MathConstants<float>::halfPi, true);
        g.setColour (ChobitsLookAndFeel::starGold.withAlpha(0.55f));
        g.strokePath (trail, juce::PathStrokeType(2.f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

        // Dot
        g.setColour (ChobitsLookAndFeel::pinkAccent);
        g.fillEllipse (dotX-4.f, dotY-4.f, 8.f, 8.f);
        g.setColour (juce::Colours::white.withAlpha(0.9f));
        g.fillEllipse (dotX-1.8f, dotY-1.8f, 3.6f, 3.6f);
    }

    // ---- Morph needle in centre ----
    {
        float needleAngle = -juce::MathConstants<float>::halfPi + morph*juce::MathConstants<float>::pi;
        float nx = cx + (R*0.45f)*std::cos(needleAngle);
        float ny = cy + (R*0.45f)*std::sin(needleAngle);
        g.setColour (ChobitsLookAndFeel::hairGold.withAlpha(0.85f));
        g.drawLine (cx, cy, nx, ny, 2.2f);
        g.setColour (ChobitsLookAndFeel::pinkAccent);
        g.fillEllipse (cx-3.5f, cy-3.5f, 7.f, 7.f);
    }

    // ---- "MORPH" text ----
    g.setFont (juce::Font("Georgia", 8.f, juce::Font::bold));
    g.setColour (ChobitsLookAndFeel::hairGold.withAlpha(0.7f));
    g.drawText ("MORPH", (int)(cx-20.f), (int)(cy+R+10.f), 40, 12, juce::Justification::centred);
}

//==============================================================================
void PersoFormantAudioProcessorEditor::drawEnvelopeMeter (juce::Graphics& g)
{
    float env = audioProcessor.getEnvelopeLevel();
    // Small vertical bar inside the ENV panel on the right side
    float mx=826.f, my=290.f, mW=8.f, mH=60.f;

    g.setColour (ChobitsLookAndFeel::bgDeep.withAlpha(0.7f));
    g.fillRoundedRectangle (mx, my, mW, mH, 3.f);
    g.setColour (ChobitsLookAndFeel::pinkAccent.withAlpha(0.35f));
    g.drawRoundedRectangle (mx, my, mW, mH, 3.f, 1.f);

    float fillH = juce::jlimit(0.f, mH, env*mH*8.f);
    if (fillH > 0.5f)
    {
        juce::ColourGradient fill (ChobitsLookAndFeel::pinkAccent, mx, my+mH,
                                    ChobitsLookAndFeel::starGold, mx, my, false);
        g.setGradientFill (fill);
        g.fillRoundedRectangle (mx, my+mH-fillH, mW, fillH, 3.f);
    }
}

//==============================================================================
void PersoFormantAudioProcessorEditor::paint (juce::Graphics& g)
{
    int W=getWidth(), H=getHeight();

    // ---- Background image ----
    if (bgImage.isValid())
    {
        float imgW=(float)bgImage.getWidth(), imgH=(float)bgImage.getHeight();
        float scale = juce::jmax(W/imgW, H/imgH);
        int dW=juce::roundToInt(imgW*scale), dH=juce::roundToInt(imgH*scale);
        int offX=(W-dW)/2, offY=(H-dH)/3;
        g.drawImage (bgImage, offX,offY,dW,dH, 0,0,bgImage.getWidth(),bgImage.getHeight());

        // Dark vignette — stronger on the sides so panels read clearly
        juce::ColourGradient vig (juce::Colours::black.withAlpha(0.72f), 0, H*0.5f,
                                   juce::Colours::transparentBlack,       W*0.5f, H*0.5f, false);
        g.setGradientFill (vig); g.fillRect(0,0,W,H);
        juce::ColourGradient vig2 (juce::Colours::black.withAlpha(0.72f), (float)W, H*0.5f,
                                    juce::Colours::transparentBlack,       W*0.5f,   H*0.5f, false);
        g.setGradientFill (vig2); g.fillRect(0,0,W,H);
        // Subtle overall darkening
        g.setColour (juce::Colours::black.withAlpha(0.28f)); g.fillRect(0,0,W,H);
    }
    else
    {
        juce::ColourGradient sky (ChobitsLookAndFeel::bgDeep,0,0,
                                   ChobitsLookAndFeel::bgMid,(float)W,(float)H,false);
        g.setGradientFill(sky); g.fillRect(0,0,W,H);
    }

    // ---- Left side panels ----
    // VOWEL panel
    drawPanel (g, {6.f,6.f,200.f,290.f},      "VOWEL  MORPH",    ChobitsLookAndFeel::pinkAccent);
    // LFO panel
    drawPanel (g, {6.f,304.f,200.f,128.f},     "LFO",             ChobitsLookAndFeel::starGold);

    // ---- Right side panels ----
    // FORMANT panel
    drawPanel (g, {794.f,6.f,200.f,160.f},     "FORMANT  FILTER", ChobitsLookAndFeel::hairGold);
    // ENV panel
    drawPanel (g, {794.f,174.f,200.f,130.f},   "ENV  FOLLOWER",   ChobitsLookAndFeel::pinkAccent);
    // OUTPUT panel
    drawPanel (g, {794.f,312.f,200.f,120.f},   "OUTPUT",          ChobitsLookAndFeel::starGold);

    // ---- Particles ----
    for (auto& p : particles)
    {
        juce::Colour col = ChobitsLookAndFeel::starGold
            .interpolatedWith(ChobitsLookAndFeel::pinkAccent, 1.f-p.life);
        drawStarShape (g, p.x, p.y, p.size, p.opacity, col, 5);
    }

    // ---- Ambient sky stars ----
    struct AmbStar { float x,y,r; };
    static const AmbStar amb[] = {
        {120,25,6},{220,14,5},{320,30,7},{440,10,5},
        {560,28,6},{680,16,5},{780,22,7},
        {150,55,5},{360,65,6},{620,50,5},
    };
    for (auto& s : amb)
    {
        float pulse = 0.6f + 0.4f*std::sin(animPhase + s.x*0.013f);
        drawStarShape (g, s.x, s.y, s.r*pulse, 0.65f*pulse, ChobitsLookAndFeel::starGold, 5);
    }

    // ---- Central morph visualiser ----
    drawMorphVisualiser (g);

    // ---- Env meter ----
    drawEnvelopeMeter (g);

    // ---- Title glow ----
    {
        juce::ColourGradient glow (ChobitsLookAndFeel::pinkAccent.withAlpha(0.15f),
                                    W*0.5f,(float)(H-55),
                                    juce::Colours::transparentBlack,
                                    W*0.5f,(float)(H-10),false);
        g.setGradientFill(glow);
        g.fillRect(320,H-70,360,55);
    }
}

//==============================================================================
void PersoFormantAudioProcessorEditor::resized()
{
    // ============================================================
    // LEFT SIDE  — VOWEL MORPH panel  (x=6, y=6, w=200, h=290)
    // ============================================================
    int lx = 10;

    vowel1Label.setBounds (lx,     22, 88, 13);
    vowel1Box  .setBounds (lx,     36, 88, 22);

    vowel2Label.setBounds (lx+104, 22, 88, 13);
    vowel2Box  .setBounds (lx+104, 36, 88, 22);

    genderLabel.setBounds (lx+60,  66, 72, 13);
    genderBox  .setBounds (lx+56,  80, 80, 22);

    morphLabel .setBounds (lx+60, 112, 72, 13);
    morphSlider.setBounds (lx+44, 124, 104, 104);

    // Second row: formant shift macro inside vowel panel
    // (below morph knob, uses remaining space)
    // (empty — extra breathing room)

    // ============================================================
    // LEFT SIDE  — LFO panel  (x=6, y=304, w=200, h=128)
    // ============================================================
    int ly = 308;

    lfoRateLabel .setBounds (lx,      ly+12, 74, 13);
    lfoRateSlider.setBounds (lx-2,    ly+24, 80, 80);
    lfoDivBox    .setBounds (lx,      ly+38, 74, 26);   // visible only in sync mode

    lfoDepthLabel .setBounds (lx+100,  ly+12, 74, 13);
    lfoDepthSlider.setBounds (lx+98,   ly+24, 80, 80);

    lfoSyncLabel .setBounds (lx+26,   ly+100, 64, 13);
    lfoSyncButton.setBounds (lx+36,   ly+110, 50, 18);

    // ============================================================
    // RIGHT SIDE — FORMANT FILTER panel  (x=794, y=6, w=200, h=160)
    // ============================================================
    int rx = 798;

    formantResLabel  .setBounds (rx,      22, 90, 13);
    formantResSlider .setBounds (rx-4,    34, 96, 96);

    formantShiftLabel.setBounds (rx+104,  22, 90, 13);
    formantShiftSlider.setBounds(rx+100,  34, 96, 96);

    // ============================================================
    // RIGHT SIDE — ENV FOLLOWER panel  (x=794, y=174, w=200, h=130)
    // ============================================================
    int ey = 178;

    envThreshLabel  .setBounds (rx,     ey+12, 88, 13);
    envThreshSlider .setBounds (rx-4,   ey+24, 96, 80);

    envDepthLabel  .setBounds (rx+104,  ey+12, 88, 13);
    envDepthSlider .setBounds (rx+100,  ey+24, 96, 80);

    // ============================================================
    // RIGHT SIDE — OUTPUT panel  (x=794, y=312, w=200, h=120)
    // ============================================================
    int oy = 316;

    driveLabel   .setBounds (rx,      oy+12, 58, 13);
    driveSlider  .setBounds (rx-4,    oy+24, 64, 70);

    wetGainLabel .setBounds (rx+66,   oy+12, 58, 13);
    wetGainSlider.setBounds (rx+62,   oy+24, 64, 70);

    mixLabel     .setBounds (rx+132,  oy+12, 58, 13);
    mixSlider    .setBounds (rx+128,  oy+24, 64, 70);

    // ============================================================
    // Title  — bottom centre (over the road area)
    // ============================================================
    titleLabel .setBounds (370, getHeight()-70, 260, 30);
    vendorLabel.setBounds (370, getHeight()-42, 260, 16);
}
