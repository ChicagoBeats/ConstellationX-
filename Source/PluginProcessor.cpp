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
    using juce::NormalisableRange;
    const auto& P = cx::kParams;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // depth: -1 = FORWARD ... +1 = BACK
    p.push_back(std::make_unique<APF>(P[cx::kDepth].id, P[cx::kDepth].name,
        NormalisableRange<float>(P[cx::kDepth].minV, P[cx::kDepth].maxV), P[cx::kDepth].def, "",
        Cat::genericParameter,
        [](float v, int) { return v < -0.005f ? "Fwd " + juce::String(juce::roundToInt(-v * 100.0f))
                         : v > 0.005f ? "Back " + juce::String(juce::roundToInt(v * 100.0f)) : juce::String("Center"); },
        nullptr));

    for (int i : { (int) cx::kSize, (int) cx::kReflections, (int) cx::kWidth, (int) cx::kWarmth, (int) cx::kAir, (int) cx::kMix })
        p.push_back(std::make_unique<APF>(P[i].id, P[i].name, NormalisableRange<float>(P[i].minV, P[i].maxV),
                                          P[i].def, "", Cat::genericParameter, pct, fromPct));

    p.push_back(std::make_unique<APF>(P[cx::kDecay].id, P[cx::kDecay].name,
        NormalisableRange<float>(P[cx::kDecay].minV, P[cx::kDecay].maxV, 0.0f, 0.5f), P[cx::kDecay].def, "s",
        Cat::genericParameter,
        [](float v, int) { return juce::String(v, 1) + " s"; },
        [](const juce::String& t) { return t.getFloatValue(); }));
    p.push_back(std::make_unique<APF>(P[cx::kOutput].id, P[cx::kOutput].name,
        NormalisableRange<float>(P[cx::kOutput].minV, P[cx::kOutput].maxV, 0.1f), P[cx::kOutput].def, "dB",
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
    history.reset(getState());
    slots[0] = slots[1] = getState();
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

// ------------------------------- state / undo / A-B ------------------------------------------
cx::ParamState ConstellationXVocalAudioProcessor::getState() const
{
    cx::ParamState s {};
    for (int i = 0; i < cx::kNumSoundParams; ++i)
        s[(size_t) i] = apvts.getRawParameterValue(cx::kParams[i].id)->load();
    return s;
}

void ConstellationXVocalAudioProcessor::applyState(const cx::ParamState& s)
{
    for (int i = 0; i < cx::kNumSoundParams; ++i)
    {
        if (auto* param = apvts.getParameter(cx::kParams[i].id))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost(param->convertTo0to1(cx::clampParam(i, s[(size_t) i])));
            param->endChangeGesture();
        }
    }
}

void ConstellationXVocalAudioProcessor::randomize()
{
    struct Range { int index; float lo, hi; };
    static const Range ranges[] = {
        { cx::kDepth, -0.7f, 0.9f },   { cx::kSize, 0.15f, 0.85f },  { cx::kDecay, 0.5f, 5.0f },
        { cx::kReflections, 0.2f, 0.85f }, { cx::kWidth, 0.4f, 0.95f }, { cx::kWarmth, 0.0f, 0.75f },
        { cx::kAir, 0.0f, 0.85f },     { cx::kMix, 0.2f, 0.55f }
    };
    cx::ParamState s = getState();                  // keeps Output untouched
    for (const auto& r : ranges)
        s[(size_t) r.index] = r.lo + (r.hi - r.lo) * rng.nextFloat();
    applyState(s);
    presetName = "Random";
    commitHistory();
}

void ConstellationXVocalAudioProcessor::commitHistory() { history.commit(getState()); }

bool ConstellationXVocalAudioProcessor::undo()
{
    cx::ParamState s;
    if (!history.undo(s)) return false;
    applyState(s);
    return true;
}

bool ConstellationXVocalAudioProcessor::redo()
{
    cx::ParamState s;
    if (!history.redo(s)) return false;
    applyState(s);
    return true;
}

void ConstellationXVocalAudioProcessor::selectSlot(int slot)
{
    slot = juce::jlimit(0, 1, slot);
    if (slot == activeSlot) return;
    slots[activeSlot] = getState();                 // remember edits made in the slot we leave
    slotValid[activeSlot] = true;
    if (!slotValid[slot]) { slots[slot] = slots[activeSlot]; slotValid[slot] = true; }   // first visit: start from the other slot
    activeSlot = slot;
    applyState(slots[slot]);
    commitHistory();
}

// ----------------------------------- presets -------------------------------------------------
juce::File ConstellationXVocalAudioProcessor::getPresetDirectory()
{
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("Chicago Beats Audio Labs").getChildFile("ConstellationX Vocal").getChildFile("Presets");
    dir.createDirectory();
    return dir;
}

juce::File ConstellationXVocalAudioProcessor::getFavoritesFile()
{
    return getPresetDirectory().getChildFile("favorites.txt");
}

void ConstellationXVocalAudioProcessor::loadFactoryPreset(int index)
{
    index = juce::jlimit(0, cx::kNumFactoryPresets - 1, index);
    applyState(cx::factoryState(index));
    presetName = cx::kFactoryPresets[index].name;
    commitHistory();
}

juce::StringArray ConstellationXVocalAudioProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& f : getPresetDirectory().findChildFiles(juce::File::findFiles, false, "*.cxpreset"))
        names.add(f.getFileNameWithoutExtension());
    names.sort(true);
    return names;
}

juce::StringArray ConstellationXVocalAudioProcessor::getAllPresetNames() const
{
    juce::StringArray names;
    for (int i = 0; i < cx::kNumFactoryPresets; ++i)
        names.add(cx::kFactoryPresets[i].name);
    names.addArray(getUserPresetNames());
    return names;
}

bool ConstellationXVocalAudioProcessor::loadPresetByName(const juce::String& name)
{
    for (int i = 0; i < cx::kNumFactoryPresets; ++i)
        if (name == cx::kFactoryPresets[i].name)
        {
            loadFactoryPreset(i);
            return true;
        }

    const auto file = getPresetDirectory().getChildFile(name + ".cxpreset");
    if (!file.existsAsFile()) return false;
    auto xml = juce::parseXML(file);
    if (xml == nullptr || !xml->hasTagName("CXPRESET")) return false;

    cx::ParamState s = getState();
    for (int i = 0; i < cx::kNumSoundParams; ++i)
        if (xml->hasAttribute(cx::kParams[i].id))
            s[(size_t) i] = (float) xml->getDoubleAttribute(cx::kParams[i].id, (double) cx::kParams[i].def);
    applyState(s);
    presetName = name;
    commitHistory();
    return true;
}

void ConstellationXVocalAudioProcessor::stepPreset(int delta)
{
    const auto names = getAllPresetNames();
    if (names.isEmpty()) return;
    int idx = names.indexOf(presetName);
    idx = idx < 0 ? 0 : (idx + delta + names.size()) % names.size();
    loadPresetByName(names[idx]);
}

juce::String ConstellationXVocalAudioProcessor::saveUserPreset()
{
    const auto dir = getPresetDirectory();
    juce::String name;
    for (int n = 1; n < 10000; ++n)
    {
        name = "User Preset " + juce::String(n);
        if (!dir.getChildFile(name + ".cxpreset").existsAsFile()) break;
    }
    juce::XmlElement xml("CXPRESET");
    xml.setAttribute("name", name);
    const auto s = getState();
    for (int i = 0; i < cx::kNumSoundParams; ++i)
        xml.setAttribute(cx::kParams[i].id, (double) s[(size_t) i]);
    if (!dir.getChildFile(name + ".cxpreset").replaceWithText(xml.toString()))
        return {};
    presetName = name;
    return name;
}

// ---------------------------------- favorites ------------------------------------------------
juce::StringArray ConstellationXVocalAudioProcessor::getFavorites() const
{
    juce::StringArray lines;
    lines.addLines(getFavoritesFile().loadFileAsString());
    lines.trim();
    lines.removeEmptyStrings();
    lines.removeDuplicates(false);
    return lines;
}

bool ConstellationXVocalAudioProcessor::isFavorite(const juce::String& name) const
{
    return getFavorites().contains(name);
}

void ConstellationXVocalAudioProcessor::setFavorite(const juce::String& name, bool shouldBeFavorite)
{
    auto favs = getFavorites();
    if (shouldBeFavorite) favs.addIfNotAlreadyThere(name);
    else favs.removeString(name);
    getFavoritesFile().replaceWithText(favs.joinIntoString("\n"));
}

// ------------------------------- persistence --------------------------------------------------
juce::String ConstellationXVocalAudioProcessor::stateToString(const cx::ParamState& s)
{
    juce::StringArray parts;
    for (float v : s) parts.add(juce::String((double) v, 6));
    return parts.joinIntoString(",");
}

bool ConstellationXVocalAudioProcessor::stringToState(const juce::String& text, cx::ParamState& out)
{
    juce::StringArray parts;
    parts.addTokens(text, ",", "");
    if (parts.size() != cx::kNumSoundParams) return false;
    for (int i = 0; i < cx::kNumSoundParams; ++i)
        out[(size_t) i] = cx::clampParam(i, parts[i].getFloatValue());
    return true;
}

void ConstellationXVocalAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
    {
        slots[activeSlot] = getState();
        auto* extra = xml->createNewChildElement("CX");
        extra->setAttribute("preset", presetName);
        extra->setAttribute("slot", activeSlot);
        extra->setAttribute("validA", slotValid[0] ? 1 : 0);
        extra->setAttribute("validB", slotValid[1] ? 1 : 0);
        extra->setAttribute("a", stateToString(slots[0]));
        extra->setAttribute("b", stateToString(slots[1]));
        copyXmlToBinary(*xml, destData);
    }
}

void ConstellationXVocalAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml == nullptr || !xml->hasTagName(apvts.state.getType()))
        return;

    if (auto* extra = xml->getChildByName("CX"))
    {
        presetName = extra->getStringAttribute("preset", "INIT");
        activeSlot = juce::jlimit(0, 1, extra->getIntAttribute("slot", 0));
        cx::ParamState a, b;
        if (stringToState(extra->getStringAttribute("a"), a)) slots[0] = a;
        if (stringToState(extra->getStringAttribute("b"), b)) slots[1] = b;
        slotValid[0] = extra->getIntAttribute("validA", 1) != 0;
        slotValid[1] = extra->getIntAttribute("validB", 0) != 0;
        xml->removeChildElement(extra, true);
    }
    apvts.replaceState(juce::ValueTree::fromXml(*xml));
    history.reset(getState());
}

juce::AudioProcessorEditor* ConstellationXVocalAudioProcessor::createEditor()
{
    return new ConstellationXVocalAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ConstellationXVocalAudioProcessor();
}
