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
#include <string>
#include <condition_variable>
#include <functional>


#pragma once

#ifndef MIDIMAPPER_H
#define MIDIMAPPER_H

namespace midimapper {

/****************************************************************
 ** class MidiMessenger
 **
 ** collect all midi events from jack_midi and send them to midi mapper
 */

class MidiMapperMessenger {
private:
    static const int max_midi_cc_cnt = 25;
    std::atomic<bool> send_cc[max_midi_cc_cnt];
    uint8_t cc_num[max_midi_cc_cnt];
    uint8_t pg_num[max_midi_cc_cnt];
    uint8_t bg_num[max_midi_cc_cnt];
    uint8_t me_num[max_midi_cc_cnt];
public:
    MidiMapperMessenger();
    int channel;
    bool send_midi_cc(const uint8_t *midi_get, const uint8_t _num) noexcept;
    int next(int i = -1) const noexcept;
    inline uint8_t size(const int i)  const noexcept { return me_num[i]; }
    void fill(uint8_t *midi_send, const int i) noexcept;
};


/****************************************************************
 ** class MidiMapper
 **
 ** map jack midi input to jack midi output via mapping matrix
 */

class MidiMapper {
private:
    // send midi message to the 'queue' for jack midi output
    std::function<void(
        int _cc, int _pg, int _bgn, int _num, bool have_channel) >
        send_to_jack;
    // the midi message 'queue' for jack midi input
    MidiMapperMessenger mmessage;
   // control the midi mapper loop
    std::atomic<bool> _execute_map;
    // wait for notify in the midi mapper loop
    std::condition_variable cv_map;
    // thread running the midi mapper loop
    std::thread _thd_map;
    // the mutex for the lock in the midi mapper loop
    std::mutex m;
    // stop the threads for mapper midi handling
    void mmapper_stop();

public:
    MidiMapper(std::function<void(
        int _cc, int _pg, int _bgn, int _num, bool have_channel)>
        send_to_jack);
    ~MidiMapper();
    // start the threads for mapper midi handling
    void mmapper_start(std::function<void(int,int,bool)> set_key);
    // push mdi message from jack into 'queue' and inform output thread that work is to do
    void mmapper_input_notify(const uint8_t *midi_get, uint8_t num) noexcept;
    // set the priority for the I/O thread
    void mmapper_set_priority(int priority);
    // check if the mapper is running
    bool is_running() const noexcept;
    // setup the kbm map
    void setup_default_kbm_map();
    // the vector holding the map file
    std::vector<int> kbm_map;

};

} // namespace midimapper

#endif //MIDIMAPPER_H_
