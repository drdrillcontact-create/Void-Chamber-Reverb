# Void Chamber Reverb

A stereo algorithmic reverb audio plugin (VST3 / AU) written in C++ with JUCE,  
inspired by the "Void Chamber" aesthetic — dark, spacey, infinite.

## Features
- Algorithmic Schroeder reverb (comb + allpass filters)
- Stereo processing with independent left/right tails
- Parameters: Size, Pre Delay, Decay Time, Damping, Modulation, Mix
- Space Type selector: Void / Hall / Plate / Cavern / Ambient
- Input / Output meters
- Dark futuristic UI with neon accents

## Build Requirements
- CMake 3.16+
- JUCE framework (https://juce.com) — set `JUCE_ROOT` or `-DJUCE_ROOT=/path/to/JUCE`
- Windows: Visual Studio 2019/2022 + VST3 SDK (bundled with JUCE)
- macOS: Xcode + AudioUnit SDK

## Configure & Build (manual)

```bash
# Clone or extract this repo
cd void-chamber-reverb

# Windows (VST3)
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_ROOT=C:/path/to/JUCE
cmake --build build --config Release --target MyPlugin.VST3

# macOS (AudioUnit)
cmake -S . -B build -G Xcode -DJUCE_ROOT=/path/to/JUCE
cmake --build build --config Release --target MyPlugin.AudioUnit
```

## GitHub Actions
The `.github/workflows/build.yml` builds:
- **Windows** → VST3 (x64)
- **macOS** → AudioUnit (AU)

You must add `JUCE_ROOT` as a secret or install JUCE in the workflow before building.

## License
This is original code written by you. The UI design is inspired by the reference image  
but all graphics are drawn procedurally with JUCE — no assets are copied from any third party.
