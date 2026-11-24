#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using APB = juce::AudioParameterBool;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addFloat = [&p](const char* id, const char* name, float lo, float hi,
                         float def, const juce::String& unit = {})
    {
        p.push_back(std::make_unique<APF>(id, name,
            juce::NormalisableRange<float>(lo, hi), def, unit));
    };

    addFloat("depth", "Depth", -1.0f, 1.0f, 0.0f);
    addFloat("size", "Size", 0.0f, 1.0f, 0.5f);
    addFloat("decay", "Decay", 0.1f, 8.0f, 2.0f, " s");
    addFloat("reflections", "Reflections", 0.0f, 1.0f, 0.35f);
    addFloat("width", "Width", 0.0f, 1.5f, 0.65f);
    addFloat("warmth", "Warmth", -1.0f, 1.0f, 0.0f);
    addFloat("air", "Air", -1.0f, 1.0f, 0.0f);
    addFloat("mix", "Wet Dry", 0.0f, 1.0f, 0.35f);
    addFloat("output", "Output", -24.0f, 12.0f, 0.0f, " dB");
    p.push_back(std::make_unique<APB>("vfxMotion", "VFX Motion", true));
    return { p.begin(), p.end() };
}
}

ConstellationXVocalAudioProcessor::ConstellationXVocalAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "CONSTELLATIONX_STATE", createParameterLayout())
{
}

void ConstellationXVocalAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    delayBuffer.setSize(2, juce::jmax(1, (int) (sampleRate * 0.25) + samplesPerBlock));
    delayBuffer.clear();
    delayWritePosition = 0;

    auto lowCutCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 35.0f);
    auto airCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 9000.0f, 0.7f, juce::Decibels::decibelsToGain(1.0f));

    for (int ch = 0; ch < 2; ++ch)
    {
        *lowCut[ch].coefficients = *lowCutCoeffs;
        *airShelf[ch].coefficients = *airCoeffs;
        lowCut[ch].reset();
        airShelf[ch].reset();
    }
}

void ConstellationXVocalAudioProcessor::releaseResources()
{
    delayBuffer.setSize(0, 0);
}

bool ConstellationXVocalAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo())
        && out == in;
}

void ConstellationXVocalAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                      juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();

    const float depth = apvts.getRawParameterValue("depth")->load();
    const float size = apvts.getRawParameterValue("size")->load();
    const float reflections = apvts.getRawParameterValue("reflections")->load();
    const float width = apvts.getRawParameterValue("width")->load();
    const float warmth = apvts.getRawParameterValue("warmth")->load();
    const float air = apvts.getRawParameterValue("air")->load();
    const float mix = apvts.getRawParameterValue("mix")->load();
    const float output = juce::Decibels::decibelsToGain(
        apvts.getRawParameterValue("output")->load());

    const int delayLength = juce::jlimit(1, delayBuffer.getNumSamples() - 1,
        (int) (currentSampleRate * (0.012 + size * 0.105)));
    const float reflectionGain = reflections * (0.08f + 0.24f * std::abs(depth));
    const float directGain = depth > 0.0f ? 1.0f : (1.0f - 0.30f * std::abs(depth));

    for (int i = 0; i < samples; ++i)
    {
        const int readPosition = (delayWritePosition - delayLength + delayBuffer.getNumSamples())
                               % delayBuffer.getNumSamples();

        float in[2] = { 0.0f, 0.0f };
        float wet[2] = { 0.0f, 0.0f };

        for (int ch = 0; ch < channels; ++ch)
        {
            in[ch] = buffer.getSample(ch, i);
            const float delayed = delayBuffer.getSample(ch, readPosition);
            delayBuffer.setSample(ch, delayWritePosition, in[ch]);

            float shaped = in[ch];
            shaped = lowCut[ch].processSample(shaped);
            shaped = airShelf[ch].processSample(shaped);
            const float toneGain = juce::Decibels::decibelsToGain(warmth * 2.0f + air * 2.0f);
            wet[ch] = shaped * directGain + delayed * reflectionGain;
            wet[ch] *= toneGain;
        }

        if (channels == 2)
        {
            const float mid = 0.5f * (wet[0] + wet[1]);
            const float side = 0.5f * (wet[0] - wet[1]) * (1.0f + width);
            wet[0] = mid + side;
            wet[1] = mid - side;
        }

        for (int ch = 0; ch < channels; ++ch)
            buffer.setSample(ch, i, ((1.0f - mix) * in[ch] + mix * wet[ch]) * output);

        delayWritePosition = (delayWritePosition + 1) % delayBuffer.getNumSamples();
    }
}

juce::AudioProcessorEditor* ConstellationXVocalAudioProcessor::createEditor()
{
    return new ConstellationXVocalAudioProcessorEditor(*this);
}

void ConstellationXVocalAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary(*xml, destData);
}

void ConstellationXVocalAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ConstellationXVocalAudioProcessor();
}
