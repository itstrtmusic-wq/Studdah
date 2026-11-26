# Studdah

Creative vocal manipulation plugin built with JUCE and CMake.

## M01 — Plugin foundation

Mono/stereo audio passthrough, smoothed input/output gain, dry/wet, DAW automation and state persistence. The generic JUCE editor is a temporary test interface. Reverse reverb is implemented in M06.

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

Try Rate 1/4, Straight, Slice Length 100%, Repeats 2, Edge Fade 5 ms, Mix 100%, Direction Alternating. Listen for forward and backward phrases. The M06 reverse reverb can also be used independently.

## M06 — Reverse reverb swell

A diffuse stereo reverb records its own wet tail, reverses the previous tempo-grid slice, and applies a rising envelope with a short end fade. Swell Length selects 1/1 to 1/16; Reverb Decay adjusts feedback decay (0.2–8 seconds); Reverb Damping darkens the tail; Swell Curve shapes the buildup; Swell Amount adds the wet swell in parallel. Enable Reverse Reverb independently of Stutter or combine them. The reverb processes the stutter output; global Mix blends the whole effected path with original input. At extreme tempos the capture is capped at four seconds; a seek or restart clears the reverb history. This is a causal reverse-tail effect using past audio, not future-aware pre-vocal reverb. It adds no mandatory dry-path latency. Leave headroom because the parallel wet path can increase peaks.

Try Stutter off, Reverse Reverb on, Swell Length 1/2, Decay 3 s, Damping 45%, Amount 50%, Curve 2, Mix 100%. Play a vocal loop and listen after the first complete capture. Try combining with reverse stutter. All four CTest targets should pass.

## M07 — Pattern sequencer

Enable Sequencer to switch from manual effect enables and Direction to a repeating 16-step pattern. The 3 clickable lanes are Stutter (forward slice), Reverse (backward slice, takes precedence over Stutter), and Swell (reverse reverb). Empty steps play live audio, subject to short effect fades. Step Rate selects 1/4 through 1/32 and Pattern Steps sets loop length 1–16. The effect Rate and Swell Length remain independent; a lane enables its effect for the step, and capture occurs at the next corresponding effect grid crossing. For immediate aligned effects, use matching divisions. The pattern follows host quarter-note position (including seeks) and all step masks and controls are saved and automatable. The temporary interface includes the editable grid and a live active-step indicator.

Try Sequencer on, Step Rate 1/16, Pattern Steps 16, Rate 1/16, Swell Length 1/16, Amount 50%, Mix 100%. Click cells during playback to customize the default pattern. Disable Sequencer to restore manual controls. All five CTest targets should pass.

## M08 — Chrome interface

The native JUCE editor uses a brushed-metal theme with red accents, custom rotary controls, an editable 16-step grid, the existing host tempo display, and a rolling input waveform. The waveform uses atomically published min/max bins (approximately 2.56 seconds of input) rather than sharing the mutable DSP history. All existing parameters and step masks retain their IDs and automation attachments. The waveform is a live input view, not a slice-position editor. Preset browsing and performance macros are planned for M09.

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

M02 circular buffer implemented; M03 host transport implemented; M04 stutter implemented; M05 reverse implemented; M06 reverse reverb implemented; M07 sequencer implemented; M08 Chrome UI implemented; M09 presets and macros.
