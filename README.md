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

## Phase 3 additions

- **Presets** - 10 factory presets (`Source/ParamTable.h`) plus user presets saved as `.cxpreset` files in
  `%APPDATA%\Chicago Beats Audio Labs\ConstellationX Vocal\Presets`. Click the preset name or PRESETS for the menu;
  `<` / `>` step through all presets; SAVE stores the current sound as "User Preset N".
- **Favorites** - FAVORITES menu lists favorited presets and adds/removes the current one.
- **A/B** - A and B slots (header buttons and the A/B button); first visit to B starts from A. Saved in plugin state.
- **Undo / Redo** - history of sound-parameter edits (knobs, randomizer, presets, A/B), 100 steps.
- **Resize grip** - red diagonal grip at the bottom-right, fixed 16:9.
- **Vortex** - Forward contracts it, Back expands it (eased), with breathing and live input level on top.

## Phase 4 - visual pass (chrome / carbon fiber / neon)

All procedural, no image assets, so it scales cleanly and builds without extra files:

- Carbon-fiber twill weave background, vignette and red atmosphere glow behind the vortex.
- Polished chrome frame and chamfered chrome-edged panels with red neon inner lines and red floor glow.
- Chamfered octagonal speaker housings with chrome ring, bolts, hex mesh, segmented neon that pulses with the
  sub-band level, glossy rubber surround, grooved cone and silver crown dust cap.
- Chrome-bezel knobs with glossy dome, neon value arc and pointer; chrome Forward/Back slider with crown thumb;
  glossy chrome-edged buttons; chrome vortex window frame; chrome title text with neon X.
- Static artwork is cached in two image layers (rebuilt on resize / skin change); the vortex and speaker cones
  are painted live between them.

## Phase 5 - the real artwork

The approved reference image (`Assets/background.png`, 1672 x 941) is embedded in the plugin and used as the
interface. Live controls are placed on top of the matching elements of the artwork:

- Knobs, the Forward/Back slider, VFX ON/OFF, Randomizer, A / B, undo / redo (top) and the preset name box are drawn
  live and cover the baked-in versions, so values and positions are always real.
- The preset, favorites, A/B, undo, redo, skin arrows, hamburger (preset menu), settings and help elements of the
  artwork are transparent hit areas over the baked icons (hover / disabled feedback only).
- The vortex is the artwork's own spiral, cut out with a soft edge, rotated while VFX Motion is ON and zoomed by
  Forward (contracts) / Back (expands) plus the live input level. The speaker cones are cut out of the artwork too and
  pulse with the real left / right sub-band level.
- Skins hue-shift the red neon / vortex / speaker glow of the artwork (chrome and the gold crown stay); the skin
  thumbnails keep their own colours. Each skin is generated on first use and cached.
- If the artwork cannot be decoded the previous drawn interface is used as a fallback.

To change the artwork replace `Assets/background.png` (keep it 16:9; control positions are defined in reference
coordinates in `PluginEditor.cpp::resized()`).

Not implemented yet: parameter locks, rename/delete of user presets inside the plugin (use "Open preset folder"),
hamburger/settings/help buttons from the reference header, and the chrome / carbon-fiber artwork pass.

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
