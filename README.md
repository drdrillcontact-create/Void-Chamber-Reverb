# Void Chamber Reverb

A JUCE audio effect with a Schroeder reverb (parallel combs and series allpasses),
independent stereo tails, and a mono-in/mono-out mode. The editor has controls
for size, pre-delay, decay, damping, modulation and wet/dry mix, a Space Type
selector, and input/output meters. The UI is drawn with JUCE, without external
graphics assets.

## Build locally

- CMake **3.22+** and a C++17 compiler.
- A JUCE **8** source tree. CI uses [8.0.17-security](https://github.com/juce-framework/JUCE/tree/dabd5ad13cd869cba41046947fffc8e5ce5cd662).
  Set `JUCE_ROOT` to the directory **containing `CMakeLists.txt` and `modules/`**,
  not to `modules/` itself or to the Projucer executable. CMake reads the
  environment variable on first configure; `-DJUCE_ROOT=...` sets or overrides
  the cached path. CMake never downloads JUCE.
- **Windows:** Visual Studio 2022 with *Desktop development with C++* and the Windows SDK.
  The VST3 SDK is included in JUCE.
- **macOS:** Xcode with the macOS SDK (including the Audio Unit frameworks).

From the root of this repository:

**Windows x64 VST3** (PowerShell):

```powershell
$env:JUCE_ROOT = 'C:\SDKs\JUCE'
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target VoidChamberReverb_VST3
```

**macOS Audio Unit** (Apple Silicon and Intel):

```sh
cmake -S . -B build -G Xcode -DJUCE_ROOT="$HOME/SDKs/JUCE" '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
cmake --build build --config Release --target VoidChamberReverb_AU
```

The bundles are `build/VoidChamberReverb_artefacts/Release/VST3/Void Chamber Reverb.vst3`
and `build/VoidChamberReverb_artefacts/Release/AU/Void Chamber Reverb.component`,
respectively. Building does not install them in a system plugin directory.

To run the DSP regression test on either platform:

```sh
cmake --build build --config Release --target ReverbDSPTests
ctest --test-dir build -C Release --output-on-failure
```

## GitHub Actions

[Build plugins](.github/workflows/build.yml) runs on **push** and **pull_request**.
Each GitHub-hosted runner checks out the plugin and the pinned official JUCE
source, sets `JUCE_ROOT` to that checkout, builds its Release target, and tests
the impulse response. On a successful job, download the ZIP from the workflow
run's **Artifacts** section. Extract the entire bundle, not just its binary.

| Job | Download |
| --- | --- |
| Windows x64 VST3 | `VoidChamberReverb-Windows-x64-VST3.zip` |
| macOS universal AU | `VoidChamberReverb-macOS-universal-AU.zip` |

**Required repository secrets: none. Required repository variables: none.**
Enable GitHub Actions and allow `actions/checkout` and `actions/upload-artifact`.
Checkout uses the read-only `GITHUB_TOKEN` supplied by GitHub; the workflow does
not persist its credentials. Do **not** set a `JUCE_ROOT` secret to a path on
your computer: hosted runners have their own filesystems.

The official JUCE source is public; no JUCE account login, personal access token,
Apple Developer certificate or notarization credentials are required for these
builds. Access to the source is separate from licensing: ensure that your use
and distribution comply with the [JUCE 8 commercial licence or AGPLv3](https://github.com/juce-framework/JUCE/blob/dabd5ad13cd869cba41046947fffc8e5ce5cd662/LICENSE.md).
These are build/test artifacts, not Developer ID-signed or notarized releases.
