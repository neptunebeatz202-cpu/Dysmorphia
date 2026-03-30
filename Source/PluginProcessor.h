#pragma once

#include <JuceHeader.h>
#include <array>
#include <random>

//==============================================================================
class DysmorphiaAudioProcessor : public juce::AudioProcessor,
                                  public juce::AudioProcessorValueTreeState::Listener
{
public:
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
    // Indices 0-2: input low/mid/high RMS  |  3-5: output low/mid/high RMS
    std::array<std::atomic<float>, 6> spectrumData;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ─── Chorus parameter pointers ────────────────────────────────────────
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

    // ─── Flanger parameter pointers ──────────────────────────────────────
    std::atomic<float>* pFlangerOn     = nullptr;  // 0=chorus, 1=flanger
    std::atomic<float>* pFlRate        = nullptr;
    std::atomic<float>* pFlDepth       = nullptr;
    std::atomic<float>* pFlFeedback    = nullptr;
    std::atomic<float>* pFlJitter      = nullptr;
    std::atomic<float>* pFlMix         = nullptr;
    std::atomic<float>* pFlGain        = nullptr;
    std::atomic<float>* pBitGritAmount = nullptr;
    std::atomic<float>* pBitGritSens   = nullptr;
    std::atomic<float>* pFilterCutoff  = nullptr;
    std::atomic<float>* pFilterRes     = nullptr;
    std::atomic<float>* pFilterMix     = nullptr;

    // ─── Granular cloud engine ────────────────────────────────────────────
    static constexpr int kMaxDelaySamples = 192001;
    static constexpr int kNumGrains       = 16;

    struct Grain
    {
        float readPos  = 0.f;
        float speed    = 1.f;
        float pan      = 0.f;
        float envPhase = 0.f;
        float grainLen = 0.f;
        bool  active   = false;
        int   channel  = 0;
    };

    std::array<Grain, kNumGrains> grains;
    juce::AudioBuffer<float>      delayBuffer;
    int   writePos   = 0;
    float spawnTimer = 0.f;

    float chaosSaHTimer = 0.f;
    float chaosSaHValue = 0.f;

    std::array<float, kNumGrains> lfoPhase {};
    std::array<float, kNumGrains> lfoFreq  {};

    // ─── SVF crossover ────────────────────────────────────────────────────
    struct SVF { float ic1eq = 0.f, ic2eq = 0.f; };
    std::array<SVF, 2> svfLow;
    std::array<SVF, 2> svfHigh;
    float svfG  = 1.f;
    float svfR2 = 1.414f;

    void  updateCrossoverCoeffs (float freqHz);
    float tickSVFLP (float x, SVF& s) const;

    // ─── Thru-Zero Flanger ────────────────────────────────────────────────
    static constexpr int kFlMaxDelay = 96001;   // ~1 s @ 96 kHz
    juce::AudioBuffer<float> flDelayBuf;
    int   flWritePos = 0;

    // Per-channel flanger LFO phases
    float flLfoPhaseL = 0.f;
    float flLfoPhaseR = 0.f;

    // Jitter state
    float flJitterL = 0.f, flJitterR = 0.f;
    float flJitterTimer = 0.f;

    // Feedback state
    float flFbL = 0.f, flFbR = 0.f;

    float flReadInterp (int ch, float pos) const;

    // ─── Bit-Grit (dynamic decimation) ───────────────────────────────────
    float bgHoldL = 0.f, bgHoldR = 0.f;
    float bgEnvL  = 0.f, bgEnvR  = 0.f;
    int   bgCounterL = 0, bgCounterR = 0;

    // ─── Melancholy ladder filter ─────────────────────────────────────────
    // 4-stage one-pole cascade per channel (Moog-style simplified)
    std::array<float, 4> ladderL {};
    std::array<float, 4> ladderR {};
    float ladderLfoPhase = 0.f;
    float ladderFbL = 0.f, ladderFbR = 0.f;

    float tickLadder (float x, std::array<float, 4>& stages,
                      float& fb, float cutoff, float res) noexcept;

    // ─── Spectrum analyser ────────────────────────────────────────────────
    std::array<float, 3> inSmooth  {};
    std::array<float, 3> outSmooth {};
    float smoothCoeff = 0.f;

    float visLP_in  = 0.f, visMH_in  = 0.f;
    float visLP_out = 0.f, visMH_out = 0.f;

    void analyseSpectrum (const juce::AudioBuffer<float>& inBuf,
                          const juce::AudioBuffer<float>& outBuf,
                          int numSamples);

    // ─── Misc ─────────────────────────────────────────────────────────────
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

    void processChorus  (juce::AudioBuffer<float>& buffer, int numSamples);
    void processFlanger (juce::AudioBuffer<float>& buffer, int numSamples);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DysmorphiaAudioProcessor)
};
