#include "../../src/plugin/parameters/StateSerialization.h"

#include <cmath>
#include <iostream>

using secretsynth::plugin::parameters::ParameterId;

namespace
{
bool approxEqual (float a, float b, float epsilon = 1.0e-4f)
{
    return std::fabs (a - b) <= epsilon;
}

int runRoundtripTest()
{
    auto state = secretsynth::plugin::parameters::makeDefaultState();
    state.values[static_cast<std::size_t> (ParameterId::oscillatorFrequency)] = 512.5f;
    state.values[static_cast<std::size_t> (ParameterId::oscillatorPdAmount)] = 0.33f;
    state.values[static_cast<std::size_t> (ParameterId::filterCutoffHz)] = 7420.0f;
    state.values[static_cast<std::size_t> (ParameterId::outputGain)] = 0.8f;

    const auto serialized = secretsynth::plugin::parameters::serializeState (state);
    const auto reloaded = secretsynth::plugin::parameters::deserializeState (serialized);

    for (std::size_t i = 0; i < state.values.size(); ++i)
    {
        if (! approxEqual (state.values[i], reloaded.values[i]))
        {
            std::cerr << "Roundtrip mismatch at index " << i << " expected=" << state.values[i]
                      << " actual=" << reloaded.values[i] << '\n';
            return 1;
        }
    }

    return 0;
}


int runStableOrderingTest()
{
    if (! secretsynth::plugin::parameters::hasStableIndexOrdering())
    {
        std::cerr << "Parameter IDs are not in stable index order\n";
        return 1;
    }

    if (secretsynth::plugin::parameters::getSpec (ParameterId::oscillatorFrequency).stableId != "osc.frequency"
        || secretsynth::plugin::parameters::getSpec (ParameterId::outputGain).stableId != "output.gain")
    {
        std::cerr << "Stable IDs changed unexpectedly\n";
        return 1;
    }

    return 0;
}

int runMigrationTest()
{
    const std::string legacy = "state.version=0\n"
                               "osc.frequency=330\n"
                               "osc.pd_amount=0.42\n"
                               "filter.cutoff_hz=1400\n";

    const auto reloaded = secretsynth::plugin::parameters::deserializeState (legacy);

    if (! approxEqual (reloaded.values[static_cast<std::size_t> (ParameterId::oscillatorFrequency)], 330.0f)
        || ! approxEqual (reloaded.values[static_cast<std::size_t> (ParameterId::oscillatorPdAmount)], 0.42f)
        || ! approxEqual (reloaded.values[static_cast<std::size_t> (ParameterId::filterCutoffHz)], 1400.0f))
    {
        std::cerr << "Legacy field values were not preserved during migration\n";
        return 1;
    }

    const auto expectedDefaultGain =
        secretsynth::plugin::parameters::getSpec (ParameterId::outputGain).defaultValue;
    if (! approxEqual (reloaded.values[static_cast<std::size_t> (ParameterId::outputGain)], expectedDefaultGain))
    {
        std::cerr << "Expected output gain default during migration\n";
        return 1;
    }

    return 0;
}
} // namespace

int main()
{
    if (runStableOrderingTest() != 0)
        return 1;

    if (runRoundtripTest() != 0)
        return 1;

    if (runMigrationTest() != 0)
        return 1;

    return 0;
}
