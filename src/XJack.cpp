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

#include "XJack.h"

namespace xjack {

/****************************************************************
 ** class XJack
 **
 ** Send content from MidiMessenger to jack_midi out
 ** pass all incoming midi events to jack_midi out.
 ** send all incomming midi events to XKeyBoard
 */

XJack::XJack(mamba::MidiMessenger *mmessage_)
    : sigc::trackable(),
     mmessage(mmessage_),
     event_count(0),
     start(0),
     stop(0),
     deltaTime(0),
     client(NULL),
     rec() {
        transport_state_changed.store(false, std::memory_order_release);
        transport_set.store(0, std::memory_order_release);
        transport_state = JackTransportStopped;
        record = 0;
        play = 0;
        pos = 0;
        fresh_take = true;
        first_play = true;
        store1.reserve(256);
        store2.reserve(256);
        st = &store1;
        rec.st = &store1;
        client_name = "Mamba";
}

XJack::~XJack() {
    if (client) jack_client_close (client);
    if (rec.is_running()) rec.stop();
}

void XJack::init_jack() {
    if ((client = jack_client_open (client_name.c_str(), JackNullOption, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        trigger_quit_by_jack();
    }

    in_port = jack_port_register(
                  client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    out_port = jack_port_register(
                   client, "out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    jack_set_xrun_callback(client, jack_xrun_callback, this);
    jack_set_sample_rate_callback(client, jack_srate_callback, this);
    jack_set_buffer_size_callback(client, jack_buffersize_callback, this);
    jack_set_process_callback(client, jack_process, this);
    jack_on_shutdown (client, jack_shutdown, this);

    if (jack_activate (client)) {
        fprintf (stderr, "cannot activate client");
        trigger_quit_by_jack();
    }

    if (!jack_is_realtime(client)) {
        fprintf (stderr, "jack isn't running with realtime priority\n");
    } else {
        fprintf (stderr, "jack running with realtime priority\n");
    }
}

inline void XJack::record_midi(unsigned char* midi_send, unsigned int n, int i) {
    stop = jack_last_frame_time(client)+n;
    deltaTime = (double)(((stop) - start)/(double)SampleRate); // seconds
    //start = jack_last_frame_time(client)+n;
    unsigned char d = i > 2 ? midi_send[2] : 0;
    mamba::MidiEvent ev = {{midi_send[0], midi_send[1], d}, i, deltaTime};
    st->push_back(ev);
    if (store1.size() >= 256) {
        st = &store2;
        rec.st = &store1;
        rec.cv.notify_one();
    } else if (store2.size() >= 256) {
        st = &store1;
        rec.st = &store2;
        rec.cv.notify_one();
    }
}

inline void XJack::play_midi(void *buf, unsigned int n) {
    if (!rec.play.size()) return;
    if (first_play) {
        start = jack_last_frame_time(client)+n;
        first_play = false;
        pos = 0;
    }
    stop = jack_last_frame_time(client)+n;
    deltaTime = (double)(((stop) - start)/(double)SampleRate); // seconds
    mamba::MidiEvent ev = rec.play[pos];
    if (deltaTime >= ev.deltaTime) {
        unsigned char* midi_send = jack_midi_event_reserve(buf, n, ev.num);
        if (midi_send) {
            midi_send[0] = ev.buffer[0];
            midi_send[1] = ev.buffer[1];
            if(ev.num > 2)
                midi_send[2] = ev.buffer[2];
            if ((ev.buffer[0] & 0xf0) == 0x90) {   // Note On
                std::async(std::launch::async, trigger_get_midi_in, ev.buffer[1], true);
            } else if ((ev.buffer[0] & 0xf0) == 0x80) {   // Note Off
                std::async(std::launch::async, trigger_get_midi_in, ev.buffer[1], false);
            }
        }
        //start = jack_last_frame_time(client)+n;
        pos++;
        if (pos >= rec.play.size()) {
            pos = 0;
            start = jack_last_frame_time(client)+n;
        }
    }
}

// jack process callback for the midi output
inline void XJack::process_midi_out(void *buf, jack_nframes_t nframes) {
    int i = mmessage->next();
    for (unsigned int n = event_count; n < nframes; n++) {
        if (i >= 0) {
            unsigned char* midi_send = jack_midi_event_reserve(buf, n, mmessage->size(i));
            if (midi_send) {
                mmessage->fill(midi_send, i);
                if (record) record_midi(midi_send, n, mmessage->size(i));
            }
            i = mmessage->next(i);
        } else if (play) {
            play_midi(buf, n);
        }
    }
}

// jack process callback for the midi input
inline void XJack::process_midi_in(void* buf, void* out_buf, void *arg) {
    XJack *xjack = (XJack*)arg;
    if (xjack->record && xjack->fresh_take) {
        xjack->start = jack_last_frame_time(xjack->client);
        xjack->fresh_take = false;
    }
    jack_midi_event_t in_event;
    event_count = jack_midi_get_event_count(buf);
    unsigned int i;
    for (i = 0; i < event_count; i++) {
        jack_midi_event_get(&in_event, buf, i);
        unsigned char* midi_send = jack_midi_event_reserve(out_buf, i, in_event.size);
        midi_send[0] = in_event.buffer[0];
        midi_send[1] = in_event.buffer[1];
        if (in_event.size>2) 
            midi_send[2] = in_event.buffer[2];
        if (record)
            record_midi(midi_send, i, in_event.size);
        bool ch = true;
        if ((xjack->mmessage->channel) != (int(in_event.buffer[0]&0x0f))) {
            ch = false;
        }
        if ((in_event.buffer[0] & 0xf0) == 0x90 && ch) {   // Note On
            std::async(std::launch::async, xjack->trigger_get_midi_in, in_event.buffer[1], true);
        } else if ((in_event.buffer[0] & 0xf0) == 0x80 && ch) {   // Note Off
            std::async(std::launch::async, xjack->trigger_get_midi_in, in_event.buffer[1], false);
        }
    }
}

// static
void XJack::jack_shutdown (void *arg) {
    XJack *xjack = (XJack*)arg;
    xjack->trigger_quit_by_jack();
}

// static
int XJack::jack_xrun_callback(void *arg) {
    fprintf (stderr, "Xrun \r");
    return 0;
}

// static
int XJack::jack_srate_callback(jack_nframes_t samplerate, void* arg) {
    XJack *xjack = (XJack*)arg;
    xjack->SampleRate = samplerate;
    xjack->srms = xjack->SampleRate/1000;
    fprintf (stderr, "Samplerate %iHz \n", samplerate);
    return 0;
}

// static
int XJack::jack_buffersize_callback(jack_nframes_t nframes, void* arg) {
    fprintf (stderr, "Buffersize is %i samples \n", nframes);
    return 0;
}

// static
int XJack::jack_process(jack_nframes_t nframes, void *arg) {
    XJack *xjack = (XJack*)arg;
    if (xjack->transport_state != jack_transport_query (xjack->client, &xjack->current)) {
        xjack->transport_state = jack_transport_query (xjack->client, &xjack->current);
        xjack->transport_state_changed.store(true, std::memory_order_release);
        xjack->transport_set.store((int)xjack->transport_state, std::memory_order_release);
    }
    void *in = jack_port_get_buffer (xjack->in_port, nframes);
    void *out = jack_port_get_buffer (xjack->out_port, nframes);
    jack_midi_clear_buffer(out);
    xjack->process_midi_in(in, out, arg);
    xjack->process_midi_out(out,nframes);
    return 0;
}

} // namespace xjack

