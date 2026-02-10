#include <cmath>
#include <iostream>

#include "../src/dsp/SimpleVoice.h"

int main()
{
    secretsynth::dsp::SimpleVoice voice;
    voice.prepare (48000.0);
    voice.setFrequency (440.0f);

    const auto firstSample = voice.renderSample();
    if (std::abs (firstSample) > 1.0e-6f)
    {
        std::cerr << "Expected first sample to start at 0, got " << firstSample << '\n';
        return 1;
    }

    auto peak = 0.0f;
    for (int i = 0; i < 480; ++i)
        peak = std::max (peak, std::abs (voice.renderSample()));

    if (peak < 0.1f)
    {
        std::cerr << "Expected oscillator output to produce audible amplitude, got peak " << peak << '\n';
        return 1;
    }

    std::cout << "SimpleVoice test passed\n";
    return 0;
}
