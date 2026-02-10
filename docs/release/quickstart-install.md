# Quickstart Install (Beta)

This guide helps beta users install SecretSynth quickly on Windows and macOS.

## 1) Download the correct artifact

Use release assets named:

- `SecretSynth-beta-vX.Y.Z-windows.zip`
- `SecretSynth-beta-vX.Y.Z-macos.zip`

`X.Y.Z` is the semantic plugin version from `cmake/PluginIdentity.cmake`.

## 2) Windows install

1. Unzip `SecretSynth-beta-vX.Y.Z-windows.zip`.
2. Run `SecretSynth-installer.msi`.
3. Verify `SecretSynth.vst3` is installed in:
   - `C:\Program Files\Common Files\VST3\`
4. Open your DAW and rescan plugins.

## 3) macOS install

1. Unzip `SecretSynth-beta-vX.Y.Z-macos.zip`.
2. Run `SecretSynth-installer.pkg`.
3. Verify installed plugin locations:
   - VST3: `/Library/Audio/Plug-Ins/VST3/SecretSynth.vst3`
   - AU: `/Library/Audio/Plug-Ins/Components/SecretSynth.component` (if included)
4. Open your DAW, rescan plugins, and load SecretSynth.

## 4) First-run smoke test (all platforms)

1. Insert SecretSynth on an instrument track.
2. Play and confirm audio output.
3. Load the `Init` preset.
4. Save and re-open a host project, confirming state restores correctly.

## 5) If install fails

- See [Known Issues](./known-issues.md).
- File a report with the `Bug / Crash Intake` GitHub issue template.
