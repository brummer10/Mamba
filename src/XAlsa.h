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

#include <alsa/asoundlib.h>

#include "Mamba.h"
#include "xkeyboard.h"

#pragma once

#ifndef XALSA_H
#define XALSA_H

namespace xalsa {


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
    std::atomic<bool> _execute;
    std::thread _thd;

public:
    XAlsa(mamba::MidiMessenger *mmessage_);
    ~XAlsa();
    void xalsa_get_ports(std::vector<std::string> *ports);
    void xalsa_get_connections(std::vector<std::string> *ports);
    void xalsa_connect(int client, int port);
    void xalsa_disconnect(int client, int port);
    void xalsa_stop();
    int  xalsa_init(const char *client, const char *port);
    void xalsa_start(MidiKeyboard *keys);
    bool is_running() const noexcept;
};

} // namespace xalsa

#endif //XALSA_H_
