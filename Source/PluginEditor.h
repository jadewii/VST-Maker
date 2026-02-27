#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_dsp/juce_dsp.h>
#include "PluginProcessor.h"

namespace PC {
    inline const juce::Colour bg   { 0xff3A7CA5 };
    inline const juce::Colour ring { 0xff5B8FAE };
    inline const juce::Colour face { 0xff8ECFC4 };
    inline const juce::Colour ndl  { 0xff3A6B5A };
    inline const juce::Colour lbl  { 0xffDAEEF2 };
}

class DreamverbLAF : public juce::LookAndFeel_V4 {
public:
    DreamverbLAF(){
        setColour(juce::Slider::textBoxTextColourId,    PC::lbl);
        setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour(juce::Label::textColourId,            PC::lbl);
    }
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
        float pos,float startA,float endA,juce::Slider&) override {
        float cx=x+w*.5f,cy=y+h*.5f,outerR=juce::jmin(w,h)*.48f,innerR=outerR*.78f;
        // Outer glow ring
        g.setColour(PC::ring.withAlpha(.18f));
        g.fillEllipse(cx-outerR-3,cy-outerR-3,(outerR+3)*2,(outerR+3)*2);
        g.setColour(PC::ring); g.fillEllipse(cx-outerR,cy-outerR,outerR*2,outerR*2);
        g.setColour(PC::face); g.fillEllipse(cx-innerR,cy-innerR,innerR*2,innerR*2);
        // Arc track
        auto toXY=[&](float r,float a){ return juce::Point<float>(cx+std::sin(a)*r,cy-std::cos(a)*r); };
        float angle=startA+pos*(endA-startA);
        juce::Path arc; arc.addCentredArc(cx,cy,outerR*.91f,outerR*.91f,0.f,startA,endA,true);
        g.setColour(PC::ring.withAlpha(.25f));
        g.strokePath(arc,juce::PathStrokeType(outerR*.1f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        juce::Path arcFill; arcFill.addCentredArc(cx,cy,outerR*.91f,outerR*.91f,0.f,startA,angle,true);
        g.setColour(PC::ndl);
        g.strokePath(arcFill,juce::PathStrokeType(outerR*.1f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        // Needle
        auto tip=toXY(innerR*.72f,angle),tail=toXY(innerR*.18f,angle+juce::MathConstants<float>::pi);
        g.setColour(PC::ndl);
        juce::Path n; n.startNewSubPath(tail); n.lineTo(tip);
        g.strokePath(n,juce::PathStrokeType(3.f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
        float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& sl) override {
        if(style!=juce::Slider::LinearVertical){LookAndFeel_V4::drawLinearSlider(g,x,y,w,h,pos,0,0,style,sl);return;}
        float cx=x+w*.5f,trackW=7.f;
        g.setColour(PC::face); g.fillRoundedRectangle(cx-trackW*.5f,(float)y,trackW,(float)h,trackW*.5f);
        float tw=32.f,th=18.f;
        g.setColour(PC::ndl); g.fillRoundedRectangle(cx-tw*.5f,pos-th*.5f,tw,th,5.f);
        g.setColour(PC::face.withAlpha(.5f)); g.drawLine(cx-tw*.3f,pos,cx+tw*.3f,pos,2.f);
    }
};

class DreamverbEditor : public juce::AudioProcessorEditor {
public:
    explicit DreamverbEditor(DreamverbProcessor&);
    ~DreamverbEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    DreamverbProcessor& proc;
    DreamverbLAF laf;
    juce::Slider mixKnob{juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::NoTextBox};
    juce::Slider sizeKnob{juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::NoTextBox};
    juce::Slider toneSlider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Slider dampSlider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Label mixLbl;
    juce::Label sizeLbl;
    juce::Label toneLbl;
    juce::Label dampLbl;
    juce::Label brandLbl,productLbl;
    using Att=juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Att> mixAtt;
    std::unique_ptr<Att> sizeAtt;
    std::unique_ptr<Att> toneAtt;
    std::unique_ptr<Att> dampAtt;
    void setupLbl(juce::Label&,const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DreamverbEditor)
};