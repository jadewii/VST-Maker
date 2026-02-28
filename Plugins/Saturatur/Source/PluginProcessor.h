#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class SaturaturProcessor : public juce::AudioProcessor {
public:
    SaturaturProcessor();
    ~SaturaturProcessor() override = default;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Saturatur"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
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
    static float saturateTape (float x, float drive, float grit);
    static float saturateTube (float x, float drive, float grit);
    static float saturateClip (float x, float drive, float grit);
    static float saturateFold (float x, float drive, float grit);

    // Tone filters
    float toneLoL=0.0f, toneLoR=0.0f;
    // Warmth (low-mid shelf)
    float warmLoL=0.0f, warmLoR=0.0f;
    // Attack envelope follower
    float envL=0.0f, envR=0.0f;
    // DC blocker
    float dcL=0.0f, dcR=0.0f, dcPrevL=0.0f, dcPrevR=0.0f;
    // Comp (soft limiter state)
    float compGainL=1.0f, compGainR=1.0f;

    juce::SmoothedValue<float,juce::ValueSmoothingTypes::Linear> smDrive,smGrit,smTone,smWarmth,smAttack,smOutput,smMix,smType,smComp;
    double sampleRate=44100.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturaturProcessor)
};
