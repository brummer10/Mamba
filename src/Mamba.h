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


#include <smf.h>

#include <atomic>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cmath>

#pragma once

#ifndef MAMBA_H
#define MAMBA_H

namespace mamba {


/****************************************************************
 ** struct MidiEvent
 **
 ** store midi events in a vector
 ** 
 */

typedef struct {
    unsigned char buffer[3];
    int num;
    double deltaTime;
    double absoluteTime;
} MidiEvent;


/****************************************************************
 ** class MidiMessenger
 **
 ** create, collect and send all midi events to jack_midi out buffer
 */

class MidiMessenger {
private:
    static const int max_midi_cc_cnt = 25;
    std::atomic<bool> send_cc[max_midi_cc_cnt];
    int cc_num[max_midi_cc_cnt];
    int pg_num[max_midi_cc_cnt];
    int bg_num[max_midi_cc_cnt];
    int me_num[max_midi_cc_cnt];
public:
    MidiMessenger();
    int channel;
    bool send_midi_cc(int _cc, int _pg, int _bgn, int _num, bool have_channel) noexcept;
    int next(int i = -1) const noexcept;
    inline int size(int i)  const noexcept { return me_num[i]; }
    void fill(unsigned char *midi_send, int i) noexcept;
};


/****************************************************************
 ** class MidiLoad
 **
 ** load data from midi file
 ** 
 */

class MidiLoad {
private:
    smf_t *smf;
    smf_track_t *tracks;
    smf_event_t *smf_event;
    MidiEvent ev;
    void reset_smf();
    double deltaTime;

public:
     MidiLoad();
    ~MidiLoad();
    bool load_from_file(std::vector<MidiEvent> *play, int *song_bpm, const char* file_name);
};


/****************************************************************
 ** class MidiSave
 **
 ** save data to midi file
 ** 
 */

class MidiSave {
private:
    smf_t *smf;
    std::vector<smf_track_t*> tracks;
    smf_event_t *smf_event;
    int channel;
    void reset_smf();
    double get_max_time(std::vector<MidiEvent> *play) noexcept;

public:
    MidiSave();
    ~MidiSave();
    int freewheel;

    void save_to_file(std::vector<MidiEvent> *play, const char* file_name);
};


/****************************************************************
 ** class MidiRecord
 **
 ** record the keyboard input in a extra thread
 ** 
 */

class MidiRecord {
private:
    std::atomic<bool> _execute;
    std::thread _thd;
    std::mutex m;

public:
    MidiRecord();
    ~MidiRecord();
    int channel;
    void stop();
    void start();
    bool is_running() const noexcept;
    std::condition_variable cv;
    MidiEvent ev;
    std::vector<MidiEvent> *st;
    std::vector<MidiEvent> play[16];
};


} // namespace mamba

#endif //MAMBA_H_
