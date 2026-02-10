#pragma once

#include "ParameterRegistry.h"

#include <array>
#include <string>
#include <string_view>

namespace secretsynth::plugin::parameters
{
struct PluginState
{
    std::array<float, parameterCount> values {};
};

inline constexpr int currentStateVersion = 2;

PluginState makeDefaultState() noexcept;
std::string serializeState (const PluginState& state);
PluginState deserializeState (std::string_view serialized);

float clampToRange (ParameterId id, float value) noexcept;
} // namespace secretsynth::plugin::parameters
