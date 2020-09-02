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
};

XSynth::~XSynth() {
    if (sf_id != -1) fluid_synth_sfunload(synth, sf_id, 1);
    if (mdriver) delete_fluid_midi_driver(mdriver);
    if (adriver) delete_fluid_audio_driver(adriver);
    if (synth) delete_fluid_synth(synth);
    if (settings) delete_fluid_settings(settings);
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
    if(sf_id == -1) {
        return 1;
    }
    return 0;
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
