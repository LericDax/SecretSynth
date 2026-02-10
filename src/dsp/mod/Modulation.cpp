#include "Modulation.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace secretsynth::dsp::mod
{
namespace
{
constexpr float twoPi = 6.28318530717958647692f;

std::uint32_t secondsToSamples (float seconds, double sampleRate) noexcept
{
    return static_cast<std::uint32_t> (std::max (1.0, std::round (std::max (0.0f, seconds) * sampleRate)));
}
} // namespace

void AdsrEnvelope::setSampleRate (double newSampleRate) noexcept
{
    sampleRate = std::max (1.0, newSampleRate);
}

void AdsrEnvelope::setParameters (const Parameters& newParameters) noexcept
{
    parameters.attackSeconds = std::max (0.0f, newParameters.attackSeconds);
    parameters.decaySeconds = std::max (0.0f, newParameters.decaySeconds);
    parameters.sustainLevel = std::clamp (newParameters.sustainLevel, 0.0f, 1.0f);
    parameters.releaseSeconds = std::max (0.0f, newParameters.releaseSeconds);
}

void AdsrEnvelope::noteOn() noexcept
{
    stage = Stage::attack;
    stageSamplesProcessed = 0;
}

void AdsrEnvelope::noteOff() noexcept
{
    releaseStartValue = currentValue;
    stage = Stage::release;
    stageSamplesProcessed = 0;
}

void AdsrEnvelope::reset() noexcept
{
    stage = Stage::idle;
    currentValue = 0.0f;
    releaseStartValue = 0.0f;
    stageSamplesProcessed = 0;
}

float AdsrEnvelope::processSample() noexcept
{
    switch (stage)
    {
        case Stage::idle:
            currentValue = 0.0f;
            break;

        case Stage::attack:
        {
            const auto attackSamples = secondsToSamples (parameters.attackSeconds, sampleRate);
            const auto progress = static_cast<float> (stageSamplesProcessed) / static_cast<float> (attackSamples);
            currentValue = std::clamp (progress, 0.0f, 1.0f);

            if (++stageSamplesProcessed >= attackSamples)
            {
                stage = Stage::decay;
                stageSamplesProcessed = 0;
                currentValue = 1.0f;
            }
            break;
        }

        case Stage::decay:
        {
            const auto decaySamples = secondsToSamples (parameters.decaySeconds, sampleRate);
            const auto progress = static_cast<float> (stageSamplesProcessed) / static_cast<float> (decaySamples);
            currentValue = 1.0f + (parameters.sustainLevel - 1.0f) * std::clamp (progress, 0.0f, 1.0f);

            if (++stageSamplesProcessed >= decaySamples)
            {
                stage = Stage::sustain;
                currentValue = parameters.sustainLevel;
            }
            break;
        }

        case Stage::sustain:
            currentValue = parameters.sustainLevel;
            break;

        case Stage::release:
        {
            const auto releaseSamples = secondsToSamples (parameters.releaseSeconds, sampleRate);
            const auto progress = static_cast<float> (stageSamplesProcessed) / static_cast<float> (releaseSamples);
            currentValue = releaseStartValue + (0.0f - releaseStartValue) * std::clamp (progress, 0.0f, 1.0f);

            if (++stageSamplesProcessed >= releaseSamples)
            {
                stage = Stage::idle;
                currentValue = 0.0f;
            }
            break;
        }
    }

    return currentValue;
}

void Lfo::setSampleRate (double newSampleRate) noexcept
{
    sampleRate = std::max (1.0, newSampleRate);
}

void Lfo::setWaveform (Waveform newWaveform) noexcept
{
    waveform = newWaveform;
}

void Lfo::setRateHz (float newRateHz) noexcept
{
    rateHz = std::max (0.0f, newRateHz);
}

void Lfo::setRateMode (RateMode newMode) noexcept
{
    rateMode = newMode;
}

void Lfo::setTempoBpm (float newTempoBpm) noexcept
{
    tempoBpm = std::max (1.0f, newTempoBpm);
}

void Lfo::setSyncDivision (SyncDivision newDivision) noexcept
{
    syncDivision = newDivision;
}

void Lfo::reset() noexcept
{
    phase = 0.0f;
    currentValue = 0.0f;
}

float Lfo::processSample() noexcept
{
    const auto frequency = getCurrentFrequencyHz();
    const auto phaseIncrement = frequency / static_cast<float> (sampleRate);
    phase += phaseIncrement;
    phase -= std::floor (phase);

    if (waveform == Waveform::triangle)
    {
        const auto triangle = 4.0f * std::abs (phase - 0.5f) - 1.0f;
        currentValue = triangle;
    }
    else
    {
        currentValue = std::sin (twoPi * phase);
    }

    return currentValue;
}

float Lfo::getCurrentFrequencyHz() const noexcept
{
    if (rateMode == RateMode::tempoSync)
    {
        const auto beatsPerSecond = tempoBpm / 60.0f;
        return beatsPerSecond * syncDivisionToCyclesPerBeat (syncDivision);
    }

    return rateHz;
}

float Lfo::syncDivisionToCyclesPerBeat (SyncDivision division) noexcept
{
    switch (division)
    {
        case SyncDivision::whole: return 0.25f;
        case SyncDivision::half: return 0.5f;
        case SyncDivision::quarter: return 1.0f;
        case SyncDivision::eighth: return 2.0f;
        case SyncDivision::sixteenth: return 4.0f;
        case SyncDivision::eighthTriplet: return 3.0f;
    }

    return 1.0f;
}

void DestinationSmoother::setSampleRate (double newSampleRate) noexcept
{
    sampleRate = std::max (1.0, newSampleRate);
    updateCoefficient();
}

void DestinationSmoother::setSmoothingTimeSeconds (float newTimeSeconds) noexcept
{
    smoothingTimeSeconds = std::max (0.0f, newTimeSeconds);
    updateCoefficient();
}

void DestinationSmoother::reset (float initialValue) noexcept
{
    currentValue = initialValue;
}

float DestinationSmoother::processSample (float targetValue) noexcept
{
    currentValue += (targetValue - currentValue) * coefficient;
    return currentValue;
}

void DestinationSmoother::updateCoefficient() noexcept
{
    if (smoothingTimeSeconds <= 0.0f)
    {
        coefficient = 1.0f;
        return;
    }

    const auto tau = static_cast<double> (smoothingTimeSeconds);
    coefficient = static_cast<float> (1.0 - std::exp (-1.0 / (tau * sampleRate)));
}

void ModulationMatrix::setSampleRate (double newSampleRate) noexcept
{
    for (auto& smoother : smoothers)
        smoother.setSampleRate (newSampleRate);
}

void ModulationMatrix::setDestinationSmoothingTimeSeconds (float timeSeconds) noexcept
{
    for (auto& smoother : smoothers)
        smoother.setSmoothingTimeSeconds (timeSeconds);
}

void ModulationMatrix::clearRoutes() noexcept
{
    routes.clear();
}

void ModulationMatrix::addRoute (const Route& route)
{
    routes.push_back (route);
}

std::array<float, static_cast<std::size_t> (Destination::count)> ModulationMatrix::process (
    const std::array<float, static_cast<std::size_t> (Source::count)>& sourceValues) noexcept
{
    std::array<float, static_cast<std::size_t> (Destination::count)> destinations {};

    for (const auto& route : routes)
    {
        auto source = sourceValues[toIndex (route.source)];
        source = route.bipolar ? (source * 2.0f - 1.0f) : source;
        destinations[toIndex (route.destination)] += source * route.depth;
    }

    for (std::size_t i = 0; i < destinations.size(); ++i)
        destinations[i] = smoothers[i].processSample (destinations[i]);

    return destinations;
}

std::string ModulationMatrix::serialize() const
{
    std::ostringstream stream;
    stream << "schema=" << schemaVersion << "\n";
    stream << "routes=" << routes.size() << "\n";

    for (const auto& route : routes)
    {
        stream << static_cast<int> (route.source) << ","
               << static_cast<int> (route.destination) << ","
               << route.depth << ","
               << (route.bipolar ? 1 : 0) << "\n";
    }

    return stream.str();
}

bool ModulationMatrix::deserialize (std::string_view text)
{
    std::istringstream stream { std::string (text) };
    std::string line;

    if (! std::getline (stream, line) || line.rfind ("schema=", 0) != 0)
        return false;

    const auto parsedSchema = std::stoul (line.substr (7));
    if (parsedSchema != schemaVersion)
        return false;

    if (! std::getline (stream, line) || line.rfind ("routes=", 0) != 0)
        return false;

    const auto routeCount = static_cast<std::size_t> (std::stoul (line.substr (7)));

    std::vector<Route> parsedRoutes;
    parsedRoutes.reserve (routeCount);

    while (std::getline (stream, line) && ! line.empty())
    {
        std::istringstream lineStream (line);
        std::string token;
        std::array<std::string, 4> fields;

        for (std::size_t i = 0; i < fields.size(); ++i)
        {
            if (! std::getline (lineStream, token, ','))
                return false;
            fields[i] = token;
        }

        Route route;
        route.source = static_cast<Source> (std::stoi (fields[0]));
        route.destination = static_cast<Destination> (std::stoi (fields[1]));
        route.depth = std::stof (fields[2]);
        route.bipolar = std::stoi (fields[3]) != 0;
        parsedRoutes.push_back (route);
    }

    if (parsedRoutes.size() != routeCount)
        return false;

    routes = std::move (parsedRoutes);
    return true;
}

void ModulationEngine::setSampleRate (double newSampleRate) noexcept
{
    ampEnv.setSampleRate (newSampleRate);
    modEnv.setSampleRate (newSampleRate);
    lfo1.setSampleRate (newSampleRate);
    lfo2.setSampleRate (newSampleRate);
}

void ModulationEngine::reset() noexcept
{
    ampEnv.reset();
    modEnv.reset();
    lfo1.reset();
    lfo2.reset();
}
} // namespace secretsynth::dsp::mod
