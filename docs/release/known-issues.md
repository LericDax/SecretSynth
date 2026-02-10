# Known Issues (Beta)

Track active beta limitations here. Move fixed items to changelog release notes.

## Open

1. **Unsigned beta installers may trigger security prompts**
   - **Platforms**: Windows/macOS
   - **Impact**: Users may need to confirm trust manually.
   - **Workaround**: Install via OS security prompts and verify artifact checksums.

2. **Some hosts require a manual plugin rescan after install**
   - **Platforms**: Windows/macOS
   - **Impact**: SecretSynth may not appear immediately.
   - **Workaround**: Force VST3/AU scan in host preferences and relaunch host.

3. **Automation mapping can differ across DAWs when upgrading between beta builds**
   - **Platforms**: Host-dependent
   - **Impact**: Existing sessions may need automation lane review.
   - **Workaround**: Validate saved sessions with release smoke-test checklist before production work.

## Reporting guidance

When filing issues, include:

- SecretSynth version (semantic version)
- Host + host version
- OS + architecture
- Repro steps and expected vs. actual behavior
- Crash log/stack trace where available
