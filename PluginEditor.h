#pragma once
#include "PluginProcessor.h"

class ConstellationXVocalAudioProcessorEditor : public juce::AudioProcessorEditor,
                                                private juce::Timer
{
public:
    explicit ConstellationXVocalAudioProcessorEditor(ConstellationXVocalAudioProcessor&);
    ~ConstellationXVocalAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void styleSlider(juce::Slider&, const juce::String&);
    void drawSpeaker(juce::Graphics&, juce::Rectangle<float>, float);
    void drawVortex(juce::Graphics&, juce::Rectangle<float>);

    ConstellationXVocalAudioProcessor& processor;
    juce::Slider depth, size, decay, reflections, width, warmth, air, mix, output;
    juce::Label depthLabel, sizeLabel, decayLabel;
    juce::ToggleButton motion { "VFX MOTION" };
    juce::TextButton randomize { "RANDOMIZE" };
    juce::ComboBox skin;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> motionAttachment;
    juce::Random rng;
    float animationPhase = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstellationXVocalAudioProcessorEditor)
};
