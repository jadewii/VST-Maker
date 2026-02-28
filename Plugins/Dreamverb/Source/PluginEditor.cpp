#include "PluginEditor.h"
#include "PluginProcessor.h"

DreamverbEditor::DreamverbEditor(DreamverbProcessor& p)
    :AudioProcessorEditor(&p),proc(p){
    setLookAndFeel(&laf); setSize(400,400);
    addAndMakeVisible(mixKnob); mixKnob.setLookAndFeel(&laf);
    setupLbl(mixLbl,"MIX");
    mixAtt=std::make_unique<Att>(proc.apvts,"mix",mixKnob);
    addAndMakeVisible(sizeKnob); sizeKnob.setLookAndFeel(&laf);
    setupLbl(sizeLbl,"SIZE");
    sizeAtt=std::make_unique<Att>(proc.apvts,"size",sizeKnob);
    addAndMakeVisible(toneSlider); toneSlider.setLookAndFeel(&laf);
    setupLbl(toneLbl,"TONE");
    toneAtt=std::make_unique<Att>(proc.apvts,"tone",toneSlider);
    addAndMakeVisible(dampSlider); dampSlider.setLookAndFeel(&laf);
    setupLbl(dampLbl,"DAMP");
    dampAtt=std::make_unique<Att>(proc.apvts,"damp",dampSlider);
    brandLbl.setText("SOUND CAPSULE",juce::dontSendNotification);
    brandLbl.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),9.f,juce::Font::plain));
    brandLbl.setColour(juce::Label::textColourId,PC::lbl.withAlpha(.45f));
    brandLbl.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(brandLbl);
    productLbl.setText("DREAMVERB",juce::dontSendNotification);
    productLbl.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),13.f,juce::Font::bold));
    productLbl.setColour(juce::Label::textColourId,PC::lbl.withAlpha(.82f));
    productLbl.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(productLbl);
}
DreamverbEditor::~DreamverbEditor(){setLookAndFeel(nullptr);}
void DreamverbEditor::setupLbl(juce::Label& l,const juce::String& t){
    l.setText(t,juce::dontSendNotification);
    l.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(),10.f,juce::Font::plain));
    l.setColour(juce::Label::textColourId,PC::lbl);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}
void DreamverbEditor::paint(juce::Graphics& g){
    auto b=getLocalBounds();
    g.fillAll(PC::bg);
    // Subtle vignette
    juce::ColourGradient vig(juce::Colours::transparentBlack,b.getCentreX(),b.getCentreY(),
        juce::Colours::black.withAlpha(.45f),0,0,true);
    g.setGradientFill(vig); g.fillRect(b);
}
void DreamverbEditor::resized(){
    auto b=getLocalBounds();
    productLbl.setBounds(16,12,200,22);
    brandLbl.setBounds(b.getWidth()-190,12,174,22);
    mixKnob.setBounds(210,50,180,180);
    mixLbl.setBounds(210,232,180,16);
    sizeKnob.setBounds(40,210,140,140);
    sizeLbl.setBounds(40,352,140,16);
    toneSlider.setBounds(260,260,44,120);
    toneLbl.setBounds(256,382,52,16);
    dampSlider.setBounds(320,260,44,120);
    dampLbl.setBounds(316,382,52,16);
}