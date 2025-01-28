#include "daisy_pod.h"
#include "daisysp.h"
#include "daisysp-lgpl.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;
using namespace daisysp;

static DaisyPod pod;
static MidiUsbHandler midi;
static Oscillator osc;
static AdEnv ad;
static MoogLadder flt;
static ReverbSc rev;

static Parameter osc_volume, osc_tuning, filter_cutoff, filter_resonance, ad_attack, ad_decay, rev_feedback, rev_filter;

enum Menu { OSCILLATOR, FILTER, ENVELOPE, REVERB };
Menu current_menu = OSCILLATOR; 

int wave_type = 0;
float midiFreq, osc_freq, volume, tune;
float cutoff, resonance, filterEnv;
float attack, decay, drive;
float revFeedback, revFilter, revWetLevelL, revWetLevelR, revLevel;
bool isVolumeOn;
float oldk1, oldk2, k1, k2;
int inc;

void MidiUpdate();
void ProcessControls();
void GetReverbSample(float &inl, float &inr);

void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update);

void UpdateLeds() {
    // Задаємо кольори залежно від активного пункту меню
    switch (current_menu) {
        case OSCILLATOR:
            pod.led1.Set(0.3, 0, 0);
            break;
        case FILTER:
            pod.led1.Set(0, 0.3, 0);
            break;
        case ENVELOPE:
            pod.led1.Set(0, 0, 0.3);
            break;
        case REVERB:
            pod.led1.Set(0.3, 0, 0.3);
            break;
    }
    pod.UpdateLeds();
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    UpdateLeds();
    ProcessControls();
    MidiUpdate();

    for(size_t i = 0; i < size; i += 2)
    {
        float sig;

        osc.SetWaveform(wave_type);
        osc.SetFreq(midiFreq + (midiFreq * tune));
        sig = osc.Process();
        sig = flt.Process(sig);
        sig *= ad.Process();

        if (isVolumeOn == 0)
            sig = 0;
        
        float wet_l = sig;
        float wet_r = sig;
        GetReverbSample(wet_l, wet_r);

        out[i] = wet_l * revWetLevelL + sig * (1.0f - revWetLevelL);
        out[i + 1] = wet_r * revWetLevelR + sig * (1.0f - revWetLevelR);  
    }
}
int main(void)
{
    float samplerate;
    volume = 0.5f;
    tune = 0.0f;
    wave_type = 0;

    cutoff = 1000.0f;
    resonance = 0.0f;
    drive = 1.0f;

    attack = 0.01f;
    decay = 0.2f;

    revFeedback = 0.7f;
    revFilter = 1000.0f;
    revWetLevelL = 0.3;
    revWetLevelR = 0.3;
    revLevel = 0.0;

    oldk1 = oldk2 = 0;
    k1 = k2   = 0;

    pod.Init();
    pod.SetAudioBlockSize(4);
    samplerate = pod.AudioSampleRate();

    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);

    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_SQUARE);
    osc.SetAmp(0.1f);

    flt.Init(samplerate);
    flt.SetFreq(cutoff);
    flt.SetRes(resonance);

    ad.Init(samplerate);
    ad.SetTime(ADENV_SEG_ATTACK, attack);
    ad.SetTime(ADENV_SEG_DECAY, decay);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    rev.Init(samplerate);
    rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.85f);

    isVolumeOn = 1;

    osc_volume.Init(pod.knob1, 0, 1, osc_volume.LINEAR);
    osc_tuning.Init(pod.knob2, -0.1, 0.1, osc_tuning.LINEAR);
    filter_cutoff.Init(pod.knob1, 50, 10000, filter_cutoff.LOGARITHMIC);
    filter_resonance.Init(pod.knob2, 0, 1, filter_resonance.LINEAR);
    ad_attack.Init(pod.knob1, 0, 1, ad_attack.LINEAR);
    ad_decay.Init(pod.knob2, 0, 1, ad_decay.LINEAR);
    rev_feedback.Init(pod.knob1, 0, 1, rev_feedback.LINEAR);
    rev_filter.Init(pod.knob2, 200, 20000, rev_filter.LOGARITHMIC);

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while (1)
    {}
}
void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update)
{
    if(abs(oldVal - newVal) > 0.00005)
    {
        param = update;
    }
}
void ProcessButtons() {
    if (pod.button1.RisingEdge()) {
        if (current_menu == OSCILLATOR)
            current_menu = REVERB;
        else
            current_menu = static_cast<Menu>(static_cast<int>(current_menu) - 1);
    }
    if (pod.button2.RisingEdge()) {
        current_menu = static_cast<Menu>((static_cast<int>(current_menu) + 1) % 4);
    }
}
void ProcessKnobs() {
    k1 = pod.knob1.Process();
    k2 = pod.knob2.Process();

    switch (current_menu)
    {
        case OSCILLATOR:
            ConditionalParameter(oldk1, k1, volume, osc_volume.Process());
            ConditionalParameter(oldk2, k2, tune, osc_tuning.Process());
            osc.SetAmp(volume);

            break;
        case FILTER:
            ConditionalParameter(oldk1, k1, cutoff, filter_cutoff.Process());
            ConditionalParameter(oldk2, k2, resonance, filter_resonance.Process());
            flt.SetFreq(cutoff);
            flt.SetRes(resonance);
            break;
        case ENVELOPE:
            ConditionalParameter(oldk1, k1, attack, ad_attack.Process());
            ConditionalParameter(oldk2, k2, decay, ad_decay.Process());
            ad.SetTime(ADENV_SEG_ATTACK, attack);
            ad.SetTime(ADENV_SEG_DECAY, decay);
            break;
        case REVERB:
            ConditionalParameter(oldk1, k1, revFeedback, rev_feedback.Process());
            ConditionalParameter(oldk2, k2, revFilter, rev_filter.Process());
            rev.SetFeedback(revFeedback);
            rev.SetLpFreq(revFilter);
            break;
        default:
        break;
    }
    oldk1 = k1;
    oldk2 = k2;
}
void ProcessEncoder() {
    inc = 0;
    float incMultiplier = 1;
    float maxEncoderValue;
    float minEncoderValue;
    
    if(pod.encoder.RisingEdge())
        isVolumeOn = !isVolumeOn;

    switch (current_menu)
    {
        case OSCILLATOR:
            incMultiplier = 1;
            minEncoderValue = 0;
            maxEncoderValue = 7;
            inc = pod.encoder.Increment();
            wave_type += inc * incMultiplier;
            if (wave_type > maxEncoderValue)
                wave_type = maxEncoderValue;
            else if(wave_type < minEncoderValue)
                wave_type = minEncoderValue;
            osc.SetWaveform(wave_type);
        case FILTER:
            // drive = pod.encoder.Increment();
        case ENVELOPE:
            // envelope_level = pod.encoder.Increment();
        case REVERB:
            incMultiplier = 0.1;
            minEncoderValue = 0;
            maxEncoderValue = 1;
            inc = pod.encoder.Increment();
            revLevel += inc * incMultiplier;
            if (revLevel > maxEncoderValue)
                revLevel = maxEncoderValue;
            else if(revLevel < minEncoderValue)
                revLevel = minEncoderValue;
        default:
        break;
    }
}

void GetReverbSample(float &inl, float &inr)
{
    float wetL, wetR;
    rev.Process(inl, inr, &wetL, &wetR);
    inl = wetL * revLevel;
    inr = wetR * revLevel;
}

void ProcessControls() {
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    ProcessButtons();
    ProcessEncoder();
    ProcessKnobs();

}
void MidiUpdate(){
    midi.Listen();
        while(midi.HasEvents())
        {
            auto msg = midi.PopEvent();
            switch(msg.type)
            {
                case NoteOn:
                {
                    ad.Trigger();
                    auto note_msg = msg.AsNoteOn();
                    if(note_msg.velocity != 0)
                        midiFreq = mtof(note_msg.note);     
                }
                break;
                default: break;
            }
        }
}
