# Studdah

Creative vocal manipulation plugin built with JUCE and CMake.

## M01 — Plugin foundation

Mono/stereo audio passthrough, smoothed input/output gain, dry/wet, DAW automation and state persistence. The generic JUCE editor is a temporary test interface. Stutter and reverse effects are not implemented yet.

## M02 — Circular audio history

The plugin records the latest 16 seconds of incoming mono/stereo audio before gain processing. The history supports wraparound, absolute sample positions, forward/reverse copying into preallocated storage, and silence padding before enough samples have arrived. Recording does not allocate memory on the audio thread. The buffer is reset when the host prepares the processor. M02 intentionally leaves audible processing unchanged; playback effects arrive in later milestones.

## M03 — Host transport and tempo

Reads host BPM, PPQ, play state and sample position once per audio block. TempoClock provides straight, dotted and triplet divisions, grid offsets and seek/restart detection. If host tempo or phase is unavailable it uses the adjustable Fallback BPM (default 120) and an internal phase. The temporary editor displays tempo, phase source and playing state. Audio is unchanged until M04.

Test in FL Studio: start playback, change project BPM, seek and stop, and check the displayed tempo and transport state. In standalone with no host tempo, change Fallback BPM and check the display. Both CTest targets should pass.

## Windows build

Requires a complete JUCE 7+ source tree, CMake 3.22+, and Visual Studio with Desktop development with C++ (use the generator matching your installed version).

```powershell
git clone https://github.com/itstrtmusic-wq/Studdah.git
cd Studdah
cmake -S . -B build-vs2026 -G "Visual Studio 18 2026" -A x64 -DJUCE_PATH="C:/Development/JUCE"
cmake --build build-vs2026 --config Release --parallel
ctest --test-dir build-vs2026 -C Release --output-on-failure
```

Adjust JUCE_PATH for your installation. No JUCE sources are bundled.

Outputs:
- build-vs2026/Studdah_artefacts/Release/VST3/Studdah.vst3
- build-vs2026/Studdah_artefacts/Release/Standalone/Studdah.exe

Copy the entire VST3 bundle to a VST3 folder scanned by FL Studio, then rescan.

## Smoke test

1. Load Studdah on a mixer track with vocals.
2. At default settings, confirm unchanged audio.
3. Set input gain to -12 dB; at 100% mix the signal should attenuate, and at 0% mix the dry signal should return.
4. Change output gain; it should affect both paths.
5. Save and reopen the DAW project; verify parameters persist.

## Roadmap

M02 circular buffer implemented; M03 host transport implemented; M04 stutter; M05 reverse; M06 reverse reverb; M07 sequencer; M08 custom UI.
