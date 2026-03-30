#pragma once

#include <JuceHeader.h>
#include <array>
#include <random>

//==============================================================================
class DysmorphiaAudioProcessor : public juce::AudioProcessor,
                                  public juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    DysmorphiaAudioProcessor();
    ~DysmorphiaAudioProcessor() override;

    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources () override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool   acceptsMidi()  const override;
    bool   producesMidi() const override;
    bool   isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int  getNumPrograms()   override;
    int  getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState apvts;

    // Thread-safe spectrum data for the visualiser
    // Indices 0-2: input  low/mid/high RMS
    // Indices 3-5: output low/mid/high RMS
    std::array<std::atomic<float>, 6> spectrumData;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //— Parameter raw pointers (set once after APVTS construction) —————————————
    std::atomic<float>* pRate    = nullptr;
    std::atomic<float>* pDepth   = nullptr;
    std::atomic<float>* pDensity = nullptr;
    std::atomic<float>* pSpread  = nullptr;
    std::atomic<float>* pChaos   = nullptr;
    std::atomic<float>* pXover   = nullptr;
    std::atomic<float>* pSpecOff = nullptr;
    std::atomic<float>* pMix     = nullptr;
    std::atomic<float>* pWidth   = nullptr;
    std::atomic<float>* pGain    = nullptr;

    //— Granular cloud engine ——————————————————————————————————————————————————
    static constexpr int kMaxDelaySamples = 192001;   // ≈ 2 s @ 96 kHz
    static constexpr int kNumGrains       = 16;

    struct Grain
    {
        float readPos     = 0.f;   // fractional index into delay buffer
        float speed       = 1.f;   // playback speed (pitch ratio)
        float pan         = 0.f;   // -1 .. +1
        float envPhase    = 0.f;   // 0 .. 1  (Hann window)
        float grainLen    = 0.f;   // length in samples
        bool  active      = false;
        int   channel     = 0;     // source channel: 0=L, 1=R
    };

    std::array<Grain, kNumGrains> grains;
    juce::AudioBuffer<float>      delayBuffer;
    int   writePos   = 0;
    float spawnTimer = 0.f;

    // Chaos sample-and-hold state
    float chaosSaHTimer = 0.f;
    float chaosSaHValue = 0.f;

    // Per-grain LFOs
    std::array<float, kNumGrains> lfoPhase {};
    std::array<float, kNumGrains> lfoFreq  {};

    //— State-variable crossover filter (Zolzer topology) ————————————————————
    struct SVF { float ic1eq = 0.f, ic2eq = 0.f; };
    std::array<SVF, 2> svfLow;   // 2 channels
    std::array<SVF, 2> svfHigh;  // unused directly; HP = input - LP

    float svfG  = 1.f;     // tan(π·fc/fs)
    float svfR2 = 1.414f;  // √2  → Butterworth 2nd-order

    void  updateCrossoverCoeffs (float freqHz);
    float tickSVFLP (float x, SVF& s) const;   // returns LP, HP = x - LP

    //— Spectrum analyser ————————————————————————————————————————————————————
    // Smoothed RMS per band (in / out)
    std::array<float, 3> inSmooth  {};
    std::array<float, 3> outSmooth {};
    float smoothCoeff = 0.f;

    // One-pole filter states for the visualiser band-split (non-static members)
    float visLP_in  = 0.f, visMH_in  = 0.f;
    float visLP_out = 0.f, visMH_out = 0.f;

    void analyseSpectrum (const juce::AudioBuffer<float>& inBuf,
                          const juce::AudioBuffer<float>& outBuf,
                          int numSamples);

    //— Miscellaneous ———————————————————————————————————————————————————————
    double currentSR = 44100.0;

    std::mt19937 rng { 42 };
    float randF (float lo, float hi)
    {
        return lo + (static_cast<float>(rng()) /
                     static_cast<float>(rng.max())) * (hi - lo);
    }

    void  spawnGrain (int idx, int ch, float baseDelay,
                      float pitch, float pan, float len);
    float readInterp (int ch, float pos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DysmorphiaAudioProcessor)
};
