#include "ParameterRegistry.h"

namespace secretsynth::plugin::parameters
{
const ParameterSpec* findSpecByStableId (std::string_view stableId) noexcept
{
    for (const auto& spec : parameterSpecs)
    {
        if (spec.stableId == stableId)
            return &spec;
    }

    return nullptr;
}

static_assert (hasStableIndexOrdering(), "Parameter indices must remain stable for automation compatibility");
} // namespace secretsynth::plugin::parameters
