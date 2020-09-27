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

#include <sigc++/sigc++.h>
#include <future>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "Mamba.h"


#pragma once

#ifndef XJACK_H
#define XJACK_H


namespace xjack {


/****************************************************************
 ** class MidiClockToBpm
 **
 ** calculate BPM from Midi Beat Clock events
 */

class MidiClockToBpm {
private:
    double time1;
    double time_diff;
    int collect;
    int collect_;
    double bpm;
    double bpm_new;
    unsigned int bpm_old;
    bool ret;

public:
    MidiClockToBpm();
    ~MidiClockToBpm() {}
    unsigned int rounded(float f);
    bool time_to_bpm(double time, unsigned int* bpm_);
};


/****************************************************************
 ** class XJack
 **
 ** Connect via MidiMessenger to jack_midi out
 ** pass all incoming midi events to jack_midi out.
 ** send all incomming midi events to the KeyBoard
 */

class XJack : public sigc::trackable {
private:
    mamba::MidiMessenger *mmessage;
    MidiClockToBpm mp;
    timespec ts1;
    jack_nframes_t event_count;
    jack_nframes_t stop;
    jack_nframes_t stopPlay[16];
    double deltaTime;
    double absoluteTime;
    jack_position_t current;
    jack_transport_state_t transport_state;
    unsigned int pos;
    unsigned int posPlay[16];

    inline void get_max_time_loop(std::vector<mamba::MidiEvent> *play, int *v);
    inline void record_midi(unsigned char* midi_send, unsigned int n, int i);
    inline void play_midi(void *buf, unsigned int n);
    inline void process_midi_out(void *buf, jack_nframes_t nframes);
    inline void process_midi_in(void* buf, void* out_buf, void *arg);
    static void jack_shutdown (void *arg);
    static int jack_xrun_callback(void *arg);
    static int jack_srate_callback(jack_nframes_t samplerate, void* arg);
    static int jack_buffersize_callback(jack_nframes_t nframes, void* arg);
    static int jack_process(jack_nframes_t nframes, void *arg);

public:
    XJack(mamba::MidiMessenger *mmessage);
    ~XJack();
    std::atomic<bool> transport_state_changed;
    std::atomic<int> transport_set;
    std::atomic<bool> bpm_changed;
    std::atomic<int> bpm_set;
    jack_client_t *client;
    jack_port_t *in_port;
    jack_port_t *out_port;
    jack_nframes_t start;
    jack_nframes_t startPlay[16];
    jack_nframes_t absoluteStart;
    std::string client_name;
    int init_jack();
    mamba::MidiRecord rec;
    std::vector<mamba::MidiEvent> store1;
    std::vector<mamba::MidiEvent> store2;
    std::vector<mamba::MidiEvent> *st;

    int record;
    int play;
    bool fresh_take;
    bool first_play;
    unsigned int SampleRate;
    double srms;
    double bpm_ratio;
    unsigned int bpm;

    sigc::signal<void > trigger_quit_by_jack;
    sigc::signal<void >& signal_trigger_quit_by_jack() { return trigger_quit_by_jack; }

    sigc::signal<void, int, int, bool> trigger_get_midi_in;
    sigc::signal<void, int, int, bool>& signal_trigger_get_midi_in() { return trigger_get_midi_in; }
};


} // namespace xjack

#endif //XJACK_H_

