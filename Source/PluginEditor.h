#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
namespace DysmorphiaColours
{
    inline constexpr juce::uint32 voidBlackV   = 0xff050304;
    inline constexpr juce::uint32 fleshRedV    = 0xff5a0a0a;
    inline constexpr juce::uint32 bloodDeepV   = 0xff2d0505;
    inline constexpr juce::uint32 arterialV    = 0xffaa1111;
    inline constexpr juce::uint32 sayaGreenV   = 0xff3de68e;
    inline constexpr juce::uint32 sickGreenV   = 0xff4faa55;
    inline constexpr juce::uint32 boneWhiteV   = 0xffe8dcc8;
    inline constexpr juce::uint32 eyeGlowV     = 0xff00ff88;
    inline constexpr juce::uint32 visceraV     = 0xff7a1515;
    inline constexpr juce::uint32 tendonV      = 0xff9a7a50;
    inline constexpr juce::uint32 flangerBlueV = 0xff113355;
    inline constexpr juce::uint32 flangerCyanV = 0xff22aacc;

    inline const juce::Colour voidBlack   { voidBlackV   };
    inline const juce::Colour fleshRed    { fleshRedV    };
    inline const juce::Colour bloodDeep   { bloodDeepV   };
    inline const juce::Colour arterial    { arterialV    };
    inline const juce::Colour sayaGreen   { sayaGreenV   };
    inline const juce::Colour sickGreen   { sickGreenV   };
    inline const juce::Colour boneWhite   { boneWhiteV   };
    inline const juce::Colour eyeGlow     { eyeGlowV     };
    inline const juce::Colour viscera     { visceraV     };
    inline const juce::Colour tendon      { tendonV      };
    inline const juce::Colour flangerBlue { flangerBlueV };
    inline const juce::Colour flangerCyan { flangerCyanV };
}

//==============================================================================
class DysmorphiaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    DysmorphiaLookAndFeel();
    ~DysmorphiaLookAndFeel() override = default;

    void drawRotarySlider (juce::Graphics& g,
                           int x, int y, int width, int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    void       drawLabel    (juce::Graphics& g, juce::Label& label) override;
    juce::Font getLabelFont (juce::Label& label) override;
};

//==============================================================================
class SpectrumVisualizer : public juce::Component, public juce::Timer
{
public:
    explicit SpectrumVisualizer (DysmorphiaAudioProcessor& p);
    ~SpectrumVisualizer() override = default;

    void paint        (juce::Graphics& g) override;
    void resized      () override;
    void timerCallback() override;

private:
    DysmorphiaAudioProcessor& processor;
    std::array<float, 3> inLevels  {};
    std::array<float, 3> outLevels {};
    float animPhase  = 0.f;
    int   frameCount = 0;

    void drawFleshBand (juce::Graphics& g,
                        float x, float y, float w, float h,
                        float level, juce::Colour col, bool isOut);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumVisualizer)
};

//==============================================================================
class DysmorphiaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    explicit DysmorphiaAudioProcessorEditor (DysmorphiaAudioProcessor&);
    ~DysmorphiaAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void timerCallback() override;

private:
    DysmorphiaAudioProcessor& audioProcessor;
    DysmorphiaLookAndFeel     lnf;

    juce::Image        bgImage;
    SpectrumVisualizer specViz;

    // Tab buttons
    juce::TextButton chorusTabBtn  { "CHORUS" };
    juce::TextButton flangerTabBtn { "FLANGER" };
    bool             showFlanger   = false;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    // Chorus knobs
    juce::Slider rateSlider, depthSlider, densitySlider, spreadSlider, mixSlider;
    juce::Slider chaosSlider, xoverSlider, specOffSlider, widthSlider, gainSlider;
    juce::Label  rateLabel, depthLabel, densityLabel, spreadLabel, mixLabel;
    juce::Label  chaosLabel, xoverLabel, specOffLabel, widthLabel, gainLabel;
    std::unique_ptr<Attachment> rateAtt, depthAtt, densityAtt, spreadAtt, mixAtt;
    std::unique_ptr<Attachment> chaosAtt, xoverAtt, specOffAtt, widthAtt, gainAtt;

    // Flanger knobs
    juce::Slider flRateSlider, flDepthSlider, flFeedbackSlider, flJitterSlider;
    juce::Slider flMixSlider, flGainSlider;
    juce::Slider bitGritAmtSlider, bitGritSensSlider;
    juce::Slider filterCutSlider, filterResSlider, filterMixSlider;
    juce::Label  flRateLabel, flDepthLabel, flFeedbackLabel, flJitterLabel;
    juce::Label  flMixLabel, flGainLabel;
    juce::Label  bitGritAmtLabel, bitGritSensLabel;
    juce::Label  filterCutLabel, filterResLabel, filterMixLabel;
    std::unique_ptr<Attachment> flRateAtt, flDepthAtt, flFeedbackAtt, flJitterAtt;
    std::unique_ptr<Attachment> flMixAtt, flGainAtt;
    std::unique_ptr<Attachment> bitGritAmtAtt, bitGritSensAtt;
    std::unique_ptr<Attachment> filterCutAtt, filterResAtt, filterMixAtt;

    // Section labels
    juce::Label titleLabel, vendorLabel;
    juce::Label granularLabel, spectralLabel, chaosSecLabel;
    juce::Label flangerSecLabel, bitGritSecLabel, filterSecLabel;

    // Animation
    float pulsePhase   = 0.f;
    float glitchTimer  = 0.f;
    bool  glitchActive = false;
    int   glitchFrame  = 0;
    mutable std::mt19937 rng { 1337 };

    void setupSlider (juce::Slider& s, juce::Label& lbl, const juce::String& text);
    void updateTabVisibility();

    void drawSectionBorder (juce::Graphics& g, juce::Rectangle<int> r,
                            juce::Colour col, float pulse) const;
    void drawFleshVeins    (juce::Graphics& g, juce::Rectangle<int> bounds,
                            float phase, float alpha) const;
    void drawOrganicNoise  (juce::Graphics& g, juce::Rectangle<int> bounds,
                            float phase) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DysmorphiaAudioProcessorEditor)
};
