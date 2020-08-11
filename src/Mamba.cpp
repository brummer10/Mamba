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


#include "Mamba.h"


namespace mamba {


/****************************************************************
 ** class MidiMessenger
 **
 ** create, collect and send all midi events to jack_midi out buffer
 */

MidiMessenger::MidiMessenger() {
    channel = 0;
    for (int i = 0; i < max_midi_cc_cnt; i++) {
        send_cc[i] = false;
    }
}

int MidiMessenger::next(int i) const {
    while (++i < max_midi_cc_cnt) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            return i;
        }
    }
    return -1;
}

void MidiMessenger::fill(unsigned char *midi_send, int i) {
    if (size(i) == 3) {
        midi_send[2] =  bg_num[i];
    }
    midi_send[1] = pg_num[i];    // program value
    midi_send[0] = cc_num[i];    // controller+ channel
    send_cc[i].store(false, std::memory_order_release);
}

bool MidiMessenger::send_midi_cc(int _cc, int _pg, int _bgn, int _num) {
    _cc |=channel;
    for(int i = 0; i < max_midi_cc_cnt; i++) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            if (cc_num[i] == _cc && pg_num[i] == _pg &&
                bg_num[i] == _bgn && me_num[i] == _num)
                return true;
        } else if (!send_cc[i].load(std::memory_order_acquire)) {
            cc_num[i] = _cc;
            pg_num[i] = _pg;
            bg_num[i] = _bgn;
            me_num[i] = _num;
            send_cc[i].store(true, std::memory_order_release);
            return true;
        }
    }
    return false;
}


/****************************************************************
 ** class MidiLoad
 **
 ** load data from midi file
 ** 
 */

MidiLoad::MidiLoad() {
    smf = NULL;
    smf_event = NULL;
}

MidiLoad::~MidiLoad() {
    if (smf) smf_delete(smf);
}

void MidiLoad::load_from_file(std::vector<MidiEvent> *play, const char* file_name) {
    smf = smf_new();
    smf = smf_load(file_name);
    play->clear();
    while ((smf_event = smf_get_next_event(smf)) !=NULL) {
        if (smf_event_is_metadata(smf_event)) continue;
        ev = {{smf_event->midi_buffer[0], smf_event->midi_buffer[1], smf_event->midi_buffer[2]},
                                        smf_event->midi_buffer_length, smf_event->time_seconds};
        play->push_back(ev);
    }
    if (smf) smf_delete(smf);
    smf = NULL;
}

/****************************************************************
 ** class MidiSave
 **
 ** save data to midi file
 ** 
 */

MidiSave::MidiSave() {
    smf = smf_new();
    tracks.reserve(16);

    for (int i = 0; i < 16; i++) {
        tracks.push_back(smf_track_new());
        if (tracks[i] == NULL)
            exit(-1);
        smf_add_track(smf, tracks[i]);
    }    
}

MidiSave::~MidiSave() {
    for (int i = 0; i < 16; i++) {
        if (tracks[i] != NULL) {
            smf_track_delete(tracks[i]);
        }
    }
    if (smf) smf_delete(smf);
}

void MidiSave::reset_smf() {
    // delete all tracks
    for (int i = 0; i < 16; i++) {
        if (tracks[i] != NULL) {
            smf_track_delete(tracks[i]);
            tracks[i] = NULL;
        }
    }
    // clear vector
    tracks.clear();
    // delete smf
    if (smf) smf_delete(smf);
    smf = NULL;
    // init new smf instance
    smf = smf_new();
    // prefill vector with empty tracks and add tracks to smf
    for (int i = 0; i < 16; i++) {
        tracks.push_back(smf_track_new());
        if (tracks[i] == NULL)
            exit(-1);
        smf_add_track(smf, tracks[i]);
    }    
}

void MidiSave::save_to_file(std::vector<MidiEvent> *play, const char* file_name) {
    for(std::vector<MidiEvent>::const_iterator i = play->begin(); i != play->end(); ++i) {
        smf_event = smf_event_new_from_pointer((void*)(*i).buffer, (*i).num);
        if (smf_event == NULL) {
            continue;
        }

        if(smf_event->midi_buffer_length < 1) continue;
        channel = smf_event->midi_buffer[0] & 0x0F;

        smf_track_add_event_seconds(tracks[channel], smf_event,(*i).deltaTime);
    }

    smf_rewind(smf);

    for (int i = 0; i < 16; i++) {
        if (tracks[i] != NULL && tracks[i]->number_of_events == 0) {
            smf_track_delete(tracks[i]);
            tracks[i] = NULL;
        }
    }

    if (smf->number_of_tracks != 0) {
        if (smf_save(smf, file_name)) {
            fprintf( stderr, "Could not save to file '%s'.\n", file_name);
        }
    }
    reset_smf();
}


/****************************************************************
 ** class MidiRecord
 **
 ** record the keyboard input in a extra thread
 ** 
 */

MidiRecord::MidiRecord() 
    : _execute(false) {
    st = NULL;
}

MidiRecord::~MidiRecord() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
}

void MidiRecord::stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) {
        cv.notify_one();
        _thd.join();
    }
}

void MidiRecord::start() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this]() {
        while (_execute.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk);
            play.reserve(play.size() + st->size());
            
            for (unsigned int i=0; i<st->size(); i++) 
                play.push_back((*st)[i]); 
            st->clear();
        }
    });
}

bool MidiRecord::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}

} //  namespace mamba
