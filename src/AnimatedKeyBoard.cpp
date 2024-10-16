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


#include "AnimatedKeyBoard.h"


/****************************************************************
 ** class AnimatedKeyBoard
 **
 ** animate midi input from jack on the keyboard in a extra thread
 ** 
 */

namespace animatedkeyboard {

AnimatedKeyBoard::AnimatedKeyBoard() 
    :_execute(false) {
}

AnimatedKeyBoard::~AnimatedKeyBoard() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
}

void AnimatedKeyBoard::stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) {
        _thd.join();
    }
}

void AnimatedKeyBoard::start(int interval, std::function<void(void)> func) {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
    _execute.store(true, std::memory_order_release);
    _thd = std::thread([this, interval, func]() {
        while (_execute.load(std::memory_order_acquire)) {
            func();                   
            std::this_thread::sleep_for(
                std::chrono::milliseconds(interval));
        }
    });
}

bool AnimatedKeyBoard::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}

} // namespace animatedkeyboard

