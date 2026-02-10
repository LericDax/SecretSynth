# SecretSynth

SecretSynth is a JUCE-based software synthesizer plugin project with a stable plugin identity, CMake-based builds, and cross-platform CI for VST3 (plus AU on macOS).

## Project Layout

- `src/dsp/` synthesis DSP implementation (`SimpleVoice` oscillator baseline)
- `src/plugin/` JUCE processor/editor wrappers and plugin entrypoint
- `src/ui/` UI components/look-and-feel surface for plugin editor
- `tests/` DSP/unit test executables integrated with CTest
- `presets/` factory patch data files
- `cmake/PluginIdentity.cmake` centralized plugin identity/version constants

## Toolchain Requirements

- CMake **3.24+**
- C++ compiler with C++20 support
  - Windows: Visual Studio 2022 (MSVC v143)
  - macOS: Xcode 15+ or Apple Clang 15+, plus Ninja (optional but recommended)
- Git (to pull JUCE through CMake `FetchContent`)

## JUCE Version

The build pulls JUCE automatically from:

- **JUCE 8.0.3** (`https://github.com/juce-framework/JUCE.git`, tag `8.0.3`)

## Stable Plugin ID and Versioning

Plugin identifiers and versions are centralized in `cmake/PluginIdentity.cmake`:

- Manufacturer code: `SeAu`
- Plugin code: `ScSy`
- Bundle ID: `com.secretaudio.secretsynth`
- Semantic version: `0.1.0`
- Build integer: `1`

Update all of these in one place when cutting releases to preserve host compatibility.

State/preset compatibility rules for parameter IDs, index stability, and migration are documented in `docs/compatibility-policy.md`.

## Local Build

### Windows (VST3)

```bash
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release --target SecretSynth
ctest --test-dir build -C Release --output-on-failure
```

### macOS (VST3 + AU)

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target SecretSynth
ctest --test-dir build --output-on-failure
```

## Output Paths

Paths can vary by generator, but Release artifacts are typically found under:

- VST3: `build/**/SecretSynth.vst3`
- AU (macOS): `build/**/SecretSynth.component`
- DSP test binary: `build/**/secretsynth_dsp_tests`

## Release + Signing Readiness Notes

- Release builds are enabled via standard `Release` configurations.
- Plugin bundle metadata (`BUNDLE_ID`, manufacturer/plugin codes, versioning) is fixed and deterministic.
- CI produces unsigned plugin bundles suitable for downstream code-signing/notarization in release pipelines.
