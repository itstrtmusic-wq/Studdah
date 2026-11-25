# Studdah

Creative vocal manipulation plugin built with JUCE and CMake.

## M01 — Plugin foundation

Mono/stereo audio passthrough, smoothed input/output gain, dry/wet, DAW automation and state persistence. The generic JUCE editor is a temporary test interface. Stutter and reverse effects are not implemented yet.

## Windows build

Requires a complete JUCE 7+ source tree, CMake 3.22+, and Visual Studio 2022 with Desktop development with C++.

```powershell
git clone https://github.com/itstrtmusic-wq/Studdah.git
cd Studdah
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_PATH="C:/Program Files/Faust/share/faust/juce"
cmake --build build --config Release
```

Adjust JUCE_PATH for your installation. No JUCE sources are bundled.

Outputs:
- build/Studdah_artefacts/Release/VST3/Studdah.vst3
- build/Studdah_artefacts/Release/Standalone/Studdah.exe

Copy the entire VST3 bundle to a VST3 folder scanned by FL Studio, then rescan.

## Smoke test

1. Load Studdah on a mixer track with vocals.
2. At default settings, confirm unchanged audio.
3. Set input gain to -12 dB; at 100% mix the signal should attenuate, and at 0% mix the dry signal should return.
4. Change output gain; it should affect both paths.
5. Save and reopen the DAW project; verify parameters persist.

## Roadmap

M02 circular buffer; M03 host transport; M04 stutter; M05 reverse; M06 reverse reverb; M07 sequencer; M08 custom UI.
