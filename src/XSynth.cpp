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


#include "XSynth.h"
#include <sstream>

namespace xsynth {


/****************************************************************
 ** class XSynth
 **
 ** create a fluidsynth instance and load sondfont
 */

XSynth::XSynth() {
    sf_id = -1;
    adriver = NULL;
    mdriver = NULL;
    synth = NULL;
    settings = NULL;

    for(int i = 0; i < 16; i++) {
        channel_instrument[i] = i;
    }

    reverb_on = 0;
    reverb_level = 0.7;
    reverb_width = 10.0;
    reverb_damp = 0.4;
    reverb_roomsize = 0.6;

    chorus_on = 0;
    chorus_type = 0;
    chorus_depth = 3.0;
    chorus_speed = 0.3;
    chorus_level = 3.0;
    chorus_voices = 3;
};

XSynth::~XSynth() {
    unload_synth();
};

void XSynth::setup(unsigned int SampleRate) {
    settings = new_fluid_settings();
    fluid_settings_setnum(settings, "synth.sample-rate", SampleRate);
    fluid_settings_setstr(settings, "audio.driver", "jack");
    fluid_settings_setstr(settings, "audio.jack.id", "mamba");
    fluid_settings_setint(settings, "audio.jack.autoconnect", 1);
    fluid_settings_setstr(settings, "midi.driver", "jack");
    fluid_settings_setstr(settings, "midi.jack.id", "mamba");
}

void XSynth::init_synth() {
    synth = new_fluid_synth(settings);
    adriver = new_fluid_audio_driver(settings, synth);
    mdriver = new_fluid_midi_driver(settings, fluid_synth_handle_midi_event, synth);
}

int XSynth::load_soundfont(const char *path) {
    if (sf_id != -1) fluid_synth_sfunload(synth, sf_id, 1);
    sf_id = fluid_synth_sfload(synth, path, 1);
    if (sf_id == -1) {
        return 1;
    }
    if (reverb_on) set_reverb_on(reverb_on);
    if (chorus_on) set_chorus_on(chorus_on);
    print_soundfont();
    return 0;
}

void XSynth::print_soundfont() {
    instruments.clear();
    fluid_preset_t *preset;
    fluid_sfont_t * sfont = fluid_synth_get_sfont_by_id(synth, sf_id);
    int offset = fluid_synth_get_bank_offset(synth, sf_id);

    if(sfont == NULL) {
        fprintf(stderr, "inst: invalid font number\n");
        return ;
    }

#if FLUIDSYNTH_VERSION_MAJOR < 2
    sfont->iteration_start(sfont);
    while ((sfont->iteration_next(sfont, preset)) != 0) {
        char inst[100];
        snprintf(inst, 100, "%03d %03d %s", preset->get_banknum(preset) + offset,
                        preset->get_num(preset),preset->get_name(preset));
        instruments.push_back(inst);
    }
#else

    fluid_sfont_iteration_start(sfont);

    while((preset = fluid_sfont_iteration_next(sfont)) != NULL) {
        char inst[100];
        snprintf(inst, 100, "%03d %03d %s", fluid_preset_get_banknum(preset) + offset,
                        fluid_preset_get_num(preset),fluid_preset_get_name(preset));
        instruments.push_back(inst);
    }
#endif
    set_default_instruments();
}

void XSynth::set_default_instruments() {
    int bank = 0;
    int program = 0;
    for (unsigned int i = 0; i < 16; i++) {
        if (i >= instruments.size()) break;
        if (i == 9) continue;
        std::istringstream buf(instruments[channel_instrument[i]]);
        buf >> bank;
        buf >> program;
        fluid_synth_program_select (synth, i, sf_id, bank, program);
    }
}

void XSynth::set_instrument_on_channel(int channel, int i) {
    if (i >= (int)instruments.size()) return;
    if (channel >15) channel = 0;
    int bank = 0;
    int program = 0;
    std::istringstream buf(instruments[i]);
    buf >> bank;
    buf >> program;
    fluid_synth_program_select (synth, channel, sf_id, bank, program);
}

int XSynth::get_instrument_for_channel(int channel) {
    if (!synth) return -1;
    if (channel >15) channel = 0;
    fluid_preset_t *preset = fluid_synth_get_channel_preset(synth, channel);
    int offset = fluid_synth_get_bank_offset(synth, sf_id);
    char inst[100];
#if FLUIDSYNTH_VERSION_MAJOR < 2
    snprintf(inst, 100, "%03d %03d %s", preset->get_banknum(preset) + offset,
                        preset->get_num(preset),preset->get_name(preset));
#else
    snprintf(inst, 100, "%03d %03d %s", fluid_preset_get_banknum(preset) + offset,
                        fluid_preset_get_num(preset),fluid_preset_get_name(preset));
#endif
    std::string name = inst;
    int ret = 0;
    for(std::vector<std::string>::const_iterator i = instruments.begin(); i != instruments.end(); ++i) {
        if ((*i).find(name) != std::string::npos) {
            return ret;
        }
        ret++;
    }
    return -1;
}

void XSynth::set_reverb_on(int on) {
    if (synth) {
        fluid_synth_set_reverb_on(synth, on);
        set_reverb_levels();
    }
}

void XSynth::set_reverb_levels() {
    if (synth) {
        fluid_synth_set_reverb (synth, reverb_roomsize, reverb_damp,
                                        reverb_width, reverb_level);
    }
}

void XSynth::set_chorus_on(int on) {
    if (synth) {
        fluid_synth_set_chorus_on(synth, on);
        set_chorus_levels();
    }
}

void XSynth::set_chorus_levels() {
    if (synth) {
        fluid_synth_set_chorus (synth, chorus_voices, chorus_level,
                            chorus_speed, chorus_depth, chorus_type);
    }
}

void XSynth::set_channel_pressure(int channel, int value) {
    if (synth) {
        fluid_synth_channel_pressure(synth, channel, value);
    }
}

void XSynth::panic() {
    if (synth) {
        fluid_synth_all_sounds_off(synth, -1);
    }
}

void XSynth::unload_synth() {
    if (sf_id) {
        fluid_synth_sfunload(synth, sf_id, 0);
        sf_id = -1;
    }
    if (mdriver) {
        delete_fluid_midi_driver(mdriver);
        mdriver = NULL;
    }
    if (adriver) {
        delete_fluid_audio_driver(adriver);
        adriver = NULL;
    }
    if (synth) {
        delete_fluid_synth(synth);
        synth = NULL;
    }
    if (settings) {
        delete_fluid_settings(settings);
        settings = NULL;
    }
}

} // namespace xsynth
