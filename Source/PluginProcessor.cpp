#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
PersoFormantAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // Vowels: 0-11
    p.push_back (std::make_unique<juce::AudioParameterInt>  ("vowel1",     "Vowel 1",       0, 11,   0));
    p.push_back (std::make_unique<juce::AudioParameterInt>  ("vowel2",     "Vowel 2",       0, 11,   2));

    p.push_back (std::make_unique<juce::AudioParameterFloat>("morph",      "Manual Morph",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));

    // LFO
    p.push_back (std::make_unique<juce::AudioParameterFloat>("lfoRate",    "LFO Rate",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.01f, 0.5f), 1.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>("lfoDepth",   "LFO Depth",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));
    p.push_back (std::make_unique<juce::AudioParameterBool> ("lfoSync",    "LFO Sync",      false));
    // 0=1/32 1=1/16 2=1/8 3=1/4 4=1/2 5=1bar
    p.push_back (std::make_unique<juce::AudioParameterInt>  ("lfoDiv",     "LFO Division",  0, 5,    3));

    // Envelope follower
    p.push_back (std::make_unique<juce::AudioParameterFloat>("envThresh",  "Env Threshold",
        juce::NormalisableRange<float>(-60.f, 0.f, 0.1f), -20.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>("envDepth",   "Env Depth",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 0.f));

    // Formant controls (wet path only — don't touch dry signal level)
    p.push_back (std::make_unique<juce::AudioParameterFloat>("formantRes",  "Formant Res",
        juce::NormalisableRange<float>(0.5f, 20.f, 0.01f, 0.5f), 8.f));
    p.push_back (std::make_unique<juce::AudioParameterFloat>("formantShiftParam", "Formant Shift",
        juce::NormalisableRange<float>(0.5f, 2.0f, 0.001f), 1.0f));

    // Drive (adds harmonics into wet chain only)
    p.push_back (std::make_unique<juce::AudioParameterFloat>("drive",      "Drive",
        juce::NormalisableRange<float>(1.f, 10.f, 0.01f, 0.5f), 1.f));

    // Gender shift still kept as macro shortcut
    p.push_back (std::make_unique<juce::AudioParameterInt>  ("genderShift","Gender Shift",  0, 2,    1));

    // Output — these affect WET signal only; dry is passed through unchanged
    p.push_back (std::make_unique<juce::AudioParameterFloat>("wetGain",    "Wet Gain",
        juce::NormalisableRange<float>(-12.f, 12.f, 0.1f), 0.f));

    // Mix — crossfades dry/wet
    p.push_back (std::make_unique<juce::AudioParameterFloat>("mix",        "Mix",
        juce::NormalisableRange<float>(0.f, 1.f, 0.001f), 1.f));

    return { p.begin(), p.end() };
}

//==============================================================================
PersoFormantAudioProcessor::PersoFormantAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PersoFormantState", createParameterLayout())
{
    for (auto* id : { "vowel1","vowel2","morph","genderShift","formantRes","formantShiftParam" })
        apvts.addParameterListener (id, this);
}

PersoFormantAudioProcessor::~PersoFormantAudioProcessor()
{
    for (auto* id : { "vowel1","vowel2","morph","genderShift","formantRes","formantShiftParam" })
        apvts.removeParameterListener (id, this);
}

void PersoFormantAudioProcessor::parameterChanged (const juce::String&, float) {}

//==============================================================================
FormantData PersoFormantAudioProcessor::interpolateFormants (
    int v1, int v2, float t, float shiftMult) const
{
    v1 = juce::jlimit (0, 11, v1);
    v2 = juce::jlimit (0, 11, v2);
    t  = juce::jlimit (0.f, 1.f, t);
    const FormantData& f1 = kVowelFormants[v1];
    const FormantData& f2 = kVowelFormants[v2];
    FormantData out;
    for (int k = 0; k < 3; ++k)
    {
        out.freq[k] = (f1.freq[k] + t*(f2.freq[k]-f1.freq[k])) * shiftMult;
        out.gain[k] =  f1.gain[k] + t*(f2.gain[k]-f1.gain[k]);
        out.q[k]    =  f1.q[k]   + t*(f2.q[k]   -f1.q[k]);
    }
    return out;
}

//==============================================================================
float PersoFormantAudioProcessor::getLfoRateHz() const
{
    bool sync = (bool)(int)getParam ("lfoSync");
    if (!sync)
        return getParam ("lfoRate");

    // BPM sync
    double bpm = 120.0;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
            if (pos->getBpm().hasValue())
                bpm = *pos->getBpm();
    }
    int div = juce::jlimit (0, 5, (int)getParam ("lfoDiv"));
    float beats = kSyncDivBeats[div];          // beats per cycle
    float beatsPerSec = (float)(bpm / 60.0);   // beats/sec
    return beatsPerSec / beats;                 // cycles/sec = Hz
}

//==============================================================================
// Called once per block — block-rate glide keeps transitions smooth
void PersoFormantAudioProcessor::updateFormantCoeffs (float morph, float shiftMult, float sr)
{
    int v1 = juce::jlimit (0, 11, (int)getParam ("vowel1"));
    int v2 = juce::jlimit (0, 11, (int)getParam ("vowel2"));
    float userRes   = getParam ("formantRes");   // per-band Q override
    FormantData fd  = interpolateFormants (v1, v2, morph, shiftMult);

    for (int k = 0; k < 3; ++k)
    {
        targetFreq[k] = fd.freq[k];
        targetGain[k] = fd.gain[k];
        // User Q control: userRes scales the preset Q
        targetQ[k]    = fd.q[k] * (userRes / 8.f);

        currentFreq[k] = currentFreq[k]*freqGlideCoeff + targetFreq[k]*(1.f-freqGlideCoeff);
        currentGain[k] = currentGain[k]*freqGlideCoeff + targetGain[k]*(1.f-freqGlideCoeff);
        currentQ[k]    = currentQ[k]   *freqGlideCoeff + targetQ[k]   *(1.f-freqGlideCoeff);

        float safeFreq = juce::jlimit (20.f, (float)(sr*0.49), currentFreq[k]);
        float safeQ    = juce::jlimit (0.5f, 50.f, currentQ[k]);

        for (int ch = 0; ch < 2; ++ch)
            formantFilters[ch][k].setCoefficients (safeFreq, currentGain[k], safeQ, (float)sr);
    }
}

//==============================================================================
void PersoFormantAudioProcessor::prepareToPlay (double sr, int blockSize)
{
    currentSampleRate = sr;
    currentBlockSize  = blockSize;

    double bps = sr / (double)juce::jmax (1, blockSize);
    freqGlideCoeff = (float)std::exp (-1.0 / (bps * 0.030));

    for (int ch = 0; ch < 2; ++ch)
    {
        for (int k = 0; k < 3; ++k) formantFilters[ch][k].reset();
        morphSmoother[ch].reset (sr, 20.0);
        envFollower[ch].reset (sr);
    }

    int v1 = juce::jlimit (0, 11, (int)getParam ("vowel1"));
    for (int k = 0; k < 3; ++k)
    {
        currentFreq[k] = targetFreq[k] = kVowelFormants[v1].freq[k];
        currentGain[k] = targetGain[k] = kVowelFormants[v1].gain[k];
        currentQ[k]    = targetQ[k]    = kVowelFormants[v1].q[k];
    }
    updateFormantCoeffs (0.f, 1.f, sr);
}

void PersoFormantAudioProcessor::releaseResources() {}

//==============================================================================
void PersoFormantAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int N  = buffer.getNumSamples();
    const int CH = juce::jmin (buffer.getNumChannels(), 2);
    if (N == 0 || CH == 0) return;

    // --- Parameters ---
    float manualMorph = getParam ("morph");
    float lfoDepth    = getParam ("lfoDepth");
    float envThreshDB = getParam ("envThresh");
    float envDepth    = getParam ("envDepth");
    float drive       = getParam ("drive");
    int   gender      = (int)getParam ("genderShift");
    float wetGainDB   = getParam ("wetGain");
    float mix         = getParam ("mix");
    float userShift   = getParam ("formantShiftParam");

    // Gender macro applies a coarse offset on top of the user shift knob
    float genderMult  = (gender==0) ? 0.87f : (gender==2) ? 1.12f : 1.0f;
    float shiftMult   = userShift * genderMult;

    float wetGain     = juce::Decibels::decibelsToGain (wetGainDB);
    float envThreshLin= juce::Decibels::decibelsToGain (envThreshDB);

    // --- LFO (block-rate) ---
    float lfoHz  = getLfoRateHz();
    float lfoInc = juce::MathConstants<float>::twoPi * lfoHz / (float)currentSampleRate;
    float lfoVal = std::sin (lfoPhase) * 0.5f + 0.5f;
    lfoPhase += lfoInc * (float)N;
    if (lfoPhase >= juce::MathConstants<float>::twoPi)
        lfoPhase -= juce::MathConstants<float>::twoPi;

    // --- Envelope follower (block average) ---
    float envIn = 0.f;
    for (int ch=0; ch<CH; ++ch)
        for (int s=0; s<N; ++s)
            envIn += std::abs (buffer.getSample(ch,s));
    envIn /= (float)(CH*N);

    float envVal = envFollower[0].process (envIn);
    envelopeLevel.store (envVal);

    float envMod = 0.f;
    if (envVal > envThreshLin)
        envMod = juce::jlimit (0.f, 1.f, (envVal-envThreshLin)/(1.f-envThreshLin+1e-6f));

    float finalMorph = juce::jlimit (0.f, 1.f, manualMorph + lfoDepth*lfoVal + envDepth*envMod);
    currentMorphDisplay.store (finalMorph);

    // --- Update formant filters once per block ---
    updateFormantCoeffs (finalMorph, shiftMult, currentSampleRate);

    // --- Save dry copy ---
    juce::AudioBuffer<float> dry;
    dry.makeCopyOf (buffer);

    // --- Wet processing ---
    for (int s=0; s<N; ++s)
        for (int ch=0; ch<CH; ++ch)
        {
            float x = buffer.getSample (ch, s);
            // Drive adds harmonics so formants have richer partials to grab
            if (drive > 1.001f) x = softClip (x, drive);
            // Three serial formant bands
            for (int k=0; k<3; ++k) x = formantFilters[ch][k].process(x);
            buffer.setSample (ch, s, x);
        }

    // --- Apply wet gain (only scales the wet signal, not the dry) ---
    buffer.applyGain (wetGain);

    // --- Dry/wet crossfade ---
    for (int ch=0; ch<CH; ++ch)
    {
        auto* w = buffer.getWritePointer(ch);
        auto* d = dry.getReadPointer(ch);
        for (int s=0; s<N; ++s)
            w[s] = d[s]*(1.f-mix) + w[s]*mix;
    }
}

//==============================================================================
void PersoFormantAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, dest);
}

void PersoFormantAudioProcessor::setStateInformation (const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, size));
    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessorEditor* PersoFormantAudioProcessor::createEditor()
{
    return new PersoFormantAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PersoFormantAudioProcessor();
}
