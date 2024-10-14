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

// check which fluidsynth version is in use

#if (FLUIDSYNTH_VERSION_MAJOR > 1 && FLUIDSYNTH_VERSION_MINOR > 1 && FLUIDSYNTH_VERSION_MICRO > 2)
#define USE_FLUID_API 2
#elif (FLUIDSYNTH_VERSION_MAJOR > 1 && FLUIDSYNTH_VERSION_MINOR > 2)
#define USE_FLUID_API 2
#else
#define USE_FLUID_API 1
#endif

/****************************************************************
 ** class XSynth
 **
 ** create a fluidsynth instance and load sondfont
 */

XSynth::XSynth() : cents{} {
    sf_id = -1;
    adriver = NULL;
    mdriver = NULL;
    synth = NULL;
    settings = NULL;

    for(int i = 0; i < 16; i++) {
        channel_instrument[i] = i;
        channel_tuning_map.emplace(i, 1);
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

    volume_level = 0.2;
    scala_size = 0;
    init_tuning_maps();
};

XSynth::~XSynth() {
    delete_envelope();
    unload_synth();
    tuning_map.clear();
    channel_tuning_map.clear();
    scala_ratios.clear();
};

void XSynth::setup(unsigned int SampleRate, const char *instance_name) {

#if FLUIDSYNTH_VERSION_MAJOR > 1
    const char* driver[] = { "jack", NULL };
    fluid_audio_driver_register(driver);
#endif
    settings = new_fluid_settings();
    fluid_settings_setnum(settings, "synth.sample-rate", SampleRate);
    fluid_settings_setstr(settings, "audio.driver", "jack");
    fluid_settings_setstr(settings, "audio.jack.id", instance_name);
    fluid_settings_setint(settings, "audio.jack.autoconnect", 1);
    fluid_settings_setstr(settings, "midi.driver", "jack");
    fluid_settings_setstr(settings, "midi.jack.id", instance_name);
}

void XSynth::setup_scala_tuning() {
    double val = 0.0;
    double oc = 1.0;
    for (unsigned int i = 0; i < 128; i++) {
        val = 1200.0 * std::log2(scala_ratios[i % scala_size] * oc);
        cents[i] = val;
        if (i % scala_size == scala_size-1) {
            oc *=2;
        }
    }
    fluid_synth_activate_key_tuning(synth, 0, 2, "scala", cents, 1);
}

void XSynth::just_intonation() {
    double steps [12] = {1.0, 1.06667, 1.125, 1.2, 1.25, 1.33333, 1.40625, 1.5, 1.6, 1.66667, 1.75, 1.875};
    double val = 0.0;
    double oc = 1.0;
    for (unsigned int i = 0; i < 128; i++) {
        val = 1200.0 * std::log2(steps[i % 12] * oc);
        cents[i] = val;
        if (i % 12 == 11) {
            oc *=2;
        }
    }
}

void XSynth::create_tuning_scala(double cent) {
    double val = 0.0;
    for (unsigned int i = 0; i < 128; i++) {
        cents[i] = val;
        val += cent;
    }
}

void XSynth::init_tuning_maps() {
    for (unsigned int i = 10; i < 24; i++) {
        std::string key = std::to_string(i)+"edo";
        double step = (100.0/double(i))*12.0;
        tuning_map.emplace(key,step);
    }
}

void XSynth::setup_key_tunnings() {
    just_intonation();
    int i = 0;
    fluid_synth_activate_key_tuning(synth, 0, i, "ji", cents, 1);
    i = 1;
    create_tuning_scala(100.0);
    fluid_synth_activate_key_tuning(synth, 0, i, "12edo", cents, 1);
   /* for (const auto& [key, steps] : tuning_map) {
        create_tuning_scala(steps);
        fluid_synth_activate_key_tuning(synth, 0, i, key.c_str(), cents, 1);
        i++;
    }*/
}

void XSynth::activate_tuning_for_channel(int channel, int set) {
    channel_tuning_map[channel] = set;
    fluid_synth_activate_tuning(synth, channel, 0, set, 1);
}

void XSynth::activate_tunning_for_all_channel(int set) {
    for(int i = 0; i < 16; i++) {
        channel_tuning_map[i] = set;
        fluid_synth_activate_tuning(synth, i, 0, set, 1);
    }
}

void XSynth::setup_tunnings_for_channelemap() {
    for(int i = 0; i < 16; i++) {
        fluid_synth_activate_tuning(synth, i, 0, channel_tuning_map[i], 1);
    }
}

int XSynth::get_tuning_for_channel(int channel) {
    return channel_tuning_map[channel];
}

void XSynth::setup_channel_tuning(int channel, int set) {
    channel_tuning_map[channel] = set;
}

void XSynth::delete_envelope() {
#if FLUIDSYNTH_VERSION_MAJOR < 2
    fluid_mod_delete(amod);
    fluid_mod_delete(dmod);
    fluid_mod_delete(smod);
    fluid_mod_delete(rmod);
    fluid_mod_delete(qmod);
    fluid_mod_delete(fmod);
#else
    delete_fluid_mod(amod);
    delete_fluid_mod(dmod);
    delete_fluid_mod(smod);
    delete_fluid_mod(rmod);
    delete_fluid_mod(qmod);
    delete_fluid_mod(fmod);
#endif
}

void XSynth::setup_envelope() {
#if FLUIDSYNTH_VERSION_MAJOR < 2
    amod = fluid_mod_new();
    fluid_mod_set_source1(amod, 73, // MIDI CC 73 Attack time
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(amod, 0, 0);
    fluid_mod_set_dest(amod, GEN_VOLENVATTACK);
    fluid_mod_set_amount(amod, 20000.0f);

    dmod = fluid_mod_new();
    fluid_mod_set_source1(dmod, 75, // MIDI CC 75 Decay Time
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(dmod, 0, 0);
    fluid_mod_set_dest(dmod, GEN_VOLENVDECAY);
    fluid_mod_set_amount(dmod, 20000.0f);

    smod = fluid_mod_new();
    fluid_mod_set_source1(smod, 77, // MIDI CC 77 (Sustain Time)
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_CONCAVE | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(smod, 0, 0);
    fluid_mod_set_dest(smod, GEN_VOLENVSUSTAIN);
    fluid_mod_set_amount(smod, 1000.0f);

    rmod = fluid_mod_new();
    fluid_mod_set_source1(rmod, 72, // MIDI CC 72 Release Time
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(rmod, 0, 0);
    fluid_mod_set_dest(rmod, GEN_VOLENVRELEASE);
    fluid_mod_set_amount(rmod, 20000.0f);

    qmod = fluid_mod_new();
    fluid_mod_set_source1(qmod, 71, // MIDI CC 71 Timbre (Resonance)
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_CONCAVE | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(qmod, 0, 0);
    fluid_mod_set_dest(qmod, GEN_FILTERQ);
    fluid_mod_set_amount(qmod, 960.0f);

    fmod = fluid_mod_new();
    fluid_mod_set_source1(fmod, 74, // MIDI CC 74 Brightness (Cutoff)
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(fmod, 0, 0);
    fluid_mod_set_dest(fmod, GEN_FILTERFC);
    fluid_mod_set_amount(fmod, -2400.0f);
#else
    amod = new_fluid_mod();
    fluid_mod_set_source1(amod, 73, // MIDI CC 73 Attack time
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(amod, 0, 0);
    fluid_mod_set_dest(amod, GEN_VOLENVATTACK);
    fluid_mod_set_amount(amod, 20000.0f);
    fluid_synth_add_default_mod(synth, amod, FLUID_SYNTH_ADD);

    dmod = new_fluid_mod();
    fluid_mod_set_source1(dmod, 75, // MIDI CC 75 Decay Time
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(dmod, 0, 0);
    fluid_mod_set_dest(dmod, GEN_VOLENVDECAY);
    fluid_mod_set_amount(dmod, 20000.0f);
    fluid_synth_add_default_mod(synth, dmod, FLUID_SYNTH_ADD);

    smod = new_fluid_mod();
    fluid_mod_set_source1(smod, 77, // MIDI CC 77 (Sustain Time)
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_CONCAVE | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(smod, 0, 0);
    fluid_mod_set_dest(smod, GEN_VOLENVSUSTAIN);
    fluid_mod_set_amount(smod, 1000.0f);
    fluid_synth_add_default_mod(synth, smod, FLUID_SYNTH_ADD);

    rmod = new_fluid_mod();
    fluid_mod_set_source1(rmod, 72, // MIDI CC 72 Release Time
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(rmod, 0, 0);
    fluid_mod_set_dest(rmod, GEN_VOLENVRELEASE);
    fluid_mod_set_amount(rmod, 20000.0f);
    fluid_synth_add_default_mod(synth, rmod, FLUID_SYNTH_ADD);

    qmod = new_fluid_mod();
    fluid_mod_set_source1(qmod, 71, // MIDI CC 71 Timbre
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_CONCAVE | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(qmod, 0, 0);
    fluid_mod_set_dest(qmod, GEN_FILTERQ);
    fluid_mod_set_amount(qmod, 960.0f);
    fluid_synth_add_default_mod(synth, qmod, FLUID_SYNTH_ADD);

    fmod = new_fluid_mod();
    fluid_mod_set_source1(fmod, 74, // MIDI CC 74 Brightness
        FLUID_MOD_CC | FLUID_MOD_UNIPOLAR | FLUID_MOD_LINEAR | FLUID_MOD_POSITIVE);
    fluid_mod_set_source2(fmod, 0, 0);
    fluid_mod_set_dest(fmod, GEN_FILTERFC);
    fluid_mod_set_amount(fmod, -2400.0f);
    fluid_synth_add_default_mod(synth, fmod, FLUID_SYNTH_ADD);
#endif
}

void XSynth::reset_modulators() {
    if (!synth) return;
    // set modulators for all channels to zero
    for (int i = 0; i<16; i++) {
        fluid_synth_cc(synth, i, 73, 0);
        fluid_synth_cc(synth, i, 75, 0);
        fluid_synth_cc(synth, i, 77, 0);
        fluid_synth_cc(synth, i, 72, 0);
        fluid_synth_cc(synth, i, 71, 0);
        fluid_synth_cc(synth, i, 74, 0);
    }
}

void XSynth::init_synth() {
    synth = new_fluid_synth(settings);
    adriver = new_fluid_audio_driver(settings, synth);
    mdriver = new_fluid_midi_driver(settings, fluid_synth_handle_midi_event, synth);
    volume_level = fluid_synth_get_gain(synth);
    setup_key_tunnings();
    if (scala_size) setup_scala_tuning();
    setup_tunnings_for_channelemap();
    setup_envelope();
    reset_modulators();
}

int XSynth::synth_send_cc(int channel, int num, int value) {
    if (!synth) return -1;
    return fluid_synth_cc(synth, channel, num, value);
}

int XSynth::load_soundfont(const char *path) {
    if (sf_id != -1) fluid_synth_sfunload(synth, sf_id, 0);
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
    fluid_sfont_t * sfont = fluid_synth_get_sfont_by_id(synth, sf_id);
    int offset = fluid_synth_get_bank_offset(synth, sf_id);

    if(sfont == NULL) {
        fprintf(stderr, "inst: invalid font number\n");
        return ;
    }

#if FLUIDSYNTH_VERSION_MAJOR < 2
    fluid_preset_t preset;
    sfont->iteration_start(sfont);
    while ((sfont->iteration_next(sfont, &preset)) != 0) {
        char inst[100];
        snprintf(inst, 100, "%03d %03d %s", preset.get_banknum(&preset) + offset,
                        preset.get_num(&preset),preset.get_name(&preset));
        instruments.push_back(inst);
    }
#else
    fluid_preset_t *preset;
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
        if ((unsigned int)channel_instrument[i] > instruments.size()) continue;
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
    if (!preset) return -1;
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
#if USE_FLUID_API == 1
        fluid_synth_set_reverb_on(synth, on);
#else
        fluid_synth_reverb_on(synth, -1, on);
#endif
        set_reverb_levels();
    }
}

void XSynth::set_reverb_levels() {
    if (synth) {
#if USE_FLUID_API == 1
        fluid_synth_set_reverb (synth, reverb_roomsize, reverb_damp,
                                        reverb_width, reverb_level);
#else
        fluid_synth_set_reverb_group_damp(synth, -1, reverb_damp);
        fluid_synth_set_reverb_group_level(synth, -1, reverb_level);
        fluid_synth_set_reverb_group_roomsize(synth, -1, reverb_roomsize);
        fluid_synth_set_reverb_group_width(synth, -1, reverb_width);
#endif
    }
}

void XSynth::set_chorus_on(int on) {
    if (synth) {
#if USE_FLUID_API == 1
        fluid_synth_set_chorus_on(synth, on);
#else
        fluid_synth_chorus_on(synth, -1, on);
#endif
        set_chorus_levels();
    }
}

void XSynth::set_chorus_levels() {
    if (synth) {
#if USE_FLUID_API == 1
        fluid_synth_set_chorus (synth, chorus_voices, chorus_level,
                            chorus_speed, chorus_depth, chorus_type);
#else
        fluid_synth_set_chorus_group_depth(synth, -1, chorus_depth);
        fluid_synth_set_chorus_group_level(synth, -1, chorus_level);
        fluid_synth_set_chorus_group_nr(synth, -1, chorus_voices);
        fluid_synth_set_chorus_group_speed(synth, -1, chorus_speed);
        fluid_synth_set_chorus_group_type(synth, -1, chorus_type);
        
#endif
    }
}

void XSynth::set_channel_pressure(int channel, int value) {
    if (synth) {
        fluid_synth_channel_pressure(synth, channel, value);
    }
}

void XSynth::set_gain() {
    if (synth) {
        fluid_synth_set_gain(synth, volume_level);
    }
}

void XSynth::panic() {
    if (synth) {
        fluid_synth_all_sounds_off(synth, -1);
    }
}

void XSynth::unload_synth() {
    if (sf_id != -1) {
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
        for(int i = 0; i < 16; i++) {
            fluid_synth_deactivate_tuning(synth, i, 1);
        }
        delete_fluid_synth(synth);
        synth = NULL;
    }
    if (settings) {
        delete_fluid_settings(settings);
        settings = NULL;
    }
}

} // namespace xsynth
