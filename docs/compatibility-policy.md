# Parameter and State Compatibility Policy

SecretSynth preserves host automation and preset/state interoperability across releases by treating parameter
identity and ordering as stable ABI.

## What will not change in minor/patch releases

- Existing parameter enum values and index ordering in `ParameterId` will not be reordered.
- Existing stable parameter IDs (for example `osc.frequency`) will not be renamed.
- Existing parameter ranges will not be narrowed in a way that invalidates previously-saved values.
- Existing serialized state keys will remain readable.

## What may change

- New parameters may be appended to the end of the parameter registry.
- State format version may increase when new parameters are added.
- Migration logic may populate newly-added parameters with explicit defaults when loading older states.

## Breaking changes (major release only)

The following are considered breaking and require a major version bump:

- Reordering existing parameter indices.
- Removing a parameter that may still be referenced by host automation.
- Renaming a stable serialized key without backward-compatible migration.

## Implementation notes

- Parameter IDs, explicit ranges, and defaults live in `src/plugin/parameters/ParameterRegistry.h`.
- State serialization is versioned via `state.version` in
  `src/plugin/parameters/StateSerialization.cpp`.
- `deserializeState` supports migration from prior versions by filling missing fields with defaults.
