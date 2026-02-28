#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

SaturaturProcessor::SaturaturProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParams())
{}

juce::AudioProcessorValueTreeState::ParameterLayout SaturaturProcessor::createParams(){
    return {
        std::make_unique<juce::AudioParameterFloat>("bias",   "GRIT",   0.0f, 1.0f, 0.3f),
        std::make_unique<juce::AudioParameterFloat>("param2", "TONE",   0.0f, 1.0f, 0.5f),
        std::make_unique<juce::AudioParameterFloat>("param3", "WARMTH", 0.0f, 1.0f, 0.4f),
        std::make_unique<juce::AudioParameterFloat>("param4", "ATTACK", 0.0f, 1.0f, 0.3f),
        std::make_unique<juce::AudioParameterFloat>("output", "OUTPUT", 0.0f, 1.0f, 0.6f),
        std::make_unique<juce::AudioParameterFloat>("mix",    "MIX",    0.0f, 1.0f, 0.8f),
        std::make_unique<juce::AudioParameterFloat>("drive",  "DRIVE",  0.0f, 1.0f, 0.35f),
        std::make_unique<juce::AudioParameterFloat>("type",   "TYPE",   0.0f, 1.0f, 0.0f),
        std::make_unique<juce::AudioParameterFloat>("param9", "COMP",   0.0f, 1.0f, 0.2f)
    };
}

// ── SATURATION ALGORITHMS ─────────────────────────────────────────

// TAPE — warm tanh, grit adds odd harmonics via polynomial
float SaturaturProcessor::saturateTape(float x, float drive, float grit){
    const float g = 1.0f + drive * 8.0f;
    float s = std::tanh(x * g) / std::tanh(g);
    // Grit: adds odd-order harmonic content (increased from 0.3f to 0.8f)
    if(grit > 0.0f)
        s += grit * 0.8f * (s*s*s - s);
    return s;
}

// TUBE — asymmetric, even harmonics, very musical
float SaturaturProcessor::saturateTube(float x, float drive, float grit){
    const float g = 1.0f + drive * 6.0f;
    // Asymmetric waveshaper — different curves per half
    float s;
    if(x >= 0.0f)
        s = 1.0f - std::exp(-x * g);
    else
        s = -(1.0f - std::exp(x * g * 0.7f)) * 1.1f;
    // Grit adds presence via 2nd+3rd harmonics (increased from 0.2f to 0.6f)
    s += grit * 0.6f * s * s * (1.0f - std::abs(s));
    return juce::jlimit(-1.0f, 1.0f, s);
}

// CLIP — hard clip with variable knee, grit makes it crunchier
float SaturaturProcessor::saturateClip(float x, float drive, float grit){
    const float g    = 1.0f + drive * 12.0f;
    const float knee = 0.85f - grit * 0.3f; // grit tightens the knee
    float driven = x * g;
    if     (driven >  knee) driven =  knee + (1.0f - knee) * std::tanh((driven - knee) * (3.0f + grit * 5.0f));
    else if(driven < -knee) driven = -knee - (1.0f - knee) * std::tanh((-driven - knee) * (3.0f + grit * 5.0f));
    return juce::jlimit(-1.0f, 1.0f, driven * (1.0f / (knee + 0.15f)));
}

// FOLD — wavefolder, grit adds extra folds
float SaturaturProcessor::saturateFold(float x, float drive, float grit){
    const float g = 1.0f + drive * 4.0f + grit * 4.0f;
    float driven = x * g;
    // Multi-fold
    for(int i = 0; i < 4; i++){
        if     (driven >  1.0f) driven =  2.0f - driven;
        else if(driven < -1.0f) driven = -2.0f - driven;
        else break;
    }
    return driven * 0.8f;
}

void SaturaturProcessor::prepareToPlay(double sr, int samplesPerBlock){
    sampleRate = sr;
    toneLoL = toneLoR = warmLoL = warmLoR = 0.0f;
    envL = envR = dcL = dcR = dcPrevL = dcPrevR = 0.0f;
    compGainL = compGainR = 1.0f;

    smDrive.reset(sr,  0.02); smDrive.setCurrentAndTargetValue(0.35f);
    smGrit.reset(sr,   0.02); smGrit.setCurrentAndTargetValue(0.3f);
    smTone.reset(sr,   0.02); smTone.setCurrentAndTargetValue(0.5f);
    smWarmth.reset(sr, 0.05); smWarmth.setCurrentAndTargetValue(0.4f);
    smAttack.reset(sr, 0.05); smAttack.setCurrentAndTargetValue(0.3f);
    smOutput.reset(sr, 0.02); smOutput.setCurrentAndTargetValue(0.6f);
    smMix.reset(sr,    0.02); smMix.setCurrentAndTargetValue(0.8f);
    smType.reset(sr,   0.08); smType.setCurrentAndTargetValue(0.0f);
    smComp.reset(sr,   0.05); smComp.setCurrentAndTargetValue(0.2f);
}

void SaturaturProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&){
    juce::ScopedNoDenormals noDenormals;

    smDrive.setTargetValue (*apvts.getRawParameterValue("drive"));
    smGrit.setTargetValue  (*apvts.getRawParameterValue("bias"));
    smTone.setTargetValue  (*apvts.getRawParameterValue("param2"));
    smWarmth.setTargetValue(*apvts.getRawParameterValue("param3"));
    smAttack.setTargetValue(*apvts.getRawParameterValue("param4"));
    smOutput.setTargetValue(*apvts.getRawParameterValue("output"));
    smMix.setTargetValue   (*apvts.getRawParameterValue("mix"));
    smType.setTargetValue  (*apvts.getRawParameterValue("type"));
    smComp.setTargetValue  (*apvts.getRawParameterValue("param9"));

    const int N  = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    auto* L = buffer.getWritePointer(0);
    auto* R = ch > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    const float dcCoef = 1.0f - (float)(2.0 * juce::MathConstants<double>::pi * 20.0 / sampleRate);

    for(int i = 0; i < N; i++){
        const float drive  = smDrive.getNextValue();
        const float grit   = smGrit.getNextValue();
        const float tone   = smTone.getNextValue();
        const float warmth = smWarmth.getNextValue();
        const float attack = smAttack.getNextValue();
        const float output = smOutput.getNextValue();
        const float mix    = smMix.getNextValue();
        const float type   = smType.getNextValue();
        const float comp   = smComp.getNextValue();

        const float dry0 = L[i], dry1 = R[i];

        // ── ATTACK: envelope-based transient control ──────────────
        // attack=0: saturation hits hard on transients (punch)
        // attack=1: saturation smoothed — more sustain, less punch
        const float atkFast = 0.002f;
        const float atkSlow = 0.001f + attack * 0.12f;
        const float aL = std::abs(dry0), aR = std::abs(dry1);
        envL = aL > envL ? envL + (aL - envL) * atkFast : envL + (aL - envL) * atkSlow;
        envR = aR > envR ? envR + (aR - envR) * atkFast : envR + (aR - envR) * atkSlow;
        // Reduce drive on transients when attack is low (preserve punch) - increased from 0.5f to 0.85f
        const float tDriveL = drive * (1.0f - (1.0f - attack) * 0.85f * juce::jmin(envL * 3.0f, 1.0f));
        const float tDriveR = drive * (1.0f - (1.0f - attack) * 0.85f * juce::jmin(envR * 3.0f, 1.0f));

        // ── SATURATION TYPE (smooth crossfade between 4 modes) ────
        const float t3   = type * 3.0f;
        const int   ti   = juce::jmin((int)t3, 2);
        const float tf   = t3 - (float)ti;

        auto getSat = [&](float x, float d, int mode) -> float {
            switch(mode){
                case 0: return saturateTape(x, d, grit);
                case 1: return saturateTube(x, d, grit);
                case 2: return saturateClip(x, d, grit);
                default: return saturateFold(x, d, grit);
            }
        };

        float wetL = getSat(dry0, tDriveL, ti) * (1.0f - tf) + getSat(dry0, tDriveL, ti+1) * tf;
        float wetR = getSat(dry1, tDriveR, ti) * (1.0f - tf) + getSat(dry1, tDriveR, ti+1) * tf;

        // ── DC BLOCKER ────────────────────────────────────────────
        float newDcL = wetL + dcCoef * dcL - dcPrevL;
        dcPrevL = wetL; dcL = newDcL; wetL = newDcL;
        float newDcR = wetR + dcCoef * dcR - dcPrevR;
        dcPrevR = wetR; dcR = newDcR; wetR = newDcR;

        // ── WARMTH — low-mid shelf boost on wet signal ────────────
        // Adds body and fullness — very audible and musical
        const float warmFreq = 300.0f;
        const float warmC    = 1.0f - (float)(2.0 * juce::MathConstants<double>::pi * warmFreq / sampleRate);
        warmLoL = warmLoL * warmC + wetL * (1.0f - warmC);
        warmLoR = warmLoR * warmC + wetR * (1.0f - warmC);
        const float warmAmt = warmth * 1.5f; // up to +150% low-mid boost
        wetL += warmAmt * warmLoL;
        wetR += warmAmt * warmLoR;

        // ── TONE — tilt EQ (dark to bright) ──────────────────────
        const float lpFreq = 500.0f + tone * 14000.0f;
        const float lpC    = 1.0f - (float)(2.0 * juce::MathConstants<double>::pi * lpFreq / sampleRate);
        toneLoL = toneLoL * lpC + wetL * (1.0f - lpC);
        toneLoR = toneLoR * lpC + wetR * (1.0f - lpC);
        if(tone < 0.5f){
            // Dark — blend toward LP
            wetL = toneLoL + (tone * 2.0f) * (wetL - toneLoL);
            wetR = toneLoR + (tone * 2.0f) * (wetR - toneLoR);
        } else {
            // Bright — boost highs (increased from 1.2f to 2.5f)
            wetL = wetL + (tone - 0.5f) * 2.5f * (wetL - toneLoL);
            wetR = wetR + (tone - 0.5f) * 2.5f * (wetR - toneLoR);
        }

        // ── COMP — soft saturation compression ───────────────────
        // Reduces gain as signal gets louder — adds glue and density
        if(comp > 0.0f){
            const float compThresh = 1.0f - comp * 0.85f; // more aggressive threshold
            const float compRatio  = 1.0f + comp * 8.0f;  // increased ratio from 4.0 to 8.0
            const float compAttack = 0.001f;
            const float compRel    = 0.0001f + (1.0f - comp) * 0.05f;
            const float levL = std::abs(wetL), levR = std::abs(wetR);
            if(levL > compThresh)
                compGainL += ((compThresh + (levL - compThresh) / compRatio) / juce::jmax(levL, 0.001f) - compGainL) * compAttack;
            else
                compGainL += (1.0f - compGainL) * compRel;
            if(levR > compThresh)
                compGainR += ((compThresh + (levR - compThresh) / compRatio) / juce::jmax(levR, 0.001f) - compGainR) * compAttack;
            else
                compGainR += (1.0f - compGainR) * compRel;
            compGainL = juce::jlimit(0.1f, 1.0f, compGainL);
            compGainR = juce::jlimit(0.1f, 1.0f, compGainR);
            wetL *= compGainL;
            wetR *= compGainR;
        }

        // ── OUTPUT GAIN ───────────────────────────────────────────
        // 0.5 = unity, range ±12dB
        const float outGain = std::pow(10.0f, (output - 0.5f) * 24.0f / 20.0f);

        // ── PARALLEL MIX + SAFETY CLIP ───────────────────────────
        L[i] = juce::jlimit(-1.0f, 1.0f, ((1.0f - mix) * dry0 + mix * wetL) * outGain);
        R[i] = juce::jlimit(-1.0f, 1.0f, ((1.0f - mix) * dry1 + mix * wetR) * outGain);
    }
}

void SaturaturProcessor::getStateInformation(juce::MemoryBlock& destData){
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void SaturaturProcessor::setStateInformation(const void* data, int sizeInBytes){
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if(xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){
    return new SaturaturProcessor();
}
