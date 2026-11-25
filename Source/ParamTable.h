#pragma once
// ConstellationX Vocal - parameter table, factory presets and undo/redo history.
// Pure C++17 (no JUCE) so it is unit-tested in CI. Single source of truth for parameter ranges.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace cx
{
struct ParamInfo
{
    const char* id;
    const char* name;
    float minV, maxV, def;
};

enum ParamIndex { kDepth = 0, kSize, kDecay, kReflections, kWidth, kWarmth, kAir, kMix, kOutput, kNumSoundParams };

inline constexpr ParamInfo kParams[kNumSoundParams] = {
    { "depth",       "Depth",       -1.0f,  1.0f, 0.0f  },
    { "size",        "Size",         0.0f,  1.0f, 0.5f  },
    { "decay",       "Decay",        0.1f,  8.0f, 2.0f  },
    { "reflections", "Reflections",  0.0f,  1.0f, 0.60f },
    { "width",       "Width",        0.0f,  1.0f, 0.75f },
    { "warmth",      "Warmth",       0.0f,  1.0f, 0.45f },
    { "air",         "Air",          0.0f,  1.0f, 0.70f },
    { "mix",         "Wet Dry",      0.0f,  1.0f, 0.40f },
    { "output",      "Output",     -24.0f, 12.0f, 0.0f  },
};

// Sound parameters in real units (seconds, dB, 0..1 ...), in ParamIndex order.
using ParamState = std::array<float, kNumSoundParams>;

inline ParamState defaultState()
{
    ParamState s {};
    for (int i = 0; i < kNumSoundParams; ++i) s[(size_t) i] = kParams[i].def;
    return s;
}

inline float clampParam(int index, float v)
{
    return std::min(std::max(v, kParams[index].minV), kParams[index].maxV);
}

struct FactoryPreset
{
    const char* name;
    float v[kNumSoundParams];   // depth, size, decay, reflections, width, warmth, air, mix, output
};

inline constexpr FactoryPreset kFactoryPresets[] = {
    { "INIT",              { 0.0f,  0.50f, 2.0f, 0.60f, 0.75f, 0.45f, 0.70f, 0.40f,  0.0f } },
    { "Close Intimate",    { -0.7f, 0.20f, 0.6f, 0.30f, 0.60f, 0.50f, 0.60f, 0.25f,  0.0f } },
    { "Studio Room",       { -0.2f, 0.35f, 1.1f, 0.60f, 0.70f, 0.40f, 0.50f, 0.30f,  0.0f } },
    { "Lead Forward",      { -0.5f, 0.40f, 1.6f, 0.50f, 0.80f, 0.50f, 0.75f, 0.28f,  0.0f } },
    { "Stage Hall",        { 0.3f,  0.70f, 2.8f, 0.55f, 0.85f, 0.35f, 0.60f, 0.38f, -1.0f } },
    { "Dark Plate",        { 0.4f,  0.55f, 2.2f, 0.25f, 0.90f, 0.70f, 0.15f, 0.35f,  0.0f } },
    { "Airy Shimmer",      { 0.2f,  0.60f, 3.2f, 0.50f, 0.95f, 0.20f, 0.95f, 0.40f, -1.0f } },
    { "Cathedral Back",    { 0.8f,  1.00f, 6.5f, 0.45f, 1.00f, 0.30f, 0.45f, 0.50f, -2.0f } },
    { "Distant Whisper",   { 0.9f,  0.80f, 4.5f, 0.40f, 0.80f, 0.60f, 0.35f, 0.55f, -2.0f } },
    { "Deep Space",        { 1.0f,  1.00f, 8.0f, 0.30f, 1.00f, 0.50f, 0.30f, 0.60f, -3.0f } },
};
inline constexpr int kNumFactoryPresets = (int) (sizeof(kFactoryPresets) / sizeof(kFactoryPresets[0]));

inline ParamState factoryState(int index)
{
    ParamState s {};
    const auto& p = kFactoryPresets[std::min(std::max(index, 0), kNumFactoryPresets - 1)];
    for (int i = 0; i < kNumSoundParams; ++i) s[(size_t) i] = p.v[i];
    return s;
}

// Linear undo/redo history of sound-parameter snapshots.
// commit() ignores a state equal to the current one, so spurious change notifications are harmless.
class ParamHistory
{
public:
    explicit ParamHistory(size_t maxEntries = 100) : limit(std::max<size_t>(2, maxEntries)) {}

    void reset(const ParamState& s) { entries.assign(1, s); pos = 0; }

    bool commit(const ParamState& s)
    {
        if (entries.empty()) { reset(s); return true; }
        if (same(entries[pos], s)) return false;
        entries.resize(pos + 1);          // drop the redo branch
        entries.push_back(s);
        ++pos;
        if (entries.size() > limit)
        {
            entries.erase(entries.begin());
            --pos;
        }
        return true;
    }

    bool canUndo() const { return pos > 0; }
    bool canRedo() const { return pos + 1 < entries.size(); }

    bool undo(ParamState& out)
    {
        if (!canUndo()) return false;
        --pos;
        out = entries[pos];
        return true;
    }
    bool redo(ParamState& out)
    {
        if (!canRedo()) return false;
        ++pos;
        out = entries[pos];
        return true;
    }

    size_t size() const { return entries.size(); }

    static bool same(const ParamState& a, const ParamState& b)
    {
        for (size_t i = 0; i < a.size(); ++i)
            if (std::fabs(a[i] - b[i]) > 1.0e-4f) return false;
        return true;
    }

private:
    std::vector<ParamState> entries;
    size_t pos = 0, limit;
};
} // namespace cx
