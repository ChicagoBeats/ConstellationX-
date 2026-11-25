#include "../Source/ParamTable.h"

#include <cstdio>
#include <cstring>
#include <set>
#include <string>

static int failures = 0;
#define CHECK(cond, ...)                                                    \
    do {                                                                    \
        if (!(cond)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } \
        else { std::printf("  ok:   "); std::printf(__VA_ARGS__); std::printf("\n"); } \
    } while (0)

// Called from engine_test.cpp's main() so CI runs everything through the one existing test target.
int runPresetTests()
{
    std::printf("== 1. Parameter table\n");
    {
        std::set<std::string> ids;
        bool ok = true;
        for (int i = 0; i < cx::kNumSoundParams; ++i)
        {
            ids.insert(cx::kParams[i].id);
            ok = ok && cx::kParams[i].minV < cx::kParams[i].maxV && cx::kParams[i].def >= cx::kParams[i].minV
                 && cx::kParams[i].def <= cx::kParams[i].maxV;
        }
        CHECK(ok, "every parameter has min < max and default inside range");
        CHECK((int) ids.size() == cx::kNumSoundParams, "all %d parameter ids are unique", cx::kNumSoundParams);
    }

    std::printf("== 2. Factory presets\n");
    {
        bool inRange = true; std::set<std::string> names;
        for (int p = 0; p < cx::kNumFactoryPresets; ++p)
        {
            names.insert(cx::kFactoryPresets[p].name);
            for (int i = 0; i < cx::kNumSoundParams; ++i)
            {
                const float v = cx::kFactoryPresets[p].v[i];
                if (v < cx::kParams[i].minV || v > cx::kParams[i].maxV)
                {
                    inRange = false;
                    std::printf("      preset '%s' param '%s' = %.3f out of range\n", cx::kFactoryPresets[p].name, cx::kParams[i].id, v);
                }
            }
            // output param has 0.1 dB step in the plugin; keep presets on the grid
            const float o = cx::kFactoryPresets[p].v[cx::kOutput] * 10.0f;
            if (std::fabs(o - std::round(o)) > 1e-4f) { inRange = false; std::printf("      '%s' output off 0.1 dB grid\n", cx::kFactoryPresets[p].name); }
        }
        CHECK(inRange, "all %d factory presets are inside parameter ranges", cx::kNumFactoryPresets);
        CHECK((int) names.size() == cx::kNumFactoryPresets, "factory preset names are unique");
        CHECK(std::strcmp(cx::kFactoryPresets[0].name, "INIT") == 0 &&
              cx::ParamHistory::same(cx::factoryState(0), cx::defaultState()), "preset 0 is INIT and equals the parameter defaults");
    }

    std::printf("== 3. Undo / redo history\n");
    {
        cx::ParamHistory h(5);
        auto s0 = cx::defaultState(); h.reset(s0);
        CHECK(!h.canUndo() && !h.canRedo(), "fresh history: nothing to undo/redo");

        auto s1 = s0; s1[cx::kMix] = 0.9f;
        auto s2 = s1; s2[cx::kDecay] = 5.0f;
        CHECK(h.commit(s1) && h.commit(s2), "two changes committed");
        CHECK(!h.commit(s2), "committing an identical state is ignored (spurious notifications are harmless)");
        CHECK(h.canUndo() && !h.canRedo(), "can undo, cannot redo");

        cx::ParamState out;
        CHECK(h.undo(out) && cx::ParamHistory::same(out, s1), "undo returns previous state");
        CHECK(h.undo(out) && cx::ParamHistory::same(out, s0), "undo again returns the first state");
        CHECK(!h.undo(out), "undo at start fails");
        CHECK(h.redo(out) && cx::ParamHistory::same(out, s1), "redo restores the state");

        auto s3 = s1; s3[cx::kAir] = 0.1f;
        h.commit(s3);
        CHECK(!h.canRedo(), "a new change after undo clears the redo branch");

        cx::ParamHistory small(3);
        small.reset(s0);
        for (int i = 1; i <= 10; ++i) { auto s = s0; s[cx::kSize] = 0.05f * (float) i; small.commit(s); }
        CHECK(small.size() <= 3, "history is bounded (size %zu <= 3)", small.size());
        int undos = 0; while (small.undo(out)) ++undos;
        CHECK(undos == 2, "bounded history allows %d undo steps", undos);
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
