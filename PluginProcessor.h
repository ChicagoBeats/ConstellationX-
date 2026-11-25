#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "ConstellationEngine.h"
#include "ParamTable.h"

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

    // ---- everything below is message-thread only -------------------------------------------
    cx::ParamState getState() const;                 // current sound parameters (real units)
    void applyState(const cx::ParamState&);          // sets them (host sees proper gestures)

    // Randomizer: controlled musical ranges; never touches Output, VFX Motion or Skin.
    void randomize();

    // Undo / redo of sound parameters
    void commitHistory();                            // call after a user edit has settled
    bool undo();
    bool redo();
    bool canUndo() const { return history.canUndo(); }
    bool canRedo() const { return history.canRedo(); }

    // A/B comparison (slot 0 = A, slot 1 = B)
    void selectSlot(int slot);
    int getActiveSlot() const { return activeSlot; }

    // Presets
    juce::String getPresetName() const { return presetName; }
    void loadFactoryPreset(int index);
    bool loadPresetByName(const juce::String& name);   // factory first, then user presets
    juce::StringArray getUserPresetNames() const;
    juce::StringArray getAllPresetNames() const;       // factory + user
    void stepPreset(int delta);
    juce::String saveUserPreset();                     // saves as "User Preset N"; returns name or empty
    static juce::File getPresetDirectory();

    // Favorites (preset names, stored next to the user presets)
    juce::StringArray getFavorites() const;
    bool isFavorite(const juce::String& name) const;
    void setFavorite(const juce::String& name, bool shouldBeFavorite);

    // Real-time meters written by the audio thread, read by the editor (drives the visuals).
    std::atomic<float> meterLowL { 0.0f }, meterLowR { 0.0f }, meterIn { 0.0f },
                       meterOutL { 0.0f }, meterOutR { 0.0f };

    juce::AudioProcessorValueTreeState apvts;

private:
    cx::EngineParams readParams() const;
    static juce::String stateToString(const cx::ParamState&);
    static bool stringToState(const juce::String&, cx::ParamState&);
    static juce::File getFavoritesFile();

    cx::Engine engine;
    juce::Random rng;

    cx::ParamHistory history;
    cx::ParamState slots[2];
    bool slotValid[2] = { true, false };
    int activeSlot = 0;
    juce::String presetName { "INIT" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstellationXVocalAudioProcessor)
};
