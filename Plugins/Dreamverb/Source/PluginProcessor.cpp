#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
#include <algorithm>

DreamverbProcessor::DreamverbProcessor()
    : AudioProcessor(BusesProperties()
          .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParams())
{}

juce::AudioProcessorValueTreeState::ParameterLayout DreamverbProcessor::createParams() {
    return {
        std::make_unique<juce::AudioParameterFloat>("mix",    "MIX",    0.0f, 1.0f, 0.4f),
        std::make_unique<juce::AudioParameterFloat>("size",   "SIZE",   0.0f, 1.0f, 0.6f),
        std::make_unique<juce::AudioParameterFloat>("damp",   "DAMP",   0.0f, 1.0f, 0.3f),
        std::make_unique<juce::AudioParameterFloat>("tone",   "TONE",   0.0f, 1.0f, 0.5f),
        std::make_unique<juce::AudioParameterFloat>("param5", "SHIMMER",0.0f, 1.0f, 0.0f)
    };
}

void DreamverbProcessor::initBuffers(double sr) {
    const double r = sr / 29761.0;
    ap1.init((size_t)(142*r));  ap2.init((size_t)(107*r));
    ap3.init((size_t)(379*r));  ap4.init((size_t)(277*r));
    tapL1.init((size_t)(672*r));  tapL2.init((size_t)(1800*r));
    tapR1.init((size_t)(908*r));  tapR2.init((size_t)(2656*r));
    dL1.init((size_t)(4453*r));   dL2.init((size_t)(3720*r));
    dR1.init((size_t)(4217*r));   dR2.init((size_t)(3163*r));
    lpL = 0.f; lpR = 0.f;
    toneLoL = toneLoR = toneHiL = toneHiR = 0.f;
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.f;
    std::fill(shimBufL, shimBufL + SHIMMER_BUF, 0.f);
    std::fill(shimBufR, shimBufR + SHIMMER_BUF, 0.f);
    shimSrcL = shimSrcR = 0.f;
    shimPostL = shimPostR = 0.f;
    shimWrite = 0;
    shimPhase = 0.0f;
}

void DreamverbProcessor::prepareToPlay(double sr, int /*samplesPerBlock*/) {
    sampleRate = sr;
    initBuffers(sr);
    smoothedMix.reset(sr, 0.02);     smoothedMix.setCurrentAndTargetValue(0.4f);
    smoothedSize.reset(sr, 0.05);    smoothedSize.setCurrentAndTargetValue(0.6f);
    smoothedDamp.reset(sr, 0.05);    smoothedDamp.setCurrentAndTargetValue(0.3f);
    smoothedTone.reset(sr, 0.15);    smoothedTone.setCurrentAndTargetValue(0.5f);
    smoothedShimmer.reset(sr, 0.20); smoothedShimmer.setCurrentAndTargetValue(0.0f);
}

static inline float softLimit(float x) {
    const float thresh = 0.95f;
    float ax = std::abs(x);
    if (ax <= thresh) return x;
    float sign = x > 0.f ? 1.f : -1.f;
    float over = ax - thresh;
    return sign * (thresh + over / (1.0f + over * 10.0f));
}

static inline float dcBlock(float x, float& xm1, float& ym1) {
    float y = x - xm1 + 0.995f * ym1;
    xm1 = x; ym1 = y;
    return y;
}

static float readInterp(const float* buf, int bufSize, float readPos) {
    float wrapped = std::fmod(readPos, (float)bufSize);
    if (wrapped < 0.f) wrapped += (float)bufSize;
    int i0 = (int)wrapped & (bufSize - 1);
    int i1 = (i0 + 1) & (bufSize - 1);
    float frac = wrapped - (float)(int)wrapped;
    return buf[i0] + frac * (buf[i1] - buf[i0]);
}

void DreamverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    smoothedMix.setTargetValue    (*apvts.getRawParameterValue("mix"));
    smoothedSize.setTargetValue   (*apvts.getRawParameterValue("size"));
    smoothedDamp.setTargetValue   (*apvts.getRawParameterValue("damp"));
    smoothedTone.setTargetValue   (*apvts.getRawParameterValue("tone"));
    smoothedShimmer.setTargetValue(*apvts.getRawParameterValue("param5"));

    const int N  = buffer.getNumSamples();
    const int ch = buffer.getNumChannels();
    auto* L = buffer.getWritePointer(0);
    auto* R = ch > 1 ? buffer.getWritePointer(1) : buffer.getWritePointer(0);

    const int   shimWindow = (SHIMMER_BUF * 3) / 4;  // 6144 — AM rate 14Hz, below audibility
    const float pitchRatio = 2.0f;             // octave up

    // Filter coefficients — computed ONCE per block, not per sample
    // std::exp is expensive; calling it 44100x/sec per filter is wasteful and wrong
    const float shimSrcA  = std::exp(-2.0f * juce::MathConstants<float>::pi * 6000.0f  / (float)sampleRate);
    const float shimPostA = std::exp(-2.0f * juce::MathConstants<float>::pi * 8000.0f  / (float)sampleRate);
    const float loAlpha   = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 400.0f  / (float)sampleRate);
    const float hiAlpha   = 1.0f - std::exp(-2.0f * juce::MathConstants<float>::pi * 3200.0f / (float)sampleRate);

    for (int i = 0; i < N; i++) {
        const float mix     = smoothedMix.getNextValue();
        const float size    = smoothedSize.getNextValue();
        const float damp    = smoothedDamp.getNextValue();
        const float tone    = smoothedTone.getNextValue();
        const float shimmer = smoothedShimmer.getNextValue();

        const float dry0 = L[i];
        const float dry1 = R[i];

        // ── Input diffusion ──────────────────────────────────────────────
        float mono = (dry0 + dry1) * 0.5f;
        float d = ap1.process(mono, 0.70f);
        d = ap2.process(d, 0.70f);
        d = ap3.process(d, 0.625f);
        d = ap4.process(d, 0.625f);

        // ── Shimmer: octave-up via two overlapping Hanning-windowed heads ─
        float shimL = 0.0f, shimR = 0.0f;
        if (shimmer > 0.001f) {
            const float base    = (float)shimWrite;
            const float windowF = (float)shimWindow;

            // Read positions: head A sweeps forward through the window,
            // head B is offset by half window for continuous crossfade coverage
            float readA = base - windowF * 1.25f + shimPhase * windowF;
            float readB = base - windowF * 1.75f + shimPhase * windowF;

            // Wrap into [0, SHIMMER_BUF)
            auto wrapRead = [](float x, float len) {
                x = std::fmod(x, len);
                if (x < 0.0f) x += len;
                return x;
            };
            readA = wrapRead(readA, (float)SHIMMER_BUF);
            readB = wrapRead(readB, (float)SHIMMER_BUF);

            // Hann windows — sum to 1.0 at all phases (complementary)
            const float winA = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * shimPhase));
            const float winB = 1.0f - winA;

            shimL = winA * readInterp(shimBufL, SHIMMER_BUF, readA)
                  + winB * readInterp(shimBufL, SHIMMER_BUF, readB);
            shimR = winA * readInterp(shimBufR, SHIMMER_BUF, readA)
                  + winB * readInterp(shimBufR, SHIMMER_BUF, readB);

            // Post-filter shimmer output — suppresses edge artifacts
            shimPostL = (1.0f - shimPostA) * shimL + shimPostA * shimPostL;
            shimPostR = (1.0f - shimPostA) * shimR + shimPostA * shimPostR;
            shimL = shimPostL;
            shimR = shimPostR;

            // FIX: phase increment must be pitchRatio/shimWindow for correct octave-up
            // (pitchRatio-1)/shimWindow gives HALF speed — incomplete crossfade = flutter
            shimPhase += pitchRatio / (float)shimWindow;
            if (shimPhase >= 1.0f) shimPhase -= 1.0f;
        }

        // FIX: shimmer feed gain 0.20 (not 0.60 — that was 3x too loud, caused crunch)
        // squared curve for natural feel at low settings
        const float shimAmt  = shimmer;
        float shimFeed = (shimL + shimR) * 0.5f * 0.35f * shimAmt;
        shimFeed = std::max(-0.80f, std::min(0.80f, shimFeed));
        d = softLimit(d + shimFeed);

        // ── Dattorro plate tank ──────────────────────────────────────────
        const float decay    = std::min(0.5f + size * 0.43f, 0.93f);
        // damp=0->20kHz (bright), damp=1->500Hz (dark)
        const float dampCoef = 0.0579f + damp * (0.9312f - 0.0579f);

        float nodeL = softLimit(d + decay * dR2.read(dR2.size() - 1));
        nodeL = tapL1.process(nodeL, 0.7f);
        dL1.push(nodeL);
        lpL = lpL + dampCoef * (dL1.read(dL1.size() - 1) - lpL);
        float tankL = decay * lpL;
        tankL = tapL2.process(tankL, 0.5f);
        dL2.push(tankL);

        float nodeR = softLimit(d + decay * dL2.read(dL2.size() - 1));
        nodeR = tapR1.process(nodeR, 0.7f);
        dR1.push(nodeR);
        lpR = lpR + dampCoef * (dR1.read(dR1.size() - 1) - lpR);
        float tankR = decay * lpR;
        tankR = tapR2.process(tankR, 0.5f);
        dR2.push(tankR);

        float outL = 0.432f * dL1.read((size_t)(dL1.size() * 0.31f))
                   + 0.180f * dL2.read((size_t)(dL2.size() * 0.18f))
                   - 0.108f * dR1.read((size_t)(dR1.size() * 0.38f))
                   - 0.072f * dR2.read((size_t)(dR2.size() * 0.27f));

        float outR = 0.432f * dR1.read((size_t)(dR1.size() * 0.31f))
                   + 0.180f * dR2.read((size_t)(dR2.size() * 0.18f))
                   - 0.108f * dL1.read((size_t)(dL1.size() * 0.38f))
                   - 0.072f * dL2.read((size_t)(dL2.size() * 0.27f));

        // DC blocker
        outL = dcBlock(outL, dcX[0], dcY[0]);
        outR = dcBlock(outR, dcX[1], dcY[1]);

        // Shimmer source buffer — low-pass before writing reduces aliasing in pitch shift
        shimSrcL = (1.0f - shimSrcA) * outL + shimSrcA * shimSrcL;
        shimSrcR = (1.0f - shimSrcA) * outR + shimSrcA * shimSrcR;
        shimBufL[shimWrite & (SHIMMER_BUF - 1)] = shimSrcL;
        shimBufR[shimWrite & (SHIMMER_BUF - 1)] = shimSrcR;
        shimWrite = (shimWrite + 1) & (SHIMMER_BUF - 1);

        // ── Tone: tilt EQ — center (0.5) is flat ─────────────────────────
        // Below 0.5: crossfade toward 400Hz LP (darker)
        // Above 0.5: add HF shelf boost via 3200Hz HP component
        toneLoL += loAlpha * (outL - toneLoL);
        toneLoR += loAlpha * (outR - toneLoR);
        toneHiL += hiAlpha * (outL - toneHiL);
        toneHiR += hiAlpha * (outR - toneHiR);

        float wetL, wetR;
        if (tone <= 0.5f) {
            float t = tone * 2.0f;
            wetL = toneLoL + t * (outL - toneLoL);
            wetR = toneLoR + t * (outR - toneLoR);
        } else {
            float t = (tone - 0.5f) * 2.0f;
            wetL = outL + t * 0.25f * (outL - toneHiL);
            wetR = outR + t * 0.25f * (outR - toneHiR);
        }

        wetL = softLimit(wetL);
        wetR = softLimit(wetR);

        L[i] = softLimit((1.0f - mix) * dry0 + mix * wetL);
        R[i] = softLimit((1.0f - mix) * dry1 + mix * wetR);
    }
}

void DreamverbProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DreamverbProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* DreamverbProcessor::createEditor() {
    return new DreamverbEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DreamverbProcessor();
}
