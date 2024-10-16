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

#include <thread>
#include <sigc++/sigc++.h>

#pragma once

#ifndef POSIXSIGNALHANDLER_H
#define POSIXSIGNALHANDLER_H

/****************************************************************
 ** class PosixSignalHandler
 **
 ** Watch for incoming system signals in a extra thread
 ** 
 */

namespace signalhandler {

class PosixSignalHandler : public sigc::trackable {
private:
    sigset_t waitset;
    std::thread *thread;
    volatile bool exit;
    void signal_helper_thread();
    void create_thread();
    
public:
    PosixSignalHandler();
    ~PosixSignalHandler();

    sigc::signal<void, int> trigger_quit_by_posix;
    sigc::signal<void, int>& signal_trigger_quit_by_posix() { return trigger_quit_by_posix; }

    sigc::signal<void, int> trigger_kill_by_posix;
    sigc::signal<void, int>& signal_trigger_kill_by_posix() { return trigger_kill_by_posix; }
};

} // namespace signalhandler

#endif // POSIXSIGNALHANDLER_H
