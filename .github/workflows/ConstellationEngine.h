#pragma once
// ConstellationX Vocal - spatial depth engine.
// Pure C++17, no JUCE dependency, so it can be unit-tested on any compiler.
//
// Signal flow (per channel pair):
//   input --> [distance shaping: HF absorption / forward presence] --> direct
//   input --> high-pass --> early reflections (6 taps / channel, size-scaled)
//   input --> high-pass --> pre-delay --> input diffusion --> 8-line FDN (late tail)
//   wet = direct*directGain + ER*erLevel + late*lateLevel
//   wet --> warmth (low shelf) --> air (high shelf) --> warmth saturation --> width (M/S)
//   out = ((1-mix)*dry + mix*wet) * outputGain
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace cx
{
constexpr double kPi = 3.14159265358979323846;

struct EngineParams
{
    float depth       = 0.0f;   // -1 = FORWARD, +1 = BACK
    float size        = 0.5f;   // 0..1  (NEAR..FAR) scales reflection/tail delay lengths
    float decay       = 2.0f;   // RT60 in seconds, 0.1..8
    float reflections = 0.6f;   // 0..1  early reflection level
    float width       = 0.5f;   // 0..1  wet side gain = width*2 (0 = mono, 0.5 = unity)
    float warmth      = 0.0f;   // 0..1  low shelf + soft saturation
    float air         = 0.0f;   // 0..1  high shelf lift
    float mix         = 0.4f;   // 0..1  wet/dry
    float outputDb    = 0.0f;   // output trim in dB
};

struct Meters
{
    float lowL = 0, lowR = 0;   // envelope of the sub band (<~150 Hz) of the input, per side
    float inLevel = 0;          // envelope of the full-band input
    float outL = 0, outR = 0;   // envelope of the output
};

// ---------------------------------------------------------------------------
class OnePoleSmoother
{
public:
    void prepare(double sampleRate, double ms)
    {
        coeff = 1.0f - std::exp(-1.0f / std::max(1.0f, (float) (0.001 * ms * sampleRate)));
    }
    void reset(float v) { cur = target = v; }
    void setTarget(float t) { target = t; }
    float next() { cur += coeff * (target - cur); return cur; }
private:
    float coeff = 0.01f, cur = 0.0f, target = 0.0f;
};

// ---------------------------------------------------------------------------
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;

    void reset() { z1 = z2 = 0.0; }

    float process(float x)
    {
        const double xd = x;
        const double y = b0 * xd + z1;
        z1 = b1 * xd - a1 * y + z2;
        z2 = b2 * xd - a2 * y;
        return (float) y;
    }

    void set(double nb0, double nb1, double nb2, double a0, double na1, double na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0;
        a1 = na1 / a0; a2 = na2 / a0;
    }

    static double clampFreq(double sr, double f) { return std::min(std::max(f, 10.0), sr * 0.45); }

    void highPass(double sr, double f, double q = 0.7071)
    {
        const double w = 2.0 * kPi * clampFreq(sr, f) / sr, c = std::cos(w), al = std::sin(w) / (2.0 * q);
        set((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void lowPass(double sr, double f, double q = 0.7071)
    {
        const double w = 2.0 * kPi * clampFreq(sr, f) / sr, c = std::cos(w), al = std::sin(w) / (2.0 * q);
        set((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void lowShelf(double sr, double f, double gainDb, double q = 0.7071)
    {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * clampFreq(sr, f) / sr, c = std::cos(w), al = std::sin(w) / (2.0 * q);
        const double t = 2.0 * std::sqrt(A) * al;
        set(A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t),
            (A + 1) + (A - 1) * c + t, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
    }
    void highShelf(double sr, double f, double gainDb, double q = 0.7071)
    {
        const double A = std::pow(10.0, gainDb / 40.0);
        const double w = 2.0 * kPi * clampFreq(sr, f) / sr, c = std::cos(w), al = std::sin(w) / (2.0 * q);
        const double t = 2.0 * std::sqrt(A) * al;
        set(A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t),
            (A + 1) - (A - 1) * c + t, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
    }
};

// ---------------------------------------------------------------------------
// Protocol: read(d) first (d >= 1 sample), then push(x).  read(d) returns x[n-d]
// where x[n] is the sample about to be pushed.
class DelayLine
{
public:
    void init(int maxSamples)
    {
        int n = 4;
        while (n < maxSamples + 4) n <<= 1;
        buf.assign((size_t) n, 0.0f);
        mask = n - 1;
        w = 0;
    }
    void clear() { std::fill(buf.begin(), buf.end(), 0.0f); w = 0; }
    void push(float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    float read(float d) const
    {
        d = std::max(1.0f, std::min(d, (float) mask - 2.0f));
        const int i = (int) d;
        const float f = d - (float) i;
        const float a = buf[(size_t) ((w - i) & mask)];
        const float b = buf[(size_t) ((w - i - 1) & mask)];
        return a + (b - a) * f;
    }
private:
    std::vector<float> buf;
    int mask = 3, w = 0;
};

// Schroeder allpass with a fixed delay (integer samples).
class Allpass
{
public:
    void init(int delaySamples, float gain) { d = std::max(1, delaySamples); g = gain; line.init(d + 2); }
    void clear() { line.clear(); }
    float process(float x)
    {
        const float delayed = line.read((float) d);
        const float v = x + g * delayed;
        line.push(v);
        return -g * v + delayed;
    }
private:
    DelayLine line;
    int d = 1;
    float g = 0.6f;
};

// ---------------------------------------------------------------------------
class Engine
{
public:
    static constexpr int kTaps = 6;
    static constexpr int kLines = 8;

    void prepare(double sampleRate)
    {
        sr = sampleRate > 8000.0 ? sampleRate : 44100.0;

        const int maxEr = (int) (0.30 * sr);
        for (int c = 0; c < 2; ++c)
        {
            erLine[c].init(maxEr);
            preDelay[c].init((int) (0.25 * sr));
            ap[c][0].init((int) (0.0051 * sr), 0.62f);
            ap[c][1].init((int) (0.0077 * sr), 0.58f);
        }
        for (int i = 0; i < kLines; ++i)
            fdn[i].init((int) (0.30 * sr));

        for (auto* s : { &sDepth, &sSize, &sDecay, &sRefl, &sWidth, &sWarmth, &sAir, &sMix, &sOut })
            s->prepare(sr, 40.0);
        sSize.prepare(sr, 120.0);   // slower: size changes move delay taps
        sDecay.prepare(sr, 80.0);

        lowA = 1.0f - std::exp((float) (-2.0 * kPi * 150.0 / sr));
        envAtk = 1.0f - std::exp((float) (-1.0 / (0.004 * sr)));
        envRel = 1.0f - std::exp((float) (-1.0 / (0.140 * sr)));

        modDepth = (float) (0.0003 * sr);   // ~0.3 ms of slow delay modulation to avoid metallic ringing
        for (int i = 0; i < kLines; ++i)
        {
            lfoPhase[i] = (float) i / (float) kLines;
            lfoInc[i] = (float) ((0.11 + 0.037 * i) / sr);   // 0.11 .. 0.37 Hz
        }

        setParams(params, true);
        reset();
    }

    void reset()
    {
        for (int c = 0; c < 2; ++c)
        {
            erLine[c].clear(); preDelay[c].clear();
            ap[c][0].clear(); ap[c][1].clear();
            inHp[c].reset(); distLp[c].reset(); fwdShelf[c].reset();
            warmShelf[c].reset(); airShelf[c].reset();
            erDamp[c] = 0.0f; lowState[c] = 0.0f;
        }
        for (int i = 0; i < kLines; ++i) { fdn[i].clear(); damp[i] = 0.0f; }
        meters = {};
        ctrlCounter = 0;
        updateControl();
    }

    // immediate = true snaps the smoothers (use on prepare / state load).
    void setParams(const EngineParams& p, bool immediate = false)
    {
        params = p;
        const float outGain = std::pow(10.0f, p.outputDb / 20.0f);
        sDepth.setTarget(clamp(p.depth, -1.0f, 1.0f));
        sSize.setTarget(clamp(p.size, 0.0f, 1.0f));
        sDecay.setTarget(clamp(p.decay, 0.1f, 8.0f));
        sRefl.setTarget(clamp(p.reflections, 0.0f, 1.0f));
        sWidth.setTarget(clamp(p.width, 0.0f, 1.0f));
        sWarmth.setTarget(clamp(p.warmth, 0.0f, 1.0f));
        sAir.setTarget(clamp(p.air, 0.0f, 1.0f));
        sMix.setTarget(clamp(p.mix, 0.0f, 1.0f));
        sOut.setTarget(outGain);
        if (immediate)
        {
            sDepth.reset(clamp(p.depth, -1.0f, 1.0f));
            sSize.reset(clamp(p.size, 0.0f, 1.0f));
            sDecay.reset(clamp(p.decay, 0.1f, 8.0f));
            sRefl.reset(clamp(p.reflections, 0.0f, 1.0f));
            sWidth.reset(clamp(p.width, 0.0f, 1.0f));
            sWarmth.reset(clamp(p.warmth, 0.0f, 1.0f));
            sAir.reset(clamp(p.air, 0.0f, 1.0f));
            sMix.reset(clamp(p.mix, 0.0f, 1.0f));
            sOut.reset(outGain);
        }
    }

    Meters getMeters() const { return meters; }

    // In-place. right == nullptr means a mono bus (wet is summed to mono).
    void process(float* left, float* right, int numSamples)
    {
        const bool stereo = (right != nullptr);
        static const float erTimesL[kTaps] = { 7.3f, 11.9f, 17.3f, 23.1f, 31.7f, 41.3f };
        static const float erTimesR[kTaps] = { 9.1f, 13.7f, 19.9f, 27.3f, 35.9f, 47.1f };
        static const float erGains[kTaps]  = { 0.80f, -0.65f, 0.55f, -0.45f, 0.38f, -0.30f };

        for (int n = 0; n < numSamples; ++n)
        {
            const float depth = sDepth.next(), size = sSize.next(), decay = sDecay.next();
            const float refl = sRefl.next(), width = sWidth.next(), warmth = sWarmth.next();
            const float air = sAir.next(), mix = sMix.next(), outGain = sOut.next();
            cDepth = depth; cWarmth = warmth; cAir = air; cDecay = decay; cSize = size;

            if ((ctrlCounter++ & 31) == 0)
                updateControl();

            const float inL = left[n];
            const float inR = stereo ? right[n] : inL;
            const float in[2] = { inL, inR };

            // ---- meters (input side)
            for (int c = 0; c < 2; ++c)
            {
                lowState[c] += lowA * (in[c] - lowState[c]);
                const float a = std::fabs(lowState[c]);
                lowEnv[c] += (a > lowEnv[c] ? envAtk : envRel) * (a - lowEnv[c]);
            }
            {
                const float a = std::max(std::fabs(inL), std::fabs(inR));
                inEnv += (a > inEnv ? envAtk : envRel) * (a - inEnv);
            }

            const float back = std::max(0.0f, depth);
            const float fwd  = std::max(0.0f, -depth);

            // ---- distance-shaped direct path
            float direct[2];
            for (int c = 0; c < 2; ++c)
            {
                float d = distLp[c].process(in[c]);
                d = fwdShelf[c].process(d);
                direct[c] = d * directGain;
            }

            // ---- reverb send (high-passed)
            float rin[2] = { inHp[0].process(inL), inHp[1].process(inR) };

            // early reflections
            const float erScale = 0.5f + 1.5f * size;       // 0.5x .. 2x
            float er[2] = { 0.0f, 0.0f };
            for (int k = 0; k < kTaps; ++k)
            {
                er[0] += erGains[k] * erLine[0].read(erTimesL[k] * 0.001f * (float) sr * erScale);
                er[1] += erGains[k] * erLine[1].read(erTimesR[k] * 0.001f * (float) sr * erScale);
            }
            for (int c = 0; c < 2; ++c)
            {
                erLine[c].push(rin[c]);
                erDamp[c] = erDampA * erDamp[c] + (1.0f - erDampA) * er[c];
                er[c] = erDamp[c];
            }

            // pre-delay + diffusion
            const float pdSamples = (0.004f + 0.030f * size) * (1.0f - 0.5f * back) * (float) sr;
            float diff[2];
            for (int c = 0; c < 2; ++c)
            {
                const float p = preDelay[c].read(pdSamples);
                preDelay[c].push(rin[c]);
                diff[c] = ap[c][1].process(ap[c][0].process(p));
            }

            // FDN
            float rd[kLines], x[kLines];
            const float lenScale = 0.6f + 0.9f * size;
            for (int i = 0; i < kLines; ++i)
            {
                lfoPhase[i] += lfoInc[i];
                if (lfoPhase[i] >= 1.0f) lfoPhase[i] -= 1.0f;
                const float mod = std::sin(2.0f * (float) kPi * lfoPhase[i]) * modDepth;
                const float len = baseDelaySec[i] * lenScale * (float) sr + mod;
                const float r = fdn[i].read(len);
                damp[i] = dampA * damp[i] + (1.0f - dampA) * r;
                rd[i] = damp[i];
                const float lineSec = baseDelaySec[i] * lenScale;
                const float g = std::pow(10.0f, -3.0f * lineSec / (decay * kDecayComp));
                x[i] = rd[i] * g;
            }
            hadamard8(x);
            for (int i = 0; i < kLines; ++i)
            {
                const float sgn = (i & 3) < 2 ? 1.0f : -1.0f;
                fdn[i].push(x[i] + sgn * ((i & 1) ? diff[1] : diff[0]));
            }
            const float lateL = 0.5f * (rd[0] - rd[2] + rd[4] - rd[6]);
            const float lateR = 0.5f * (rd[1] - rd[3] + rd[5] - rd[7]);

            // late-level normalisation: steady-state energy ~ RT60 / mean delay
            const float meanDelay = meanBaseSec * lenScale;
            const float lateNorm = kLateTrim / std::sqrt(std::max(0.05f, decay) / meanDelay);
            const float lateLevel = (0.30f + 0.70f * back) * (1.0f - 0.5f * fwd);
            const float erLevel = refl * (0.5f + 0.5f * back) * (1.0f - 0.4f * fwd);

            float wet[2] = { direct[0] + er[0] * erLevel + lateL * lateLevel * lateNorm,
                             direct[1] + er[1] * erLevel + lateR * lateLevel * lateNorm };

            // ---- tone, saturation, width
            const float satK = 1.0f + warmth * 1.5f;
            for (int c = 0; c < 2; ++c)
            {
                float v = airShelf[c].process(warmShelf[c].process(wet[c]));
                v = std::tanh(v * satK) / satK;
                wet[c] = v;
            }
            const float mid = 0.5f * (wet[0] + wet[1]);
            const float side = 0.5f * (wet[0] - wet[1]) * (width * 2.0f);
            wet[0] = mid + side;
            wet[1] = mid - side;

            // ---- mix / output
            float out[2];
            if (stereo)
            {
                out[0] = ((1.0f - mix) * inL + mix * wet[0]) * outGain;
                out[1] = ((1.0f - mix) * inR + mix * wet[1]) * outGain;
                left[n] = out[0];
                right[n] = out[1];
            }
            else
            {
                const float wm = 0.5f * (wet[0] + wet[1]);
                out[0] = out[1] = ((1.0f - mix) * inL + mix * wm) * outGain;
                left[n] = out[0];
            }

            for (int c = 0; c < 2; ++c)
            {
                const float a = std::fabs(out[c]);
                outEnv[c] += (a > outEnv[c] ? envAtk : envRel) * (a - outEnv[c]);
            }
        }

        meters.lowL = lowEnv[0]; meters.lowR = lowEnv[1];
        meters.inLevel = inEnv;
        meters.outL = outEnv[0]; meters.outR = outEnv[1];
    }

private:
    static float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

    static void hadamard8(float* x)
    {
        for (int len = 1; len < 8; len <<= 1)
            for (int i = 0; i < 8; i += len << 1)
                for (int j = i; j < i + len; ++j)
                {
                    const float a = x[j], b = x[j + len];
                    x[j] = a + b;
                    x[j + len] = a - b;
                }
        for (int i = 0; i < 8; ++i) x[i] *= 0.35355339f;
    }

    // Control-rate update (every 32 samples) of filters and derived constants.
    void updateControl()
    {
        const float depth = cDepth, warmth = cWarmth, air = cAir, size = cSize;
        const float back = std::max(0.0f, depth);
        const float fwd  = std::max(0.0f, -depth);

        directGain = 1.0f - 0.5f * back + 0.18f * fwd;
        const double lpHz = 20000.0 * std::pow(4500.0 / 20000.0, (double) back);   // distance HF absorption
        for (int c = 0; c < 2; ++c)
        {
            inHp[c].highPass(sr, 40.0);
            distLp[c].lowPass(sr, lpHz);
            fwdShelf[c].highShelf(sr, 3000.0, 3.0 * fwd);
            warmShelf[c].lowShelf(sr, 180.0, 5.0 * warmth);
            airShelf[c].highShelf(sr, 11000.0, 8.0 * air);
        }

        const float dampHz = std::max(2500.0f, 10000.0f - 4500.0f * back - 2000.0f * size);
        dampA = std::exp((float) (-2.0 * kPi * dampHz / sr));
        erDampA = std::exp((float) (-2.0 * kPi * 7000.0 / sr));
    }

    double sr = 44100.0;
    EngineParams params;
    OnePoleSmoother sDepth, sSize, sDecay, sRefl, sWidth, sWarmth, sAir, sMix, sOut;
    float cDepth = 0, cWarmth = 0, cAir = 0, cDecay = 2, cSize = 0.5f;

    DelayLine erLine[2], preDelay[2], fdn[kLines];
    Allpass ap[2][2];
    Biquad inHp[2], distLp[2], fwdShelf[2], warmShelf[2], airShelf[2];
    float erDamp[2] = { 0, 0 }, damp[kLines] = {};
    float dampA = 0.5f, erDampA = 0.5f, directGain = 1.0f;

    // FDN line lengths in seconds (mutually non-harmonic), mean = 0.0537 s
    const float baseDelaySec[kLines] = { 0.0297f, 0.0371f, 0.0411f, 0.0437f, 0.0530f, 0.0599f, 0.0673f, 0.0797f };
    const float meanBaseSec = 0.0539f;
    float lfoInc[kLines] = {};
    float lfoPhase[kLines] = {};
    float modDepth = 0.0f;
    // Calibrated in tests/engine_test.cpp (late-tail RMS ~ input RMS at lateLevel = 1).
    static constexpr float kLateTrim = 0.20f;
    // The in-loop damping filter shortens the measured RT60 by ~15%; compensate so the Decay knob reads true.
    static constexpr float kDecayComp = 1.17f;

    float lowA = 0.01f, envAtk = 0.01f, envRel = 0.001f;
    float lowState[2] = { 0, 0 }, lowEnv[2] = { 0, 0 }, inEnv = 0.0f, outEnv[2] = { 0, 0 };
    int ctrlCounter = 0;
    Meters meters;
};
} // namespace cx
