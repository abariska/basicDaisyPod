#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisyPod pod;
static Oscillator osc;
static AdEnv ad;
static Parameter  p_freq;
static Parameter p_release;

float freq;
float adRelease;
bool adCycle;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();


    if(pod.button1.RisingEdge() || (adCycle && !ad.IsRunning()))
    {
        ad.Trigger();
    }

    // Button 2 set to enable/disable envelop cycling
    if(pod.button2.RisingEdge())
    {
        adCycle = !adCycle;
    }   

    for(size_t i = 0; i < size; i += 2)
    {
        float sig;
        float adOut;

        freq = p_freq.Process();
        osc.SetFreq(freq);
        adRelease = p_release.Process();
        ad.SetTime(ADENV_SEG_DECAY, adRelease); // Set decay parameter from knob2
        adOut = ad.Process(); // Include AD changes into the audio processing
        sig = osc.Process();
        sig *= adOut;
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
    osc.SetFreq(880.0f);
    osc.SetWaveform(osc.WAVE_TRI);
    osc.SetAmp(0.1f);

    // Set initial Attack/Decay values
    ad.Init(samplerate);
    ad.SetTime(ADENV_SEG_ATTACK, 0.01f);
    ad.SetTime(ADENV_SEG_DECAY, 1.0f);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    // Define knob's parameters
    p_freq.Init(pod.knob1, 50.0f, 5000.0f, Parameter::LOGARITHMIC);
    p_release.Init(pod.knob2, 0.01f, 0.7f, Parameter::LINEAR);

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while (1) {}
}

