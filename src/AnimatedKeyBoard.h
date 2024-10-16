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


#include <cstdlib>
#include <functional>
#include <thread>
#include <atomic>

#pragma once

#ifndef ANIMATEDKEYBOARD_H
#define ANIMATEDKEYBOARD_H

/****************************************************************
 ** class AnimatedKeyBoard
 **
 ** animate midi input from jack on the keyboard in a extra thread
 ** 
 */

namespace animatedkeyboard {

class AnimatedKeyBoard {
private:
    std::atomic<bool> _execute;
    std::thread _thd;

public:
    AnimatedKeyBoard();
    ~AnimatedKeyBoard();
    void stop();
    void start(int interval, std::function<void(void)> func);
    bool is_running() const noexcept;
};

} // namespace animatedkeyboard

#endif
