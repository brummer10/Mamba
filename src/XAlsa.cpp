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


#include "XAlsa.h"

namespace xalsa {


/****************************************************************
 ** class XAlsa
 **
 ** forward alsa midi to jack midi
 ** 
 */

XAlsa::XAlsa(mamba::MidiMessenger *mmessage_) 
    :mmessage(mmessage_),
    _execute(false) {
    sequencer = snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_INPUT, 0);
    if (sequencer == 0) {
        snd_seq_set_client_name(seq_handle, "Mamba");
        in_port = snd_seq_create_simple_port(seq_handle, "input",
                          SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                          SND_SEQ_PORT_TYPE_APPLICATION);
        if (in_port < 0) {
            snd_seq_close(seq_handle);
            sequencer = -1;
        }
    }
    if (sequencer < 0) {
        fprintf(stderr, "ALSA sequencer faild to open port, ALSA input is disabled\n");
    }
}

XAlsa::~XAlsa() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
    if (in_port < 0)
        snd_seq_delete_simple_port(seq_handle, in_port);
    if (sequencer == 0)
        snd_seq_close(seq_handle);
}

void XAlsa::stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) {
        _thd.join();
    }
}

void XAlsa::start(MidiKeyboard *keys) {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this, keys]() {
        while (_execute.load(std::memory_order_acquire)) {
            if (sequencer < 0) {
                _execute.store(false, std::memory_order_release);
                continue;
            }
            snd_seq_event_t *ev = NULL;
            snd_seq_event_input(seq_handle, &ev);
            if (ev->type == SND_SEQ_EVENT_NOTEON) {
                mmessage->send_midi_cc(0x90 | ev->data.control.channel, ev->data.note.note,ev->data.note.velocity, 3, true);
                set_key_in_matrix(keys->in_key_matrix[ev->data.control.channel], ev->data.note.note, true);
            } else if (ev->type == SND_SEQ_EVENT_NOTEOFF) {
                mmessage->send_midi_cc(0x80 | ev->data.control.channel, ev->data.note.note,ev->data.note.velocity, 3, true);
                set_key_in_matrix(keys->in_key_matrix[ev->data.control.channel], ev->data.note.note, false);
            } else if(ev->type == SND_SEQ_EVENT_CONTROLLER) {
                mmessage->send_midi_cc(0xB0 | ev->data.control.channel, ev->data.control.param, ev->data.control.value, 3, true);
            } else if(ev->type == SND_SEQ_EVENT_PGMCHANGE) {
                mmessage->send_midi_cc( 0xC0| ev->data.control.channel, ev->data.control.value, 0, 2, true);
            } else if(ev->type == SND_SEQ_EVENT_PITCHBEND) {
                unsigned int change = (unsigned int)(ev->data.control.value);
                unsigned int low = change & 0x7f;  // Low 7 bits
                unsigned int high = (change >> 7) & 0x7f;  // High 7 bits
                mmessage->send_midi_cc(0xE0| ev->data.control.channel,  low, high, 3, true);
            }
        } 
    });
}

bool XAlsa::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}

} // namespace xalsa
