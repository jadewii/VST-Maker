#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cstddef>

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
        size_t writePos = 0, sz = 0;
        void init(size_t n) { sz = n; buf.assign(n, 0.f); writePos = 0; }
        float process(float in, float g) {
            float delayed = buf[writePos];
            float w = in + g * delayed;
            buf[writePos] = w;
            writePos = (writePos + 1) % sz;
            return delayed - g * w;
        }
        size_t size() const { return sz; }
    };
    struct DelayTap {
        std::vector<float> buf;
        size_t writePos = 0, sz = 0;
        void init(size_t n) { sz = n; buf.assign(n, 0.f); writePos = 0; }
        void push(float v) { buf[writePos] = v; writePos = (writePos + 1) % sz; }
        float read(size_t d) const { return buf[(writePos + sz - (d % sz)) % sz]; }
        size_t size() const { return sz; }
    };

    AllpassFilter ap1, ap2, ap3, ap4;
    AllpassFilter tapL1, tapL2, tapR1, tapR2;
    DelayTap dL1, dL2, dR1, dR2;
    float lpL = 0.f, lpR = 0.f;
    float toneLoL = 0.f, toneLoR = 0.f, toneHiL = 0.f, toneHiR = 0.f;

    // DC blocker state (one per channel)
    float dcX[2] = {}, dcY[2] = {};

    static constexpr int SHIMMER_BUF = 8192;
    float shimBufL[SHIMMER_BUF] = {};
    float shimBufR[SHIMMER_BUF] = {};
    // FIX: shimWrite stays bounded in [0, SHIMMER_BUF) — prevents float precision
    // loss when cast to float for read head calculation after ~6 minutes of playback
    int shimWrite = 0;
    float shimPhase = 0.0f;
    float shimSrcL = 0.f, shimSrcR = 0.f;
    float shimPostL = 0.f, shimPostR = 0.f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>
        smoothedMix, smoothedSize, smoothedDamp, smoothedTone, smoothedShimmer;
    double sampleRate = 44100.0;
    void initBuffers(double sr);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamverbProcessor)
};
