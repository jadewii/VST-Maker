#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

class ECHODLYProcessor : public juce::AudioProcessor {
public:
    ECHODLYProcessor();
    ~ECHODLYProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "ECHODLY"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParams();
private:
    struct RingBuffer {
        std::vector<float> buf;
        int writePos = 0, capacity = 0;
        void init(int n){ capacity=n; buf.assign(n,0.f); writePos=0; }
        void push(float v){ buf[writePos]=v; writePos=(writePos+1)%capacity; }
        float read(float d) const {
            float ds=juce::jmin(d,(float)(capacity-2));
            int idx=(int)ds; float frac=ds-idx;
            auto get=[&](int o){ return buf[(writePos-1-idx+o+capacity*4)%capacity]; };
            float y0=get(-1),y1=get(0),y2=get(1),y3=get(2);
            float c0=y1,c1=.5f*(y2-y0),c2=y0-2.5f*y1+2.f*y2-.5f*y3,c3=.5f*(y3-y0)+1.5f*(y1-y2);
            return ((c3*frac+c2)*frac+c1)*frac+c0;
        }
    };
    RingBuffer delayBufL1, delayBufR1, delayBufL2, delayBufR2;
    float lfoPhase=0.f, lfoPhase2=0.f;
    float hiFilterL=0.f, hiFilterR=0.f;
    float loFilterL=0.f, loFilterR=0.f;
    float fbFilterL=0.f, fbFilterR=0.f;
    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> smMix,smTime,smFeedback,smTone,smSub,smPing,smMod;
    double sampleRate=44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ECHODLYProcessor)
};
