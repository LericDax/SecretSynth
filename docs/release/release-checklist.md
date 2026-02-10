# Release Checklist

Use this checklist for every beta and public release candidate.

## 1) Build

- [ ] Update version/build constants in `cmake/PluginIdentity.cmake`.
- [ ] Build Release binaries for Windows (VST3) and macOS (VST3 + AU).
- [ ] Run unit/integration test suite and confirm all passing.

## 2) Package artifacts

- [ ] Create platform installers (`.msi` for Windows, `.pkg` for macOS).
- [ ] Run packaging scripts:
  - `scripts/release/package-beta.sh --platform windows ...`
  - `scripts/release/package-beta.sh --platform macos ...`
  - or `scripts/release/package-beta.ps1` on Windows CI.
- [ ] Verify output artifact naming follows semantic version format:
  - `SecretSynth-beta-vX.Y.Z-windows.zip`
  - `SecretSynth-beta-vX.Y.Z-macos.zip`

## 3) Sign / notarize (as applicable)

- [ ] Sign Windows installer and plugin binaries (code-signing cert).
- [ ] Sign macOS installer/components and submit notarization.
- [ ] Staple notarization ticket and verify on clean machine.

## 4) Smoke test in live hosts

- [ ] Install on clean Windows and macOS environments.
- [ ] Verify plugin discovery in at least one target host per platform.
- [ ] Run quick functional pass:
  - [ ] Audio output
  - [ ] Preset load/save
  - [ ] Parameter automation
  - [ ] Project save/reload state restore

## 5) Publish artifacts

- [ ] Update `CHANGELOG.md` release section.
- [ ] Attach signed artifacts and checksums to release notes.
- [ ] Link `quickstart-install.md`, `known-issues.md`, and issue template for bug intake.
- [ ] Announce release with supported hosts/platform matrix and known limitations.
