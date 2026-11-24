#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
juce::String pct(float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }
float fromPct(const juce::String& t) { return t.getFloatValue() / 100.0f; }

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using APF = juce::AudioParameterFloat;
    using Cat = juce::AudioProcessorParameter;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // depth: -1 = FORWARD ... +1 = BACK
    p.push_back(std::make_unique<APF>("depth", "Depth", juce::NormalisableRange<float>(-1.0f, 1.0f), 0.0f, "",
        Cat::genericParameter,
        [](float v, int) { return v < -0.005f ? "Fwd " + juce::String(juce::roundToInt(-v * 100.0f))
                         : v > 0.005f ? "Back " + juce::String(juce::roundToInt(v * 100.0f)) : juce::String("Center"); },
        nullptr));
    p.push_back(std::make_unique<APF>("size", "Size", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f, "",
        Cat::genericParameter, pct, fromPct));
    p.push_back(std::make_unique<APF>("decay", "Decay", juce::NormalisableRange<float>(0.1f, 8.0f, 0.0f, 0.5f), 2.0f, "s",
        Cat::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " s"; },
        [](const juce::String& t) { return t.getFloatValue(); }));
    p.push_back(std::make_unique<APF>("reflections", "Reflections", juce::NormalisableRange<float>(0.0f, 1.0f), 0.60f, "",
        Cat::genericParameter, pct, fromPct));
    p.push_back(std::make_unique<APF>("width", "Width", juce::NormalisableRange<float>(0.0f, 1.0f), 0.75f, "",
        Cat::genericParameter, pct, fromPct));
    p.push_back(std::make_unique<APF>("warmth", "Warmth", juce::NormalisableRange<float>(0.0f, 1.0f), 0.45f, "",
        Cat::genericParameter, pct, fromPct));
    p.push_back(std::make_unique<APF>("air", "Air", juce::NormalisableRange<float>(0.0f, 1.0f), 0.70f, "",
        Cat::genericParameter, pct, fromPct));
    p.push_back(std::make_unique<APF>("mix", "Wet Dry", juce::NormalisableRange<float>(0.0f, 1.0f), 0.40f, "",
        Cat::genericParameter, pct, fromPct));
    p.push_back(std::make_unique<APF>("output", "Output", juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f), 0.0f, "dB",
        Cat::genericParameter,
        [](float v, int) { return juce::String(v, 1) + "dB"; },
        [](const juce::String& t) { return t.getFloatValue(); }));
    p.push_back(std::make_unique<juce::AudioParameterBool>("vfxMotion", "VFX Motion", true));
    p.push_back(std::make_unique<juce::AudioParameterChoice>("skin", "Skin",
        juce::StringArray { "Crimson", "Sapphire", "Violet", "Emerald", "Amber" }, 0));
    return { p.begin(), p.end() };
}
} // namespace

ConstellationXVocalAudioProcessor::ConstellationXVocalAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "CONSTELLATIONX_STATE", createParameterLayout())
{
}

double ConstellationXVocalAudioProcessor::getTailLengthSeconds() const
{
    return (double) apvts.getRawParameterValue("decay")->load() + 0.3;
}

cx::EngineParams ConstellationXVocalAudioProcessor::readParams() const
{
    auto get = [this](const char* id) { return apvts.getRawParameterValue(id)->load(); };
    cx::EngineParams p;
    p.depth = get("depth");
    p.size = get("size");
    p.decay = get("decay");
    p.reflections = get("reflections");
    p.width = get("width");
    p.warmth = get("warmth");
    p.air = get("air");
    p.mix = get("mix");
    p.outputDb = get("output");
    return p;
}

void ConstellationXVocalAudioProcessor::prepareToPlay(double sampleRate, int)
{
    engine.setParams(readParams(), true);
    engine.prepare(sampleRate);
}

void ConstellationXVocalAudioProcessor::releaseResources() {}

bool ConstellationXVocalAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    return (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo()) && out == in;
}

void ConstellationXVocalAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    const int numCh = buffer.getNumChannels();
    if (numCh < 1 || buffer.getNumSamples() < 1)
        return;

    engine.setParams(readParams());
    float* left = buffer.getWritePointer(0);
    float* right = numCh > 1 ? buffer.getWritePointer(1) : nullptr;
    engine.process(left, right, buffer.getNumSamples());

    const auto m = engine.getMeters();
    meterLowL.store(m.lowL);  meterLowR.store(m.lowR);  meterIn.store(m.inLevel);
    meterOutL.store(m.outL);  meterOutR.store(m.outR);
}

void ConstellationXVocalAudioProcessor::randomize()
{
    struct Range { const char* id; float lo, hi; };
    static const Range ranges[] = {
        { "depth", -0.7f, 0.9f },   { "size", 0.15f, 0.85f },  { "decay", 0.5f, 5.0f },
        { "reflections", 0.2f, 0.85f }, { "width", 0.4f, 0.95f }, { "warmth", 0.0f, 0.75f },
        { "air", 0.0f, 0.85f },     { "mix", 0.2f, 0.55f }
    };
    for (const auto& r : ranges)
    {
        if (auto* param = apvts.getParameter(r.id))
        {
            const float value = r.lo + (r.hi - r.lo) * rng.nextFloat();
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(value));
            param->endChangeGesture();
        }
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
