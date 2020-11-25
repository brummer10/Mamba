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

#include <atomic>
#include <vector>
#include <thread>
#include <condition_variable>

#include <alsa/asoundlib.h>

#include "Mamba.h"

#pragma once

#ifndef XALSA_H
#define XALSA_H

namespace xalsa {


/****************************************************************
 ** class MidiMessenger
 **
 ** collect all midi events from jack_midi and send to alsa midi out buffer
 */

class XAlsaMidiMessenger {
private:
    static const int max_midi_cc_cnt = 25;
    std::atomic<bool> send_cc[max_midi_cc_cnt];
    uint8_t cc_num[max_midi_cc_cnt];
    uint8_t pg_num[max_midi_cc_cnt];
    uint8_t bg_num[max_midi_cc_cnt];
    uint8_t me_num[max_midi_cc_cnt];
public:
    XAlsaMidiMessenger();
    int channel;
    bool send_midi_cc(uint8_t *midi_get, uint8_t _num) noexcept;
    int next(int i = -1) const noexcept;
    inline int size(int i)  const noexcept { return me_num[i]; }
    void fill(uint8_t *midi_send, int i) noexcept;
};

/****************************************************************
 ** class XAlsa
 **
 ** forward alsa midi to jack midi
 ** 
 */

class XAlsa {
private:
    mamba::MidiMessenger *mmessage;
    snd_seq_t *seq_handle;
    int sequencer;
    int in_port;
    int out_port;
    std::atomic<bool> _execute;
    std::thread _thd;
    std::atomic<bool> _execute_out;
    std::thread _thd_out;
    std::mutex m;

public:
    XAlsaMidiMessenger xamessage;
    XAlsa(mamba::MidiMessenger *mmessage_);
    ~XAlsa();
    void xalsa_get_ports(std::vector<std::string> *ports,
                        std::vector<std::string> *oports);
    void xalsa_get_iconnections(std::vector<std::string> *ports);
    void xalsa_get_oconnections(std::vector<std::string> *ports);
    void xalsa_connect(int client, int port);
    void xalsa_odisconnect(int client, int port);
    void xalsa_oconnect(int client, int port);
    void xalsa_disconnect(int client, int port);
    void xalsa_stop();
    int  xalsa_init(const char *client, const char *port);
    void xalsa_start(void *keys);
    void xalsa_start_out();
    void xalsa_output_notify(uint8_t *midi_get, uint8_t num);
    bool is_running() const noexcept;
    std::condition_variable cv_out;
};

} // namespace xalsa

#endif //XALSA_H_
