#include "PluginEditor.h"
#include "PluginProcessor.h"

const juce::Colour PC::bg(0xff705DBC);
const juce::Colour PC::lbl(0xffDAEEF2);

SaturaturEditor::SaturaturEditor(SaturaturProcessor& p)
    :AudioProcessorEditor(&p),proc(p){
    setLookAndFeel(&laf); setSize(400,400);
    biasSlider.setLookAndFeel(&laf); addAndMakeVisible(biasSlider);
    param2Slider.setLookAndFeel(&laf); addAndMakeVisible(param2Slider);
    param3Slider.setLookAndFeel(&laf); addAndMakeVisible(param3Slider);
    param4Slider.setLookAndFeel(&laf); addAndMakeVisible(param4Slider);
    outputSlider.setLookAndFeel(&laf); addAndMakeVisible(outputSlider);
    mixSlider.setLookAndFeel(&laf); addAndMakeVisible(mixSlider);
    driveKnob.setName("drive"); driveKnob.setLookAndFeel(&laf); addAndMakeVisible(driveKnob);
    typeKnob.setName("type"); typeKnob.setLookAndFeel(&laf); addAndMakeVisible(typeKnob);
    param9Knob.setName("comp"); param9Knob.setLookAndFeel(&laf); addAndMakeVisible(param9Knob);
    setupLbl(biasLbl,"GRIT");
    setupLbl(param2Lbl,"TONE");
    setupLbl(param3Lbl,"WARMTH");
    setupLbl(param4Lbl,"ATTACK");
    setupLbl(outputLbl,"OUTPUT");
    setupLbl(mixLbl,"MIX");
    setupLbl(driveLbl,"DRIVE");
    setupLbl(typeLbl,"TYPE");
    setupLbl(param9Lbl,"COMP");
    biasAtt=std::make_unique<Att>(proc.apvts,"bias",biasSlider);
    param2Att=std::make_unique<Att>(proc.apvts,"param2",param2Slider);
    param3Att=std::make_unique<Att>(proc.apvts,"param3",param3Slider);
    param4Att=std::make_unique<Att>(proc.apvts,"param4",param4Slider);
    outputAtt=std::make_unique<Att>(proc.apvts,"output",outputSlider);
    mixAtt=std::make_unique<Att>(proc.apvts,"mix",mixSlider);
    driveAtt=std::make_unique<Att>(proc.apvts,"drive",driveKnob);
    typeAtt=std::make_unique<Att>(proc.apvts,"type",typeKnob);
    param9Att=std::make_unique<Att>(proc.apvts,"param9",param9Knob);
    brandLbl.setText("SOUND CAPSULE",juce::dontSendNotification);
    brandLbl.setFont(juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(9.0f)));
    brandLbl.setColour(juce::Label::textColourId,juce::Colour(0xffdaeef2).withAlpha(.38f));
    brandLbl.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(brandLbl);
    productLbl.setText("SATURATUR",juce::dontSendNotification);
    productLbl.setFont(juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(13.0f).withStyle("Bold")));
    productLbl.setColour(juce::Label::textColourId,juce::Colour(0xffdaeef2).withAlpha(.82f));
    productLbl.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(productLbl);
}

SaturaturEditor::~SaturaturEditor(){setLookAndFeel(nullptr);}

void SaturaturEditor::setupLbl(juce::Label& l,const juce::String& t){
    l.setText(t,juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions().withName(juce::Font::getDefaultMonospacedFontName()).withHeight(10.0f)));
    l.setColour(juce::Label::textColourId,juce::Colour(0xffdaeef2));
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void SaturaturEditor::paint(juce::Graphics& g){
    auto b=getLocalBounds();
    g.fillAll(juce::Colour(0xff705DBC));
    juce::ColourGradient vig(juce::Colours::transparentBlack,b.getCentreX(),b.getCentreY(),
        juce::Colours::black.withAlpha(.45f),0,0,true);
    g.setGradientFill(vig); g.fillRect(b);
}

void SaturaturEditor::resized(){
    productLbl.setBounds(juce::roundToInt(16*scale),juce::roundToInt(12*scale),juce::roundToInt(200*scale),juce::roundToInt(22*scale));
    brandLbl.setBounds(getWidth()-juce::roundToInt(190*scale),juce::roundToInt(12*scale),juce::roundToInt(174*scale),juce::roundToInt(22*scale));
    biasSlider.setBounds(juce::roundToInt(20*scale),juce::roundToInt(60*scale),juce::roundToInt(44*scale),juce::roundToInt(130*scale));
    biasLbl.setBounds(juce::roundToInt(16*scale),juce::roundToInt(192*scale),juce::roundToInt(52*scale),juce::roundToInt(16*scale));
    param2Slider.setBounds(juce::roundToInt(80*scale),juce::roundToInt(60*scale),juce::roundToInt(44*scale),juce::roundToInt(130*scale));
    param2Lbl.setBounds(juce::roundToInt(76*scale),juce::roundToInt(192*scale),juce::roundToInt(52*scale),juce::roundToInt(16*scale));
    param3Slider.setBounds(juce::roundToInt(200*scale),juce::roundToInt(60*scale),juce::roundToInt(44*scale),juce::roundToInt(130*scale));
    param3Lbl.setBounds(juce::roundToInt(196*scale),juce::roundToInt(192*scale),juce::roundToInt(52*scale),juce::roundToInt(16*scale));
    param4Slider.setBounds(juce::roundToInt(140*scale),juce::roundToInt(60*scale),juce::roundToInt(44*scale),juce::roundToInt(130*scale));
    param4Lbl.setBounds(juce::roundToInt(136*scale),juce::roundToInt(192*scale),juce::roundToInt(52*scale),juce::roundToInt(16*scale));
    outputSlider.setBounds(juce::roundToInt(260*scale),juce::roundToInt(60*scale),juce::roundToInt(44*scale),juce::roundToInt(130*scale));
    outputLbl.setBounds(juce::roundToInt(256*scale),juce::roundToInt(192*scale),juce::roundToInt(52*scale),juce::roundToInt(16*scale));
    mixSlider.setBounds(juce::roundToInt(320*scale),juce::roundToInt(60*scale),juce::roundToInt(44*scale),juce::roundToInt(130*scale));
    mixLbl.setBounds(juce::roundToInt(316*scale),juce::roundToInt(192*scale),juce::roundToInt(52*scale),juce::roundToInt(16*scale));
    driveKnob.setBounds(juce::roundToInt(244*scale),juce::roundToInt(235*scale),juce::roundToInt(130*scale),juce::roundToInt(130*scale));
    driveLbl.setBounds(juce::roundToInt(244*scale),juce::roundToInt(367*scale),juce::roundToInt(130*scale),juce::roundToInt(16*scale));
    typeKnob.setBounds(juce::roundToInt(30*scale),juce::roundToInt(285*scale),juce::roundToInt(80*scale),juce::roundToInt(80*scale));
    typeLbl.setBounds(juce::roundToInt(30*scale),juce::roundToInt(367*scale),juce::roundToInt(80*scale),juce::roundToInt(16*scale));
    param9Knob.setBounds(juce::roundToInt(140*scale),juce::roundToInt(235*scale),juce::roundToInt(70*scale),juce::roundToInt(70*scale));
    param9Lbl.setBounds(juce::roundToInt(140*scale),juce::roundToInt(307*scale),juce::roundToInt(70*scale),juce::roundToInt(16*scale));
}

void SaturaturEditor::setScale(float s){
    scale = s;
    setSize(juce::roundToInt(400*s), juce::roundToInt(400*s));
}

void SaturaturEditor::mouseDown(const juce::MouseEvent& e){
    if(!e.mods.isRightButtonDown()) return;
    juce::PopupMenu m;
    m.addSectionHeader("Window Size");
    m.addItem(1, "75%",  true, std::abs(scale-0.75f)<0.01f);
    m.addItem(2, "100%", true, std::abs(scale-1.0f)<0.01f);
    m.addItem(3, "125%", true, std::abs(scale-1.25f)<0.01f);
    m.addItem(4, "150%", true, std::abs(scale-1.5f)<0.01f);
    m.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
        [this](int r){ if(r>0){ const float s[]={0.75f,1.0f,1.25f,1.5f}; setScale(s[r-1]); } });
}

juce::AudioProcessorEditor* SaturaturProcessor::createEditor(){
    return new SaturaturEditor(*this);
}
