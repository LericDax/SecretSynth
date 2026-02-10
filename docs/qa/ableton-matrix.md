# Ableton Live QA Matrix

Date: 2026-02-10  
Owner: Codex agent run in CI/container environment (`/workspace/SecretSynth`)

## Scope requested
1. Validate in Ableton Live: plugin scan/load, preset recall, automation write/read, offline bounce parity.
2. Run test matrix: sample rates (44.1/48/96 kHz), buffer sizes (64–1024), mono/poly/unison heavy sessions.
3. Verify no crashes on transport start/stop, project reopen, or device duplication.
4. Capture defects with repro + severity.
5. Triage/fix P0/P1 before content lock.

## Environment constraints
- Ableton Live is not installed in this Linux container, so host-level validation (scan/load, automation, transport, project lifecycle, duplication, and bounce parity in Live) cannot be executed here.
- Plugin build/test pipeline is currently blocked in this environment due missing Linux X11 development headers needed by JUCE tooling.

## Matrix execution status

| Area | Planned coverage | Status | Notes |
|---|---|---|---|
| Ableton scan/load | VST3 discovery + insert in Live | **Blocked** | Ableton unavailable in container. |
| Preset recall | Save/reload presets in Live set | **Blocked** | Ableton unavailable in container. |
| Automation write/read | Record and playback parameter automation | **Blocked** | Ableton unavailable in container. |
| Offline bounce parity | Render compare vs realtime | **Blocked** | Ableton unavailable in container. |
| Sample rates | 44.1/48/96 kHz | **Blocked** | Depends on Ableton session runs. |
| Buffer sizes | 64/128/256/512/1024 | **Blocked** | Depends on Ableton session runs. |
| Voice modes | mono/poly/unison-heavy | **Blocked** | Depends on Ableton session runs. |
| Stability lifecycle | transport/reopen/duplicate | **Blocked** | Depends on Ableton session runs. |

## Defects

### SSYN-001 — P1 — Build/Test environment missing JUCE prerequisite (`Xrandr.h`)
- **Severity:** P1 (blocks automated local validation in this container; not necessarily product-runtime severity)
- **Category:** Environment / CI prerequisites
- **Observed during:** Local configure/build attempt for test target
- **Repro steps:**
  1. `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release`
  2. `cmake --build build --target secretsynth_dsp_tests -j4`
  3. Observe `juceaide` build failure with missing `X11/extensions/Xrandr.h`.
- **Expected:** Configure/build proceeds so DSP tests can run.
- **Actual:** Configure fails while building JUCE `juceaide` helper due missing X11 randr headers.
- **Evidence excerpt:** `fatal error: X11/extensions/Xrandr.h: No such file or directory`
- **Suggested fix:** Install Linux dependency package providing Xrandr headers (commonly `libxrandr-dev`) in the validation image before configuring.
- **Triage decision:** Keep as **P1** environment blocker; no source fix applied in this pass.

### SSYN-002 — P0 — Requested Ableton validation cannot run in current environment
- **Severity:** P0 (release-gate task cannot be completed without DAW execution)
- **Category:** Test infrastructure gap
- **Observed during:** Attempt to execute requested Ableton QA workflow
- **Repro steps:**
  1. Start from this container environment.
  2. Attempt to run Ableton Live based validation tasks.
  3. No Ableton Live binary/session environment available.
- **Expected:** Runnable Ableton environment for plugin-host validation matrix.
- **Actual:** Host validation is blocked entirely.
- **Suggested fix:** Execute this matrix on a provisioned QA workstation (Windows/macOS) with Ableton Live installed and plugin artifact deployed.
- **Triage decision:** **Open (P0)** until host-backed QA run is completed.

## P0/P1 triage/fix status
- **P0 (SSYN-002):** Not fixable inside repository code in this container; requires external QA environment provisioning.
- **P1 (SSYN-001):** Not fixed in repository source; requires dependency installation in build image/runner.

## Next-run checklist (when Ableton environment is available)
1. Install/build `SecretSynth.vst3` artifact for target OS.
2. In Ableton Live, verify plugin scan + insert on MIDI track.
3. Run matrix:
   - Sample rates: 44.1k, 48k, 96k
   - Buffer sizes: 64, 128, 256, 512, 1024
   - Patches: mono lead, poly pad, unison-heavy supersaw
4. For each cell, test:
   - Preset save/recall
   - Automation write/read for at least 3 key params
   - Transport start/stop stress (rapid toggles)
   - Device duplication (x10)
   - Project save/reopen
   - Offline render parity vs realtime (null/ear check)
5. Log any additional defects in this file with severity + repro.
