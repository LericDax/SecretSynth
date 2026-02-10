#include <array>
#include <cmath>
#include <iostream>

#include "../../src/dsp/filter/MultiModeFilter.h"

namespace
{
using Filter = secretsynth::dsp::filter::MultiModeFilter;

bool testModesAndResonanceBoundaries()
{
    Filter filter;
    filter.prepare (48000.0);
    filter.reset();
    filter.setCutoffHz (1200.0f);

    for (const auto mode : { Filter::Mode::lowPass, Filter::Mode::bandPass, Filter::Mode::highPass })
    {
        filter.setMode (mode);
        filter.setResonance (1.5f); // clamp
        for (int i = 0; i < 2048; ++i)
        {
            const auto x = (i == 0 ? 1.0f : 0.0f);
            const auto y = filter.processSample (x, 440.0f);
            if (! std::isfinite (y))
            {
                std::cerr << "Filter produced non-finite output in mode boundary test\n";
                return false;
            }
        }
    }

    return true;
}

bool testKeyTrackingResponse()
{
    Filter lowKey;
    lowKey.prepare (48000.0);
    lowKey.reset();
    lowKey.setMode (Filter::Mode::lowPass);
    lowKey.setCutoffHz (1000.0f);
    lowKey.setResonance (0.2f);
    lowKey.setKeyTracking (1.0f);
    lowKey.setKeyTrackingReferenceHz (440.0f);

    Filter highKey = lowKey;

    auto lowAccum = 0.0f;
    auto highAccum = 0.0f;

    for (int i = 0; i < 4096; ++i)
    {
        const auto input = std::sin (2.0f * 3.14159265358979323846f * 3000.0f * static_cast<float> (i) / 48000.0f);
        lowAccum += std::abs (lowKey.processSample (input, 110.0f));
        highAccum += std::abs (highKey.processSample (input, 1760.0f));
    }

    if (highAccum <= lowAccum)
    {
        std::cerr << "Expected high-key tracked filter to pass more HF content\n";
        return false;
    }

    return true;
}

bool testAutomationSweepNoNans()
{
    Filter filter;
    filter.prepare (48000.0);
    filter.reset();
    filter.setMode (Filter::Mode::bandPass);
    filter.setKeyTracking (0.5f);

    float peak = 0.0f;
    for (int i = 0; i < 48000; ++i)
    {
        const auto t = static_cast<float> (i) / 47999.0f;
        filter.setCutoffHz (20.0f + t * (20000.0f - 20.0f));
        filter.setResonance (t);

        const auto y = filter.processSample (0.3f * std::sin (2.0f * 3.14159265358979323846f * 220.0f * t), 220.0f + 660.0f * t);
        if (! std::isfinite (y))
        {
            std::cerr << "Non-finite output during automation sweep\n";
            return false;
        }

        peak = std::max (peak, std::abs (y));
    }

    if (peak < 1.0e-5f)
    {
        std::cerr << "Unexpected near-silent output during automation sweep\n";
        return false;
    }

    return true;
}

bool testDenormalAndSilenceBehavior()
{
    Filter filter;
    filter.prepare (48000.0);
    filter.reset();
    filter.setMode (Filter::Mode::lowPass);
    filter.setCutoffHz (200.0f);
    filter.setResonance (0.95f);

    // excite state
    for (int i = 0; i < 1024; ++i)
        (void) filter.processSample (i == 0 ? 1.0f : 0.0f, 440.0f);

    // verify it decays to exact zeros (denormal flush)
    auto becameZero = false;
    for (int i = 0; i < 200000; ++i)
    {
        const auto y = filter.processSample (0.0f, 440.0f);
        if (! std::isfinite (y))
            return false;
        if (y == 0.0f)
        {
            becameZero = true;
            break;
        }
    }

    if (! becameZero)
    {
        std::cerr << "Filter did not flush to silence under zero-input tail\n";
        return false;
    }

    return true;
}
} // namespace

int main()
{
    if (! testModesAndResonanceBoundaries())
        return 1;

    if (! testKeyTrackingResponse())
        return 1;

    if (! testAutomationSweepNoNans())
        return 1;

    if (! testDenormalAndSilenceBehavior())
        return 1;

    std::cout << "Filter tests passed\n";
    return 0;
}
