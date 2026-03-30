#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
class ChobitsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ChobitsLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&,
                                const juce::Colour&, bool highlighted, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool isButtonDown,
                       int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;

    static const juce::Colour bgDeep, bgMid, starGold, pinkAccent,
                               silhouetteBlue, hairGold, panelFill;
};

//==============================================================================
struct StarParticle { float x, y, size, opacity, speed, angle, life; };

//==============================================================================
class PersoFormantAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit PersoFormantAudioProcessorEditor (PersoFormantAudioProcessor&);
    ~PersoFormantAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;

private:
    void timerCallback() override;
    void spawnStars (int n, float cx, float cy);
    void drawStarShape (juce::Graphics&, float cx, float cy, float r,
                        float opacity, juce::Colour, int pts = 5);
    void drawPanel (juce::Graphics&, juce::Rectangle<float>, const juce::String&, juce::Colour);
    void drawMorphVisualiser (juce::Graphics&);   // replaces old LFO indicator
    void drawEnvelopeMeter   (juce::Graphics&);
    void updateSyncMode();                        // swap rate knob ↔ div combo

    PersoFormantAudioProcessor& audioProcessor;
    ChobitsLookAndFeel laf;
    juce::Image bgImage;

    // ---- Vowel morph section (left panel) ----
    juce::ComboBox vowel1Box, vowel2Box;
    juce::Slider   morphSlider;
    juce::ComboBox genderBox;
    juce::Label    vowel1Label, vowel2Label, morphLabel, genderLabel;

    // ---- LFO section (top-right panel) ----
    juce::Slider      lfoRateSlider;     // visible when sync OFF
    juce::ComboBox    lfoDivBox;         // visible when sync ON
    juce::Slider      lfoDepthSlider;
    juce::ToggleButton lfoSyncButton;
    juce::Label       lfoRateLabel, lfoDepthLabel, lfoSyncLabel;

    // ---- Env section (bottom-right panel) ----
    juce::Slider envThreshSlider, envDepthSlider;
    juce::Label  envThreshLabel,  envDepthLabel;

    // ---- Formant controls (centre panel) ----
    juce::Slider formantResSlider, formantShiftSlider;
    juce::Label  formantResLabel,  formantShiftLabel;

    // ---- Wet chain controls (far-right panel) ----
    juce::Slider driveSlider, wetGainSlider, mixSlider;
    juce::Label  driveLabel,  wetGainLabel,  mixLabel;

    // ---- Title ----
    juce::Label titleLabel, vendorLabel;

    // ---- APVTS attachments ----
    using SlAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CbAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BtAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SlAtt> morphAtt, lfoRateAtt, lfoDepthAtt,
                            envThreshAtt, envDepthAtt,
                            driveAtt, wetGainAtt, mixAtt,
                            formantResAtt, formantShiftAtt;
    std::unique_ptr<CbAtt> vowel1Att, vowel2Att, genderAtt, lfoDivAtt;
    std::unique_ptr<BtAtt> lfoSyncAtt;

    // ---- Animation ----
    std::vector<StarParticle> particles;
    juce::Random rng;
    float animPhase   = 0.f;
    bool  lastEnvTrig = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PersoFormantAudioProcessorEditor)
};
