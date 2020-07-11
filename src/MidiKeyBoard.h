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


#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>

#include <cstdlib>
#include <cmath>
#include <atomic>
#include <thread>
#include <system_error>
#include <sigc++/sigc++.h>
#include <fstream>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "NsmHandler.h"
#include "xwidgets.h"


#pragma once

#ifndef MIDIKEYBOARD_H
#define MIDIKEYBOARD_H

namespace midikeyboard {

/****************************************************************
 ** class MidiMessenger
 **
 ** create, collect and send all midi events to jack_midi out buffer
 */

class MidiMessenger {
private:
    static const int max_midi_cc_cnt = 25;
    std::atomic<bool> send_cc[max_midi_cc_cnt];
    int cc_num[max_midi_cc_cnt];
    int pg_num[max_midi_cc_cnt];
    int bg_num[max_midi_cc_cnt];
    int me_num[max_midi_cc_cnt];
public:
    MidiMessenger();
    int channel;
    bool send_midi_cc(int _cc, int _pg, int _bgn, int _num);
    inline int next(int i = -1) const;
    inline int size(int i)  const { return me_num[i]; }
    inline void fill(unsigned char *midi_send, int i);
};


/****************************************************************
 ** class XJackKeyBoard
 **
 ** Create Keyboard layout and connect via MidiMessenger to jack_midi out
 ** pass all incoming midi events to jack_midi out.
 */

class XJackKeyBoard {
private:
    MidiMessenger *mmessage;
    nsmhandler::NsmSignalHandler& nsmsig;
    jack_port_t *in_port;
    jack_port_t *out_port;

    Widget_t *channel;
    Widget_t *bank;
    Widget_t *program;
    Widget_t *layout;
    Widget_t *keymap;
    Pixmap *icon;

    inline void process_midi_cc(void *buf, jack_nframes_t nframes);
    static void jack_shutdown (void *arg);
    static int jack_xrun_callback(void *arg);
    static int jack_srate_callback(jack_nframes_t samplerate, void* arg);
    static int jack_buffersize_callback(jack_nframes_t nframes, void* arg);
    static int jack_process(jack_nframes_t nframes, void *arg);

    static void get_note(Widget_t *w, int *key, bool on_off);
    static void get_velocity(Widget_t *w,int *value);
    static void get_volume(Widget_t *w,int *value);
    static void get_mod(Widget_t *w,int *value);
    static void get_detune(Widget_t *w,int *value);
    static void get_all_notes_off(Widget_t *w,int *value);
    static void get_attack(Widget_t *w,int *value);
    static void get_pitch(Widget_t *w,int *value);
    static void get_balance(Widget_t *w,int *value);
    static void get_expression(Widget_t *w,int *value);
    static void get_release(Widget_t *w,int *value);
    static void get_sustain(Widget_t *w,int *value);
    static void get_sostenuto(Widget_t *w,int *value);
    static void channel_callback(void *w_, void* user_data);
    static void bank_callback(void *w_, void* user_data);
    static void program_callback(void *w_, void* user_data);
    static void layout_callback(void *w_, void* user_data);
    static void octave_callback(void *w_, void* user_data);
    static void key_press(void *w_, void *key_, void *user_data);
    static void key_release(void *w_, void *key_, void *user_data);
    static void win_configure_callback(void *w_, void* user_data);
    static void unmap_callback(void *w_, void* user_data);
    static void map_callback(void *w_, void* user_data);
    static void draw_board(void *w_, void* user_data);
    static void win_mem_free(void *w_, void* user_data);
    void nsm_show_ui();
    void nsm_hide_ui();
public:
    XJackKeyBoard(MidiMessenger *mmessage, nsmhandler::NsmSignalHandler& nsmsig);
    ~XJackKeyBoard();
    jack_client_t *client;
    std::string client_name;
    std::string config_file;
    std::string path;
    bool has_config;
    Widget_t *win;
    Widget_t *w;
    int main_x;
    int main_y;
    int main_w;
    int main_h;
    int visible;
    int velocity;
    int mbank;
    int mprogram;
    int keylayout;
    int mchannel;

    void init_ui(Xputty *app);
    void init_jack();
    void show_ui(int present);
    void read_config();
    void save_config();
    void set_config(const char *name, const char *client_id, bool op_gui);

    static void signal_handle (int sig, XJackKeyBoard *xjmkb);
    static void exit_handle (int sig, XJackKeyBoard *xjmkb);
};


/****************************************************************
 ** class PosixSignalHandler
 **
 ** Watch for incomming system signals in a extra thread
 ** 
 */

class PosixSignalHandler {
private:
    sigset_t waitset;
    std::thread *thread;
    XJackKeyBoard *xjmkb;
    volatile bool exit;
    void signal_helper_thread();
    void create_thread();
    
public:
    PosixSignalHandler(XJackKeyBoard *xjmkb);
    ~PosixSignalHandler();
};

} // namespace midikeyboard

#endif //MIDIKEYBOARD_H_

