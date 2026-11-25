# Studdah

Creative vocal manipulation plugin built with JUCE and CMake.

## M01 — Plugin foundation

Mono/stereo audio passthrough, smoothed input/output gain, dry/wet, DAW automation and state persistence. The generic JUCE editor is a temporary test interface. The reverse reverb effect is planned for M06.

## M02 — Circular audio history

The plugin records the latest 16 seconds of incoming mono/stereo audio before gain processing. The history supports wraparound, absolute sample positions, forward/reverse copying into preallocated storage, and silence padding before enough samples have arrived. Recording does not allocate memory on the audio thread. The buffer is reset when the host prepares the processor. M02 intentionally leaves audible processing unchanged; playback effects arrive in later milestones.

## M03 — Host transport and tempo

Reads host BPM, PPQ, play state and sample position once per audio block. TempoClock provides straight, dotted and triplet divisions, grid offsets and seek/restart detection. If host tempo or phase is unavailable it uses the adjustable Fallback BPM (default 120) and an internal phase. The temporary editor displays tempo, phase source and playing state. Audio is unchanged until M04.

Test in FL Studio: start playback, change project BPM, seek and stop, and check the displayed tempo and transport state. In standalone with no host tempo, change Fallback BPM and check the display. All CTest targets should pass.

## M04 — Stutter playback

Enable Stutter during playback to capture the preceding slice at the next musical grid boundary. The slice repeats for the selected number of steps before a fresh capture. Rate selects 1/1 through 1/128, Rhythm selects straight/dotted/triplet, and Slice Length controls the proportion of each step played from history. Short slices return to live audio for the remainder of the step. Edge Fade crossfades between live and captured audio (1–30 ms, bounded by slice length). Mix blends the dry input and the effected path. Stutter is disabled by default. A seek or transport restart clears the history and playback phase. Extremely long captures are limited to the available history; repeats are refreshed if history expires.

Try a vocal loop at 120 BPM with Stutter on, Rate 1/16, Straight, 4 repeats, 100% slice, 5 ms edge fade, Mix 100%. Change controls and confirm dry passthrough when stutter is off. Test stop/start and mono/stereo. The generic editor is temporary; abrupt host seeks or rate changes may interrupt a slice.

## M05 — Reverse and alternating slices

Direction is an automatable choice: Forward (default), Reverse, or Alternating (forward then reverse, starting forward with each new capture). Enable Stutter to use any of these modes. Direction changes take effect at the next grid crossing; the current pulse keeps its direction. Set Repeats to 1 for a continuously refreshed reverse effect. Reverse-capable captures are limited to half the history capacity to avoid reading overwritten samples during playback; older captures are refreshed before expiration. This is causal live reversal of past audio, not lookahead or reverse reverb.

Try Rate 1/4, Straight, Slice Length 100%, Repeats 2, Edge Fade 5 ms, Mix 100%, Direction Alternating. Listen for forward and backward phrases. Reverse reverb comes next.

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

M02 circular buffer implemented; M03 host transport implemented; M04 stutter implemented; M05 reverse implemented; M06 reverse reverb; M07 sequencer; M08 custom UI.
