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
#include <queue>

#ifdef ENABLE_NLS
#include <libintl.h>
#include <clocale>
#define _(S) gettext(S)
#else
#define _(S) S
#endif

#include "NsmHandler.h"
#include "Mamba.h"
#include "XJack.h"
#include "XAlsa.h"
#include "xwidgets.h"
#include "xfile-dialog.h"
#include "xmessage-dialog.h"
#include "XSynth.h"

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
    xalsa::XAlsa *xalsa;
    xsynth::XSynth *xsynth;
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
    Widget_t *filemenu;
    Widget_t *looper;
    Widget_t *view_channels;
    Widget_t *free_wheel;
    Widget_t *info;
    Widget_t *mapping;
    Widget_t *keymap;
    Widget_t *connection;
    Widget_t *inputs;
    Widget_t *outputs;
    Widget_t *alsa_inputs;
    Widget_t *alsa_outputs;
    Widget_t *bpm;
    Widget_t *songbpm;
    Widget_t *time_line;
    Widget_t *synth;
    Widget_t *synth_ui;
    Widget_t *menubar;
    Widget_t *file_remove_menu;
    Widget_t *load_midi;
    Widget_t *add_midi;
    Widget_t *sfont_menu;
    Widget_t *fs_instruments;
    Widget_t *fs_soundfont;
    Widget_t *knob_box;
    Widget_t *proc_box;
    Widget_t *view_menu;
    Widget_t *view_controller;
    Widget_t *view_proc;
    Widget_t *key_size_menu;
    Pixmap *icon;

    std::string filepath;
    std::string soundfontpath;
    std::string soundfontname;
    std::vector<std::string> alsa_ports;
    std::vector<std::string> alsa_connections;
    std::vector<std::string> alsa_oports;
    std::vector<std::string> alsa_oconnections;
    std::vector<std::string> file_names;
    std::vector<std::string> recent_files;
    std::vector<std::string> recent_sfonts;

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
    int freewheel;
    int run_one_more;
    int lchannels;
    bool need_save;
    bool pitch_scroll;
    bool view_has_changed;
    bool key_size_changed;
    int key_size;
    int view_controls;
    int view_program;
    int width_inc;

    std::string remove_sub (std::string a, std::string b);
    static void get_note(Widget_t *w, const int *key, const bool on_off) noexcept;
    static void get_all_notes_off(Widget_t *w, const int *value) noexcept;

    static void info_callback(void *w_, void* user_data);
    static void file_callback(void *w_, void* user_data);
    static void view_callback(void *w_, void* user_data);
    static void key_size_callback(void *w_, void* user_data);
    static void load_midi_callback(void *w_, void* user_data);
    static void add_midi_callback(void *w_, void* user_data);
    static void sfont_callback(void *w_, void* user_data);
    static void dialog_add_response(void *w_, void* user_data);
    static void file_remove_callback(void *w_, void* user_data);
    static void rebuild_remove_menu(void *w_, void* button, void* user_data);
    static void channel_callback(void *w_, void* user_data) noexcept;
    static void bank_callback(void *w_, void* user_data) noexcept;
    static void program_callback(void *w_, void* user_data);
    static void bpm_callback(void *w_, void* user_data) noexcept;
    static void layout_callback(void *w_, void* user_data);
    static void octave_callback(void *w_, void* user_data) noexcept;
    static void keymap_callback(void *w_, void* user_data);
    static void grab_callback(void *w_, void* user_data);
    static void synth_callback(void *w_, void* user_data);
    static void modwheel_callback(void *w_, void* user_data) noexcept;
    static void detune_callback(void *w_, void* user_data) noexcept;
    static void attack_callback(void *w_, void* user_data) noexcept;
    static void expression_callback(void *w_, void* user_data) noexcept;
    static void release_callback(void *w_, void* user_data) noexcept;
    static void volume_callback(void *w_, void* user_data) noexcept;
    static void velocity_callback(void *w_, void* user_data) noexcept;
    static void pitchwheel_callback(void *w_, void* user_data) noexcept;
    static void balance_callback(void *w_, void* user_data) noexcept;
    static void pitchwheel_release_callback(void *w_, void* button, void* user_data) noexcept;
    static void pitchwheel_press_callback(void *w_, void* button, void* user_data) noexcept;
    static void sustain_callback(void *w_, void* user_data) noexcept;
    static void sostenuto_callback(void *w_, void* user_data) noexcept;
    static void record_callback(void *w_, void* user_data);
    static void play_callback(void *w_, void* user_data) noexcept;
    static void freewheel_callback(void *w_, void* user_data) noexcept;
    static void clear_loops_callback(void *w_, void* user_data) noexcept;
    static void view_channels_callback(void *w_, void* user_data) noexcept;
    static void animate_midi_keyboard(void *w_);
    static void dialog_save_response(void *w_, void* user_data);
    static void dnd_load_response(void *w_, void* user_data);

    static void make_connection_menu(void *w_, void* button, void* user_data);
    static void connection_in_callback(void *w_, void* user_data);
    static void connection_out_callback(void *w_, void* user_data);
    static void alsa_connection_callback(void *w_, void* user_data);
    static void alsa_oconnection_callback(void *w_, void* user_data);
    static void key_press(void *w_, void *key_, void *user_data);
    static void key_release(void *w_, void *key_, void *user_data);
    static void win_configure_callback(void *w_, void* user_data);
    static void unmap_callback(void *w_, void* user_data) noexcept;
    static void map_callback(void *w_, void* user_data);
    static void draw_my_combobox_entrys(void *w_, void* user_data) noexcept;
    static void draw_menubar(void *w_, void* user_data) noexcept;
    static void draw_topbox(void *w_, void* user_data) noexcept;
    static void draw_middlebox(void *w_, void* user_data) noexcept;
    static void draw_synth_ui(void *w_, void* user_data) noexcept;
    static void mk_draw_knob(void *w_, void* user_data) noexcept;
    static void draw_button(void *w_, void* user_data) noexcept;
    static void set_play_label(void *w_, void* user_data) noexcept;
    static void win_mem_free(void *w_, void* user_data);

    static void synth_ui_callback(void *w_, void* user_data);

    static void chorus_type_callback(void *w_, void* user_data);
    static void chorus_depth_callback(void *w_, void* user_data);
    static void chorus_speed_callback(void *w_, void* user_data);
    static void chorus_level_callback(void *w_, void* user_data);
    static void chorus_voices_callback(void *w_, void* user_data);
    static void chorus_on_callback(void *w_, void* user_data);

    static void reverb_level_callback(void *w_, void* user_data);
    static void reverb_width_callback(void *w_, void* user_data);
    static void reverb_damp_callback(void *w_, void* user_data);
    static void reverb_roomsize_callback(void *w_, void* user_data);
    static void reverb_on_callback(void *w_, void* user_data);
    static void set_on_off_label(void *w_, void* user_data) noexcept;
    static void channel_pressure_callback(void *w_, void* user_data);
    static void instrument_callback(void *w_, void* user_data);
    static void soundfont_callback(void *w_, void* user_data);

    Widget_t *add_keyboard_knob(Widget_t *parent, const char * label,
                                int x, int y, int width, int height);
    Widget_t *add_keyboard_button(Widget_t *parent, const char * label,
                                int x, int y, int width, int height);
    void get_port_entrys(Widget_t *parent, jack_port_t *my_port,
                                                JackPortFlags type);
    int remove_low_dash(char *str) noexcept;
    void rounded_rectangle(cairo_t *cr,float x, float y, float width, float height);
    void pattern_in(Widget_t *w, Color_state st, int height);
    void pattern_out(Widget_t *w, int height);
    void find_next_beat_time(double *absoluteTime);
    void get_alsa_port_menu();
    void nsm_show_ui();
    void nsm_hide_ui();
    void signal_handle (int sig);
    void exit_handle (int sig);
    void quit_by_jack();
    void get_midi_in(int c, int n, bool on);
    void recent_file_manager(const char* file_);
    void build_remove_menu();
    void build_recent_menu();
    void recent_sfont_manager(const char* file_);
    void build_sfont_menu();
public:
    XKeyBoard(xjack::XJack *xjack, xalsa::XAlsa *xalsa, xsynth::XSynth *xsynth,
        mamba::MidiMessenger *mmessage, nsmhandler::NsmSignalHandler& nsmsig,
        PosixSignalHandler& xsig, AnimatedKeyBoard * animidi);
    ~XKeyBoard();

    std::string client_name;
    std::string config_file;
    std::string keymap_file;
    std::string multikeymap_file;
    std::string path;
    std::string soundfont;

    bool has_config;
    Widget_t *win;
    Widget_t *wid;
    Widget_t *fs[3];
    int visible;
    int volume;

    void init_ui(Xputty *app);
    void init_synth_ui(Widget_t *win);
    void rebuild_instrument_list();
    void rebuild_soundfont_list();
    void show_ui(int present);
    void show_synth_ui(int present);
    void read_config();
    void save_config();
    void set_config(const char *name, const char *client_id, bool op_gui);

    static void dialog_load_response(void *w_, void* user_data);
    static void synth_load_response(void *w_, void* user_data);
    static XKeyBoard* get_instance(void *w_);
};

} // namespace midikeyboard

#endif //MIDIKEYBOARD_H_

