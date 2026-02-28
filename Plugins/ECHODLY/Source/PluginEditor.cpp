#include "PluginEditor.h"
#include "PluginProcessor.h"

ECHODLYEditor::ECHODLYEditor(ECHODLYProcessor& p)
    :AudioProcessorEditor(&p),proc(p){
    setLookAndFeel(&laf); setSize(400,400);
    addAndMakeVisible(mixKnob); mixKnob.setLookAndFeel(&laf);
    setupLbl(mixLbl,"MIX");
    mixAtt=std::make_unique<Att>(proc.apvts,"mix",mixKnob);
    addAndMakeVisible(sizeKnob); sizeKnob.setLookAndFeel(&laf);
    setupLbl(sizeLbl,"SIZE");
    sizeAtt=std::make_unique<Att>(proc.apvts,"size",sizeKnob);
    addAndMakeVisible(preSlider); preSlider.setLookAndFeel(&laf);
    setupLbl(preLbl,"PRE");
    preAtt=std::make_unique<Att>(proc.apvts,"pre",preSlider);
    addAndMakeVisible(dampSlider); dampSlider.setLookAndFeel(&laf);
    setupLbl(dampLbl,"DAMP");
    dampAtt=std::make_unique<Att>(proc.apvts,"damp",dampSlider);
    addAndMakeVisible(param5Knob); param5Knob.setLookAndFeel(&laf);
    setupLbl(param5Lbl,"KNOB 5");
    param5Att=std::make_unique<Att>(proc.apvts,"param5",param5Knob);
    addAndMakeVisible(param6Knob); param6Knob.setLookAndFeel(&laf);
    setupLbl(param6Lbl,"KNOB 6");
    param6Att=std::make_unique<Att>(proc.apvts,"param6",param6Knob);
    addAndMakeVisible(param7Slider); param7Slider.setLookAndFeel(&laf);
    setupLbl(param7Lbl,"SLD 7");
    param7Att=std::make_unique<Att>(proc.apvts,"param7",param7Slider);
    brandLbl.setText("SOUND CAPSULE",juce::dontSendNotification);
    brandLbl.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),9.f,juce::Font::plain));
    brandLbl.setColour(juce::Label::textColourId,PC::lbl.withAlpha(.45f));
    brandLbl.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(brandLbl);
    productLbl.setText("ECHODLY",juce::dontSendNotification);
    productLbl.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),13.f,juce::Font::bold));
    productLbl.setColour(juce::Label::textColourId,PC::lbl.withAlpha(.82f));
    productLbl.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(productLbl);
}
ECHODLYEditor::~ECHODLYEditor(){setLookAndFeel(nullptr);}
void ECHODLYEditor::setupLbl(juce::Label& l,const juce::String& t){
    l.setText(t,juce::dontSendNotification);
    l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),10.f,juce::Font::plain));
    l.setColour(juce::Label::textColourId,PC::lbl);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}
void ECHODLYEditor::paint(juce::Graphics& g){
    auto b=getLocalBounds();
    g.fillAll(PC::bg);
    // Subtle vignette
    juce::ColourGradient vig(juce::Colours::transparentBlack,b.getCentreX(),b.getCentreY(),
        juce::Colours::black.withAlpha(.45f),0,0,true);
    g.setGradientFill(vig); g.fillRect(b);
}
void ECHODLYEditor::resized(){
    auto b=getLocalBounds();
    productLbl.setBounds(16,12,200,22);
    brandLbl.setBounds(b.getWidth()-190,12,174,22);
    mixKnob.setBounds(110,40,180,180);
    mixLbl.setBounds(110,222,180,16);
    sizeKnob.setBounds(20,280,90,90);
    sizeLbl.setBounds(20,372,90,16);
    preSlider.setBounds(20,70,44,120);
    preLbl.setBounds(16,192,52,16);
    dampSlider.setBounds(400,230,44,120);
    dampLbl.setBounds(396,352,52,16);
    param5Knob.setBounds(155,280,90,90);
    param5Lbl.setBounds(155,372,90,16);
    param6Knob.setBounds(290,280,90,90);
    param6Lbl.setBounds(290,372,90,16);
    param7Slider.setBounds(336,70,44,120);
    param7Lbl.setBounds(332,192,52,16);
}