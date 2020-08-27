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
#include <vector>
#include <thread>
#include <condition_variable>
#include <system_error>
#include <sigc++/sigc++.h>
#include <fstream>
#include <iostream>
#include <sstream>

#include "NsmHandler.h"
#include "Mamba.h"
#include "XJack.h"
#include "xwidgets.h"
#include "xfile-dialog.h"
#include "xmessage-dialog.h"


#pragma once

#ifndef MIDIKEYBOARD_H
#define MIDIKEYBOARD_H


namespace midikeyboard {

#define CPORTS 11

typedef enum {
    PITCHBEND,
    MODULATION,
    CELESTE,
    ATTACK_TIME,
    RELEASE_TIME,
    VOLUME,
    VELOCITY,
    SUSTAIN,
    SOSTENUTO,
    BALANCE,
    EXPRESSION,
    KEYMAP,
    LAYOUT,
    
}ControlPorts;


/****************************************************************
 ** class PosixSignalHandler
 **
 ** Watch for incomming system signals in a extra thread
 ** 
 */

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

/****************************************************************
 ** class AnimatedKeyBoard
 **
 ** animate midi input from jack on the keyboard in a extra thread
 ** 
 */

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


/****************************************************************
 ** class XKeyBoard
 **
 ** Create Keyboard layout and connect via MidiMessenger to jack_midi out
 **
 */

class XKeyBoard {
private:
    xjack::XJack *xjack;
    mamba::MidiSave save;
    mamba::MidiLoad load;
    mamba::MidiMessenger *mmessage;
    AnimatedKeyBoard * animidi;
    nsmhandler::NsmSignalHandler& nsmsig;
    PosixSignalHandler& xsig;

    Widget_t *w[CPORTS];
    Widget_t *channel;
    Widget_t *bank;
    Widget_t *program;
    Widget_t *octavemap;
    Widget_t *record;
    Widget_t *play;
    Widget_t *menubar;
    Widget_t *info;
    Widget_t *mapping;
    Widget_t *keymap;
    Widget_t *connection;
    Widget_t *bpm;
    Widget_t *songbpm;
    Pixmap *icon;

    int main_x;
    int main_y;
    int main_w;
    int main_h;
    int velocity;
    int mbank;
    int mprogram;
    int mbpm;
    int song_bpm;
    int keylayout;
    int octave;
    int mchannel;
    int run_one_more;
    bool need_save;

    static void get_note(Widget_t *w, const int *key, const bool on_off);
    static void get_all_notes_off(Widget_t *w, const int *value);

    static void info_callback(void *w_, void* user_data);
    static void file_callback(void *w_, void* user_data);
    static void channel_callback(void *w_, void* user_data);
    static void bank_callback(void *w_, void* user_data);
    static void program_callback(void *w_, void* user_data);
    static void bpm_callback(void *w_, void* user_data);
    static void layout_callback(void *w_, void* user_data);
    static void octave_callback(void *w_, void* user_data);
    static void modwheel_callback(void *w_, void* user_data);
    static void detune_callback(void *w_, void* user_data);
    static void attack_callback(void *w_, void* user_data);
    static void expression_callback(void *w_, void* user_data);
    static void release_callback(void *w_, void* user_data);
    static void volume_callback(void *w_, void* user_data);
    static void velocity_callback(void *w_, void* user_data);
    static void pitchwheel_callback(void *w_, void* user_data);
    static void balance_callback(void *w_, void* user_data);
    static void sustain_callback(void *w_, void* user_data);
    static void sostenuto_callback(void *w_, void* user_data);
    static void record_callback(void *w_, void* user_data);
    static void play_callback(void *w_, void* user_data);
    static void animate_midi_keyboard(void *w_);
    static void dialog_save_response(void *w_, void* user_data);

    static void make_connection_menu(void *w_, void* button, void* user_data);
    static void connection_callback(void *w_, void* user_data);
    static void key_press(void *w_, void *key_, void *user_data);
    static void key_release(void *w_, void *key_, void *user_data);
    static void win_configure_callback(void *w_, void* user_data);
    static void unmap_callback(void *w_, void* user_data);
    static void map_callback(void *w_, void* user_data);
    static void draw_board(void *w_, void* user_data);
    static void mk_draw_knob(void *w_, void* user_data);
    static void win_mem_free(void *w_, void* user_data);

    Widget_t *add_keyboard_knob(Widget_t *parent, const char * label,
                                int x, int y, int width, int height);
    void nsm_show_ui();
    void nsm_hide_ui();
    void signal_handle (int sig);
    void exit_handle (int sig);
    void quit_by_jack();
    void get_midi_in(int n, bool on);
public:
    XKeyBoard(xjack::XJack *xjack, mamba::MidiMessenger *mmessage,
        nsmhandler::NsmSignalHandler& nsmsig, PosixSignalHandler& xsig, 
        AnimatedKeyBoard * animidi);
    ~XKeyBoard();

    std::string client_name;
    std::string config_file;
    std::string path;
    std::string filepath;

    bool has_config;
    Widget_t *win;
    Widget_t *wid;
    int visible;

    void init_ui(Xputty *app);
    void show_ui(int present);
    void read_config();
    void save_config();
    void set_config(const char *name, const char *client_id, bool op_gui);

    static void dialog_load_response(void *w_, void* user_data);
};

} // namespace midikeyboard

#endif //MIDIKEYBOARD_H_

