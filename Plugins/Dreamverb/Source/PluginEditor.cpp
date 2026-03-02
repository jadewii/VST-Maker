#include "PluginEditor.h"
#include "PluginProcessor.h"

DreamverbEditor::DreamverbEditor(DreamverbProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setLookAndFeel(&laf);
    setSize(BASE_W, BASE_H);

    addAndMakeVisible(mixKnob);  mixKnob.setLookAndFeel(&laf);
    setupLbl(mixLbl, "MIX");
    mixAtt  = std::make_unique<Att>(proc.apvts, "mix",    mixKnob);

    addAndMakeVisible(sizeKnob); sizeKnob.setLookAndFeel(&laf);
    setupLbl(sizeLbl, "SIZE");
    sizeAtt = std::make_unique<Att>(proc.apvts, "size",   sizeKnob);

    addAndMakeVisible(toneSlider); toneSlider.setLookAndFeel(&laf);
    setupLbl(toneLbl, "TONE");
    toneAtt = std::make_unique<Att>(proc.apvts, "tone",   toneSlider);

    addAndMakeVisible(shimSlider); shimSlider.setLookAndFeel(&laf);
    setupLbl(shimLbl, "SHIMMER");
    shimAtt = std::make_unique<Att>(proc.apvts, "param5", shimSlider);

    addAndMakeVisible(dampSlider); dampSlider.setLookAndFeel(&laf);
    setupLbl(dampLbl, "DAMP");
    dampAtt = std::make_unique<Att>(proc.apvts, "damp",   dampSlider);

    brandLbl.setText("SOUND CAPSULE", juce::dontSendNotification);
    brandLbl.setFont(juce::Font(juce::FontOptions()
        .withName(juce::Font::getDefaultMonospacedFontName()).withHeight(9.0f)));
    brandLbl.setColour(juce::Label::textColourId, PC::lbl.withAlpha(0.45f));
    brandLbl.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(brandLbl);

    productLbl.setText("DREAMVERB", juce::dontSendNotification);
    productLbl.setFont(juce::Font(juce::FontOptions()
        .withName(juce::Font::getDefaultMonospacedFontName())
        .withHeight(13.0f).withStyle("Bold")));
    productLbl.setColour(juce::Label::textColourId, PC::lbl.withAlpha(0.82f));
    productLbl.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(productLbl);

    addAndMakeVisible(resizeBtn);
    resizeBtn.onClick = [this]() {
        juce::PopupMenu m;
        m.addSectionHeader("Window Size");
        m.addItem(1, "50%",  true, std::abs(scale - 0.50f) < 0.01f);
        m.addItem(2, "75%",  true, std::abs(scale - 0.75f) < 0.01f);
        m.addItem(3, "100%", true, std::abs(scale - 1.00f) < 0.01f);
        m.addItem(4, "125%", true, std::abs(scale - 1.25f) < 0.01f);
        m.addItem(5, "150%", true, std::abs(scale - 1.50f) < 0.01f);
        m.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(&resizeBtn),
            [this](int r) {
                if (r > 0) {
                    const float s[] = { 0.50f, 0.75f, 1.00f, 1.25f, 1.50f };
                    setScale(s[r - 1]);
                }
            });
    };
}

DreamverbEditor::~DreamverbEditor() { setLookAndFeel(nullptr); }

void DreamverbEditor::setupLbl(juce::Label& l, const juce::String& t) {
    l.setText(t, juce::dontSendNotification);
    l.setFont(juce::Font(juce::FontOptions()
        .withName(juce::Font::getDefaultMonospacedFontName()).withHeight(10.0f)));
    l.setColour(juce::Label::textColourId, PC::lbl);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void DreamverbEditor::paint(juce::Graphics& g) {
    g.fillAll(PC::bg);
    auto b = getLocalBounds();
    juce::ColourGradient vig(juce::Colours::transparentBlack, b.getCentreX(), b.getCentreY(),
                             juce::Colours::black.withAlpha(0.45f), 0.f, 0.f, true);
    g.setGradientFill(vig);
    g.fillRect(b);
}

void DreamverbEditor::resized() {
    const int W = getWidth(), H = getHeight();

    productLbl.setBounds(juce::roundToInt(16*scale),  juce::roundToInt(12*scale),
                         juce::roundToInt(200*scale), juce::roundToInt(22*scale));
    brandLbl.setBounds(W - juce::roundToInt(190*scale), juce::roundToInt(12*scale),
                       juce::roundToInt(174*scale),    juce::roundToInt(22*scale));

    // MIX knob — scproj x:194 y:40 size:180
    mixKnob.setBounds(juce::roundToInt(194*scale), juce::roundToInt(40*scale),
                      juce::roundToInt(180*scale), juce::roundToInt(180*scale));
    mixLbl.setBounds (juce::roundToInt(194*scale), juce::roundToInt(222*scale),
                      juce::roundToInt(180*scale), juce::roundToInt(16*scale));

    // SIZE knob — scproj x:40 y:210 size:140
    sizeKnob.setBounds(juce::roundToInt(40*scale),  juce::roundToInt(210*scale),
                       juce::roundToInt(140*scale), juce::roundToInt(140*scale));
    sizeLbl.setBounds (juce::roundToInt(40*scale),  juce::roundToInt(352*scale),
                       juce::roundToInt(140*scale), juce::roundToInt(16*scale));

    // Three sliders — scproj: TONE x:220, SHIMMER x:275, DAMP x:330, y:260, size:100
    const int slW  = juce::roundToInt(44  * scale);
    const int slH  = juce::roundToInt(100 * scale);
    const int slY  = juce::roundToInt(260 * scale);
    const int lblH = juce::roundToInt(16  * scale);
    const int lblY = slY + slH + juce::roundToInt(4 * scale);

    toneSlider.setBounds(juce::roundToInt(220*scale), slY, slW, slH);
    toneLbl   .setBounds(juce::roundToInt(216*scale), lblY, juce::roundToInt(52*scale), lblH);

    shimSlider.setBounds(juce::roundToInt(275*scale), slY, slW, slH);
    shimLbl   .setBounds(juce::roundToInt(260*scale), lblY, juce::roundToInt(72*scale), lblH);

    dampSlider.setBounds(juce::roundToInt(330*scale), slY, slW, slH);
    dampLbl   .setBounds(juce::roundToInt(326*scale), lblY, juce::roundToInt(52*scale), lblH);

    // Resize dot — bottom-right corner
    resizeBtn.setBounds(W - juce::roundToInt(30*scale), H - juce::roundToInt(30*scale),
                        juce::roundToInt(24*scale),     juce::roundToInt(24*scale));
}

void DreamverbEditor::setScale(float s) {
    scale = s;
    setSize(juce::roundToInt(BASE_W * s), juce::roundToInt(BASE_H * s));
    repaint();
}

// ── NOTE: createEditor() lives in PluginProcessor.cpp ONLY ────────────────
// Adding it here = duplicate symbol linker error. Never add it here again.
