// Offline sanity test: renders every preset through the real Voice code (no JUCE needed).
// Build:  g++ -O2 -std=c++17 -I Source tools/render_test.cpp -o /tmp/render_test
#include <cstdio>
#include <cstring>
#include <cmath>
#include <string>
#include "Voice.h"

using namespace Orange;

static int noteFor (const std::string& cat)
{
    if (cat == "Bass") return 36;      // C2
    if (cat == "Leads" || cat == "Plucks") return 72;
    if (cat == "FX") return 48;
    return 60;
}

int main()
{
    const float sr = 48000.0f;
    int problems = 0;
    std::printf("%-8s %-22s %8s %8s %8s %8s\n", "cat", "name", "peak", "rms dB", "tail", "dc");

    for (int i = 0; i < kNumPresets; ++i)
    {
        const Preset& pr = kPresets[i];
        Patch p;
        for (int k = 0; k < pCount; ++k) p.v[k] = pr.v[k];

        Voice v;
        const bool fx = std::strcmp (pr.category, "FX") == 0;
        const float holdSec = fx ? 4.5f : 1.2f, relSec = 1.5f;
        v.start (noteFor (pr.category), 0.9f, 440.0f, true, 1);

        double sumSq = 0.0, sum = 0.0; float peak = 0.0f, tail = 0.0f;
        bool bad = false;
        const int nHold = (int) (holdSec * sr), nRel = (int) (relSec * sr);
        float lfoPhase = 0.0f;
        for (int n = 0; n < nHold + nRel; ++n)
        {
            if (n == nHold) v.release();
            lfoPhase += p[pLfoRate] / sr; if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
            float s = v.active() ? v.render (p, sr, std::sin (6.2831853f * lfoPhase), 0.0f) : 0.0f;
            float out = std::tanh (s * p[pVolume] * 2.0f);   // keep in sync with PluginProcessor::renderRange
            if (! std::isfinite (out)) { bad = true; break; }
            if (n < nHold) { sumSq += out * out; sum += out; peak = std::max (peak, std::fabs (out)); }
            if (n >= nHold + nRel - 480) tail = std::max (tail, std::fabs (out));
        }
        const double rms = std::sqrt (sumSq / nHold);
        const double db = 20.0 * std::log10 (rms + 1e-9);
        const double dc = sum / nHold;
        std::string flag;
        if (bad) flag += " NaN!";
        if (peak > 0.97f) flag += " HOT";
        if (db < -32.0 && ! fx) flag += " QUIET";
        if (db < -45.0) flag += " SILENT";
        if (tail > 0.001f) flag += " NOTAIL";
        if (std::fabs (dc) > 0.05) flag += " DC";
        if (! flag.empty()) ++problems;
        std::printf("%-8s %-22s %8.3f %8.1f %8.4f %8.3f%s\n", pr.category, pr.name, peak, db, tail, dc, flag.c_str());
    }
    std::printf("\n%d presets, %d flagged\n", kNumPresets, problems);
    return 0;
}
