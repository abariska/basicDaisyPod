#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisyPod pod;
static Oscillator osc;
static Parameter p_freq;

float sig;
float freq;

static void AudioCallback(AudioHandle::InterleavingInputBuffer in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t size)
{
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    for (size_t i = 0; i < size; i += 2)
    {
        // Apply parameter to oscillator freq
        float freq = p_freq.Process();

        osc.SetFreq(freq);
        sig = osc.Process();
        out[i] = sig;
        out[i + 1] = sig;
    }
}

int main(void)
{
    float samplerate;

    pod.Init();
    pod.SetAudioBlockSize(4);
    samplerate = pod.AudioSampleRate();

    osc.Init(samplerate);
    osc.SetWaveform(Oscillator::WAVE_TRI);
    osc.SetFreq(400.0);
    osc.SetAmp(0.1f);

    // Set parameter for knob1 
    p_freq.Init(pod.knob1, 100, 2000, Parameter::LOGARITHMIC);
    // Apply parameter to oscillator freq
    freq = p_freq.Process();

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while (1) {}
}
