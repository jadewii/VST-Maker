#include "PluginProcessor.h"
#include "PluginEditor.h"

DreamverbProcessor::DreamverbProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParams())
{}

juce::AudioProcessorValueTreeState::ParameterLayout DreamverbProcessor::createParams(){
    return {
        std::make_unique<juce::AudioParameterFloat>("mix",   "MIX",   0.0f, 1.0f, 0.4f),
        std::make_unique<juce::AudioParameterFloat>("size",  "SIZE",  0.0f, 1.0f, 0.6f),
        std::make_unique<juce::AudioParameterFloat>("damp",  "DAMP",  0.0f, 1.0f, 0.3f),
        std::make_unique<juce::AudioParameterFloat>("tone",  "TONE",  0.0f, 1.0f, 0.5f)
    };
}

void DreamverbProcessor::initBuffers(double sr){
    const double r = sr / 29761.0;
    ap1.init((int)(142*r)); ap2.init((int)(107*r));
    ap3.init((int)(379*r)); ap4.init((int)(277*r));
    tapL1.init((int)(672*r));  tapL2.init((int)(1800*r));
    tapR1.init((int)(908*r));  tapR2.init((int)(2656*r));
    dL1.init((int)(4453*r));   dL2.init((int)(3720*r));
    dR1.init((int)(4217*r));   dR2.init((int)(3163*r));
    lpL = 0.f; lpR = 0.f;
    toneLoL = toneLoR = toneHiL = toneHiR = 0.f;
}

void DreamverbProcessor::prepareToPlay(double sr, int samplesPerBlock){
    sampleRate = sr;
    initBuffers(sr);
    smoothedMix.reset(sr,  0.02); smoothedMix.setCurrentAndTargetValue(0.4f);
    smoothedSize.reset(sr, 0.05); smoothedSize.setCurrentAndTargetValue(0.6f);
    smoothedDamp.reset(sr, 0.05); smoothedDamp.setCurrentAndTargetValue(0.3f);
    smoothedTone.reset(sr, 0.05); smoothedTone.setCurrentAndTargetValue(0.5f);
}

void DreamverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&){
    juce::ScopedNoDenormals noDenormals;

    smoothedMix.setTargetValue (*apvts.getRawParameterValue("mix"));
    smoothedSize.setTargetValue(*apvts.getRawParameterValue("size"));
    smoothedDamp.setTargetValue(*apvts.getRawParameterValue("damp"));
    smoothedTone.setTargetValue(*apvts.getRawParameterValue("tone"));

    const int N  = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    auto* L = buffer.getWritePointer(0);
    auto* R = ch > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    for(int i = 0; i < N; i++){
        const float mix  = smoothedMix.getNextValue();
        const float size = smoothedSize.getNextValue();
        const float damp = smoothedDamp.getNextValue();
        const float tone = smoothedTone.getNextValue();

        const float dry0 = L[i];
        const float dry1 = R[i];

        // Input diffusion
        float mono = (dry0 + dry1) * 0.5f;
        float d = ap1.process(mono, 0.75f);
        d = ap2.process(d, 0.75f);
        d = ap3.process(d, 0.625f);
        d = ap4.process(d, 0.625f);

        // Dattorro plate tank
        const float decay    = 0.5f + size * 0.45f;
        const float dampCoef = 1.0f - damp * 0.7f;

        float nodeL = d + decay * dR2.read(dR2.size-1);
        nodeL = tapL1.process(nodeL, 0.7f);
        dL1.push(nodeL);
        lpL = lpL + dampCoef * (dL1.read(dL1.size-1) - lpL);
        float tankL = decay * lpL;
        tankL = tapL2.process(tankL, 0.5f);
        dL2.push(tankL);

        float nodeR = d + decay * dL2.read(dL2.size-1);
        nodeR = tapR1.process(nodeR, 0.7f);
        dR1.push(nodeR);
        lpR = lpR + dampCoef * (dR1.read(dR1.size-1) - lpR);
        float tankR = decay * lpR;
        tankR = tapR2.process(tankR, 0.5f);
        dR2.push(tankR);

        float outL = 0.6f  * dL1.read((int)(dL1.size*0.31f))
                   + 0.25f * dL2.read((int)(dL2.size*0.18f))
                   - 0.15f * dR1.read((int)(dR1.size*0.38f))
                   - 0.1f  * dR2.read((int)(dR2.size*0.27f));

        float outR = 0.6f  * dR1.read((int)(dR1.size*0.31f))
                   + 0.25f * dR2.read((int)(dR2.size*0.18f))
                   - 0.15f * dL1.read((int)(dL1.size*0.38f))
                   - 0.1f  * dL2.read((int)(dL2.size*0.27f));

        // TONE — shelving filter on wet signal only
        // Below 0.5: low-pass (warm, dark) — above 0.5: high-pass boost (bright, airy)
        // Uses two one-pole filters: a lo-shelf and a hi-shelf crossfaded by tone
        const float loFreq  = 400.0f;
        const float hiFreq  = 3200.0f;
        const float loCoef  = 1.0f - (float)(2.0 * juce::MathConstants<double>::pi * loFreq / sampleRate);
        const float hiCoef  = 1.0f - (float)(2.0 * juce::MathConstants<double>::pi * hiFreq / sampleRate);

        // Low shelf: warm body
        toneLoL = toneLoL + (1.0f - loCoef) * (outL - toneLoL);
        toneLoR = toneLoR + (1.0f - loCoef) * (outR - toneLoR);
        // High shelf: air
        toneHiL = toneHiL + (1.0f - hiCoef) * (outL - toneHiL);
        toneHiR = toneHiR + (1.0f - hiCoef) * (outR - toneHiR);

        // tone=0: pure low shelf (dark), tone=0.5: flat, tone=1: boosted highs (bright)
        float toneL, toneR;
        if(tone <= 0.5f){
            float t = tone * 2.0f;  // 0..1
            toneL = toneLoL + t * (outL - toneLoL);
            toneR = toneLoR + t * (outR - toneLoR);
        } else {
            float t = (tone - 0.5f) * 2.0f;  // 0..1
            // Boost presence: mix in the difference between original and lo-filtered
            toneL = outL + t * 0.6f * (outL - toneHiL);
            toneR = outR + t * 0.6f * (outR - toneHiR);
        }

        L[i] = (1.0f - mix) * dry0 + mix * toneL;
        R[i] = (1.0f - mix) * dry1 + mix * toneR;
    }
}

void DreamverbProcessor::getStateInformation(juce::MemoryBlock& destData){
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void DreamverbProcessor::setStateInformation(const void* data, int sizeInBytes){
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if(xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* DreamverbProcessor::createEditor(){
    return new DreamverbEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){
    return new DreamverbProcessor();
}
