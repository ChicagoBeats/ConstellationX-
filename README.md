# ConstellationX Vocal — JUCE VST3 starter project

This project is an original JUCE-based **audio effect** starter for ConstellationX Vocal.
It follows the general CMake/JUCE project workflow shown in the supplied UniverseX files,
but it is not a copy of UniverseX's synthesizer code.

## Status (Phase 2)

Implemented and wired to real audio processing (`Source/ConstellationEngine.h`, JUCE-free):

- **Depth (Forward / Back)** - distance gain, HF absorption, forward presence, reverb send, pre-delay.
- **Size** - scales early-reflection taps, FDN line lengths and pre-delay.
- **Decay** - real RT60 (0.1-8 s) of an 8-line feedback delay network (Hadamard matrix, in-loop damping, modulation).
- **Reflections** - 6-tap stereo early-reflection network.
- **Width** - mid/side on the wet path (0 = mono, 50 % = unity, 100 % = 2x side).
- **Warmth** - low shelf + soft saturation. **Air** - high shelf.
- **Wet/Dry**, **Output**, per-parameter smoothing, mono and stereo buses.
- **Randomizer** - controlled musical ranges; never touches Output, VFX Motion or Skin.
- **Skin selector** - five skins (Crimson default) stored in plugin state, recolour the whole UI.
- **VFX Motion** - animates the vortex; the vortex expands/contracts with Depth and the live input level.
- **Dual subwoofers** - cone excursion follows the real sub-band (<150 Hz) level of the left/right input.
- **Resizing** - fixed 16:9 aspect, everything laid out in reference-design coordinates and scaled.

Not implemented yet (deliberately not drawn as dead buttons): preset browser, favorites, A/B, undo/redo,
parameter locks. The header bar of the reference design depends on these.

## CI

`.github/workflows/main.yml` runs on every push and pull request to `main` (and manually): configure, build and
run the DSP engine tests, build VST3 + Standalone (Windows x64, Release), verify outputs, upload artifacts.

Run the engine tests locally on any compiler:

```
g++ -std=c++17 -O2 -Wall -Wextra -o engine_test tests/engine_test.cpp && ./engine_test
```

## Build on Windows

1. Install Visual Studio 2019 or 2022 with **Desktop development with C++**.
2. Install CMake and Git.
3. Put JUCE 6.1.6 in this folder so `JUCE/CMakeLists.txt` exists, or allow CMake
   FetchContent to retrieve the pinned JUCE release.
4. Open a Developer PowerShell in this folder and run:

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

For Visual Studio 2019, use `-G "Visual Studio 16 2019"`.

The VST3 should be generated under:

`build/ConstellationXVocal_artefacts/Release/VST3/ConstellationX Vocal.vst3`

Copy the bundle to:

`C:\Program Files\Common Files\VST3`

Then rescan plugins in your DAW.

## Next development phases

1. Fix whatever the first Windows CI run reports (JUCE glue code has not been compiled yet).
2. Preset system: browser, favorites, A/B, undo/redo, parameter locks.
3. Visual fidelity pass against the approved reference image (textures, glow, bezels, header bar).
4. Listening tests on real vocals; tune level/EQ ranges; latency/CPU checks in FL Studio and other hosts.

## Branding

The CMake manufacturer code `ChBt` and plugin code `CxV1` are provisional identifiers.
Confirm uniqueness before public distribution. Do not change plugin codes after release
without considering host preset and plugin identity compatibility.
