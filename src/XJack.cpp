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
#include <jack/thread.h>

namespace xjack {


/****************************************************************
 ** class MidiClockToBpm
 **
 ** calculate BPM from Midi Beat Clock events
 */

MidiClockToBpm::MidiClockToBpm()
    : time1(0),
      time_diff(0),
      collect(0),
      collect_(0),
      bpm(0),
      bpm_new(0),
      bpm_old(0),
      ret(false) {}

unsigned int MidiClockToBpm::rounded(float f) {
    if (f >= 0x1.0p23) return (unsigned int) f;
    return (unsigned int) (f + 0.49999997f);
}

bool MidiClockToBpm::time_to_bpm(double time, unsigned int* bpm_) {
    ret = false;
    // if time drift to far, reset bpm detection.
    if ((time-time1)> (1.05*time_diff) || (time-time1)*1.05 < (time_diff)) { 
        bpm = 0;
        collect = 0;
        collect_ = 0;
    } else {
        bpm_new = ((1000000000. / (time-time1) / 24) * 60);
        bpm += bpm_new;
        collect++;
        
        if (collect >= (bpm_new*bpm_new*0.0002)+1) {
            bpm = (bpm/collect);
            if (collect_>=2) {
                (*bpm_) = rounded(std::min(360.,std::max(24.,bpm))); 
                collect_ = 0;
                if (bpm_old != (*bpm_)) {
                    ret = true;
                    bpm_old = (*bpm_);
                }
            }
            collect_++;
            collect = 1;
        }
    }
    time_diff = time-time1;
    time1 = time;
    return ret;
}

/****************************************************************
 ** class XJack
 **
 ** Send content from MidiMessenger to jack_midi out
 ** pass all incoming midi events to jack_midi out.
 ** send all incomming midi events to XKeyBoard
 */

XJack::XJack(mamba::MidiMessenger *mmessage_,
        std::function<void(const uint8_t*,uint8_t) >  send_to_alsa_,
        std::function<void(int)>  set_alsa_priority_,
        std::function<void(const uint8_t*,uint8_t) >  send_to_midimapper_,
        std::function<void(int)>  set_midimapper_priority_)
    : sigc::trackable(),
     mmessage(mmessage_),
     mp(),
     send_to_alsa(send_to_alsa_),
     set_alsa_priority(set_alsa_priority_),
     send_to_midimapper(send_to_midimapper_),
     set_midimapper_priority(set_midimapper_priority_),
     event_count(0),
     stop(0),
     deltaTime(0),
     client(NULL),
     rec() {
        transport_state_changed.store(false, std::memory_order_release);
        transport_set.store(0, std::memory_order_release);
        transport_state = JackTransportStopped;
        bpm_changed.store(false, std::memory_order_release);
        bpm_set.store(0, std::memory_order_release);
        record_off.store(false, std::memory_order_release);
        record_finished.store(0, std::memory_order_release);
        record.store(0, std::memory_order_release);
        play.store(0, std::memory_order_release);
        start = 0;
        NotOn = 0;
        absoluteStart = 0;
        absoluteRecordTime = 0.0;
        absoluteRecordStart = 0.0;
        pos = 0;
        bank = 0;
        program = 0;
        freewheel = 0;
        view_channels = 0;
        max_loop_time = 0;
        playPosTime = 0.0;
        fresh_take = true;
        first_play = true;
        second_play = false;
        midi_through = 1;
        store1.reserve(256);
        store2.reserve(256);
        st = &store1;
        rec.st = &store1;
        client_name = "Mamba";
        bpm_ratio = 1.0;
        stPlay = 0;
        stStart = 0;
        rcStart = 0;
        priority = -1;
        midi_map = 0;
        for ( int i = 0; i < 16; i++) posPlay[i] = 0;
        for ( int i = 0; i < 16; i++) startPlay[i] = 0;
        for ( int i = 0; i < 16; i++) stopPlay[i] = 0;
        for ( int i = 0; i < 16; i++) channel_matrix[i].store(0, std::memory_order_release);
}

XJack::~XJack() {
    if (client) jack_client_close (client);
    if (rec.is_running()) rec.stop();
}

int XJack::init_jack() {
    if ((client = jack_client_open (client_name.c_str(), JackNullOption, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        return 0;
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
        return 0;
    }
    client_name = jack_get_client_name(client);
    if (!jack_is_realtime(client)) {
        fprintf (stderr, "jack isn't running with realtime priority\n");
    } else {
        priority = jack_client_real_time_priority(client);
        fprintf (stderr, "jack running with realtime priority %i\n", priority);
        if (priority > 2) {
            set_alsa_priority(priority);
            set_midimapper_priority(priority);
        }
    }
    return 1;
}

// record MIDI events 
inline void XJack::record_midi(unsigned char* midi_send, unsigned int n, int i) noexcept {
    stop = jack_last_frame_time(client)+n;
    deltaTime = (double)(((stop) - start)/(double)SampleRate); // seconds
    absoluteTime = (double)(((stop) - absoluteStart)/(double)SampleRate); // seconds
    absoluteRecordTime = (double)(((stop) - absoluteRecordStart)/(double)SampleRate); // seconds
    start = jack_last_frame_time(client)+n;
    if (((midi_send[0] & 0xf0) == 0x90) && midi_send[2] > 0) NotOn++;
    else if (((midi_send[0] & 0xf0) == 0x90) && midi_send[2] == 0) NotOn--;
    else if ((midi_send[0] & 0xf0) == 0x80) NotOn--;
    if (absoluteRecordTime >= max_loop_time && !NotOn && (get_max_time_loop() > -1)) {
        record_off.store(true, std::memory_order_release);
    }
    unsigned char d = i > 2 ? midi_send[2] : 0;
    const mamba::MidiEvent ev = {{midi_send[0], midi_send[1], d}, i, deltaTime, absoluteTime};
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

// get the master loop
inline int XJack::get_max_time_loop() noexcept {
    int v = -1;
     max_loop_time = 0.0;
    for (int j = 0; j<16;j++) {
        if (!rec.play[j].size()) continue;
        const mamba::MidiEvent ev = rec.play[j][rec.play[j].size()-1];
        if (ev.absoluteTime > max_loop_time) {
            max_loop_time = ev.absoluteTime;
            v = j;
        }
    }
    return v;
}

// sync fresh recorded vector to play position
inline int XJack::find_pos_for_playtime() noexcept {
    int pos = 0;
    for(std::vector<mamba::MidiEvent>::const_iterator i = rec.play[mmessage->channel].begin();
                                    i != rec.play[mmessage->channel].end(); ++i) {
        if ((*i).absoluteTime >= playPosTime) return pos;
        pos++;
    }
    return pos;
}

// play all MIDI loops
inline void XJack::play_midi(void *buf, unsigned int n) {
    if (first_play) {
        first_play = false;
        get_max_time_loop();
        pos = 0;
        for (int i = 0; i < 16; i++) posPlay[i] = 0;
        for (int i = 0; i < 16; i++) startPlay[i] = jack_last_frame_time(client)+n;
        start = jack_last_frame_time(client)+n;
        absoluteStart = jack_last_frame_time(client)+n;
        stStart = jack_last_frame_time(client)+n;
    } else if (second_play) {
        second_play = false;
        stStart = jack_last_frame_time(client)+n - (stPlay - stStart);
    }

    for ( int i = 0; i < 16; i++) {
        stPlay = jack_last_frame_time(client)+n;
        if (!rec.play[i].size()) continue;
        if (record.load(std::memory_order_acquire) && i == mmessage->channel) continue;
        stopPlay[i] = jack_last_frame_time(client)+n;
        if (posPlay[i] >= rec.play[i].size()) {
            
            // this will sync all loops to the first recorded one
            if (!freewheel) {
                int ml = get_max_time_loop();
                if (i != ml) continue;
                for (int i = 0; i < 16; i++) posPlay[i] = 0;
                for (int i = 0; i < 16; i++) startPlay[i] = jack_last_frame_time(client)+n;
                start = jack_last_frame_time(client)+n;
                absoluteStart = jack_last_frame_time(client)+n;
                stStart = jack_last_frame_time(client)+n;
            } else {
                posPlay[i] = 0;
                startPlay[i] = jack_last_frame_time(client)+n;
            }
        }
        deltaTime = (double)(((stopPlay[i]) - startPlay[i])/(double)SampleRate); // seconds
        const mamba::MidiEvent ev = rec.play[i][posPlay[i]];
        if (deltaTime >= ev.deltaTime * bpm_ratio) {
            playPosTime = ev.absoluteTime;
            // check if channel is muted
            if (!channel_matrix[int(ev.buffer[0]&0x0f)].load(std::memory_order_acquire) ||
                                                        ((ev.buffer[0] & 0xf0) == 0x80 )) {
                unsigned char* midi_send = jack_midi_event_reserve(buf, n, ev.num);
                if (midi_send) {
                    midi_send[0] = ev.buffer[0];
                    midi_send[1] = ev.buffer[1];
                    if(ev.num > 2)
                        midi_send[2] = ev.buffer[2];
                    bool ch = true;
                    if (mmessage->channel < 16 && view_channels) {
                        if ((mmessage->channel) != (int(ev.buffer[0]&0x0f))) {
                            ch = false;
                        }
                    }
                    send_to_alsa(midi_send, ev.num);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
                    if ((ev.buffer[0] & 0xf0) == 0x90 && ch) {   // Note On
                        if (ev.buffer[2] > 0) // velocity 0 treaded as Note Off
                            std::async(std::launch::async, trigger_get_midi_in, (int(ev.buffer[0]&0x0f)), ev.buffer[1], true);
                        else 
                            std::async(std::launch::async, trigger_get_midi_in, (int(ev.buffer[0]&0x0f)), ev.buffer[1], false);
                    } else if ((ev.buffer[0] & 0xf0) == 0x80 && ch) {   // Note Off
                        std::async(std::launch::async, trigger_get_midi_in, (int(ev.buffer[0]&0x0f)), ev.buffer[1], false);
                    }
#pragma GCC diagnostic pop
                }
            }
            startPlay[i] = jack_last_frame_time(client)+n;
            posPlay[i]++;
            return;
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
                send_to_alsa(midi_send, mmessage->size(i));
                if (record.load(std::memory_order_acquire)) record_midi(midi_send, n, mmessage->size(i));
            }
            i = mmessage->next(i);
        } else if (play.load(std::memory_order_acquire)) {
            play_midi(buf, n);
        }
    }
    if (record.load(std::memory_order_acquire)) {
        stop = jack_last_frame_time(client);
        deltaTime = (double)(((stop) - start)/(double)SampleRate); // seconds
        absoluteTime = (double)(((stop) - absoluteStart)/(double)SampleRate); // seconds
        absoluteRecordTime = (double)(((stop) - absoluteRecordStart)/(double)SampleRate); // seconds
        if (absoluteRecordTime >= max_loop_time && !NotOn && (get_max_time_loop() > -1)) {
            record_off.store(true, std::memory_order_release);
        }
    }

}

// jack process callback for the midi input
inline void XJack::process_midi_in(void* buf, void* out_buf) {
    if (record.load(std::memory_order_acquire) && fresh_take) {
        start = jack_last_frame_time(client);
        absoluteStart = jack_last_frame_time(client);
        absoluteRecordStart = jack_last_frame_time(client);
        rcStart = jack_last_frame_time(client);
        fresh_take = false;
        NotOn = 0;
        int b = 0xB0 | mmessage->channel;
        int p = 0xC0 | mmessage->channel;
        const mamba::MidiEvent evb = {{(unsigned char)b, 32, (unsigned char)bank}, 3, 0, 0};
        st->push_back(evb);
        const mamba::MidiEvent evp = {{(unsigned char)p, (unsigned char)program, 0}, 2, 0, 0};
        st->push_back(evp);

        if (!freewheel && play && (get_max_time_loop() > -1)) {
            start = startPlay[mmessage->channel];
            absoluteStart = startPlay[mmessage->channel];
        }
    } else if (record_finished.load(std::memory_order_acquire) && !freewheel && (get_max_time_loop() > -1)) {
        record_finished.store(0, std::memory_order_release);
        if (rec.is_sorted.load(std::memory_order_acquire)) {
            posPlay[mmessage->channel] = find_pos_for_playtime();
            rec.is_sorted.store(false, std::memory_order_release);
        } else {
            posPlay[mmessage->channel] = posPlay[get_max_time_loop()];
        }
        stStart = jack_last_frame_time(client);
    }
    jack_midi_event_t in_event;
    event_count = jack_midi_get_event_count(buf);
    unsigned int i;
    for (i = 0; i < event_count; i++) {
        jack_midi_event_get(&in_event, buf, i);
        // only mapping note on/off messages
        if (midi_map && (((in_event.buffer[0] & 0xf0) == 0x90) ||
                        ((in_event.buffer[0] & 0xf0) == 0x80))) {
            // record and send_to_also happen now in process_midi_out
            send_to_midimapper(in_event.buffer, in_event.size);
        } else {
            if (midi_through) {
                unsigned char* midi_send = jack_midi_event_reserve(out_buf, i, in_event.size);
                midi_send[0] = in_event.buffer[0];
                midi_send[1] = in_event.buffer[1]; // implement mapping
                if (in_event.size>2) 
                    midi_send[2] = in_event.buffer[2];
                if (record.load(std::memory_order_acquire))
                    record_midi(midi_send, i, in_event.size);
                send_to_alsa(midi_send, in_event.size);
            }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
            if ((in_event.buffer[0] & 0xf0) == 0x90) {   // Note On
                std::async(std::launch::async, trigger_get_midi_in, (int(in_event.buffer[0]&0x0f)), in_event.buffer[1], true);
            } else if ((in_event.buffer[0] & 0xf0) == 0x80) {   // Note Off
                std::async(std::launch::async, trigger_get_midi_in, (int(in_event.buffer[0]&0x0f)), in_event.buffer[1], false);
            } else if ((in_event.buffer[0] ) == 0xf8) {   // midi beat clock
                clock_gettime(CLOCK_MONOTONIC, &ts1);
                double time0 = (ts1.tv_sec*1000000000.0)+(ts1.tv_nsec)+
                        (1000000000.0/(double)(SampleRate/(double)in_event.time));
                if (mp.time_to_bpm(time0, &bpm)) {
                    bpm_changed.store(true, std::memory_order_release);
                    bpm_set.store((int)bpm, std::memory_order_release);
                }
            }
#pragma GCC diagnostic pop
        }
    }
}

float XJack::get_max_loop_time() noexcept {
    max_loop_time = 0.0;
    for (int j = 0; j<16;j++) {
        if (!rec.play[j].size()) continue;
        const mamba::MidiEvent ev = rec.play[j][rec.play[j].size()-1];
        if (ev.absoluteTime > max_loop_time) {
            max_loop_time = ev.absoluteTime;
        }
    }
    return max_loop_time;
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
    if (xjack->current.valid && xjack->current.beats_per_minute != (double)xjack->bpm) {
        xjack->bpm = (unsigned int)xjack->current.beats_per_minute;
        xjack->bpm_changed.store(true, std::memory_order_release);
        xjack->bpm_set.store((int)xjack->bpm, std::memory_order_release);
    } 
    void *in = jack_port_get_buffer (xjack->in_port, nframes);
    void *out = jack_port_get_buffer (xjack->out_port, nframes);
    jack_midi_clear_buffer(out);
    xjack->process_midi_in(in, out);
    xjack->process_midi_out(out,nframes);
    return 0;
}

} // namespace xjack

