#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

class ConstellationXVocalAudioProcessor : public juce::AudioProcessor
{
public:
    ConstellationXVocalAudioProcessor();
    ~ConstellationXVocalAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "ConstellationX Vocal"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePosition = 0;
    double currentSampleRate = 44100.0;
    juce::dsp::IIR::Filter<float> lowCut[2];
    juce::dsp::IIR::Filter<float> airShelf[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstellationXVocalAudioProcessor)
};
