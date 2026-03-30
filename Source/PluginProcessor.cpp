#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DysmorphiaAudioProcessor::DysmorphiaAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (auto& a : spectrumData)
        a.store (0.f);

    pRate    = apvts.getRawParameterValue ("rate");
    pDepth   = apvts.getRawParameterValue ("depth");
    pDensity = apvts.getRawParameterValue ("density");
    pSpread  = apvts.getRawParameterValue ("spread");
    pChaos   = apvts.getRawParameterValue ("chaos");
    pXover   = apvts.getRawParameterValue ("xover");
    pSpecOff = apvts.getRawParameterValue ("specOff");
    pMix     = apvts.getRawParameterValue ("mix");
    pWidth   = apvts.getRawParameterValue ("width");
    pGain    = apvts.getRawParameterValue ("gain");

    apvts.addParameterListener ("xover",   this);
    apvts.addParameterListener ("rate",    this);
    apvts.addParameterListener ("density", this);
}

DysmorphiaAudioProcessor::~DysmorphiaAudioProcessor()
{
    apvts.removeParameterListener ("xover",   this);
    apvts.removeParameterListener ("rate",    this);
    apvts.removeParameterListener ("density", this);
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
DysmorphiaAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "rate",    "Rate",
        juce::NormalisableRange<float> (0.05f, 8.f, 0.01f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "depth",   "Depth",
        juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.4f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "density", "Density",
        juce::NormalisableRange<float> (1.f, 16.f, 1.f), 6.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "spread",  "Spread",
        juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "chaos",   "Chaos",
        juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "xover",   "X-Over",
        juce::NormalisableRange<float> (80.f, 800.f, 1.f, 0.5f), 200.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "specOff", "Spec Off",
        juce::NormalisableRange<float> (0.f, 4.f, 0.01f), 1.f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix",     "Mix",
        juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "width",   "Width",
        juce::NormalisableRange<float> (0.f, 1.f, 0.001f), 0.7f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "gain",    "Gain",
        juce::NormalisableRange<float> (-18.f, 18.f, 0.1f), 0.f));

    return { params.begin(), params.end() };
}

//==============================================================================
void DysmorphiaAudioProcessor::parameterChanged (const juce::String& paramID,
                                                  float newValue)
{
    if (paramID == "xover")
        updateCrossoverCoeffs (newValue);
}

//==============================================================================
void DysmorphiaAudioProcessor::prepareToPlay (double sRate, int /*samplesPerBlock*/)
{
    currentSR = sRate;

    delayBuffer.setSize (2, kMaxDelaySamples, false, true, false);
    delayBuffer.clear();
    writePos   = 0;
    spawnTimer = 0.f;

    for (auto& g : grains)
        g = Grain{};

    for (size_t i = 0; i < kNumGrains; ++i)
    {
        lfoPhase[i] = randF (0.f, juce::MathConstants<float>::twoPi);
        lfoFreq[i]  = randF (0.1f, 1.5f);
    }

    chaosSaHTimer = 0.f;
    chaosSaHValue = 0.f;

    // Reset crossover filter states
    for (auto& s : svfLow)  { s.ic1eq = 0.f; s.ic2eq = 0.f; }
    for (auto& s : svfHigh) { s.ic1eq = 0.f; s.ic2eq = 0.f; }
    updateCrossoverCoeffs (pXover->load());

    // Spectrum smoothing (~50 ms time constant)
    smoothCoeff = std::exp (-1.f / (static_cast<float>(sRate) * 0.05f));
    inSmooth    = {};
    outSmooth   = {};
    visLP_in  = 0.f;  visMH_in  = 0.f;
    visLP_out = 0.f;  visMH_out = 0.f;
}

void DysmorphiaAudioProcessor::releaseResources() {}

//==============================================================================
bool DysmorphiaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    const auto in = layouts.getMainInputChannelSet();
    if (in != juce::AudioChannelSet::stereo() &&
        in != juce::AudioChannelSet::mono())
        return false;
    return true;
}

//==============================================================================
// State-variable filter — returns LP output; HP = x - LP
float DysmorphiaAudioProcessor::tickSVFLP (float x, SVF& s) const
{
    const float v1 = svfG * (x - s.ic2eq) / (1.f + svfG * (svfR2 + svfG));
    const float v2 = s.ic2eq + svfG * v1;
    s.ic1eq = 2.f * v1 - s.ic1eq;
    s.ic2eq = 2.f * v2 - s.ic2eq;
    return v2;
}

void DysmorphiaAudioProcessor::updateCrossoverCoeffs (float freqHz)
{
    const float fc = juce::jlimit (20.f, 20000.f, freqHz);
    svfG  = std::tan (juce::MathConstants<float>::pi
                      * fc / static_cast<float>(currentSR));
    svfR2 = 1.414f;   // √2 → Butterworth
}

//==============================================================================
float DysmorphiaAudioProcessor::readInterp (int ch, float pos) const
{
    const int   bufLen = delayBuffer.getNumSamples();
    const float* data  = delayBuffer.getReadPointer (ch);

    // Wrap into [0, bufLen)
    int   i0 = static_cast<int>(pos) % bufLen;
    if (i0 < 0) i0 += bufLen;
    const int   i1   = (i0 + 1) % bufLen;
    const float frac = pos - std::floor (pos);

    return data[i0] + frac * (data[i1] - data[i0]);
}

void DysmorphiaAudioProcessor::spawnGrain (int idx, int ch, float baseDelay,
                                            float pitch, float pan, float len)
{
    auto& g    = grains[static_cast<size_t>(idx)];
    g.channel  = ch;
    const int bufLen = delayBuffer.getNumSamples();
    // Start reading from: writePos - baseDelay (samples behind current write)
    float rp = static_cast<float>(writePos) - baseDelay;
    while (rp < 0.f) rp += static_cast<float>(bufLen);
    g.readPos  = rp;
    g.speed    = pitch;
    g.pan      = pan;
    g.envPhase = 0.f;
    g.grainLen = juce::jmax (1.f, len);
    g.active   = true;
}

//==============================================================================
void DysmorphiaAudioProcessor::analyseSpectrum (const juce::AudioBuffer<float>& inBuf,
                                                 const juce::AudioBuffer<float>& outBuf,
                                                 int numSamples)
{
    if (numSamples <= 0) return;

    const int nCh = juce::jmin (inBuf.getNumChannels(), 2);

    const float xf    = pXover->load();
    const float capL  = juce::jlimit (0.f, 0.99f,
                            2.f * xf / static_cast<float>(currentSR));
    const float capH  = juce::jlimit (0.f, 0.99f,
                            2.f * (xf * 4.f) / static_cast<float>(currentSR));

    float inLow = 0.f, inMid = 0.f, inHigh = 0.f;
    float outLow = 0.f, outMid = 0.f, outHigh = 0.f;

    for (int s = 0; s < numSamples; ++s)
    {
        float xi = 0.f, xo = 0.f;
        for (int c = 0; c < nCh; ++c)
        {
            xi += inBuf .getReadPointer (c)[s];
            xo += outBuf.getReadPointer (c)[s];
        }
        if (nCh > 1) { xi *= 0.5f; xo *= 0.5f; }

        // One-pole band-split (visualiser only — does not touch audio path)
        visLP_in  += capL * (xi - visLP_in);
        visMH_in  += capH * (xi - visMH_in);
        visLP_out += capL * (xo - visLP_out);
        visMH_out += capH * (xo - visMH_out);

        const float midIn  = visLP_in  - visMH_in;   // band between LP and 4×LP
        const float midOut = visLP_out - visMH_out;

        // Actually: low = visLP, mid = band between LP and MH, high = xi - visMH
        // Let's be explicit:
        inLow  += visLP_in  * visLP_in;
        inMid  += midIn     * midIn;
        inHigh += (xi - visMH_in)  * (xi - visMH_in);

        outLow  += visLP_out * visLP_out;
        outMid  += midOut    * midOut;
        outHigh += (xo - visMH_out) * (xo - visMH_out);
    }

    const float invN = 1.f / static_cast<float>(numSamples);
    const float a    = smoothCoeff;
    const float b    = 1.f - a;

    auto smooth = [&](float& smth, float sumSq)
    {
        smth = a * smth + b * std::sqrt (sumSq * invN);
    };

    smooth (inSmooth[0],  inLow);
    smooth (inSmooth[1],  inMid);
    smooth (inSmooth[2],  inHigh);
    smooth (outSmooth[0], outLow);
    smooth (outSmooth[1], outMid);
    smooth (outSmooth[2], outHigh);

    for (size_t i = 0; i < 3; ++i)
    {
        spectrumData[i].store     (inSmooth[i]);
        spectrumData[i + 3].store (outSmooth[i]);
    }
}

//==============================================================================
void DysmorphiaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numCh      = buffer.getNumChannels();
    if (numSamples == 0 || numCh < 2) return;

    // Capture input for analysis before we overwrite it
    juce::AudioBuffer<float> inCopy;
    inCopy.makeCopyOf (buffer);

    // Load parameters once per block
    const float rate     = pRate->load();
    const float depth    = pDepth->load();
    const float density  = pDensity->load();
    const float spread   = pSpread->load();
    const float chaos    = pChaos->load();
    const float specOff  = pSpecOff->load();
    const float mix      = pMix->load();
    const float width    = pWidth->load();
    const float gainLin  = juce::Decibels::decibelsToGain (pGain->load());

    const int   bufLen      = delayBuffer.getNumSamples();
    const float srF         = static_cast<float>(currentSR);

    // Grain parameters
    const float grainLenSmp  = srF * 0.08f;           // 80 ms grains
    const float maxDelaySmp  = srF * 0.03f;           // up to 30 ms base delay
    const float spawnInterval = grainLenSmp / juce::jmax (1.f, density);

    // Chaos S&H rate: 2 .. 20 Hz
    const float saHRate = 2.f + chaos * 18.f;

    for (int s = 0; s < numSamples; ++s)
    {
        const float inL = buffer.getSample (0, s);
        const float inR = buffer.getSample (1, s);

        // Write input into ring buffer
        delayBuffer.setSample (0, writePos, inL);
        delayBuffer.setSample (1, writePos, inR);

        // --- Chaos S&H ---
        chaosSaHTimer += 1.f / srF;
        if (chaosSaHTimer >= (1.f / saHRate))
        {
            chaosSaHTimer = 0.f;
            chaosSaHValue = randF (-1.f, 1.f);
        }

        // --- Grain spawn ---
        spawnTimer += 1.f;
        if (spawnTimer >= spawnInterval)
        {
            spawnTimer = 0.f;

            for (int ch = 0; ch < 2; ++ch)
            {
                for (size_t gi = 0; gi < kNumGrains; ++gi)
                {
                    if (!grains[gi].active)
                    {
                        // Pitch detune (spread + chaos jitter)
                        const float detuneSemis =
                            spread * 0.25f + chaos * chaosSaHValue * 0.15f;
                        const float pitchRatio =
                            std::pow (2.f, detuneSemis / 12.f);

                        // Base delay + random scatter
                        float baseDelay =
                            srF * 0.010f + randF (0.f, maxDelaySmp * spread);
                        baseDelay = juce::jlimit (
                            1.f, static_cast<float>(bufLen - 2), baseDelay);

                        // Stereo pan: L grains go left, R grains go right
                        const float pan =
                            (ch == 0 ? -1.f : 1.f) * (0.3f + spread * 0.7f);

                        spawnGrain (gi, ch, baseDelay, pitchRatio, pan,
                                    grainLenSmp);
                        break;
                    }
                }
            }
        }

        // --- Accumulate grain outputs ---
        float wetL = 0.f, wetR = 0.f;
        int   activeCount = 0;

        for (size_t gi = 0; gi < kNumGrains; ++gi)
        {
            auto& g = grains[gi];
            if (!g.active) continue;

            // Advance LFO
            const float freqMult = (g.channel == 1)
                                    ? juce::jmax (0.1f, specOff) : 1.f;
            lfoPhase[gi] += lfoFreq[gi] * freqMult
                            * juce::MathConstants<float>::twoPi / srF;
            if (lfoPhase[gi] > juce::MathConstants<float>::twoPi)
                lfoPhase[gi] -= juce::MathConstants<float>::twoPi;

            const float lfoSin = std::sin (lfoPhase[gi]);

            // Chaos jitter on delay time
            const float jitter = chaos * chaosSaHValue * maxDelaySmp * 0.12f;

            // Modulate read position
            const float modDepth  = depth * maxDelaySmp * 0.25f;
            const float modOffset = rate * lfoSin * modDepth + jitter;

            float readIdx = g.readPos + modOffset;
            // Wrap into [0, bufLen)
            readIdx = std::fmod (readIdx, static_cast<float>(bufLen));
            if (readIdx < 0.f) readIdx += static_cast<float>(bufLen);

            const float sample = readInterp (g.channel, readIdx);

            // Hann window envelope
            const float env = 0.5f * (1.f - std::cos (
                juce::MathConstants<float>::twoPi * g.envPhase));

            // Constant-power panning
            const float panAngle =
                (g.pan + 1.f) * 0.5f * juce::MathConstants<float>::halfPi;
            const float panL = std::cos (panAngle);
            const float panR = std::sin (panAngle);

            wetL += sample * env * panL;
            wetR += sample * env * panR;
            ++activeCount;

            // Advance grain read position
            g.readPos += g.speed;
            if (g.readPos >= static_cast<float>(bufLen))
                g.readPos -= static_cast<float>(bufLen);

            // Advance envelope
            g.envPhase += 1.f / g.grainLen;
            if (g.envPhase >= 1.f)
                g.active = false;
        }

        // Normalise grain cloud
        if (activeCount > 0)
        {
            const float norm = 1.f / std::sqrt (static_cast<float>(activeCount));
            wetL *= norm;
            wetR *= norm;
        }

        // --- Frequency split ---
        // Low band: dry. Mid+High band: wet mix applied.
        const float lpL = tickSVFLP (inL, svfLow[0]);
        const float lpR = tickSVFLP (inR, svfLow[1]);
        const float hpL = inL - lpL;
        const float hpR = inR - lpR;

        const float outL = lpL + (hpL * (1.f - mix) + wetL * mix);
        const float outR = lpR + (hpR * (1.f - mix) + wetR * mix);

        // --- Width & gain ---
        const float mid  = (outL + outR) * 0.5f;
        const float side = (outL - outR) * 0.5f * (1.f + width);

        buffer.setSample (0, s, (mid + side) * gainLin);
        buffer.setSample (1, s, (mid - side) * gainLin);

        // Advance write pointer
        writePos = (writePos + 1) % bufLen;
    }

    analyseSpectrum (inCopy, buffer, numSamples);
}

//==============================================================================
// Boilerplate
//==============================================================================
const juce::String DysmorphiaAudioProcessor::getName() const { return "Dysmorphia"; }
bool   DysmorphiaAudioProcessor::acceptsMidi()  const { return false; }
bool   DysmorphiaAudioProcessor::producesMidi() const { return false; }
bool   DysmorphiaAudioProcessor::isMidiEffect() const { return false; }
double DysmorphiaAudioProcessor::getTailLengthSeconds() const { return 0.1; }

int  DysmorphiaAudioProcessor::getNumPrograms()    { return 1; }
int  DysmorphiaAudioProcessor::getCurrentProgram() { return 0; }
void DysmorphiaAudioProcessor::setCurrentProgram (int) {}
const juce::String DysmorphiaAudioProcessor::getProgramName (int) { return {}; }
void DysmorphiaAudioProcessor::changeProgramName (int, const juce::String&) {}

bool DysmorphiaAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* DysmorphiaAudioProcessor::createEditor()
{
    return new DysmorphiaAudioProcessorEditor (*this);
}

void DysmorphiaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void DysmorphiaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DysmorphiaAudioProcessor();
}
