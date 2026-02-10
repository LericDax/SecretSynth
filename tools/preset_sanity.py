#!/usr/bin/env python3
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRESET_DIR = ROOT / 'presets' / 'factory'

MAX_CPU = 6.0
GAIN_RANGE = (0.06, 0.12)
TRIM_RANGE = (-3.0, -1.0)

failures = []
category_counts = {}

for p in sorted(PRESET_DIR.glob('*.synthpreset')):
    data = json.loads(p.read_text())
    category = data.get('metadata', {}).get('category')
    category_counts[category] = category_counts.get(category, 0) + 1

    for field in ('author', 'category', 'description'):
        if not data.get('metadata', {}).get(field):
            failures.append(f"{p.name}: missing metadata.{field}")

    macros = data.get('macros', [])
    if len(macros) < 4:
        failures.append(f"{p.name}: expected >=4 macros")

    gain = data.get('oscillator', {}).get('gain')
    if gain is None or not (GAIN_RANGE[0] <= gain <= GAIN_RANGE[1]):
        failures.append(f"{p.name}: oscillator.gain {gain} outside {GAIN_RANGE}")

    trim = data.get('normalization', {}).get('outputTrimDb')
    if trim is None or not (TRIM_RANGE[0] <= trim <= TRIM_RANGE[1]):
        failures.append(f"{p.name}: normalization.outputTrimDb {trim} outside {TRIM_RANGE}")

    cpu = data.get('sanity', {}).get('estimatedCpuScore')
    if cpu is None or cpu > MAX_CPU:
        failures.append(f"{p.name}: estimatedCpuScore {cpu} > {MAX_CPU}")

print('Preset sanity summary')
print(f"- Total presets: {sum(category_counts.values())}")
for cat, count in sorted(category_counts.items()):
    print(f"- {cat}: {count}")
print(f"- Failures: {len(failures)}")

if failures:
    for f in failures:
        print(f"  * {f}")
    raise SystemExit(1)
