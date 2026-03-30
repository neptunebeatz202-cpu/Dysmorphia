#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Colour palette — declared here so both header and cpp can use it
//==============================================================================
namespace DysmorphiaColours
{
    inline constexpr juce::uint32 voidBlackV  = 0xff050304;
    inline constexpr juce::uint32 fleshRedV   = 0xff5a0a0a;
    inline constexpr juce::uint32 bloodDeepV  = 0xff2d0505;
    inline constexpr juce::uint32 arterialV   = 0xffaa1111;
    inline constexpr juce::uint32 sayaGreenV  = 0xff3de68e;
    inline constexpr juce::uint32 sickGreenV  = 0xff4faa55;
    inline constexpr juce::uint32 boneWhiteV  = 0xffe8dcc8;
    inline constexpr juce::uint32 eyeGlowV    = 0xff00ff88;
    inline constexpr juce::uint32 visceraV    = 0xff7a1515;
    inline constexpr juce::uint32 tendonV     = 0xff9a7a50;

    inline const juce::Colour voidBlack  { voidBlackV  };
    inline const juce::Colour fleshRed   { fleshRedV   };
    inline const juce::Colour bloodDeep  { bloodDeepV  };
    inline const juce::Colour arterial   { arterialV   };
    inline const juce::Colour sayaGreen  { sayaGreenV  };
    inline const juce::Colour sickGreen  { sickGreenV  };
    inline const juce::Colour boneWhite  { boneWhiteV  };
    inline const juce::Colour eyeGlow    { eyeGlowV    };
    inline const juce::Colour viscera    { visceraV    };
    inline const juce::Colour tendon     { tendonV     };
}

//==============================================================================
// Custom LookAndFeel — Saya no Uta / body-horror aesthetic
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
// 3-band spectrum visualiser  (LOW / MID / HIGH  ×  in/out)
//==============================================================================
class SpectrumVisualizer : public juce::Component,
                           public juce::Timer
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
// Main plugin editor
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

    //— Knobs ——————————————————————————————————————————————————————————————
    juce::Slider rateSlider, depthSlider, densitySlider, spreadSlider, mixSlider;
    juce::Slider chaosSlider, xoverSlider, specOffSlider, widthSlider, gainSlider;

    //— Labels attached to knobs ——————————————————————————————————————————
    juce::Label rateLabel, depthLabel, densityLabel, spreadLabel, mixLabel;
    juce::Label chaosLabel, xoverLabel, specOffLabel, widthLabel, gainLabel;

    //— Section / title labels ————————————————————————————————————————————
    juce::Label titleLabel, vendorLabel;
    juce::Label granularLabel, spectralLabel, chaosSecLabel;

    //— APVTS attachments ——————————————————————————————————————————————————
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> rateAtt, depthAtt, densityAtt, spreadAtt, mixAtt;
    std::unique_ptr<Attachment> chaosAtt, xoverAtt, specOffAtt, widthAtt, gainAtt;

    //— Animation ——————————————————————————————————————————————————————————
    float pulsePhase  = 0.f;
    float glitchTimer = 0.f;
    bool  glitchActive = false;
    int   glitchFrame  = 0;
    mutable std::mt19937 rng { 1337 };

    //— Private helpers ————————————————————————————————————————————————————
    void setupSlider (juce::Slider& s, juce::Label& lbl,
                      const juce::String& text);

    void drawSectionBorder (juce::Graphics& g,
                            juce::Rectangle<int> r,
                            juce::Colour col,
                            float pulse) const;

    void drawFleshVeins (juce::Graphics& g,
                         juce::Rectangle<int> bounds,
                         float phase,
                         float alpha) const;

    void drawOrganicNoise (juce::Graphics& g,
                           juce::Rectangle<int> bounds,
                           float phase) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DysmorphiaAudioProcessorEditor)
};
