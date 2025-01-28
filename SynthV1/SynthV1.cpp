// Include required libraries
#include "daisy_pod.h"
#include "daisysp.h"

// Use library namespaces
using namespace daisy;
using namespace daisysp;

// Create objects
DaisyPod pod;
Oscillator osc;

// Create and define necessary variables
float noteFreq = 440.0;
float sig;

// Audio callback function to process each block of audio
static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    // Loop through each audio sample in the block
    for(size_t i = 0; i < size; i += 2)
    {
        // Set the frequency of the oscillator to the current note frequency
        osc.SetFreq(noteFreq);
        // Generate the audio signal for this block
        sig = osc.Process();
        // left out
        out[i] = sig;
        // right out
        out[i + 1] = sig;
    }
}
// Main program entry point
int main(void)
{
    float samplerate;

    // Daisy Pod initialisation
    pod.Init();
    pod.SetAudioBlockSize(4);
    samplerate = pod.AudioSampleRate();

    // Oscilator configurations
    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(noteFreq);
    osc.SetAmp(0.1f);

    // Start the ADC (Analog-to-Digital Converter) processing
    pod.StartAdc();
    // Start the audio processing with the defined callback function
    pod.StartAudio(AudioCallback);

    // Enter an infinite loop to keep the program running
    while (1) {}
}

    
