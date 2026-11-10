#pragma once
#include <cmath>
#include <algorithm>
#include "Presets.h"

namespace Orange
{
// Snapshot of all parameters for one audio block.
struct Patch
{
    float v[pCount] {};
    float operator[] (int i) const noexcept { return v[i]; }
};

static inline float polyBlep (float t, float dt) noexcept
{
    if (t < dt)         { t /= dt;           return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt)  { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

static inline float oscSample (int wave, float t, float dt) noexcept
{
    switch (wave)
    {
        case 0:  return 2.0f * t - 1.0f - polyBlep (t, dt);                       // saw
        case 1:  { float t2 = t + 0.5f; if (t2 >= 1.0f) t2 -= 1.0f;               // square
                   return (t < 0.5f ? 1.0f : -1.0f) + polyBlep (t, dt) - polyBlep (t2, dt); }
        case 2:  return 4.0f * std::fabs (t - 0.5f) - 1.0f;                       // triangle
        default: return std::sin (6.2831853f * t);                                // sine
    }
}

struct Envelope
{
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float level = 0.0f;

    void gate (bool on) noexcept
    {
        if (on) stage = Attack;
        else if (stage != Idle) stage = Release;
    }
    void reset() noexcept { stage = Idle; level = 0.0f; }
    bool active() const noexcept { return stage != Idle; }

    float next (float a, float d, float s, float r, float sr) noexcept
    {
        switch (stage)
        {
            case Attack:
                level += 1.0f / (std::max (a, 0.001f) * sr);
                if (level >= 1.0f) { level = 1.0f; stage = Decay; }
                break;
            case Decay:
                level += (s - level) * (1.0f - std::exp (-4.6f / (std::max (d, 0.005f) * sr)));
                if (std::fabs (level - s) < 0.0005f) { level = s; stage = Sustain; }
                break;
            case Sustain:
                level = s;
                break;
            case Release:
                level -= level * (1.0f - std::exp (-4.6f / (std::max (r, 0.005f) * sr)));
                if (level < 0.0002f) { level = 0.0f; stage = Idle; }
                break;
            default: break;
        }
        return level;
    }
};

struct Voice
{
    static constexpr int maxUni = 5;

    bool  held = false;
    int   note = -1;
    float velocity = 0.0f;
    float targetFreq = 440.0f, freq = 440.0f;
    float ph1[maxUni] {}, ph2[maxUni] {}, phSub = 0.0f;
    float ic1 = 0.0f, ic2 = 0.0f;
    unsigned int rng = 22222u;
    unsigned long long age = 0;
    Envelope ampEnv, fltEnv;

    bool active() const noexcept { return ampEnv.active(); }

    void start (int n, float vel, float startFreq, bool resetGlide, unsigned long long a) noexcept
    {
        note = n; velocity = vel; held = true; age = a;
        targetFreq = 440.0f * std::pow (2.0f, ((float) n - 69.0f) / 12.0f);
        if (resetGlide || ! active()) freq = targetFreq; else freq = startFreq;
        if (! active())
        {
            for (int i = 0; i < maxUni; ++i)
            {
                ph1[i] = (float) i / (float) maxUni;
                ph2[i] = (float) (maxUni - i) / (float) maxUni * 0.5f;
            }
            ic1 = ic2 = 0.0f;
        }
        ampEnv.gate (true);
        fltEnv.gate (true);
    }

    void release() noexcept { held = false; ampEnv.gate (false); fltEnv.gate (false); }
    void kill() noexcept { held = false; ampEnv.reset(); fltEnv.reset(); note = -1; }

    float render (const Patch& p, float sr, float lfo, float bendSemis) noexcept
    {
        // glide
        float g = p[pGlide];
        if (g > 0.0005f) freq += (targetFreq - freq) * (1.0f - std::exp (-1.0f / (g * 0.35f * sr)));
        else freq = targetFreq;

        const float vib = std::pow (2.0f, (lfo * p[pLfoPitch] + bendSemis) / 12.0f);
        const float f0  = freq * vib;

        const int   uni  = std::min (maxUni, std::max (1, (int) (p[pUnison] + 0.5f)));
        const int   wave1 = (int) (p[pOsc1Wave] + 0.5f);
        const int   wave2 = (int) (p[pOsc2Wave] + 0.5f);
        const float oct1 = std::pow (2.0f, p[pOsc1Oct]);
        const float semi = std::pow (2.0f, p[pOsc2Semi] / 12.0f + p[pOsc2Fine] / 1200.0f);
        const float mix2 = p[pOscMix];
        const float uniScale = 1.0f / std::sqrt ((float) uni);

        float o1 = 0.0f, o2 = 0.0f;
        for (int i = 0; i < uni; ++i)
        {
            float pos = (uni > 1) ? (2.0f * (float) i / (float) (uni - 1) - 1.0f) : 0.0f;
            float det = std::pow (2.0f, pos * p[pUniDetune] / 1200.0f);

            float fr1 = std::min (f0 * oct1 * det, sr * 0.45f);
            float dt1 = fr1 / sr;
            o1 += oscSample (wave1, ph1[i], dt1);
            ph1[i] += dt1; if (ph1[i] >= 1.0f) ph1[i] -= 1.0f;

            float fr2 = std::min (f0 * oct1 * semi * det, sr * 0.45f);
            float dt2 = fr2 / sr;
            o2 += oscSample (wave2, ph2[i], dt2);
            ph2[i] += dt2; if (ph2[i] >= 1.0f) ph2[i] -= 1.0f;
        }
        o1 *= uniScale; o2 *= uniScale;

        float x = o1 * (1.0f - 0.5f * mix2) + o2 * mix2;

        float dtS = std::min (f0 * 0.5f, sr * 0.45f) / sr;
        x += std::sin (6.2831853f * phSub) * p[pSub];
        phSub += dtS; if (phSub >= 1.0f) phSub -= 1.0f;

        rng = rng * 1664525u + 1013904223u;
        x += ((float) (rng >> 8) / 8388608.0f - 1.0f) * p[pNoise];

        // envelopes
        const float ae = ampEnv.next (p[pAA], p[pAD], p[pAS], p[pAR], sr);
        const float fe = fltEnv.next (p[pFA], p[pFD], p[pFS], p[pFR], sr);

        // filter: cutoff modulated in octaves
        float keyOct = p[pKeyTrack] * std::log2 (std::max (targetFreq, 20.0f) / 261.63f);
        float oct = p[pFltEnv] * fe * 5.0f + lfo * p[pLfoDepth] * 3.0f + keyOct;
        float fc = std::min (std::max (p[pCutoff] * std::pow (2.0f, oct), 20.0f), sr * 0.45f);

        if (p[pDrive] > 0.001f)
        {
            float dg = 1.0f + p[pDrive] * 6.0f;
            x = std::tanh (x * dg) / std::tanh (dg * 0.7f) * 0.7f;
        }

        const float gc = std::tan (3.14159265f * fc / sr);
        const float k  = 2.0f - 1.95f * p[pRes];
        const float a1 = 1.0f / (1.0f + gc * (gc + k));
        const float a2 = gc * a1, a3 = gc * a2;
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + a2 * v3;
        const float v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;

        float y;
        switch ((int) (p[pFltType] + 0.5f))
        {
            case 1:  y = v1 * k * 2.0f; break;                  // band-pass (unity peak, +6 dB)
            case 2:  y = x - k * v1 - v2; break;                // high-pass
            default: y = v2; break;                             // low-pass
        }

        return y * ae * (0.4f + 0.6f * velocity) * 0.35f;
    }
};
} // namespace Orange
