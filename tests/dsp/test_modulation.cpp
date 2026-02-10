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

    std::cout << "Modulation tests passed\n";
    return 0;
}
