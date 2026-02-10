#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace secretsynth::plugin::parameters
{
enum class ParameterId : std::size_t
{
    oscillatorFrequency = 0,
    oscillatorPdAmount,
    oscillatorPdShape,
    oscillatorTune,
    oscillatorFine,
    oscillatorMix,
    modLfo1RateHz,
    modLfo2RateHz,
    filterCutoffHz,
    filterResonance,
    ampAttackSeconds,
    ampReleaseSeconds,
    outputGain,
    performanceVoices,
    count
};

struct ParameterSpec
{
    ParameterId id;
    std::string_view stableId;
    float minimum;
    float maximum;
    float defaultValue;
};

inline constexpr std::array<ParameterSpec, static_cast<std::size_t> (ParameterId::count)> parameterSpecs {{
    { ParameterId::oscillatorFrequency, "osc.frequency", 20.0f, 20000.0f, 220.0f },
    { ParameterId::oscillatorPdAmount, "osc.pd_amount", 0.0f, 1.0f, 0.6f },
    { ParameterId::oscillatorPdShape, "osc.pd_shape", 0.0f, 1.0f, 0.5f },
    { ParameterId::oscillatorTune, "osc.tune", -24.0f, 24.0f, 0.0f },
    { ParameterId::oscillatorFine, "osc.fine", -100.0f, 100.0f, 0.0f },
    { ParameterId::oscillatorMix, "osc.mix", 0.0f, 1.0f, 1.0f },
    { ParameterId::modLfo1RateHz, "mod.lfo1_rate_hz", 0.05f, 20.0f, 2.0f },
    { ParameterId::modLfo2RateHz, "mod.lfo2_rate_hz", 0.05f, 20.0f, 5.0f },
    { ParameterId::filterCutoffHz, "filter.cutoff_hz", 20.0f, 20000.0f, 1800.0f },
    { ParameterId::filterResonance, "filter.resonance", 0.1f, 1.2f, 0.6f },
    { ParameterId::ampAttackSeconds, "amp.attack_s", 0.001f, 3.0f, 0.005f },
    { ParameterId::ampReleaseSeconds, "amp.release_s", 0.01f, 5.0f, 0.3f },
    { ParameterId::outputGain, "output.gain", 0.0f, 1.0f, 1.0f },
    { ParameterId::performanceVoices, "performance.voices", 1.0f, 16.0f, 8.0f },
}};

inline constexpr std::size_t parameterCount = parameterSpecs.size();

inline constexpr const ParameterSpec& getSpec (ParameterId id) noexcept
{
    return parameterSpecs[static_cast<std::size_t> (id)];
}

inline constexpr bool hasStableIndexOrdering() noexcept
{
    for (std::size_t index = 0; index < parameterSpecs.size(); ++index)
    {
        if (static_cast<std::size_t> (parameterSpecs[index].id) != index)
            return false;
    }

    return true;
}

const ParameterSpec* findSpecByStableId (std::string_view stableId) noexcept;
} // namespace secretsynth::plugin::parameters
