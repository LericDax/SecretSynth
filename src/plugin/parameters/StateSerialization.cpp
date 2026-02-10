#include "StateSerialization.h"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <string>

namespace secretsynth::plugin::parameters
{
namespace
{
bool parseFloat (std::string_view text, float& parsed)
{
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    const auto result = std::from_chars (begin, end, parsed);
    return result.ec == std::errc() && result.ptr == end;
}

bool parseInt (std::string_view text, int& parsed)
{
    const auto* begin = text.data();
    const auto* end = begin + text.size();
    const auto result = std::from_chars (begin, end, parsed);
    return result.ec == std::errc() && result.ptr == end;
}

PluginState migrateToCurrentVersion (int sourceVersion, const PluginState& parsedState) noexcept
{
    PluginState migrated = parsedState;

    if (sourceVersion <= 0)
    {
        migrated.values[static_cast<std::size_t> (ParameterId::outputGain)] =
            getSpec (ParameterId::outputGain).defaultValue;
    }

    return migrated;
}
} // namespace

PluginState makeDefaultState() noexcept
{
    PluginState state;

    for (const auto& spec : parameterSpecs)
        state.values[static_cast<std::size_t> (spec.id)] = spec.defaultValue;

    return state;
}

float clampToRange (ParameterId id, float value) noexcept
{
    const auto& spec = getSpec (id);
    return std::clamp (value, spec.minimum, spec.maximum);
}

std::string serializeState (const PluginState& state)
{
    std::ostringstream stream;
    stream << "state.version=" << currentStateVersion << "\n";

    for (const auto& spec : parameterSpecs)
    {
        const auto index = static_cast<std::size_t> (spec.id);
        stream << spec.stableId << "=" << clampToRange (spec.id, state.values[index]) << "\n";
    }

    return stream.str();
}

PluginState deserializeState (std::string_view serialized)
{
    PluginState parsedState = makeDefaultState();
    int stateVersion = 0;

    std::size_t lineStart = 0;
    while (lineStart < serialized.size())
    {
        const auto lineEnd = serialized.find ('\n', lineStart);
        const auto lineLength = (lineEnd == std::string_view::npos ? serialized.size() : lineEnd) - lineStart;
        const auto line = serialized.substr (lineStart, lineLength);

        const auto separator = line.find ('=');
        if (separator != std::string_view::npos)
        {
            const auto key = line.substr (0, separator);
            const auto valueText = line.substr (separator + 1);

            if (key == "state.version")
            {
                parseInt (valueText, stateVersion);
            }
            else if (const auto* spec = findSpecByStableId (key))
            {
                float parsedValue = 0.0f;
                if (parseFloat (valueText, parsedValue))
                    parsedState.values[static_cast<std::size_t> (spec->id)] = clampToRange (spec->id, parsedValue);
            }
        }

        if (lineEnd == std::string_view::npos)
            break;

        lineStart = lineEnd + 1;
    }

    return migrateToCurrentVersion (stateVersion, parsedState);
}
} // namespace secretsynth::plugin::parameters
