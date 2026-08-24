// Standalone diagnostic: exercises BroadcastCompressor / DownwardExpander directly,
// without needing the full plugin (JucePlugin_* macros) built.
#include <juce_dsp/juce_dsp.h>
#include "../Source/DSP/BroadcastCompressor.h"
#include "../Source/DSP/DownwardExpander.h"
#include <iostream>
#include <cmath>

// Pass 6: with a genuinely all-zero (silent) buffer, does changing a stage's
// parameter (e.g. Density) while no signal is present produce any nonzero
// gain-reduction reading? It should not - GR against silence must be 0
// regardless of where the knob is set.
static void probeSilenceVsParameterChange()
{
    constexpr double sr = 44100.0;
    constexpr int blockSize = 512;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };

    BroadcastCompressor comp;
    comp.prepare (spec);

    DownwardExpander gate;
    gate.prepare (spec);
    gate.setThresholdDb (-55.0f);

    std::cout << "\n--- silence-vs-parameter-drag probe ---\n";
    for (float density = 0.0f; density <= 1.0f; density += 0.1f)
    {
        comp.setDensity01 (density);
        gate.setRatio (density * 99.0f + 1.0f); // sweep gate ratio too, arbitrary

        juce::AudioBuffer<float> buf (2, blockSize);
        buf.clear(); // exact digital silence

        gate.process (buf);
        comp.process (buf);

        std::cout << "density=" << density
                   << " densGr=" << comp.getDensityGrDb()
                   << " gateGr=" << gate.getLastReductionDb()
                   << " bufferAllZero=" << (buf.getMagnitude (0, buf.getNumSamples()) == 0.0f ? "yes" : "NO")
                   << "\n";
    }
}

// After the noise-floor fix: confirm silence still stays at 0 regardless of ratio,
// AND that genuine quiet program material (e.g. -70dBFS room tone) still gets
// gated/expanded correctly (not accidentally zeroed out by the same fix).
static void probeQuietButRealSignal()
{
    constexpr double sr = 44100.0;
    constexpr int blockSize = 512;
    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };

    DownwardExpander gate;
    gate.prepare (spec);
    gate.setThresholdDb (-55.0f);
    gate.setRatio (50.0f);

    std::cout << "\n--- quiet-but-real signal probe (-70dBFS tone, well below -55 threshold) ---\n";
    double phase = 0.0;
    for (int block = 0; block < 60; ++block)
    {
        juce::AudioBuffer<float> buf (2, blockSize);
        const float amp = 0.000316f; // ~ -70 dBFS
        for (int i = 0; i < blockSize; ++i)
        {
            const float s = amp * (float) std::sin (phase);
            phase += 2.0 * M_PI * 440.0 / sr;
            buf.setSample (0, i, s);
            buf.setSample (1, i, s);
        }
        gate.process (buf);
        if (block % 10 == 0 || block == 59)
            std::cout << "block " << block << " gateGr=" << gate.getLastReductionDb() << "\n";
    }
}

int main()
{
    probeSilenceVsParameterChange();
    probeQuietButRealSignal();
    return 0;
}
