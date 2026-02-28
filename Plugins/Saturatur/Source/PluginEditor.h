#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class SaturaturLAF : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,
        float pos,float startA,float endA,juce::Slider& slider) override {
        float cx=x+w*.5f,cy=y+h*.5f,outerR=juce::jmin(w,h)*.48f,innerR=outerR*.78f;
        auto toXY=[&](float r,float a){ return juce::Point<float>(cx+std::sin(a)*r,cy-std::cos(a)*r); };
        float angle=startA+pos*(endA-startA);

        // Determine colors and style based on slider name
        auto name = slider.getName();
        bool isDrive = (name == "drive");
        bool isType = (name == "type");
        bool isComp = (name == "comp");

        juce::Colour ringCol = isDrive ? juce::Colour(0xffAA91FE) :
                              (isType || isComp) ? juce::Colour(0xff11053B) :
                              juce::Colour(0xff738AF8);
        juce::Colour faceCol = juce::Colour(0xff4D22B3);
        juce::Colour needleCol = juce::Colour(0xffB18CFE);

        if(isDrive){
            // SKEUOMORPH style - 3D appearance
            // Outer ring with radial gradient (light top-left → dark bottom-right)
            juce::ColourGradient ringGrad(ringCol.brighter(0.5f), cx-outerR*.3f, cy-outerR*.25f,
                                         ringCol.darker(0.4f), cx+outerR*.7f, cy+outerR*.7f, true);
            g.setGradientFill(ringGrad);
            g.fillEllipse(cx-outerR,cy-outerR,outerR*2,outerR*2);

            // Shadow circle
            g.setColour(juce::Colours::black.withAlpha(0.28f));
            g.fillEllipse(cx-innerR*1.04f,cy-innerR*1.04f,innerR*2.08f,innerR*2.08f);

            // Face with radial gradient
            juce::ColourGradient faceGrad(faceCol.brighter(0.28f), cx-innerR*.35f, cy-innerR*.3f,
                                         faceCol.darker(0.08f), cx+innerR*.6f, cy+innerR*.6f, true);
            g.setGradientFill(faceGrad);
            g.fillEllipse(cx-innerR,cy-innerR,innerR*2,innerR*2);

            // Needle
            auto tip=toXY(innerR*.76f,angle),tail=toXY(innerR*.18f,angle+juce::MathConstants<float>::pi);
            g.setColour(needleCol);
            juce::Path n; n.startNewSubPath(tail); n.lineTo(tip);
            g.strokePath(n,juce::PathStrokeType(juce::jmin(w,h)*.015f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));

            // Center dot
            float dotR = juce::jmin(w,h)*.055f;
            g.setColour(faceCol.darker(0.18f));
            g.fillEllipse(cx-dotR,cy-dotR,dotR*2,dotR*2);
        } else {
            // CLASSIC style - flat with arc track
            // Outer glow ring
            g.setColour(ringCol.withAlpha(.18f));
            g.fillEllipse(cx-outerR-3,cy-outerR-3,(outerR+3)*2,(outerR+3)*2);
            g.setColour(ringCol); g.fillEllipse(cx-outerR,cy-outerR,outerR*2,outerR*2);
            g.setColour(faceCol); g.fillEllipse(cx-innerR,cy-innerR,innerR*2,innerR*2);

            // Arc track
            juce::Path arc; arc.addCentredArc(cx,cy,outerR*.91f,outerR*.91f,0.0f,startA,endA,true);
            g.setColour(ringCol.withAlpha(.25f));
            g.strokePath(arc,juce::PathStrokeType(outerR*.1f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
            juce::Path arcFill; arcFill.addCentredArc(cx,cy,outerR*.91f,outerR*.91f,0.0f,startA,angle,true);
            g.setColour(needleCol);
            g.strokePath(arcFill,juce::PathStrokeType(outerR*.1f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));

            // Needle
            auto tip=toXY(innerR*.72f,angle),tail=toXY(innerR*.18f,angle+juce::MathConstants<float>::pi);
            g.setColour(needleCol);
            juce::Path n; n.startNewSubPath(tail); n.lineTo(tip);
            g.strokePath(n,juce::PathStrokeType(3.0f,juce::PathStrokeType::curved,juce::PathStrokeType::rounded));
        }
    }
    void drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,
        float pos,float,float,juce::Slider::SliderStyle style,juce::Slider&) override{
        const float cx=x+w*.5f;
        // Track (dark purple)
        g.setColour(juce::Colour(0xff4D22B3));
        g.fillRoundedRectangle(cx-3.5f,(float)y,7.0f,(float)h,3.5f);
        // Fill above thumb (blue)
        g.setColour(juce::Colour(0xff738AF8));
        g.fillRoundedRectangle(cx-3.5f,pos,7.0f,(float)(y+h)-pos,3.5f);
        // Thumb (dark purple)
        g.setColour(juce::Colour(0xff4D22B3));
        g.fillRoundedRectangle(cx-16.0f,pos-9.0f,32.0f,18.0f,5.0f);
        // Thumb highlight line (white)
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        g.fillRoundedRectangle(cx-12.0f,pos-3.0f,24.0f,4.0f,2.0f);
    }
};

struct PC{
    static const juce::Colour bg;
    static const juce::Colour lbl;
};

class SaturaturEditor : public juce::AudioProcessorEditor {
public:
    explicit SaturaturEditor(SaturaturProcessor&);
    ~SaturaturEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void setScale(float s);
private:
    SaturaturProcessor& proc;
    SaturaturLAF laf;
    float scale = 1.0f;
    juce::Slider driveKnob{juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::NoTextBox};
    juce::Slider typeKnob{juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::NoTextBox};
    juce::Slider param9Knob{juce::Slider::RotaryHorizontalVerticalDrag,juce::Slider::NoTextBox};
    juce::Slider biasSlider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Slider param2Slider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Slider param3Slider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Slider param4Slider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Slider outputSlider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Slider mixSlider{juce::Slider::LinearVertical,juce::Slider::NoTextBox};
    juce::Label biasLbl;
    juce::Label param2Lbl;
    juce::Label param3Lbl;
    juce::Label param4Lbl;
    juce::Label outputLbl;
    juce::Label mixLbl;
    juce::Label driveLbl;
    juce::Label typeLbl;
    juce::Label param9Lbl;
    juce::Label brandLbl, productLbl;
    using Att = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Att> biasAtt;
    std::unique_ptr<Att> param2Att;
    std::unique_ptr<Att> param3Att;
    std::unique_ptr<Att> param4Att;
    std::unique_ptr<Att> outputAtt;
    std::unique_ptr<Att> mixAtt;
    std::unique_ptr<Att> driveAtt;
    std::unique_ptr<Att> typeAtt;
    std::unique_ptr<Att> param9Att;
    void setupLbl(juce::Label&, const juce::String&);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturaturEditor)
};
