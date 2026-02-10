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
    filterCutoffHz,
    outputGain,
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
    { ParameterId::filterCutoffHz, "filter.cutoff_hz", 20.0f, 20000.0f, 1800.0f },
    { ParameterId::outputGain, "output.gain", 0.0f, 1.0f, 1.0f },
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
