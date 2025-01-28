#include "daisy_pod.h"
#include "daisysp.h"
#include <stdio.h>

using namespace daisy;
using namespace daisysp;

static DaisyPod pod;
static Oscillator osc;
static AdEnv ad;
static Parameter  p_freq;
static Parameter p_release;

Color color;

static float volChange; // variable for Volume 
static float inc;
float freq;
float adRelease;
bool adCycle;
bool isVolumeOn; // Variable for muting audio

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

    if(pod.button2.RisingEdge())
    {
        adCycle = !adCycle;
    }
    freq = p_freq.Process();
    adRelease = p_release.Process();

    // Set encoder change point
    inc = pod.encoder.Increment();
    if(inc > 0)
        volChange += 0.02;
    else if(inc < 0)
        volChange -= 0.02;
    // Handle max and min encoder values
    if (volChange > 1)
        volChange = 1;
    else if(volChange < 0)
        volChange = 0;
    // Set mute states
    if(pod.encoder.RisingEdge())
        isVolumeOn = !isVolumeOn;

    for(size_t i = 0; i < size; i += 2)
    {
        float sig;
        float adOut;

        ad.SetTime(ADENV_SEG_DECAY, adRelease);
        adOut = ad.Process();
        osc.SetFreq(freq);
        sig = osc.Process();
        sig *= adOut;
        sig *= volChange;
        sig *= isVolumeOn; // Mute point
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

    volChange = 0.5;
    isVolumeOn = true;
    inc = 0;

    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_TRI);
    osc.SetAmp(0.1f);

    ad.Init(samplerate);
    ad.SetTime(ADENV_SEG_ATTACK, 0.01f);
    ad.SetTime(ADENV_SEG_DECAY, 1.0f);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    p_freq.Init(pod.knob1, 50.0f, 5000.0f, Parameter::LOGARITHMIC);
    p_release.Init(pod.knob2, 0.01f, 0.7f, Parameter::LINEAR);

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while (1) {}
}

