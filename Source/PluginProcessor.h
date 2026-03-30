#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>

//==============================================================================
struct FormantData { float freq[3]; float gain[3]; float q[3]; };

// 12 vowel/character presets — richer spread, much stronger gains so the
// effect is impossible to miss even on dense mixes.
// Index 0-11 maps to the combo box items.
static const FormantData kVowelFormants[12] = {
    // 0  "A"  (father)  — open, warm
    { { 800.f, 1200.f, 2500.f }, { 18.f, 12.f,  6.f }, { 7.f, 10.f, 14.f } },
    // 1  "E"  (bed)     — mid-front
    { { 600.f, 1900.f, 2550.f }, { 16.f, 16.f,  8.f }, { 7.f, 10.f, 14.f } },
    // 2  "I"  (feet)    — high-front, very bright
    { { 280.f, 2250.f, 3000.f }, { 10.f, 20.f, 10.f }, { 7.f, 10.f, 14.f } },
    // 3  "O"  (go)      — back, dark rounded
    { { 450.f,  750.f, 2400.f }, { 20.f,  8.f,  4.f }, { 7.f, 10.f, 14.f } },
    // 4  "U"  (boot)    — very dark, closed
    { { 290.f,  640.f, 2200.f }, { 12.f,  8.f,  2.f }, { 7.f, 10.f, 14.f } },
    // 5  "AE" (cat)     — spread, nasal
    { { 860.f, 1720.f, 2480.f }, { 18.f, 14.f,  6.f }, { 7.f, 10.f, 14.f } },
    // 6  "OE" (French ö)— front-rounded
    { { 400.f, 1500.f, 2400.f }, { 16.f, 14.f,  5.f }, { 7.f, 10.f, 14.f } },
    // 7  "UH" (cup)     — central schwa
    { { 600.f, 1200.f, 2400.f }, { 14.f, 10.f,  4.f }, { 7.f, 10.f, 14.f } },
    // 8  "AW" (law)     — low-back
    { { 700.f,  900.f, 2400.f }, { 20.f,  6.f,  4.f }, { 7.f, 10.f, 14.f } },
    // 9  "EH" (gate)    — tense mid-front
    { { 430.f, 2050.f, 2600.f }, { 14.f, 18.f,  8.f }, { 7.f, 10.f, 14.f } },
    // 10 "IH" (bit)     — lax high-front
    { { 390.f, 1990.f, 2550.f }, { 12.f, 16.f,  7.f }, { 7.f, 10.f, 14.f } },
    // 11 "UW" (you)     — high-back glide
    { { 310.f,  870.f, 2250.f }, { 10.f, 10.f,  3.f }, { 7.f, 10.f, 14.f } },
};

static const char* kVowelNames[12] = {
    "A  (father)", "E  (bed)", "I  (feet)", "O  (go)", "U  (boot)",
    "AE (cat)", "OE (ö)", "UH (cup)", "AW (law)", "EH (gate)",
    "IH (bit)", "UW (you)"
};

// BPM-sync divisions: 1/32, 1/16, 1/8, 1/4, 1/2, 1 bar (in beats)
static constexpr float kSyncDivBeats[6] = { 0.125f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
static const char* kSyncDivLabels[6]    = { "1/32", "1/16", "1/8", "1/4", "1/2", "1 bar" };

//==============================================================================
class ParameterSmoother
{
public:
    void reset (double sr, double ms)
    {
        coeff = std::exp (-1.0 / (sr * ms * 0.001));
        val = 0.f;
    }
    float process (float t) { val = val*(float)coeff + t*(1.f-(float)coeff); return val; }
    void  snap (float v) { val = v; }
private:
    double coeff = 0.0; float val = 0.f;
};

//==============================================================================
// Peaking-EQ biquad — one formant band
class FormantFilter
{
public:
    void reset() { x1=x2=y1=y2=0.f; }

    void setCoefficients (float freq, float gainDB, float q, float sr)
    {
        float A     = std::pow (10.f, gainDB/40.f);
        float w0    = juce::MathConstants<float>::twoPi * freq / sr;
        float cosw  = std::cos(w0), sinw = std::sin(w0);
        float alpha = sinw / (2.f*q);
        float b0= 1.f+alpha*A, b1=-2.f*cosw, b2=1.f-alpha*A;
        float a0= 1.f+alpha/A, a1=-2.f*cosw, a2=1.f-alpha/A;
        float inv=1.f/a0;
        cb0=b0*inv; cb1=b1*inv; cb2=b2*inv; ca1=a1*inv; ca2=a2*inv;
    }

    inline float process (float x)
    {
        float y = cb0*x + cb1*x1 + cb2*x2 - ca1*y1 - ca2*y2;
        x2=x1; x1=x; y2=y1; y1=y; return y;
    }
private:
    float cb0=1,cb1=0,cb2=0,ca1=0,ca2=0, x1=0,x2=0,y1=0,y2=0;
};

//==============================================================================
class EnvelopeFollower
{
public:
    void reset (double sr)
    {
        atk = (float)std::exp(-1.0/(sr*0.010));
        rel = (float)std::exp(-1.0/(sr*0.100));
        env = 0.f;
    }
    float process (float in)
    {
        float a = std::abs(in);
        float c = a>env ? atk : rel;
        env = env*c + a*(1.f-c);
        return env;
    }
private:
    float atk=0,rel=0,env=0;
};

//==============================================================================
class PersoFormantAudioProcessor : public juce::AudioProcessor,
                                   public juce::AudioProcessorValueTreeState::Listener
{
public:
    PersoFormantAudioProcessor();
    ~PersoFormantAudioProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock   (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;
    void parameterChanged (const juce::String&, float) override;

    juce::AudioProcessorValueTreeState apvts;

    float getLfoPhase()      const { return lfoPhase; }
    float getEnvelopeLevel() const { return envelopeLevel.load(); }
    // Expose current smoothed morph for visualiser
    float getSmoothedMorph() const { return currentMorphDisplay.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    float getParam (const juce::String& id) const { return apvts.getRawParameterValue(id)->load(); }

    void updateFormantCoeffs (float morph, float formantShift, float sr);
    FormantData interpolateFormants (int v1, int v2, float t, float shift) const;
    float getLfoRateHz() const;   // respects BPM-sync

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    // Per-channel, 3-band formant filters
    FormantFilter     formantFilters[2][3];
    ParameterSmoother morphSmoother[2];

    float currentFreq[3]={0,0,0}, currentGain[3]={0,0,0}, currentQ[3]={0,0,0};
    float targetFreq[3] ={0,0,0}, targetGain[3] ={0,0,0}, targetQ[3] ={0,0,0};
    float freqGlideCoeff = 0.5f;

    float lfoPhase = 0.f;

    EnvelopeFollower   envFollower[2];
    std::atomic<float> envelopeLevel   { 0.f };
    std::atomic<float> currentMorphDisplay { 0.f };

    static inline float softClip (float x, float drive)
    { x *= drive; return x / (1.f + std::abs(x)); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PersoFormantAudioProcessor)
};
