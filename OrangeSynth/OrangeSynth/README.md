# Orange Synth by Robby B

A simple subtractive synth (VST3) with 200 presets for UK garage / bass house / breaks style
production: Bass, Keys, Leads, Pads, Plucks, Stabs and FX.

Two oscillators (saw / square / triangle / sine) with unison, sub and noise, a resonant
low / band / high-pass filter with drive, amp and filter ADSRs, an LFO (to cutoff and pitch),
and glide. 16-voice polyphony.

## Get the plugin without compiling anything

1. Make a free account at github.com and create a **new repository** (private is fine).
2. Upload every file in this folder to it, keeping the folders. The easiest way is
   **Add file -> Upload files**, then drag the *contents* of the unzipped folder in.
   Make sure the hidden `.github/workflows/build.yml` file goes up too. If it doesn't
   (Windows can hide dot-folders), use **Add file -> Create new file**, type
   `.github/workflows/build.yml` as the name, and paste the file's contents in.
3. Open the **Actions** tab. The "Build Orange Synth" run starts by itself (about 10 minutes
   the first time, since it downloads the JUCE framework). Wait for the green tick.
4. Click the finished run, scroll to **Artifacts**, and download **Orange-Synth-VST3-Windows**.
5. Unzip it and follow `INSTALL.txt`.

## Changing presets

`tools/gen_presets.py` generates `Source/Presets.h` (the parameter table and all 200 presets).
Edit the archetypes/styles in it, run `python3 tools/gen_presets.py`, and commit.

## Layout

- `Source/Voice.h` - oscillators, filter, envelopes (one synth voice)
- `Source/PluginProcessor.*` - parameters, MIDI, voice allocation, presets/programs
- `Source/PluginEditor.*` - the orange UI
- `tools/render_test.cpp` - offline audio test that renders every preset (no JUCE needed)
