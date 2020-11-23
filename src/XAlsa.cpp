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
#include "xkeyboard.h"

namespace xalsa {


/****************************************************************
 ** class XAlsaMidiMessenger
 **
 ** create, collect and send all midi events to jack_midi out buffer
 */

XAlsaMidiMessenger::XAlsaMidiMessenger() {
    channel = 0;
    for (int i = 0; i < max_midi_cc_cnt; i++) {
        send_cc[i] = false;
    }
}

int XAlsaMidiMessenger::next(int i) const noexcept {
    while (++i < max_midi_cc_cnt) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            return i;
        }
    }
    return -1;
}

void XAlsaMidiMessenger::fill(uint8_t *event, int i) noexcept {
    if (size(i) == 3) {
        event[2] =  bg_num[i]; // velocity
    }
    event[1] = pg_num[i];    // program value
    event[0] = cc_num[i];    // controller+ channel
    send_cc[i].store(false, std::memory_order_release);
}

bool XAlsaMidiMessenger::send_midi_cc(int _cc, int _pg, int _bgn, int _num, bool have_channel) noexcept {
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
 ** class XAlsa
 **
 ** forward alsa midi to jack midi
 ** 
 */

XAlsa::XAlsa(mamba::MidiMessenger *mmessage_) 
    :mmessage(mmessage_),
    _execute(false),
    _execute_out(false),
    xamessage() {
    sequencer = -1;
    in_port = -1;
    out_port = -1;
}

XAlsa::~XAlsa() {
    if( _execute.load(std::memory_order_acquire) ) {
        xalsa_stop();
    };
    if (in_port < 0)
        snd_seq_delete_simple_port(seq_handle, in_port);
    if (out_port < 0)
        snd_seq_delete_simple_port(seq_handle, out_port);
    if (sequencer == 0)
        snd_seq_close(seq_handle);
}

int XAlsa::xalsa_init(const char *client, const char *port) {
    sequencer = snd_seq_open(&seq_handle, "default", SND_SEQ_OPEN_DUPLEX, 0);
    if (sequencer < 0) return sequencer;
    snd_seq_set_client_name(seq_handle, client);
    out_port = snd_seq_create_simple_port(seq_handle, "output",
                      SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
                      SND_SEQ_PORT_TYPE_APPLICATION);
    in_port = snd_seq_create_simple_port(seq_handle, port,
                      SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE,
                      SND_SEQ_PORT_TYPE_APPLICATION);
    if (in_port < 0) {
        snd_seq_close(seq_handle);
        sequencer = -1;
    }
    return sequencer;
}

void XAlsa::xalsa_get_ports(std::vector<std::string> *iports, std::vector<std::string> *oports) {
    if (sequencer < 0) return;
    iports->clear();
    oports->clear();
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;

    snd_seq_client_info_malloc(&cinfo);
    snd_seq_port_info_malloc(&pinfo);

    snd_seq_client_info_set_client(cinfo, -1);

    while (snd_seq_query_next_client(seq_handle, cinfo) >= 0) {
        snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
        snd_seq_port_info_set_port(pinfo, -1);
        while (snd_seq_query_next_port(seq_handle, pinfo) >= 0) {
            char port[256];
            if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ)) &&
                                 ((snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_NO_EXPORT) == 0) &&
                                         (snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC)) {
                snprintf(port,256,"%3d %d %s:%s",
                    snd_seq_port_info_get_client(pinfo),
                    snd_seq_port_info_get_port(pinfo),
                    snd_seq_client_info_get_name(cinfo),
                    snd_seq_port_info_get_name(pinfo));
                iports->push_back(port);
            } else if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE)) &&
                                 ((snd_seq_port_info_get_capability(pinfo) & SND_SEQ_PORT_CAP_NO_EXPORT) == 0) &&
                                         (snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC)) {
                snprintf(port,256,"%3d %d %s:%s",
                    snd_seq_port_info_get_client(pinfo),
                    snd_seq_port_info_get_port(pinfo),
                    snd_seq_client_info_get_name(cinfo),
                    snd_seq_port_info_get_name(pinfo));
                oports->push_back(port);
            }
        }
    }

    snd_seq_port_info_free(pinfo);
    snd_seq_client_info_free(cinfo);
}

void XAlsa::xalsa_get_iconnections(std::vector<std::string> *ports) {
    if (sequencer < 0) return;
    ports->clear();
    snd_seq_port_info_t *pinfo;
    snd_seq_query_subscribe_t *subs;
    snd_seq_port_info_malloc(&pinfo);
    snd_seq_query_subscribe_malloc(&subs);

    snd_seq_get_port_info(seq_handle, in_port,pinfo);
    snd_seq_query_subscribe_set_root(subs, snd_seq_port_info_get_addr(pinfo));
    snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_WRITE);
    snd_seq_query_subscribe_set_index(subs, 0);

    while (!snd_seq_query_port_subscribers(seq_handle, subs)) {
        char port[256];
        const snd_seq_addr_t *addr;
        addr = snd_seq_query_subscribe_get_addr(subs);
        snd_seq_get_any_port_info(seq_handle, addr->client, addr->port, pinfo);
        if (snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC) {
            snprintf(port,256,"%3d %d",addr->client, addr->port);
            ports->push_back(port);
        }
        snd_seq_query_subscribe_set_index(subs, snd_seq_query_subscribe_get_index(subs) + 1);
    }

    snd_seq_port_info_free(pinfo);
    snd_seq_query_subscribe_free(subs);
}

void XAlsa::xalsa_get_oconnections(std::vector<std::string> *ports) {
    if (sequencer < 0) return;
    ports->clear();
    snd_seq_port_info_t *pinfo;
    snd_seq_query_subscribe_t *subs;
    snd_seq_port_info_malloc(&pinfo);
    snd_seq_query_subscribe_malloc(&subs);

    snd_seq_get_port_info(seq_handle, out_port,pinfo);
    snd_seq_query_subscribe_set_root(subs, snd_seq_port_info_get_addr(pinfo));
    snd_seq_query_subscribe_set_type(subs, SND_SEQ_QUERY_SUBS_READ);
    snd_seq_query_subscribe_set_index(subs, 0);

    while (!snd_seq_query_port_subscribers(seq_handle, subs)) {
        char port[256];
        const snd_seq_addr_t *addr;
        addr = snd_seq_query_subscribe_get_addr(subs);
        snd_seq_get_any_port_info(seq_handle, addr->client, addr->port, pinfo);
        if (snd_seq_port_info_get_type(pinfo) & SND_SEQ_PORT_TYPE_MIDI_GENERIC) {
            snprintf(port,256,"%3d %d",addr->client, addr->port);
            ports->push_back(port);
        }
        snd_seq_query_subscribe_set_index(subs, snd_seq_query_subscribe_get_index(subs) + 1);
    }

    snd_seq_port_info_free(pinfo);
    snd_seq_query_subscribe_free(subs);
}

void XAlsa::xalsa_connect(int client, int port) {
    if (sequencer < 0) return;
    snd_seq_connect_from(seq_handle, in_port, client, port);
}

void XAlsa::xalsa_disconnect(int client, int port) {
    if (sequencer < 0) return;
    snd_seq_disconnect_from(seq_handle, in_port, client, port);
}

void XAlsa::xalsa_oconnect(int client, int port) {
    if (sequencer < 0) return;
    snd_seq_connect_to(seq_handle, out_port, client, port);
}

void XAlsa::xalsa_odisconnect(int client, int port) {
    if (sequencer < 0) return;
    snd_seq_disconnect_to(seq_handle, out_port, client, port);
}

void XAlsa::xalsa_stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) {
        _thd.join();
    }
    _execute_out.store(false, std::memory_order_release);
    if (_thd_out.joinable()) {
        _thd_out.join();
    }
}

void XAlsa::xalsa_start_out() {
    if( _execute_out.load(std::memory_order_acquire) ) {
        xalsa_stop();
    };
    _execute_out.store(true, std::memory_order_release);
    _thd_out = std::thread([this]() {
        snd_seq_event_t ev;
        uint8_t event[3] = {0};
        while (_execute_out.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m);
            cv_out.wait(lk);
            //do output
            if (_execute_out.load(std::memory_order_acquire)) {
                int i = xamessage.next();
                while ( i>=0) {
                    xamessage.fill(event, i);
                    uint8_t channel = event[0]&0x0f;
                    uint8_t num = event[0] & 0xf0;
                    snd_seq_ev_clear(&ev);
                    snd_seq_ev_set_subs(&ev);
                    snd_seq_ev_set_direct(&ev);

                    if (num == 0x90) {
                        snd_seq_ev_set_noteon(&ev, channel, event[1], event[2]);
                    } else if (num == 0x80) {
                        snd_seq_ev_set_noteoff(&ev, channel, event[1], event[2]);
                    } else if (num == 0xB0) {
                        snd_seq_ev_set_controller(&ev, channel, event[1], event[2]);
                    } else if (num == 0xC0) {
                        snd_seq_ev_set_pgmchange(&ev, channel, event[1]);
                    } else if (num == 0xE0) {
                        snd_seq_ev_set_pitchbend(&ev, channel, ((event[2] <<7 | event[1]) -8192));
                    }

                    // send now
                    snd_seq_event_output(seq_handle, &ev);
                    snd_seq_drain_output(seq_handle);
                    i = xamessage.next();
                }
            }
        } 
    });
}            

void XAlsa::xalsa_start(void *keys_) {
    if( _execute.load(std::memory_order_acquire) ) {
        xalsa_stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this, keys_]() {
        MidiKeyboard *keys = (MidiKeyboard*)keys_;
        while (_execute.load(std::memory_order_acquire)) {
            if (sequencer < 0) {
                _execute.store(false, std::memory_order_release);
                continue;
            }
            snd_seq_event_t *ev = NULL;
            snd_seq_event_input(seq_handle, &ev);
            if (ev->type == SND_SEQ_EVENT_NOTEON) {
                mmessage->send_midi_cc(0x90 | ev->data.control.channel, ev->data.note.note,ev->data.note.velocity, 3, true);
                if (ev->data.note.velocity)
                    set_key_in_matrix(keys->in_key_matrix[ev->data.control.channel], ev->data.note.note, true);
                else
                    set_key_in_matrix(keys->in_key_matrix[ev->data.control.channel], ev->data.note.note, false);
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
            snd_seq_free_event(ev);
        } 
    });
}

bool XAlsa::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() && _execute_out.load(std::memory_order_acquire)
             && _thd_out.joinable());
}

} // namespace xalsa
