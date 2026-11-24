#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "ConstellationEngine.h"

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
    double getTailLengthSeconds() const override;

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Message-thread only. Randomises the musical parameters inside controlled ranges
    // (never Output, VFX Motion or Skin).
    void randomize();

    // Real-time meters written by the audio thread, read by the editor (drives the visuals).
    std::atomic<float> meterLowL { 0.0f }, meterLowR { 0.0f }, meterIn { 0.0f },
                       meterOutL { 0.0f }, meterOutR { 0.0f };

    juce::AudioProcessorValueTreeState apvts;

private:
    cx::EngineParams readParams() const;

    cx::Engine engine;
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstellationXVocalAudioProcessor)
};
