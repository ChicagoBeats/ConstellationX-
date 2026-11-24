#include "../Source/ConstellationEngine.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <vector>

static int failures = 0;
#define CHECK(cond, ...)                                                    \
    do {                                                                    \
        if (!(cond)) { ++failures; std::printf("  FAIL: "); std::printf(__VA_ARGS__); std::printf("\n"); } \
        else { std::printf("  ok:   "); std::printf(__VA_ARGS__); std::printf("\n"); } \
    } while (0)

static const double SR = 48000.0;

static std::vector<float> noise(int n, float amp, unsigned seed)
{
    std::mt19937 g(seed);
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    std::vector<float> v((size_t) n);
    for (auto& x : v) x = amp * d(g);
    return v;
}

static double rms(const std::vector<float>& v, size_t a, size_t b)
{
    double s = 0; for (size_t i = a; i < b; ++i) s += (double) v[i] * v[i];
    return std::sqrt(s / (double) (b - a));
}

static cx::EngineParams base()
{
    cx::EngineParams p;
    p.depth = 0; p.size = 0.5f; p.decay = 2.0f; p.reflections = 0.6f; p.width = 0.5f;
    p.warmth = 0; p.air = 0; p.mix = 1.0f; p.outputDb = 0;
    return p;
}

// Schroeder-integral RT60 estimate (T20 extrapolated) of the tail starting at tStart seconds.
static double measureRT60(const std::vector<float>& ir, double tStart)
{
    size_t s = (size_t) (tStart * SR);
    std::vector<double> edc(ir.size() - s);
    double acc = 0;
    for (size_t i = ir.size(); i-- > s;) { acc += (double) ir[i] * ir[i]; edc[i - s] = acc; }
    const double e0 = edc[0];
    double t5 = -1, t25 = -1;
    for (size_t i = 0; i < edc.size(); ++i)
    {
        const double db = 10.0 * std::log10(edc[i] / e0 + 1e-30);
        if (t5 < 0 && db <= -5.0) t5 = (double) i / SR;
        if (t25 < 0 && db <= -25.0) { t25 = (double) i / SR; break; }
    }
    if (t5 < 0 || t25 < 0) return -1;
    return 3.0 * (t25 - t5);
}

static std::vector<float> impulseResponse(const cx::EngineParams& p, double seconds, bool wetOnly)
{
    cx::Engine e; e.setParams(p, true); e.prepare(SR);
    const int n = (int) (seconds * SR);
    std::vector<float> l((size_t) n, 0.0f), r((size_t) n, 0.0f);
    l[0] = 1.0f; r[0] = 1.0f;
    e.process(l.data(), r.data(), n);
    (void) wetOnly;
    return l;
}

static bool allFinite(const std::vector<float>& v)
{
    for (float x : v) if (!std::isfinite(x)) return false;
    return true;
}

int main()
{
    std::printf("== 1. Stability at extreme settings\n");
    {
        cx::Engine e;
        cx::EngineParams p = base();
        p.decay = 8; p.size = 1; p.depth = 1; p.warmth = 1; p.air = 1; p.mix = 1; p.outputDb = 12; p.reflections = 1; p.width = 1;
        e.setParams(p, true); e.prepare(SR);
        auto l = noise((int) (SR * 10), 0.9f, 1), r = noise((int) (SR * 10), 0.9f, 2);
        e.process(l.data(), r.data(), (int) l.size());
        double peak = 0; for (float x : l) peak = std::max(peak, (double) std::fabs(x));
        CHECK(allFinite(l) && allFinite(r), "output finite for 10 s of loud noise");
        CHECK(peak < 40.0, "peak %.2f stays bounded (<40)", peak);
        for (float depth : { -1.0f, 1.0f })
        {
            p.depth = depth; p.decay = 8; e.setParams(p);
            auto a = noise(48000, 0.9f, 3), b = noise(48000, 0.9f, 4);
            e.process(a.data(), b.data(), 48000);
            CHECK(allFinite(a) && allFinite(b), "finite after depth jump to %+.0f", depth);
        }
    }

    std::printf("== 2. Silence in, silence out; tail dies away\n");
    {
        auto ir = impulseResponse(base(), 12.0, true);
        const double early = rms(ir, (size_t) (0.5 * SR), (size_t) (1.0 * SR));
        const double late = rms(ir, (size_t) (10.0 * SR), (size_t) (12.0 * SR));
        CHECK(late < early * 0.001, "decay=2s: tail at 10-12 s is %.1f dB below 0.5-1 s", 20 * std::log10(early / (late + 1e-30)));
    }

    std::printf("== 3. mix = 0 is transparent\n");
    {
        cx::Engine e; cx::EngineParams p = base(); p.mix = 0; p.depth = 0.7f; p.warmth = 1; p.air = 1;
        e.setParams(p, true); e.prepare(SR);
        auto l = noise(48000, 0.5f, 5), r = noise(48000, 0.5f, 6);
        auto l0 = l, r0 = r;
        e.process(l.data(), r.data(), 48000);
        double md = 0; for (size_t i = 0; i < l.size(); ++i) md = std::max({ md, (double) std::fabs(l[i] - l0[i]), (double) std::fabs(r[i] - r0[i]) });
        CHECK(md < 1e-6, "max |out-in| = %.2e at mix 0, output 0 dB", md);
    }

    std::printf("== 4. Decay control sets RT60\n");
    for (float d : { 1.0f, 2.0f, 4.0f })
    {
        cx::EngineParams p = base(); p.decay = d; p.reflections = 0; p.size = 0.5f;
        auto ir = impulseResponse(p, d * 2.0 + 3.0, true);
        const double rt = measureRT60(ir, 0.15);
        CHECK(rt > 0 && rt > d * 0.6 && rt < d * 1.6, "decay %.1f s -> measured RT60 %.2f s (accept 0.6x..1.6x)", d, rt);
    }

    std::printf("== 5. Late level is ~constant across decay times (calibration)\n");
    {
        double lv[3]; int k = 0;
        for (float d : { 0.5f, 2.0f, 8.0f })
        {
            cx::Engine e; cx::EngineParams p = base(); p.decay = d; p.reflections = 0; p.depth = 1.0f;
            e.setParams(p, true); e.prepare(SR);
            auto l = noise((int) (SR * 20), 0.3f, 7), r = noise((int) (SR * 20), 0.3f, 8);
            auto in = l;
            e.process(l.data(), r.data(), (int) l.size());
            lv[k] = rms(l, (size_t) (SR * 12), l.size()) / rms(in, (size_t) (SR * 12), in.size());
            std::printf("      decay %.1f: wet/in RMS ratio %.3f\n", d, lv[k]); ++k;
        }
        const double mx = std::max({ lv[0], lv[1], lv[2] }), mn = std::min({ lv[0], lv[1], lv[2] });
        CHECK(mx / mn < 1.8, "level spread across decay 0.5..8 s is %.2fx (<1.8x)", mx / mn);
    }

    std::printf("== 6. Depth: FORWARD is drier/brighter, BACK is wetter/duller\n");
    {
        auto measure = [](float depth, double& directHf, double& tailEnergy)
        {
            cx::EngineParams p = base(); p.depth = depth;
            auto ir = impulseResponse(p, 3.0, true);
            double hf = 0; for (size_t i = 1; i < (size_t) (0.002 * SR); ++i) { double d = ir[i] - ir[i - 1]; hf += d * d; }
            directHf = hf;
            double e = 0; for (size_t i = (size_t) (0.2 * SR); i < (size_t) (1.5 * SR); ++i) e += (double) ir[i] * ir[i];
            tailEnergy = e;
        };
        double hfF, tF, hfB, tB;
        measure(-1.0f, hfF, tF); measure(1.0f, hfB, tB);
        CHECK(tB > tF * 1.5 && tF > 1e-9, "tail energy back %.3e vs forward %.3e", tB, tF);
        CHECK(hfB < hfF * 0.5, "direct HF energy back %.4f vs forward %.4f", hfB, hfF);
    }

    std::printf("== 7. Width 0 collapses wet to mono; width 1 widens\n");
    {
        auto corr = [](float width)
        {
            cx::Engine e; cx::EngineParams p = base(); p.width = width;
            e.setParams(p, true); e.prepare(SR);
            auto l = noise(96000, 0.4f, 11), r = noise(96000, 0.4f, 12);
            e.process(l.data(), r.data(), 96000);
            double sd = 0, sl = 0, sr_ = 0;
            for (size_t i = 0; i < l.size(); ++i) { sd += (double) l[i] * r[i]; sl += (double) l[i] * l[i]; sr_ += (double) r[i] * r[i]; }
            return sd / std::sqrt(sl * sr_ + 1e-30);
        };
        const double c0 = corr(0.0f), c1 = corr(1.0f);
        CHECK(c0 > 0.999, "width 0: L/R correlation %.4f (should be ~1)", c0);
        CHECK(c1 < c0 - 0.2, "width 1: L/R correlation %.4f is lower than width 0", c1);
    }

    std::printf("== 8. Mono path\n");
    {
        cx::Engine e; cx::EngineParams p = base(); p.mix = 0.5f;
        e.setParams(p, true); e.prepare(SR);
        auto l = noise(48000, 0.5f, 13);
        e.process(l.data(), nullptr, 48000);
        CHECK(allFinite(l) && rms(l, 0, l.size()) > 0.05, "mono output finite and non-silent (rms %.3f)", rms(l, 0, l.size()));
    }

    std::printf("== 9. Meters respond (drive the visuals)\n");
    {
        auto run = [](double hz, double& low, double& full)
        {
            cx::Engine e; e.setParams(base(), true); e.prepare(SR);
            std::vector<float> l(48000), r(48000);
            for (size_t i = 0; i < l.size(); ++i) l[i] = r[i] = 0.5f * (float) std::sin(2 * cx::kPi * hz * (double) i / SR);
            e.process(l.data(), r.data(), (int) l.size());
            auto m = e.getMeters(); low = m.lowL; full = m.inLevel;
        };
        double lowA, fullA, lowB, fullB;
        run(60.0, lowA, fullA); run(4000.0, lowB, fullB);
        CHECK(lowA > 0.2 && lowB < 0.02, "sub-band meter: 60 Hz %.3f vs 4 kHz %.3f", lowA, lowB);
        CHECK(fullA > 0.3 && fullB > 0.3, "full-band meter: %.3f / %.3f", fullA, fullB);
    }

    std::printf("== 10. Parameter smoothing (no zipper jump on output change)\n");
    {
        cx::Engine e; cx::EngineParams p = base(); p.mix = 0; e.setParams(p, true); e.prepare(SR);
        std::vector<float> l(4800), r(4800);
        for (size_t i = 0; i < l.size(); ++i) l[i] = r[i] = 0.5f * (float) std::sin(2 * cx::kPi * 100.0 * (double) i / SR);
        e.process(l.data(), r.data(), (int) l.size());
        p.outputDb = -24.0f; e.setParams(p);
        for (size_t i = 0; i < l.size(); ++i) l[i] = r[i] = 0.5f * (float) std::sin(2 * cx::kPi * 100.0 * (double) (i + 4800) / SR);
        e.process(l.data(), r.data(), (int) l.size());
        double maxJump = 0; for (size_t i = 1; i < l.size(); ++i) maxJump = std::max(maxJump, (double) std::fabs(l[i] - l[i - 1]));
        CHECK(maxJump < 0.05, "max sample-to-sample step during -24 dB change = %.4f", maxJump);
        CHECK(std::fabs(l[l.size() - 1]) < 0.5f * 0.2f, "gain actually reduced after change");
    }

    std::printf("\n%s (%d failure%s)\n", failures ? "TESTS FAILED" : "ALL TESTS PASSED", failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
