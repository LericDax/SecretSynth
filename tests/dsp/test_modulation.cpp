#include <array>
#include <cmath>
#include <iostream>

#include "../../src/dsp/mod/Modulation.h"

namespace
{
using namespace secretsynth::dsp::mod;

bool almostEqual (float a, float b, float tolerance)
{
    return std::abs (a - b) <= tolerance;
}

bool routesEqual (const std::vector<Route>& lhs, const std::vector<Route>& rhs)
{
    if (lhs.size() != rhs.size())
        return false;

    for (std::size_t i = 0; i < lhs.size(); ++i)
    {
        if (lhs[i].source != rhs[i].source
            || lhs[i].destination != rhs[i].destination
            || ! almostEqual (lhs[i].depth, rhs[i].depth, 1.0e-6f)
            || lhs[i].bipolar != rhs[i].bipolar)
        {
            return false;
        }
    }

    return true;
}

bool testDepthScaling()
{
    ModulationMatrix matrix;
    matrix.setSampleRate (48000.0);
    matrix.setDestinationSmoothingTimeSeconds (0.0f);
    matrix.addRoute ({ Source::modEnv, Destination::pitch, 0.5f, false });

    std::array<float, static_cast<std::size_t> (Source::count)> sources {};
    sources[static_cast<std::size_t> (Source::modEnv)] = 0.8f;

    const auto result = matrix.process (sources);
    const auto pitch = result[static_cast<std::size_t> (Destination::pitch)];
    if (! almostEqual (pitch, 0.4f, 1.0e-6f))
    {
        std::cerr << "Depth scaling test failed. expected 0.4, got " << pitch << '\n';
        return false;
    }

    return true;
}

bool testBipolarVsUnipolar()
{
    ModulationMatrix matrix;
    matrix.setSampleRate (48000.0);
    matrix.setDestinationSmoothingTimeSeconds (0.0f);
    matrix.addRoute ({ Source::lfo1, Destination::pdAmount, 1.0f, true });
    matrix.addRoute ({ Source::lfo2, Destination::pdAmount, 1.0f, false });

    std::array<float, static_cast<std::size_t> (Source::count)> sources {};
    sources[static_cast<std::size_t> (Source::lfo1)] = 0.25f;
    sources[static_cast<std::size_t> (Source::lfo2)] = 0.25f;

    const auto result = matrix.process (sources);
    const auto pdAmount = result[static_cast<std::size_t> (Destination::pdAmount)];

    // bipolar transform: 0.25 -> -0.5, unipolar remains 0.25, sum = -0.25
    if (! almostEqual (pdAmount, -0.25f, 1.0e-6f))
    {
        std::cerr << "Bipolar/unipolar test failed. expected -0.25, got " << pdAmount << '\n';
        return false;
    }

    return true;
}

bool testTempoSyncRateAccuracy()
{
    Lfo lfo;
    constexpr int sampleRate = 48000;
    constexpr float bpm = 120.0f;

    lfo.setSampleRate (sampleRate);
    lfo.reset();
    lfo.setRateMode (Lfo::RateMode::tempoSync);
    lfo.setTempoBpm (bpm);
    lfo.setSyncDivision (Lfo::SyncDivision::quarter);

    const auto expectedHz = bpm / 60.0f;
    if (! almostEqual (lfo.getCurrentFrequencyHz(), expectedHz, 1.0e-6f))
    {
        std::cerr << "Tempo sync frequency mismatch. expected " << expectedHz << ", got " << lfo.getCurrentFrequencyHz() << '\n';
        return false;
    }

    constexpr int totalSamples = sampleRate * 4;
    std::array<float, totalSamples> values {};
    for (auto& value : values)
        value = lfo.processSample();

    int positiveCrossings = 0;
    for (int i = 1; i < totalSamples; ++i)
    {
        if (values[static_cast<std::size_t> (i - 1)] <= 0.0f && values[static_cast<std::size_t> (i)] > 0.0f)
            ++positiveCrossings;
    }

    const auto measuredHz = static_cast<float> (positiveCrossings) / 4.0f;
    if (! almostEqual (measuredHz, expectedHz, 0.3f))
    {
        std::cerr << "Tempo sync crossing measurement mismatch. expected " << expectedHz << ", got " << measuredHz << '\n';
        return false;
    }

    return true;
}

bool testSerializationRoundTrip()
{
    ModulationMatrix matrix;
    matrix.addRoute ({ Source::ampEnv, Destination::amp, 1.0f, false });
    matrix.addRoute ({ Source::lfo1, Destination::filterCutoff, 0.37f, true });

    const auto serialized = matrix.serialize();

    ModulationMatrix restored;
    if (! restored.deserialize (serialized))
    {
        std::cerr << "Serialization round-trip deserialize failed.\n";
        return false;
    }

    const auto& routes = restored.getRoutes();
    if (routes.size() != 2)
    {
        std::cerr << "Serialization route count mismatch.\n";
        return false;
    }

    if (routes[0].source != Source::ampEnv || routes[0].destination != Destination::amp || ! almostEqual (routes[0].depth, 1.0f, 1.0e-6f) || routes[0].bipolar)
    {
        std::cerr << "Route 0 mismatch after deserialize.\n";
        return false;
    }

    if (routes[1].source != Source::lfo1 || routes[1].destination != Destination::filterCutoff || ! almostEqual (routes[1].depth, 0.37f, 1.0e-6f) || ! routes[1].bipolar)
    {
        std::cerr << "Route 1 mismatch after deserialize.\n";
        return false;
    }

    return true;
}

bool testDeserializeRejectsMalformedHeader()
{
    ModulationMatrix matrix;
    matrix.addRoute ({ Source::ampEnv, Destination::amp, 0.25f, false });
    const auto originalRoutes = matrix.getRoutes();

    if (matrix.deserialize ("schema=abc\nroutes=0\n"))
    {
        std::cerr << "Expected malformed schema to fail deserialize.\n";
        return false;
    }

    if (! routesEqual (matrix.getRoutes(), originalRoutes))
    {
        std::cerr << "Malformed schema should not mutate routes.\n";
        return false;
    }

    return true;
}

bool testDeserializeRejectsMismatchedRouteCount()
{
    ModulationMatrix matrix;
    matrix.addRoute ({ Source::lfo1, Destination::pitch, 0.5f, true });
    const auto originalRoutes = matrix.getRoutes();

    if (matrix.deserialize ("schema=1\nroutes=2\n0,0,0.5,1\n"))
    {
        std::cerr << "Expected missing route line to fail deserialize.\n";
        return false;
    }

    if (! routesEqual (matrix.getRoutes(), originalRoutes))
    {
        std::cerr << "Missing route line should not mutate routes.\n";
        return false;
    }

    if (matrix.deserialize ("schema=1\nroutes=1\n0,0,0.5,1\n1,1,0.25,0\n"))
    {
        std::cerr << "Expected extra route line to fail deserialize.\n";
        return false;
    }

    if (! routesEqual (matrix.getRoutes(), originalRoutes))
    {
        std::cerr << "Extra route line should not mutate routes.\n";
        return false;
    }

    return true;
}

bool testDeserializeRejectsMalformedRouteLines()
{
    ModulationMatrix matrix;
    matrix.addRoute ({ Source::modEnv, Destination::filterCutoff, 0.4f, false });
    const auto originalRoutes = matrix.getRoutes();

    if (matrix.deserialize ("schema=1\nroutes=1\n0,1,0.5\n"))
    {
        std::cerr << "Expected malformed route with missing field to fail deserialize.\n";
        return false;
    }

    if (matrix.deserialize ("schema=1\nroutes=1\n0,1,0.5,1,extra\n"))
    {
        std::cerr << "Expected malformed route with extra field to fail deserialize.\n";
        return false;
    }

    if (matrix.deserialize ("schema=1\nroutes=1\n0,1,not-a-float,1\n"))
    {
        std::cerr << "Expected malformed float to fail deserialize.\n";
        return false;
    }

    if (! routesEqual (matrix.getRoutes(), originalRoutes))
    {
        std::cerr << "Malformed route lines should not mutate routes.\n";
        return false;
    }

    return true;
}

bool testDeserializeRejectsOutOfRangeEnums()
{
    ModulationMatrix matrix;
    matrix.addRoute ({ Source::ampEnv, Destination::amp, 1.0f, false });
    const auto originalRoutes = matrix.getRoutes();

    if (matrix.deserialize ("schema=1\nroutes=1\n-1,0,0.1,0\n"))
    {
        std::cerr << "Expected negative source enum to fail deserialize.\n";
        return false;
    }

    if (matrix.deserialize ("schema=1\nroutes=1\n99,0,0.1,0\n"))
    {
        std::cerr << "Expected out-of-range source enum to fail deserialize.\n";
        return false;
    }

    if (matrix.deserialize ("schema=1\nroutes=1\n0,-1,0.1,0\n"))
    {
        std::cerr << "Expected negative destination enum to fail deserialize.\n";
        return false;
    }

    if (matrix.deserialize ("schema=1\nroutes=1\n0,99,0.1,0\n"))
    {
        std::cerr << "Expected out-of-range destination enum to fail deserialize.\n";
        return false;
    }

    if (! routesEqual (matrix.getRoutes(), originalRoutes))
    {
        std::cerr << "Out-of-range enums should not mutate routes.\n";
        return false;
    }

    return true;
}
} // namespace

int main()
{
    if (! testDepthScaling())
        return 1;

    if (! testBipolarVsUnipolar())
        return 1;

    if (! testTempoSyncRateAccuracy())
        return 1;

    if (! testSerializationRoundTrip())
        return 1;

    if (! testDeserializeRejectsMalformedHeader())
        return 1;

    if (! testDeserializeRejectsMismatchedRouteCount())
        return 1;

    if (! testDeserializeRejectsMalformedRouteLines())
        return 1;

    if (! testDeserializeRejectsOutOfRangeEnums())
        return 1;

    std::cout << "Modulation tests passed\n";
    return 0;
}
