#!/usr/bin/env python3
"""Generates Source/Presets.h for Orange Synth (parameter table + 200 presets).

Run from the project root:  python3 tools/gen_presets.py
"""
import random
import sys
from pathlib import Path

# kind: 0 = float, 1 = wave choice, 2 = filter type choice, 3 = integer
# centre: if > 0, slider is skewed so this value sits mid-way (Hz / seconds)
# (id, name, min, max, default, step, centre, group, kind)
PARAMS = [
    ("osc1Wave", "Osc1 Wave", 0, 3, 0, 1, 0, 0, 1),
    ("osc1Oct", "Osc1 Oct", -2, 2, 0, 1, 0, 0, 3),
    ("osc2Wave", "Osc2 Wave", 0, 3, 0, 1, 0, 0, 1),
    ("osc2Semi", "Osc2 Semi", -24, 24, 0, 1, 0, 0, 3),
    ("osc2Fine", "Osc2 Fine", -50, 50, 7, 0.1, 0, 0, 0),
    ("oscMix", "Osc2 Mix", 0, 1, 0, 0.001, 0, 0, 0),
    ("sub", "Sub", 0, 1, 0, 0.001, 0, 0, 0),
    ("noise", "Noise", 0, 1, 0, 0.001, 0, 0, 0),
    ("unison", "Unison", 1, 5, 1, 1, 0, 0, 3),
    ("uniDetune", "Uni Detune", 0, 50, 10, 0.1, 0, 0, 0),
    ("fltType", "Type", 0, 2, 0, 1, 0, 1, 2),
    ("cutoff", "Cutoff", 20, 20000, 4000, 1, 1000, 1, 0),
    ("res", "Reso", 0, 1, 0.15, 0.001, 0, 1, 0),
    ("fltEnv", "Env Amt", -1, 1, 0, 0.001, 0, 1, 0),
    ("keyTrack", "Key Track", 0, 1, 0.3, 0.001, 0, 1, 0),
    ("drive", "Drive", 0, 1, 0, 0.001, 0, 1, 0),
    ("aA", "Attack", 0.001, 5, 0.005, 0.001, 0.3, 2, 0),
    ("aD", "Decay", 0.005, 5, 0.3, 0.001, 0.4, 2, 0),
    ("aS", "Sustain", 0, 1, 0.8, 0.001, 0, 2, 0),
    ("aR", "Release", 0.005, 5, 0.2, 0.001, 0.4, 2, 0),
    ("fA", "Attack", 0.001, 5, 0.005, 0.001, 0.3, 3, 0),
    ("fD", "Decay", 0.005, 5, 0.3, 0.001, 0.4, 3, 0),
    ("fS", "Sustain", 0, 1, 0.3, 0.001, 0, 3, 0),
    ("fR", "Release", 0.005, 5, 0.2, 0.001, 0.4, 3, 0),
    ("lfoRate", "LFO Rate", 0.05, 20, 4, 0.01, 2, 4, 0),
    ("lfoDepth", "LFO>Cutoff", 0, 1, 0, 0.001, 0, 4, 0),
    ("lfoPitch", "LFO>Pitch", 0, 1, 0, 0.001, 0, 4, 0),
    ("glide", "Glide", 0, 0.5, 0, 0.001, 0, 4, 0),
    ("volume", "Volume", 0, 1, 0.7, 0.001, 0, 5, 0),
]
GROUPS = ["OSCILLATORS", "FILTER", "AMP ENVELOPE", "FILTER ENVELOPE", "MODULATION", "MASTER"]
IDS = [p[0] for p in PARAMS]
LIM = {p[0]: (p[2], p[3], p[5]) for p in PARAMS}
DEFAULT = {p[0]: p[4] for p in PARAMS}

# ---- style flavours (applied on top of an archetype) -------------------------
# keys: cutoff*, res+, drive+, env*, decay*, detune*, noise+, sub+, attack*
STYLES = {
    "Warehouse": dict(cutoff=0.6, res=0.10, drive=0.30, env=1.2, decay=1.0, detune=1.0, noise=0.02, sub=0.10, attack=1.0),
    "Garage":    dict(cutoff=1.0, res=0.05, drive=0.05, env=1.0, decay=0.7, detune=0.8, noise=0.00, sub=0.05, attack=1.0),
    "Breaks":    dict(cutoff=0.85, res=0.12, drive=0.25, env=1.1, decay=0.6, detune=1.3, noise=0.06, sub=0.00, attack=0.8),
    "Rave":      dict(cutoff=1.5, res=0.08, drive=0.10, env=1.3, decay=0.9, detune=1.6, noise=0.00, sub=0.00, attack=1.0),
    "Sunrise":   dict(cutoff=1.2, res=0.00, drive=0.00, env=0.8, decay=1.4, detune=0.7, noise=0.00, sub=0.00, attack=1.6),
    "Dub":       dict(cutoff=0.5, res=0.05, drive=0.15, env=0.9, decay=1.3, detune=0.6, noise=0.00, sub=0.20, attack=1.2),
}

def A(**kw):
    return kw

ARCH = {
    "Bass": {
        "Reese":  A(osc1Wave=0, osc2Wave=0, osc2Fine=14, oscMix=0.7, unison=3, uniDetune=12, cutoff=700, res=0.2, fltEnv=0.15, aD=0.5, aS=0.9, aR=0.15, fD=0.4, fS=0.4, drive=0.2, sub=0.3),
        "Sub":    A(osc1Wave=3, oscMix=0, sub=0.5, cutoff=600, res=0.0, aD=0.4, aS=1.0, aR=0.12, fltEnv=0.0),
        "Acid":   A(osc1Wave=0, cutoff=350, res=0.7, fltEnv=0.65, fD=0.18, fS=0.0, aD=0.25, aS=0.8, aR=0.08, glide=0.06, drive=0.2),
        "Wobble": A(osc1Wave=0, osc2Wave=1, oscMix=0.5, osc2Semi=-12, cutoff=900, res=0.35, lfoRate=2.5, lfoDepth=0.7, sub=0.3, aS=1.0, aR=0.1),
        "Square": A(osc1Wave=1, oscMix=0.3, osc2Wave=1, osc2Semi=-12, cutoff=900, res=0.25, fltEnv=0.4, fD=0.2, fS=0.1, aD=0.3, aS=0.6, aR=0.1),
        "Growl":  A(osc1Wave=0, osc2Wave=1, osc2Semi=7, oscMix=0.5, unison=2, uniDetune=18, cutoff=500, res=0.5, fltEnv=0.6, fD=0.3, fS=0.2, drive=0.65, lfoRate=6, lfoDepth=0.25),
        "Rubber": A(osc1Wave=0, osc2Wave=1, oscMix=0.6, cutoff=650, res=0.3, fltEnv=0.35, fD=0.12, fS=0.2, glide=0.09, aD=0.2, aS=0.7, aR=0.1),
        "Punch":  A(osc1Wave=2, osc2Wave=0, osc2Semi=-12, oscMix=0.4, sub=0.4, cutoff=800, res=0.15, fltEnv=0.5, fD=0.1, fS=0.0, aD=0.18, aS=0.5, aR=0.08, drive=0.25),
    },
    "Keys": {
        "Rhodes": A(osc1Wave=3, osc2Wave=2, osc2Semi=12, oscMix=0.35, cutoff=3200, res=0.05, fltEnv=0.25, fD=0.6, fS=0.2, aD=1.2, aS=0.35, aR=0.4, lfoRate=4.5, lfoPitch=0.03),
        "Organ":  A(osc1Wave=1, osc2Wave=0, osc2Semi=12, oscMix=0.45, sub=0.3, cutoff=5000, res=0.05, aD=0.05, aS=1.0, aR=0.08, lfoRate=6, lfoPitch=0.05),
        "Piano":  A(osc1Wave=2, osc2Wave=0, osc2Semi=0, osc2Fine=5, oscMix=0.35, cutoff=3500, res=0.05, fltEnv=0.4, fD=0.5, fS=0.15, aD=1.6, aS=0.2, aR=0.3),
        "HousePiano": A(osc1Wave=0, osc2Wave=1, oscMix=0.5, cutoff=2400, res=0.1, fltEnv=0.45, fD=0.3, fS=0.1, aD=0.45, aS=0.25, aR=0.2, unison=2, uniDetune=8),
        "Wurly":  A(osc1Wave=2, osc2Wave=1, oscMix=0.3, cutoff=2200, res=0.15, fltEnv=0.3, fD=0.4, fS=0.2, aD=0.9, aS=0.4, aR=0.25, drive=0.2),
        "Glass":  A(osc1Wave=3, osc2Wave=3, osc2Semi=19, oscMix=0.3, cutoff=7000, res=0.0, aD=1.4, aS=0.3, aR=0.6, lfoRate=5, lfoPitch=0.02),
    },
    "Leads": {
        "Supersaw": A(osc1Wave=0, osc2Wave=0, oscMix=0.8, unison=5, uniDetune=24, cutoff=6000, res=0.1, aS=0.9, aR=0.25, keyTrack=0.6),
        "Square":   A(osc1Wave=1, osc2Wave=1, osc2Semi=12, oscMix=0.25, cutoff=4500, res=0.2, glide=0.07, aS=0.9, aR=0.15, lfoRate=5.5, lfoPitch=0.08),
        "Hoover":   A(osc1Wave=0, osc2Wave=0, osc2Fine=-35, oscMix=1.0, unison=4, uniDetune=40, cutoff=3500, res=0.25, glide=0.12, lfoRate=3, lfoPitch=0.25, aS=0.9, drive=0.3),
        "Pluck":    A(osc1Wave=0, osc2Wave=1, oscMix=0.4, cutoff=1500, res=0.3, fltEnv=0.6, fD=0.25, fS=0.1, aD=0.5, aS=0.3, aR=0.2, unison=2, uniDetune=10),
        "Whistle":  A(osc1Wave=3, osc2Wave=2, osc2Semi=12, oscMix=0.2, cutoff=9000, res=0.0, glide=0.1, lfoRate=5.5, lfoPitch=0.12, aA=0.03, aS=1.0, aR=0.2),
        "Screamer": A(osc1Wave=0, osc2Wave=1, osc2Semi=7, oscMix=0.4, cutoff=2200, res=0.55, fltEnv=0.3, fD=0.4, fS=0.5, drive=0.6, glide=0.04, aS=0.9),
    },
    "Pads": {
        "Warm":    A(osc1Wave=0, osc2Wave=0, oscMix=0.7, unison=3, uniDetune=14, cutoff=1800, res=0.1, fltEnv=0.2, fA=0.8, fD=1.5, fS=0.6, aA=0.9, aD=1.0, aS=0.9, aR=1.6),
        "Glass":   A(osc1Wave=2, osc2Wave=3, osc2Semi=12, oscMix=0.5, cutoff=6500, res=0.05, aA=1.0, aD=1.0, aS=0.9, aR=1.8, lfoRate=0.4, lfoDepth=0.15),
        "Choir":   A(osc1Wave=0, osc2Wave=2, oscMix=0.5, fltType=1, cutoff=1200, res=0.45, unison=3, uniDetune=10, aA=0.7, aS=0.9, aR=1.4, lfoRate=5, lfoPitch=0.04),
        "Dark":    A(osc1Wave=0, osc2Wave=1, osc2Semi=-12, oscMix=0.5, unison=3, uniDetune=18, cutoff=700, res=0.2, fltEnv=0.15, fA=1.5, fD=2.0, fS=0.5, aA=1.3, aS=0.9, aR=2.2, sub=0.2),
        "Evolving": A(osc1Wave=0, osc2Wave=2, oscMix=0.6, unison=4, uniDetune=20, cutoff=1500, res=0.3, lfoRate=0.25, lfoDepth=0.5, aA=1.5, aS=0.9, aR=2.5),
        "Strings": A(osc1Wave=0, osc2Wave=0, osc2Fine=10, oscMix=0.8, unison=4, uniDetune=16, cutoff=3200, res=0.05, aA=0.6, aS=0.95, aR=1.0, lfoRate=5.2, lfoPitch=0.03),
    },
    "Plucks": {
        "Marimba": A(osc1Wave=3, osc2Wave=3, osc2Semi=24, oscMix=0.25, cutoff=4000, res=0.1, fltEnv=0.3, fD=0.15, fS=0.0, aD=0.35, aS=0.0, aR=0.2),
        "SawPluck": A(osc1Wave=0, osc2Wave=0, oscMix=0.5, unison=2, uniDetune=10, cutoff=1200, res=0.25, fltEnv=0.7, fD=0.2, fS=0.0, aD=0.4, aS=0.0, aR=0.2),
        "Bell":    A(osc1Wave=3, osc2Wave=3, osc2Semi=19, osc2Fine=4, oscMix=0.45, cutoff=9000, res=0.0, aD=1.4, aS=0.0, aR=0.8),
        "Kalimba": A(osc1Wave=2, osc2Wave=3, osc2Semi=12, oscMix=0.4, cutoff=3000, res=0.1, fltEnv=0.2, fD=0.2, fS=0.0, aD=0.7, aS=0.0, aR=0.5),
        "Harp":    A(osc1Wave=2, osc2Wave=0, oscMix=0.25, cutoff=2800, res=0.1, fltEnv=0.4, fD=0.3, fS=0.0, aD=0.9, aS=0.0, aR=0.6),
    },
    "Stabs": {
        "RaveStab": A(osc1Wave=0, osc2Wave=0, oscMix=0.9, unison=5, uniDetune=30, cutoff=2500, res=0.2, fltEnv=0.5, fD=0.12, fS=0.1, aD=0.18, aS=0.0, aR=0.1),
        "ChordStab": A(osc1Wave=0, osc2Wave=1, oscMix=0.5, unison=3, uniDetune=14, cutoff=1800, res=0.15, fltEnv=0.5, fD=0.15, fS=0.1, aD=0.22, aS=0.0, aR=0.12),
        "Brass":   A(osc1Wave=0, osc2Wave=0, osc2Fine=9, oscMix=0.8, unison=2, uniDetune=10, cutoff=900, res=0.15, fltEnv=0.6, fA=0.03, fD=0.3, fS=0.3, aA=0.02, aD=0.35, aS=0.4, aR=0.15),
        "OrganStab": A(osc1Wave=1, osc2Wave=0, osc2Semi=12, oscMix=0.5, sub=0.2, cutoff=4500, res=0.05, aD=0.18, aS=0.0, aR=0.08),
        "Hit":     A(osc1Wave=0, osc2Wave=1, osc2Semi=7, oscMix=0.6, noise=0.1, cutoff=2000, res=0.25, fltEnv=0.6, fD=0.08, fS=0.0, aD=0.12, aS=0.0, aR=0.06, drive=0.3),
    },
    "FX": {
        "Riser":   A(osc1Wave=0, osc2Wave=0, osc2Fine=30, oscMix=0.8, unison=4, uniDetune=30, noise=0.25, cutoff=300, res=0.4, fltEnv=1.0, fA=4.0, fD=0.5, fS=1.0, aA=3.5, aD=0.5, aS=1.0, aR=0.4, lfoPitch=0.1),
        "Sweep":   A(osc1Wave=0, noise=0.9, oscMix=0, fltType=1, cutoff=600, res=0.7, fltEnv=0.9, fA=2.0, fD=2.0, fS=0.2, aA=0.5, aD=1.5, aS=0.5, aR=1.5, lfoRate=0.3, lfoDepth=0.3),
        "Zap":     A(osc1Wave=1, osc2Wave=0, oscMix=0.5, cutoff=3000, res=0.5, lfoRate=14, lfoPitch=0.9, aD=0.3, aS=0.0, aR=0.1, fltEnv=-0.5, fD=0.2, fS=0.0),
        "Impact":  A(osc1Wave=3, osc1Oct=-2, noise=0.7, sub=0.6, cutoff=400, res=0.1, fltEnv=0.4, fD=0.5, fS=0.0, aD=0.9, aS=0.0, aR=0.6, drive=0.4),
    },
}
COUNTS = {"Bass": 40, "Keys": 30, "Leads": 30, "Pads": 30, "Plucks": 25, "Stabs": 25, "FX": 20}


def clamp(pid, v):
    lo, hi, step = LIM[pid]
    v = max(lo, min(hi, v))
    if step >= 1:
        v = round(v)
    return float(v)


def make(cat, arch_name, style_name, rng):
    p = dict(DEFAULT)
    p.update(ARCH[cat][arch_name])
    s = STYLES[style_name]
    j = lambda spread: 1.0 + rng.uniform(-spread, spread)
    p["cutoff"] = p["cutoff"] * s["cutoff"] * j(0.12)
    p["res"] += s["res"] + rng.uniform(-0.03, 0.03)
    p["drive"] += s["drive"]
    p["fltEnv"] *= s["env"] * j(0.08)
    for k in ("aD", "fD"):
        p[k] *= s["decay"] * j(0.1)
    p["aA"] *= s["attack"]
    p["fA"] *= s["attack"]
    p["uniDetune"] *= s["detune"] * j(0.1)
    if p["unison"] == 1 and style_name in ("Rave", "Breaks") and cat in ("Leads", "Pads", "Stabs"):
        p["unison"] = 2
    p["noise"] += s["noise"]
    p["sub"] += s["sub"] if cat in ("Bass",) else 0
    p["volume"] = 0.7
    return {k: clamp(k, v) for k, v in p.items()}


def build():
    rng = random.Random(2026)
    out = []
    for cat, n in COUNTS.items():
        archs = list(ARCH[cat].keys())
        styles = list(STYLES.keys()) if cat == "Bass" else list(STYLES.keys())[:5]
        combos = [(a, s) for s in styles for a in archs]
        rng.shuffle(combos)
        combos = sorted(combos[:n], key=lambda c: (archs.index(c[0]), styles.index(c[1])))
        for a, s in combos:
            nice = {"HousePiano": "House Piano", "SawPluck": "Saw Pluck", "RaveStab": "Rave Stab",
                    "ChordStab": "Chord Stab", "OrganStab": "Organ Stab"}.get(a, a)
            out.append((cat, f"{s} {nice}", make(cat, a, s, rng)))
    return out


def fmt(x):
    return repr(round(float(x), 4)) + "f"


def main():
    presets = build()
    assert len(presets) == 200, len(presets)
    names = [(c, n) for c, n, _ in presets]
    assert len(set(names)) == len(names), "duplicate preset names"

    L = ["// GENERATED by tools/gen_presets.py - do not edit by hand.", "#pragma once", "namespace Orange {", ""]
    L.append("enum ParamIndex {")
    L.append("    " + ", ".join("p" + i[0].upper() + i[1:] for i in IDS) + ", pCount")
    L.append("};\n")
    L.append("struct ParamDef { const char* id; const char* name; float minV, maxV, defV, step, centre; int group; int kind; };")
    L.append("static const ParamDef kParams[pCount] = {")
    for p in PARAMS:
        L.append('    {"%s", "%s", %s, %s, %s, %s, %s, %d, %d},' % (p[0], p[1], fmt(p[2]), fmt(p[3]), fmt(p[4]), fmt(p[5]), fmt(p[6]), p[7], p[8]))
    L.append("};\n")
    L.append("static const char* const kGroups[] = {" + ", ".join('"%s"' % g for g in GROUPS) + "};")
    L.append("static const int kNumGroups = %d;" % len(GROUPS))
    cats = list(COUNTS.keys())
    L.append("static const char* const kCategories[] = {" + ", ".join('"%s"' % c for c in cats) + "};")
    L.append("static const int kNumCategories = %d;\n" % len(cats))
    L.append("struct Preset { const char* category; const char* name; float v[pCount]; };")
    L.append("static const int kNumPresets = %d;" % len(presets))
    L.append("static const Preset kPresets[kNumPresets] = {")
    for c, n, p in presets:
        L.append('    {"%s", "%s", {%s}},' % (c, n, ", ".join(fmt(p[i]) for i in IDS)))
    L.append("};\n")
    L.append("} // namespace Orange")

    root = Path(__file__).resolve().parent.parent
    (root / "Source").mkdir(exist_ok=True)
    (root / "Source" / "Presets.h").write_text("\n".join(L) + "\n")
    counts = {}
    for c, _, _ in presets:
        counts[c] = counts.get(c, 0) + 1
    print("presets:", len(presets), counts)


if __name__ == "__main__":
    sys.exit(main())
