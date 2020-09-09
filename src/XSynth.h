/*
 *                           0BSD 
 * 
 *                    BSD Zero Clause License
 * 
 *  Copyright (c) 2020 Hermann Meyer
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.

 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <fluidsynth.h>
#include <vector>
#include <string>

#pragma once

#ifndef XSYNTH_H
#define XSYNTH_H


namespace xsynth {


/****************************************************************
 ** class XSynth
 **
 ** create a fluidsynth instance and load sondfont
 */

class XSynth {
private:
    fluid_settings_t* settings;
    fluid_synth_t* synth;
    fluid_audio_driver_t* adriver;
    fluid_midi_driver_t* mdriver;
    int sf_id;

public:
    XSynth();
    ~XSynth();

    std::vector<std::string> instruments;
    int reverb_on;
    double reverb_level;
    double reverb_width;
    double reverb_damp;
    double reverb_roomsize;
    int chorus_on;
    int chorus_type;
    double chorus_depth;
    double chorus_speed;
    double chorus_level;
    int chorus_voices;

    void setup(unsigned int SampleRate);
    void init_synth();
    int synth_is_active() {return synth ? 1 : 0;}
    int load_soundfont(const char *path);
    void print_soundfont();

    void set_reverb_on(int on);
    void set_reverb_levels();

    void set_chorus_on(int on);
    void set_chorus_levels();

    void set_channel_pressure(int channel, int value);

    void panic();
    void unload_synth();
};

} // namespace xsynth

#endif //XSYNTH_H_
