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
#include <functional>

#include <alsa/asoundlib.h>


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
    bool send_midi_cc(const uint8_t *midi_get, uint8_t _num) noexcept;
    int next(int i = -1) const noexcept;
    inline int size(int i)  const noexcept { return me_num[i]; }
    void fill(uint8_t *midi_send, int i) noexcept;
};

/****************************************************************
 ** class XAlsa
 **
 ** forward alsa midi input to jack midi output
 ** forward jack midi input to alsa midi output
 */

class XAlsa {
private:
    // send midi message to the 'queue' for jack midi output
    std::function<void(
        int _cc, int _pg, int _bgn, int _num, bool have_channel) noexcept>
        send_to_jack;
    // the midi message 'queue' for alsa midi output
    XAlsaMidiMessenger xamessage;
    // the sequencer
    snd_seq_t *seq_handle;
    // ident if sequencer starts successfully
    int sequencer;
    // input port number
    int in_port;
    // output port number
    int out_port;
    // control the midi input loop
    std::atomic<bool> _execute;
    // thread running the midi input loop
    std::thread _thd;
    // control the midi output loop
    std::atomic<bool> _execute_out;
    // wait for notify in the midi output loop
    std::condition_variable cv_out;
    // thread running the midi output loop
    std::thread _thd_out;
    // the mutex for the lock in the midi output loop
    std::mutex m;

public:
    XAlsa(std::function<void(
        int _cc, int _pg, int _bgn, int _num, bool have_channel) noexcept>
        send_to_jack);
    ~XAlsa();
    // get all available ports for alsa midi in/output
    void xalsa_get_ports(std::vector<std::string> *ports,
                        std::vector<std::string> *oports);
    // get all connections for input port
    void xalsa_get_iconnections(std::vector<std::string> *ports);
    // get all connections for output port
    void xalsa_get_oconnections(std::vector<std::string> *ports);
    // connect the input port to 'port'
    void xalsa_connect(int client, int port);
    // disconnect the input port from 'port'
    void xalsa_disconnect(int client, int port);
    // connect the output port to 'port'
    void xalsa_oconnect(int client, int port);
    // disconnect the output port from 'port'
    void xalsa_odisconnect(int client, int port);
    // stop the threads for alsa midi handling
    void xalsa_stop();
    // init the sequencer and create ports
    int  xalsa_init(const char *client, const char *port);
    // start the thread for midi input handling
    void xalsa_start(void *keys);
    // start the port for midi output handling
    void xalsa_start_out();
    // push mdi message from jack into 'queue' and inform output thread that work is to do
    void xalsa_output_notify(const uint8_t *midi_get, uint8_t num) noexcept;
    // set the priority for the I/O threads
    void xalsa_set_priority(int priority);
    // check if the sequencer is running
    bool is_running() const noexcept;
};

} // namespace xalsa

#endif //XALSA_H_
