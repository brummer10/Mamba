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
#include <ostream>
#include <iostream>

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

int MidiMessenger::next(int i) const noexcept {
    while (++i < max_midi_cc_cnt) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            return i;
        }
    }
    return -1;
}

void MidiMessenger::fill(unsigned char *midi_send, int i) noexcept {
    if (size(i) == 3) {
        midi_send[2] =  bg_num[i];
    }
    midi_send[1] = pg_num[i];    // program value
    midi_send[0] = cc_num[i];    // controller+ channel
    send_cc[i].store(false, std::memory_order_release);
}

bool MidiMessenger::send_midi_cc(uint8_t _cc, const uint8_t _pg, const uint8_t _bgn,
                                const uint8_t _num, const bool have_channel) noexcept {
    if (!have_channel && channel < 16) _cc |=channel;
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
    deltaTime = 0.0;
    absoluteTime = 0.0;
}

MidiLoad::~MidiLoad() {
    if (smf) smf_delete(smf);
}

bool MidiLoad::load_file(std::vector<MidiEvent> *play, int *song_bpm, const char* file_name) {
    smf = smf_new();
    deltaTime = 0.0;
    if(!(smf = smf_load(file_name))) return false;
    // fprintf(stderr, "ppqn = %i\n", smf->ppqn);
    // fprintf(stderr, "length = %f sec\n", smf_get_length_seconds(smf));
    // fprintf(stderr, "tracks %i\n", smf->number_of_tracks);
    *(song_bpm) = 120;
    int count = 0;
    int stamp = positions[positions.size()-1];
    while ((smf_event = smf_get_next_event(smf)) !=NULL) {
        if (smf_event_is_metadata(smf_event)) {
            // char *decoded = smf_event_decode(smf_event);
            // if (decoded ) fprintf(stderr,"%s\n", decoded);
            // Fetch Song BPM
            if (smf_event->midi_buffer[1]==0x51) {
                double mspqn = (smf_event->midi_buffer[3] << 16) + 
                    (smf_event->midi_buffer[4] << 8) + smf_event->midi_buffer[5];
                *(song_bpm) = round(60000000.0 / mspqn);
                 //fprintf(stderr,"SONG BPM: %i\n",*(song_bpm));
            } else {
                continue;
            }
        }
        // filter out 0xFF system reset message
        if ((smf_event->midi_buffer[0] == 0xff)) {
            continue;
        }
        ev = {{smf_event->midi_buffer[0], smf_event->midi_buffer[1], smf_event->midi_buffer[2]},
                                        smf_event->midi_buffer_length, smf_event->time_seconds - deltaTime,
                                                                    smf_event->time_seconds + absoluteTime};
        play->push_back(ev);
        deltaTime = smf_event->time_seconds;
        count++;
        //fprintf(stderr,"%d: %f seconds, %d pulses, %d delta pulses\n", smf_event->event_number,
        //    smf_event->time_seconds, smf_event->time_pulses, smf_event->delta_time_pulses);

    }
    positions.push_back(count+stamp);
    // time_pulse to seconds = time_pulse*(song_bpm x smf->ppqn)*(1/60)

    if (smf) smf_delete(smf);
    smf = NULL;
    return true;
}

bool MidiLoad::load_from_file(std::vector<MidiEvent> *play, int *song_bpm, const char* file_name) {
    play->clear();
    positions.clear();
    positions.push_back(0);
    absoluteTime = 0.0;
    return load_file(play, song_bpm, file_name);
}

bool MidiLoad::add_from_file(std::vector<MidiEvent> *play, int *song_bpm, const char* file_name) {
    if (!positions.size()) positions.push_back(0);
    if (play->size()) {
        const mamba::MidiEvent ev = play[0][play->size()-1];
        absoluteTime = ev.absoluteTime;
    } else {
        absoluteTime = 0.0;
    }
    return load_file(play, song_bpm, file_name);
}

void MidiLoad::remove_file(std::vector<MidiEvent> *play, int f) {
    if (!((int)positions.size()>f)) return;
    int stamp = positions[f];
    int stamp2 = positions[f+1];
    play[0].erase(play[0].begin()+stamp,play[0].begin()+stamp2);

    for(std::vector<int>::iterator i = positions.begin()+f+1;
                                    i != positions.end(); i++) {
        (*i) -= stamp2-stamp;
    }
    positions.erase(positions.begin()+f+1);
    
    double aTime = 0.0;
    for(std::vector<MidiEvent>::iterator i = play[0].begin();
                                    i != play[0].end(); ++i) {
        (*i).absoluteTime = (*i).deltaTime + aTime;
        aTime = (*i).absoluteTime;
    }
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
    tracks.reserve(16);
    for (int i = 0; i < 16; i++) {
        tracks.push_back(smf_track_new());
        if (tracks[i] == NULL)
            exit(-1);
        smf_add_track(smf, tracks[i]);
    }    
}

double MidiSave::get_max_time(std::vector<MidiEvent> *play) noexcept {
    double ret = 0;
    for (int j = 0; j<16;j++) {
        if (!play[j].size()) continue;
        const MidiEvent ev = play[j][play[j].size()-1];
        if (ev.absoluteTime > ret) ret = ev.absoluteTime;
    }
    return ret;
}

void MidiSave::save_to_file(std::vector<MidiEvent> *play, const char* file_name) {
    double t[16] = {0.0};
    double max_time = get_max_time(play);
    for (int j = 0; j<16;j++) {
        for(std::vector<MidiEvent>::const_iterator i = play[j].begin(); i != play[j].end(); ++i) {
            smf_event = smf_event_new_from_pointer((void*)(*i).buffer, (*i).num);

            if (smf_event == NULL) continue;
            if(smf_event->midi_buffer_length < 1) continue;

            channel = smf_event->midi_buffer[0] & 0x0F;

            smf_track_add_event_seconds(tracks[channel], smf_event,(*i).deltaTime + t[j]);
            t[j] += (*i).deltaTime;
            if (!freewheel) {
                if (t[j]<max_time && i == play[j].end()-1) {
                    i = play[j].begin();
                } 
                if ( t[j] >= max_time) {
                    i = play[j].end()-1;
                    if (t[j] > max_time) {
                        smf_event_remove_from_track(smf_event);
                    }
                }
            }
        }
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
    : _execute(false),
    is_sorted(false) {
    st = NULL;
    channel = 0;
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
            // wait for signal from jack that record vector is ready
            cv.wait(lk);
            // reserve space in play vector to push the recorded vector into
            play[channel].reserve(play[channel].size() + st->size());

            // push recorded vector to play vector
            for (unsigned int i=0; i<st->size(); i++) 
                play[channel].push_back((*st)[i]);
            st->clear();

            // sort vector ascending to absolute time in loop
            std::sort( play[channel].begin(), play[channel].end(),
                    [this](const MidiEvent& lhs, const MidiEvent& rhs) {
                if (lhs.absoluteTime > rhs.absoluteTime)
                    is_sorted.store(true, std::memory_order_release);
                return lhs.absoluteTime < rhs.absoluteTime;
            });
        }
        // when record stop, recalculate the delta time for sorted vector
        double aTime = 0.0;
        for(std::vector<MidiEvent>::iterator i = play[channel].begin();
                                        i != play[channel].end(); ++i) {
            (*i).deltaTime = (*i).absoluteTime - aTime;
            aTime = (*i).absoluteTime;
        }
    });
}

bool MidiRecord::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}

} //  namespace mamba
