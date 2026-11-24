# ConstellationX Vocal — JUCE VST3 starter project

This project is an original JUCE-based **audio effect** starter for ConstellationX Vocal.
It follows the general CMake/JUCE project workflow shown in the supplied UniverseX files,
but it is not a copy of UniverseX's synthesizer code.

## Included in this starter

- VST3 and Standalone targets.
- Audio input/output effect configuration (not a MIDI instrument).
- APVTS parameter state and host automation.
- Initial spatial/depth processing prototype with a short reflection delay, width,
  tonal controls, wet/dry and output.
- Custom resizable interface.
- Animated red vortex visualizer with VFX Motion toggle.
- Two animated speaker-cone visual elements.
- Basic randomize action.
- Skin selector UI placeholder.

## Important prototype limitations

This is a **Phase 1 scaffold**, not a release-ready vocal processor. It has not been
compiled or tested in this environment. The DSP is a simple starting point and does
not yet reproduce Black Vortex's proprietary processing or establish perceptually
accurate room simulation. The Decay control and skin selector are currently UI/state
placeholders; the visualizer is not yet driven by a proper audio analysis feed.
The current randomizer is basic and does not yet have parameter locks, preset browser,
favorites, A/B snapshots, undo/redo, or a controlled-randomization range.

The VFX toggle controls the editor animation only; it does not bypass audio processing.

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

1. Compile, resolve toolchain/API issues, and validate mono/stereo routing.
2. Implement proper early-reflection network, decay behavior, smoothing and latency tests.
3. Add a real-time audio analysis feed for the vortex and both speaker visualizers.
4. Implement full skin system, resize-aware layout, parameter locking, controlled randomizer,
   preset management, A/B, undo/redo and factory presets.
5. Refine visual fidelity against the approved ConstellationX Vocal concept.
6. Test in FL Studio 11 and current hosts; confirm supported plugin format and GUI behavior.

## Branding

The CMake manufacturer code `ChBt` and plugin code `CxV1` are provisional identifiers.
Confirm uniqueness before public distribution. Do not change plugin codes after release
without considering host preset and plugin identity compatibility.
