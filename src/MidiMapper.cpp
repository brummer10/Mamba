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


#include "MidiMapper.h"

namespace midimapper {


/****************************************************************
 ** class MidiMapperMessenger
 **
 ** collect all midi events from jack_midi and send to MidiMapper
 */

MidiMapperMessenger::MidiMapperMessenger() {
    channel = 0;
    for (int i = 0; i < max_midi_cc_cnt; i++) {
        send_cc[i] = false;
    }
}

int MidiMapperMessenger::next(int i) const noexcept {
    while (++i < max_midi_cc_cnt) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            return i;
        }
    }
    return -1;
}

void MidiMapperMessenger::fill(uint8_t *event, const int i) noexcept {
    if (size(i) == 3) {
        event[2] =  bg_num[i]; // velocity
    }
    event[1] = pg_num[i];    // program value
    event[0] = cc_num[i];    // controller+ channel
    send_cc[i].store(false, std::memory_order_release);
}

bool MidiMapperMessenger::send_midi_cc(const uint8_t *midi_get,
                                    const uint8_t _num) noexcept {
    for(int i = 0; i < max_midi_cc_cnt; i++) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            if (cc_num[i] == midi_get[0] && pg_num[i] == midi_get[1] &&
                bg_num[i] == midi_get[2] && me_num[i] == _num)
                return true;
        } else if (!send_cc[i].load(std::memory_order_acquire)) {
            cc_num[i] = midi_get[0];
            pg_num[i] = midi_get[1];
            bg_num[i] = midi_get[2];
            me_num[i] = _num;
            send_cc[i].store(true, std::memory_order_release);
            return true;
        }
    }
    return false;
}


/****************************************************************
 ** class MidiMapper
 **
 ** map jack midi input to jack midi output via maaping matrix
 */

MidiMapper::MidiMapper(std::function<void(
        int _cc, int _pg, int _bgn, int _num, bool have_channel) > 
        send_to_jack_) 
    :send_to_jack(send_to_jack_),
    mmessage(),
    _execute_map(false) {
    setup_default_kbm_map();
}


MidiMapper::~MidiMapper() {
    if( _execute_map.load(std::memory_order_acquire) ) {
        mmapper_stop();
    };
}

void MidiMapper::mmapper_set_priority(int priority) {
    sched_param sch;
    sch.sched_priority = priority/2;
    pthread_setschedparam(_thd_map.native_handle(), SCHED_FIFO, &sch);
}

void MidiMapper::mmapper_stop() {
    _execute_map.store(false, std::memory_order_release);
    if (_thd_map.joinable()) {
        _thd_map.join();
    }
}

void MidiMapper::mmapper_input_notify(const uint8_t *midi_get, uint8_t num) noexcept {
    if (is_running()) {
        if (mmessage.send_midi_cc(midi_get, num))
            cv_map.notify_one();
    }
}

void MidiMapper::setup_default_kbm_map() {
    kbm_map.clear();
    for ( int i = 0; i < 128; i++) {
        kbm_map.push_back(i);
    }
}

void MidiMapper::mmapper_start(std::function<void(int,int,bool)> set_key) {
    if( _execute_map.load(std::memory_order_acquire) ) {
        mmapper_stop();
    };
    _execute_map.store(true, std::memory_order_release);
    _thd_map = std::thread([this, set_key]() {
        uint8_t event[3] = {0};
        while (_execute_map.load(std::memory_order_acquire)) {
            std::unique_lock<std::mutex> lk(m);
            cv_map.wait(lk);
            //do output
            if (_execute_map.load(std::memory_order_acquire)) {
                int i = mmessage.next();
                while ( i>=0) {
                    mmessage.fill(event, i);
                    uint8_t channel = event[0]&0x0f;
                    uint8_t num = event[0] & 0xf0;
                    uint8_t note = event[1];
                    uint8_t velocity = event[2];
                    // do mapping here
                    if (kbm_map[int(note)]<128) {
                        note = uint8_t(kbm_map[int(note)]);
                        send_to_jack(num | channel, note, velocity, 3, true);
                        if (num == 0x90) // note pn 
                            set_key(channel, note, true);
                        else if (num == 0x80) // note off
                            set_key(channel, note, false);
                    }
                    i = mmessage.next();
                }
            }
        } 
    });
}            

bool MidiMapper::is_running() const noexcept {
    return ( _thd_map.joinable() && _execute_map.load(std::memory_order_acquire)
             && _thd_map.joinable());
}


} // namespace midimapper
