#pragma once
#include "PluginProcessor.h"

// Black-and-red look (skins recolour the accent). Draws knobs, the Forward/Back slider,
// the VFX ON/OFF pill and buttons.
class ConstellationLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour accent { 0xffff2638 };
    bool drawGrip = true;   // off when the artwork already contains the resize grip

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                          float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawLinearSlider(juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                          float minSliderPos, float maxSliderPos, juce::Slider::SliderStyle,
                          juce::Slider&) override;
    int getSliderThumbRadius(juce::Slider&) override;
    void drawToggleButton(juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool highlighted,
                              bool down) override;
    juce::Font getTextButtonFont(juce::TextButton&, int buttonHeight) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool highlighted, bool down) override;
    void drawCornerResizer(juce::Graphics&, int w, int h, bool isMouseOver, bool isMouseDragging) override;
};

class ConstellationXVocalAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit ConstellationXVocalAudioProcessorEditor(ConstellationXVocalAudioProcessor&);
    ~ConstellationXVocalAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void styleKnob(juce::Slider&, const char* paramId);
    void setSkin(int index);
    int currentSkin() const;

    void markDirty();
    void doUndo();
    void doRedo();
    void refreshButtons();
    void showPresetsMenu(juce::Component& target);
    void showFavoritesMenu(juce::Component& target);
    juce::Rectangle<float> presetBoxRect() const;

    // Reference-design coordinates (1672 x 941) scaled to the current window size.
    float scale() const { return (float) getWidth() / 1672.0f; }
    juce::Rectangle<float> R(float x, float y, float w, float h) const;
    juce::Rectangle<float> swatchRect(int i) const;

    void drawPanel(juce::Graphics&, juce::Rectangle<float>);
    struct ArtSet
    {
        juce::Image full, vortexCore, speakerL, speakerR;   // artwork (per skin) and feathered animated cut-outs
    };
    const ArtSet* getArt(int skin);
    void drawArtLive(juce::Graphics&, const ArtSet&);
    void drawPresetBox(juce::Graphics&);
    void paintFallback(juce::Graphics&);
    void showSettingsMenu(juce::Component& target);
    void showHelpMenu(juce::Component& target);
    void ensureLayers();
    void renderLayers();
    void renderBackground(juce::Graphics&);
    void renderForeground(juce::Graphics&);
    void drawSpeakerHousing(juce::Graphics&, juce::Point<float> centre, float radius);
    void fillCarbon(juce::Graphics&, const juce::Path&);
    void drawSpeaker(juce::Graphics&, juce::Point<float> centre, float radius, float level);
    void drawVortex(juce::Graphics&, juce::Rectangle<float>);
    void drawLabel(juce::Graphics&, const juce::String&, float cx, float cy, float refFontHeight,
                   juce::Colour, bool bold = true);

    ConstellationXVocalAudioProcessor& processor;
    ConstellationLookAndFeel lnf;

    juce::Slider depth, size, decay, reflections, width, warmth, air, mix, output;
    juce::ToggleButton motion { "VFX MOTION" };
    juce::TextButton randomize { "RANDOM" }, skinPrev { "<" }, skinNext { ">" };
    juce::TextButton saveBtn { "SAVE" }, presetPrev { "<" }, presetNext { ">" }, slotA { "A" }, slotB { "B" },
                     undoTop { "UNDO" }, redoTop { "REDO" };
    juce::TextButton presetsBtn { "PRESETS" }, favoritesBtn { "FAVORITES" }, abBtn { "A/B" },
                     undoBtn { "UNDO" }, redoBtn { "REDO" };
    juce::TextButton menuBtn { "MENU" }, settingsBtn { "SETTINGS" }, helpBtn { "HELP" };
    std::unique_ptr<juce::ResizableCornerComponent> resizer;
    juce::Image artSource;      // the approved reference artwork (embedded), hue-shifted per skin on demand
    ArtSet artSets[5];
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> motionAttachment;

    double phase = 0.0, lastTimeMs = 0.0;
    float lowL = 0.0f, lowR = 0.0f, inLevel = 0.0f;   // smoothed meter values for animation
    int shownSkin = -1;
    // cached static artwork (rebuilt on resize / skin change); live parts are painted between the layers
    juce::Image bgImage, fgImage, carbonTile;
    int layerW = 0, layerH = 0;
    float layerScale = 1.0f;
    juce::Colour layerAccent;

    float depthVis = 0.0f;           // eased copy of the Depth parameter, drives the vortex
    bool dirty = false;              // a user edit is waiting to be committed to the undo history
    double lastChangeMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstellationXVocalAudioProcessorEditor)
};
