# Public v1.0 Go / No-Go Criteria

This gate determines whether SecretSynth can ship as public v1.0.

## Go criteria (all required)

### Product quality
- [ ] No open **P0** defects.
- [ ] No open **P1 crash/data-loss** defects in supported hosts.
- [ ] Startup, preset browsing, and playback are stable in release test matrix.

### Compatibility
- [ ] Passes compatibility policy requirements for parameter/state persistence.
- [ ] Backward session loading validated against latest beta and release candidate.
- [ ] Supported host matrix smoke-tested on current Windows and macOS targets.

### Release engineering
- [ ] Version/build identifiers finalized and tagged.
- [ ] Signed installers produced for Windows/macOS.
- [ ] macOS notarization completed and stapled.
- [ ] Artifact hashes generated and verified.

### Documentation and support readiness
- [ ] `CHANGELOG.md` finalized for v1.0.
- [ ] Quickstart install and known issues docs published.
- [ ] Bug/crash intake template enabled and tested.
- [ ] On-call triage owner assigned for launch week.

## No-go triggers (any one blocks release)

- Any reproducible host crash in supported matrix without mitigation.
- Any regression that breaks project/session reload in a supported host.
- Unsigned or unnotarized distribution artifacts for required platform.
- Missing or incorrect version identifiers in packaged artifacts.
- Critical documentation gaps that would block user installation or support.

## Decision protocol

1. Release owner reviews checklist evidence.
2. Engineering + QA + Product each provide explicit `GO` or `NO-GO`.
3. If no-go, log blocking issues, owners, and re-evaluation date.
