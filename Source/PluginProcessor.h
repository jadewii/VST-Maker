#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>

class DreamverbProcessor : public juce::AudioProcessor {
public:
    DreamverbProcessor();
    ~DreamverbProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Dreamverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }
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
    struct AllpassFilter {
        std::vector<float> buf;
        int writePos=0, size=0;
        void init(int n){ size=n; buf.assign(n,0.f); writePos=0; }
        float process(float in, float g){
            float delayed=buf[writePos];
            float w=in+g*delayed;
            buf[writePos]=w;
            writePos=(writePos+1)%size;
            return delayed-g*w;
        }
    };
    struct DelayTap {
        std::vector<float> buf;
        int writePos=0, size=0;
        void init(int n){ size=n; buf.assign(n,0.f); writePos=0; }
        void push(float v){ buf[writePos]=v; writePos=(writePos+1)%size; }
        float read(int d){ return buf[(writePos-d+size)%size]; }
    };

    AllpassFilter ap1,ap2,ap3,ap4;
    AllpassFilter tapL1,tapL2,tapR1,tapR2;
    DelayTap dL1,dL2,dR1,dR2;
    float lpL=0.f, lpR=0.f;
    float toneLoL=0.f, toneLoR=0.f, toneHiL=0.f, toneHiR=0.f;

    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> smoothedMix,smoothedSize,smoothedDamp,smoothedTone;
    double sampleRate=44100.0;
    void initBuffers(double sr);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamverbProcessor)
};
