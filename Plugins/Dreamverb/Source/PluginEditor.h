#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

// ── Colours — exact hex from DreamVerb.scproj ─────────────────────────────
// DO NOT CHANGE. Any UI fix must come from re-reading the .scproj file,
// not from guessing. The VST must always look identical to the browser preview.
namespace PC {
    inline const juce::Colour bg        = juce::Colour(0xff3a7ca5); // project bgColor
    // Knobs: mix, size  (knobStyle: neon)
    inline const juce::Colour knobFace  = juce::Colour(0xff8ecfc4); // faceColor
    inline const juce::Colour knobRing  = juce::Colour(0xff5b8fae); // ringColor
    inline const juce::Colour knobNdl   = juce::Colour(0xff3a6b5a); // needleColor
    inline const juce::Colour knobTrack = juce::Colour(0xff8ecfc4); // trackFillColor
    // Sliders: tone, shimmer, damp  (knobStyle: classic, thumbShape: rect)
    inline const juce::Colour slFace    = juce::Colour(0xff7bbfb5); // faceColor  → track bg
    inline const juce::Colour slRing    = juce::Colour(0xff5b8fae); // ringColor  → fill below thumb
    inline const juce::Colour slNdl     = juce::Colour(0xff3a6b5a); // needleColor → thumb body
    inline const juce::Colour slTrack   = juce::Colour(0xff7bbfb5); // trackFillColor (same as face)
    inline const juce::Colour lbl       = juce::Colour(0xffdaeef2); // labelColor
}

// ── Resize dot — bottom-right corner ──────────────────────────────────────
class ResizeButton : public juce::Component {
public:
    std::function<void()> onClick;
    ResizeButton() { setSize(24, 24); setRepaintsOnMouseActivity(true); }
    void paint(juce::Graphics& g) override {
        float alpha = isMouseOver() ? 0.75f : 0.30f;
        float cx = getWidth() * 0.5f, cy = getHeight() * 0.5f;
        float r  = isMouseOver() ? 4.5f : 3.5f;
        g.setColour(PC::lbl.withAlpha(alpha));
        g.fillEllipse(cx - r, cy - r, r * 2.f, r * 2.f);
    }
    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isLeftButtonDown() && onClick) onClick();
    }
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResizeButton)
};

// ── LAF ───────────────────────────────────────────────────────────────────
class DreamverbLAF : public juce::LookAndFeel_V4 {
public:
    DreamverbLAF() {
        setColour(juce::Slider::textBoxTextColourId,    PC::lbl);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId,            PC::lbl);
    }

    // ── Neon knob (mix, size)
    // Face circle floats on bg (bgColor=transparent in scproj, so no outer ring drawn).
    // Arc track sits at outerR*0.88, dim at trackFillColor 0.25 alpha, fill at needleColor.
    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
        float pos, float, float, juce::Slider&) override
    {
        const float kStart = -2.356f, kEnd = 2.356f;
        const float angle  = kStart + pos * (kEnd - kStart);
        float cx = x + w * 0.5f, cy = y + h * 0.5f;
        float outerR  = juce::jmin(w, h) * 0.48f;
        float innerR  = outerR * 0.76f;
        float arcR    = outerR * 0.88f;
        float strokeW = outerR * 0.10f;

        // Face circle (no outer ring — bgColor is transparent)
        g.setColour(PC::knobFace);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.f, innerR * 2.f);

        // Highlight — rgba(255,255,255,0.22) top half per browser source
        juce::ColourGradient hl(juce::Colours::white.withAlpha(0.22f), cx, cy - innerR,
                                juce::Colours::transparentBlack, cx, cy + innerR * 0.3f, false);
        g.setGradientFill(hl);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.f, innerR * 2.f);

        // Dim arc track — trackFillColor at 0.25 alpha
        juce::Path dimArc;
        dimArc.addCentredArc(cx, cy, arcR, arcR, 0.f, kStart, kEnd, true);
        g.setColour(PC::knobTrack.withAlpha(0.25f));
        g.strokePath(dimArc, juce::PathStrokeType(strokeW,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Glow behind fill arc
        juce::Path glowArc;
        glowArc.addCentredArc(cx, cy, arcR, arcR, 0.f, kStart, angle, true);
        g.setColour(PC::knobNdl.withAlpha(0.28f));
        g.strokePath(glowArc, juce::PathStrokeType(strokeW * 1.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Fill arc — needleColor
        juce::Path fillArc;
        fillArc.addCentredArc(cx, cy, arcR, arcR, 0.f, kStart, angle, true);
        g.setColour(PC::knobNdl);
        g.strokePath(fillArc, juce::PathStrokeType(strokeW * 0.6f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Needle
        auto tip  = juce::Point<float>(cx + std::sin(angle) * innerR * 0.68f,
                                       cy - std::cos(angle) * innerR * 0.68f);
        auto tail = juce::Point<float>(cx - std::sin(angle) * innerR * 0.22f,
                                       cy + std::cos(angle) * innerR * 0.22f);
        g.setColour(PC::knobNdl);
        juce::Path ndl; ndl.startNewSubPath(tail); ndl.lineTo(tip);
        g.strokePath(ndl, juce::PathStrokeType(2.5f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ── Classic slider (tone, shimmer, damp)
    // From browser source (buildSlider vert):
    //   track bg  = trackFillColor (#7bbfb5 = slFace)  full height
    //   track fill = ringColor (#5b8fae = slRing) opacity 0.72, from thumbY DOWN to bottom
    //   thumb body = needleColor (#3a6b5a = slNdl), rect 32x18 rx:5
    //   thumb shadow = rgba(0,0,0,0.18) offset (1,2)
    //   thumb gloss  = rgba(255,255,255,0.22) top strip
    //   no thumbLine deco (null in scproj)
    void drawLinearSlider(juce::Graphics& g, int x, int y, int w, int h,
        float pos, float, float, juce::Slider::SliderStyle style, juce::Slider&) override
    {
        if (style != juce::Slider::LinearVertical) return;
        float cx = x + w * 0.5f;
        float trackT = (float)y, trackB = (float)(y + h);
        float trackW = 5.f;

        // Track bg — trackFillColor / slFace (#7bbfb5 teal)
        g.setColour(PC::slFace);
        g.fillRoundedRectangle(cx - trackW * 0.5f, trackT, trackW, trackB - trackT, trackW * 0.5f);

        // Track fill — ringColor (#5b8fae blue) at 0.72, from pos (thumb centre) DOWN to bottom
        g.setColour(PC::slRing.withAlpha(0.72f));
        g.fillRoundedRectangle(cx - trackW * 0.5f, pos, trackW, trackB - pos, trackW * 0.5f);

        // Thumb: rect 32x18 rx:5 — exactly matching browser drawThumbShape 'rect'
        float tw = 32.f, th = 18.f;
        float ty = pos - th * 0.5f;

        // Shadow — rgba(0,0,0,0.18) offset (1,2)
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.fillRoundedRectangle(cx - tw * 0.5f + 1.f, ty + 2.f, tw, th, 5.f);

        // Body — needleColor (#3a6b5a dark teal)
        g.setColour(PC::slNdl);
        g.fillRoundedRectangle(cx - tw * 0.5f, ty, tw, th, 5.f);

        // Gloss — rgba(255,255,255,0.22) top strip, rx:2, height 6px
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.fillRoundedRectangle(cx - tw * 0.5f + (tw - 20.f) * 0.5f, ty, 20.f, 6.f, 2.f);
    }
};

// ── Editor ────────────────────────────────────────────────────────────────
class DreamverbEditor : public juce::AudioProcessorEditor {
public:
    explicit DreamverbEditor(DreamverbProcessor&);
    ~DreamverbEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void setScale(float s);

private:
    static constexpr int BASE_W = 400;
    static constexpr int BASE_H = 400;
    float scale = 1.0f;

    DreamverbProcessor& proc;
    DreamverbLAF laf;

    juce::Slider mixKnob    { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider sizeKnob   { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider toneSlider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Slider shimSlider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };
    juce::Slider dampSlider { juce::Slider::LinearVertical, juce::Slider::NoTextBox };

    juce::Label mixLbl, sizeLbl, toneLbl, shimLbl, dampLbl;
    juce::Label brandLbl, productLbl;
    ResizeButton resizeBtn;

    using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Att> mixAtt, sizeAtt, toneAtt, shimAtt, dampAtt;

    void setupLbl(juce::Label&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamverbEditor)
};
