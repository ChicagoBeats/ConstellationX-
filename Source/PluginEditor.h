#pragma once
#include "PluginProcessor.h"

// Black-and-red look (skins recolour the accent). Draws knobs, the Forward/Back slider,
// the VFX ON/OFF pill and buttons.
class ConstellationLookAndFeel : public juce::LookAndFeel_V4
{
public:
    juce::Colour accent { 0xffff2638 };

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

    // Reference-design coordinates (1672 x 941) scaled to the current window size.
    float scale() const { return (float) getWidth() / 1672.0f; }
    juce::Rectangle<float> R(float x, float y, float w, float h) const;
    juce::Rectangle<float> swatchRect(int i) const;

    void drawPanel(juce::Graphics&, juce::Rectangle<float>);
    void drawSpeaker(juce::Graphics&, juce::Point<float> centre, float radius, float level);
    void drawVortex(juce::Graphics&, juce::Rectangle<float>);
    void drawLabel(juce::Graphics&, const juce::String&, float cx, float cy, float refFontHeight,
                   juce::Colour, bool bold = true);

    ConstellationXVocalAudioProcessor& processor;
    ConstellationLookAndFeel lnf;

    juce::Slider depth, size, decay, reflections, width, warmth, air, mix, output;
    juce::ToggleButton motion { "VFX MOTION" };
    juce::TextButton randomize { "RANDOM" }, skinPrev { "<" }, skinNext { ">" };
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> motionAttachment;

    double phase = 0.0, lastTimeMs = 0.0;
    float lowL = 0.0f, lowR = 0.0f, inLevel = 0.0f;   // smoothed meter values for animation
    int shownSkin = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstellationXVocalAudioProcessorEditor)
};
