#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

ECHODLYProcessor::ECHODLYProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParams())
{}

juce::AudioProcessorValueTreeState::ParameterLayout ECHODLYProcessor::createParams(){
    return {
        std::make_unique<juce::AudioParameterFloat>("mix",    "MIX",    0.0f, 1.0f,   0.4f),
        std::make_unique<juce::AudioParameterFloat>("size",   "TIME",   0.0f, 1.0f,   0.35f),
        std::make_unique<juce::AudioParameterFloat>("param5", "FDBK",   0.0f, 1.0f,   0.3f),
        std::make_unique<juce::AudioParameterFloat>("param6", "TONE",   0.0f, 1.0f,   0.5f),
        std::make_unique<juce::AudioParameterFloat>("damp",   "SUB",    0.0f, 1.0f,   0.5f),
        std::make_unique<juce::AudioParameterFloat>("pre",    "PING",   0.0f, 1.0f,   0.0f),
        std::make_unique<juce::AudioParameterFloat>("param7", "MOD",    0.0f, 1.0f,   0.15f)
    };
}

void ECHODLYProcessor::prepareToPlay(double sr, int samplesPerBlock){
    sampleRate = sr;
    const int maxSamples = (int)(sr * 1.65);
    delayBufL1.init(maxSamples); delayBufR1.init(maxSamples);
    delayBufL2.init(maxSamples); delayBufR2.init(maxSamples);
    lfoPhase = 0.f; lfoPhase2 = 0.13f;
    hiFilterL = hiFilterR = loFilterL = loFilterR = 0.f;
    fbFilterL = fbFilterR = 0.f;

    smMix.reset(sr, 0.05);      smMix.setCurrentAndTargetValue(0.4f);
    smTime.reset(sr, 0.2);      smTime.setCurrentAndTargetValue(0.35f);
    smFeedback.reset(sr, 0.05); smFeedback.setCurrentAndTargetValue(0.3f);
    smTone.reset(sr, 0.05);     smTone.setCurrentAndTargetValue(0.5f);
    smSub.reset(sr, 0.2);       smSub.setCurrentAndTargetValue(0.5f);
    smPing.reset(sr, 0.05);     smPing.setCurrentAndTargetValue(0.0f);
    smMod.reset(sr, 0.05);      smMod.setCurrentAndTargetValue(0.15f);
}

void ECHODLYProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&){
    juce::ScopedNoDenormals noDenormals;
    if(delayBufL1.capacity == 0){ buffer.clear(); return; }

    smMix.setTargetValue     (*apvts.getRawParameterValue("mix"));
    smTime.setTargetValue    (*apvts.getRawParameterValue("size"));
    smFeedback.setTargetValue(*apvts.getRawParameterValue("param5"));
    smTone.setTargetValue    (*apvts.getRawParameterValue("param6"));
    smSub.setTargetValue     (*apvts.getRawParameterValue("damp"));
    smPing.setTargetValue    (*apvts.getRawParameterValue("pre"));
    smMod.setTargetValue     (*apvts.getRawParameterValue("param7"));

    const int N  = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    auto* L = buffer.getWritePointer(0);
    auto* R = ch > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    // LFO for modulation
    const float lfoRate = 0.4f / (float)sampleRate;

    for(int i = 0; i < N; i++){
        const float mix      = smMix.getNextValue();
        const float timeParm = smTime.getNextValue();
        const float feedback = juce::jmin(smFeedback.getNextValue(), 0.88f);
        const float tone     = smTone.getNextValue();
        const float sub      = smSub.getNextValue();
        const float ping     = smPing.getNextValue();
        const float mod      = smMod.getNextValue();

        // Delay 1: exponential time mapping 20ms - 1600ms
        const float delayMs1 = 20.0f * std::pow(80.0f, timeParm);
        const float d1 = juce::jmax(1.0f, delayMs1 * 0.001f * (float)sampleRate);

        // Delay 2: subdivision of delay 1
        // sub knob maps to musical ratios: 0=triplet(0.667), 0.25=8th(0.5), 0.5=dotted8th(0.75), 0.75=dotted qtr(1.5), 1=golden(1.618)
        float subRatio;
        if      (sub < 0.2f)  subRatio = 0.667f;
        else if (sub < 0.4f)  subRatio = 0.5f;
        else if (sub < 0.6f)  subRatio = 0.75f;
        else if (sub < 0.8f)  subRatio = 1.5f;
        else                  subRatio = 1.618f;
        const float d2 = juce::jmax(1.0f, d1 * subRatio);

        // LFO modulation — subtle chorus on repeats
        const float lfoDepth = mod * mod * 12.0f; // quadratic for fine control at low values
        const float lfoA = lfoDepth * std::sin(juce::MathConstants<float>::twoPi * lfoPhase);
        const float lfoB = lfoDepth * std::sin(juce::MathConstants<float>::twoPi * lfoPhase2);
        lfoPhase  = std::fmod(lfoPhase  + lfoRate, 1.0f);
        lfoPhase2 = std::fmod(lfoPhase2 + lfoRate, 1.0f);

        const float dry0 = L[i], dry1 = R[i];

        // Read delay lines with modulation
        float w1L = juce::jlimit(-1.0f, 1.0f, delayBufL1.read(juce::jmax(1.0f, d1 + lfoA)));
        float w1R = juce::jlimit(-1.0f, 1.0f, delayBufR1.read(juce::jmax(1.0f, d1 - lfoA)));
        float w2L = juce::jlimit(-1.0f, 1.0f, delayBufL2.read(juce::jmax(1.0f, d2 + lfoB)));
        float w2R = juce::jlimit(-1.0f, 1.0f, delayBufR2.read(juce::jmax(1.0f, d2 - lfoB)));

        // TONE — dual filter on feedback path (like DIG)
        // tone < 0.5: hi-cut (dark warm repeats)
        // tone = 0.5: flat
        // tone > 0.5: lo-cut (bright airy repeats)
        float toneWetL = w1L + w2L * 0.7f;
        float toneWetR = w1R + w2R * 0.7f;

        if(tone < 0.5f){
            // Hi cut — low pass filter
            const float cutoff = 800.0f + tone * 2.0f * 14000.0f; // 800Hz-14800Hz
            const float coef = 1.0f - (float)(juce::MathConstants<double>::twoPi * cutoff / sampleRate);
            hiFilterL = hiFilterL * coef + toneWetL * (1.0f - coef);
            hiFilterR = hiFilterR * coef + toneWetR * (1.0f - coef);
            toneWetL = hiFilterL;
            toneWetR = hiFilterR;
        } else {
            // Lo cut — high pass filter
            const float cutoff = (tone - 0.5f) * 2.0f * 400.0f; // 0-400Hz cut
            const float coef = 1.0f - (float)(juce::MathConstants<double>::twoPi * juce::jmax(20.0f, cutoff) / sampleRate);
            loFilterL = loFilterL * coef + toneWetL * (1.0f - coef);
            loFilterR = loFilterR * coef + toneWetR * (1.0f - coef);
            toneWetL = toneWetL - loFilterL;
            toneWetR = toneWetR - loFilterR;
        }

        // Feedback path with safety clamp
        float fb0 = juce::jlimit(-0.9f, 0.9f, toneWetL * feedback);
        float fb1 = juce::jlimit(-0.9f, 0.9f, toneWetR * feedback);

        // PING PONG routing
        // ping=0: parallel (L feeds L, R feeds R)
        // ping=1: ping pong (L feeds R, R feeds L)
        float feedL = fb0 + ping * (fb1 - fb0);
        float feedR = fb1 + ping * (fb0 - fb1);

        // Write to delay lines
        // Delay 1: direct input + feedback
        delayBufL1.push(dry0 + feedL);
        delayBufR1.push(dry1 + feedR);
        // Delay 2: feeds from delay 1 output (series routing)
        delayBufL2.push(w1L * 0.7f);
        delayBufR2.push(w1R * 0.7f);

        // Final output
        L[i] = (1.0f - mix) * dry0 + mix * toneWetL;
        R[i] = (1.0f - mix) * dry1 + mix * toneWetR;
    }
}

void ECHODLYProcessor::getStateInformation(juce::MemoryBlock& destData){
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void ECHODLYProcessor::setStateInformation(const void* data, int sizeInBytes){
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if(xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessorEditor* ECHODLYProcessor::createEditor(){
    return new ECHODLYEditor(*this);
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){
    return new ECHODLYProcessor();
}
