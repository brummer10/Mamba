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
#include <stdio.h>
#include <cstdlib>
#include <atomic>
#include <sigc++/sigc++.h>

#include "nsm.h"

#pragma once

#ifndef NSMHANDLER_H
#define NSMHANDLER_H


namespace nsmhandler {


/****************************************************************
 ** class NsmSignalHandler
 **
 ** signal messages from GxNsmHandler to MainWindow and back.
 **
 */

class NsmSignalHandler : public sigc::trackable {

private:

public:
    bool nsm_session_control;

    sigc::signal<void > trigger_nsm_show_gui;
    sigc::signal<void >& signal_trigger_nsm_show_gui() { return trigger_nsm_show_gui; }

    sigc::signal<void > trigger_nsm_hide_gui;
    sigc::signal<void >& signal_trigger_nsm_hide_gui() { return trigger_nsm_hide_gui; }

    sigc::signal<void > trigger_nsm_save_gui;
    sigc::signal<void >& signal_trigger_nsm_save_gui() { return trigger_nsm_save_gui; }

    sigc::signal<void, const char*, const char*, bool> trigger_nsm_gui_open;
    sigc::signal<void, const char*, const char*, bool>& signal_trigger_nsm_gui_open() { return trigger_nsm_gui_open; }

public:
    sigc::signal<void > trigger_nsm_gui_is_shown;
    sigc::signal<void >& signal_trigger_nsm_gui_is_shown() { return trigger_nsm_gui_is_shown; }

    sigc::signal<void > trigger_nsm_gui_is_hidden;
    sigc::signal<void >& signal_trigger_nsm_gui_is_hidden() { return trigger_nsm_gui_is_hidden; }

    NsmSignalHandler() : sigc::trackable(), nsm_session_control(false) {};
    ~NsmSignalHandler() {};
};


/****************************************************************
 ** class NsmWatchDog
 **
 ** Watch for incomming messages from NSM server in a extra thread
 ** 
 */

class NsmWatchDog {
private:
    std::atomic<bool> _execute;
    std::thread _thd;

public:
    NsmWatchDog();
    ~NsmWatchDog();
    void stop();
    void start(int interval, std::function<void(void)> func);
    bool is_running() const noexcept;
};


/****************************************************************
 ** class NsmHandler
 **
 ** Check if NSM is calling, if so, connect the handlers
 */

class NsmHandler {
  private:
    NsmWatchDog poll;
    NsmSignalHandler *nsmsig;
    int _nsm_open (const char *name, const char *display_name,
            const char *client_id, char **out_msg);
    int _nsm_save ( char **out_msg);
    void _nsm_show ();
    void _nsm_hide ();
    void _on_nsm_is_shown();
    void _on_nsm_is_hidden();
    void _nsm_start_poll();
    nsm_client_t *nsm;
    bool wait_id;
    bool optional_gui;

  public:
    bool check_nsm(const char *name, char *argv[]);
    static void _poll_nsm(void* arg);
    static int nsm_open (const char *name, const char *display_name,
            const char *client_id, char **out_msg,  void *userdata);
    static int nsm_save ( char **out_msg, void *userdata );
    static void nsm_show ( void *userdata );
    static void nsm_hide ( void *userdata );
    NsmHandler(NsmSignalHandler *nsmsig);
    ~NsmHandler();
};

} // namespace nsmhandler

#endif //NSMHANDLER_H_

