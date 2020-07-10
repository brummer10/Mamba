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


#include "MidiKeyBoard.h"
#include "xkeyboard.h"

//   g++ -Wall NsmHandler.cpp MidiKeyBoard.cpp xkeyboard.c -L. ../libxputty/libxputty/libxputty.a -o midikeyboard -I./ -I../libxputty/libxputty/include/ `pkg-config --cflags --libs jack cairo x11 sigc++-2.0 liblo` -lm -pthread

namespace midikeyboard {


/****************************************************************
 ** class MidiMessenger
 **
 ** create, collect and send all midi events to jack_midi out buffer
 */

MidiMessenger::MidiMessenger() {
    channel = 0;
    for (int i = 0; i < max_midi_cc_cnt; i++) {
        send_cc[i] = false;
    }
}

inline int MidiMessenger::next(int i) const {
    while (++i < max_midi_cc_cnt) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            return i;
        }
    }
    return -1;
}

inline void MidiMessenger::fill(unsigned char *midi_send, int i) {
    if (size(i) == 3) {
        midi_send[2] =  bg_num[i];
    }
    midi_send[1] = pg_num[i];    // program value
    midi_send[0] = cc_num[i];    // controller+ channel
    send_cc[i].store(false, std::memory_order_release);
}

bool MidiMessenger::send_midi_cc(int _cc, int _pg, int _bgn, int _num) {
    _cc |=channel;
    for(int i = 0; i < max_midi_cc_cnt; i++) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            if (cc_num[i] == _cc && pg_num[i] == _pg &&
                bg_num[i] == _bgn && me_num[i] == _num)
                return true;
        } else if (!send_cc[i].load(std::memory_order_acquire)) {
            cc_num[i] = _cc;
            pg_num[i] = _pg;
            bg_num[i] = _bgn;
            me_num[i] = _num;
            send_cc[i].store(true, std::memory_order_release);
            return true;
        }
    }
    return false;
}


/****************************************************************
 ** class XJackKeyBoard
 **
 ** Create Keyboard layout and connect via MidiMessenger to jack_midi out
 ** pass all incoming midi events to jack_midi out.
 */

XJackKeyBoard::XJackKeyBoard(MidiMessenger *mmessage_, nsmhandler::NsmSignalHandler& nsmsig_)
    : mmessage(mmessage_),
    nsmsig(nsmsig_) {
    icon = NULL;
    client_name = "MidiKeyBoard";
    if (getenv("XDG_CONFIG_HOME")) {
        path = getenv("XDG_CONFIG_HOME");
        config_file = path +"/MidiKeyBoard.conf";
    } else {
        path = getenv("HOME");
        config_file = path +"/.config/MidiKeyBoard.conf";
    }
    has_config = false;
    main_x = 0;
    main_y = 0;
    main_w = 700;
    main_h = 240;
    visible = 1;
    velocity = 127;
    mbank = 0;
    mprogram = 0;
    keylayout = 0;
    mchannel = 0;

    nsmsig.signal_trigger_nsm_show_gui().connect(
        sigc::mem_fun(this, &XJackKeyBoard::nsm_show_ui));

    nsmsig.signal_trigger_nsm_hide_gui().connect(
        sigc::mem_fun(this, &XJackKeyBoard::nsm_hide_ui));

    nsmsig.signal_trigger_nsm_save_gui().connect(
        sigc::mem_fun(this, &XJackKeyBoard::save_config));

    nsmsig.signal_trigger_nsm_gui_open().connect(
        sigc::mem_fun(this, &XJackKeyBoard::set_config));
}

XJackKeyBoard::~XJackKeyBoard() {
    if (icon) {
        XFreePixmap(w->app->dpy, (*icon));
        icon = NULL;
    }
}

void XJackKeyBoard::init_jack() {
    if ((client = jack_client_open (client_name.c_str(), JackNullOption, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        quit(win);
        exit (1);
    }

    in_port = jack_port_register(
                  client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    out_port = jack_port_register(
                   client, "out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    jack_set_xrun_callback(client, jack_xrun_callback, this);
    jack_set_sample_rate_callback(client, jack_srate_callback, this);
    jack_set_buffer_size_callback(client, jack_buffersize_callback, this);
    jack_set_process_callback(client, jack_process, this);
    jack_on_shutdown (client, jack_shutdown, this);

    if (jack_activate (client)) {
        fprintf (stderr, "cannot activate client");
        quit(win);
        exit (1);
    }

    if (!jack_is_realtime(client)) {
        fprintf (stderr, "jack isn't running with realtime priority\n");
    } else {
        fprintf (stderr, "jack running with realtime priority\n");
    }
}

inline void XJackKeyBoard::process_midi_cc(void *buf, jack_nframes_t nframes) {
    // midi output processing
    for (int i = mmessage->next(); i >= 0; i = mmessage->next(i)) {
        unsigned char* midi_send = jack_midi_event_reserve(buf, i, mmessage->size(i));
        if (midi_send) {
            mmessage->fill(midi_send, i);
        }
    }
}

// static
void XJackKeyBoard::jack_shutdown (void *arg) {
    XJackKeyBoard *xjmkb = (XJackKeyBoard*)arg;
    quit(xjmkb->win);
    exit (1);
}

// static
int XJackKeyBoard::jack_xrun_callback(void *arg) {
    fprintf (stderr, "Xrun \r");
    return 0;
}

// static
int XJackKeyBoard::jack_srate_callback(jack_nframes_t samplerate, void* arg) {
    fprintf (stderr, "Samplerate %iHz \n", samplerate);
    return 0;
}

// static
int XJackKeyBoard::jack_buffersize_callback(jack_nframes_t nframes, void* arg) {
    fprintf (stderr, "Buffersize is %i samples \n", nframes);
    return 0;
}

// static
int XJackKeyBoard::jack_process(jack_nframes_t nframes, void *arg) {
    XJackKeyBoard *xjmkb = (XJackKeyBoard*)arg;
    void *in = jack_port_get_buffer (xjmkb->in_port, nframes);
    void *out = jack_port_get_buffer (xjmkb->out_port, nframes);
    memcpy (out, in,
        sizeof (unsigned char) * nframes);
    xjmkb->process_midi_cc(out,nframes);
    return 0;
}

// GUI stuff starts here
void XJackKeyBoard::set_config(const char *name, const char *client_id, bool op_gui) {
    client_name = client_id;
    path = name;
    config_file = path + ".config";
    if (op_gui) {
        visible = 0;
    } else {
        visible = 1;
    }
}


void XJackKeyBoard::read_config() {
    std::ifstream infile(config_file);
    std::string line;
    if (infile.is_open()) {
        getline( infile, line );
        if (!line.empty()) main_x = std::stoi(line);
        getline( infile, line );
        if (!line.empty()) main_y = std::stoi(line);
        getline( infile, line );
        if (!line.empty()) main_w = std::stoi(line);
        getline( infile, line );
        if (!line.empty()) main_h = std::stoi(line);
        getline( infile, line );
        if (!line.empty()) visible = std::stoi(line);
        getline( infile, line );
        if (!line.empty()) keylayout = std::stoi(line);
        getline( infile, line );
        if (!line.empty()) mchannel = std::stoi(line);
        getline( infile, line );
        if (!line.empty()) velocity = std::stoi(line);
        infile.close();
        has_config = true;
    }
}

void XJackKeyBoard::save_config() {
    std::ofstream outfile(config_file);
     if (outfile.is_open()) {
         outfile << main_x << std::endl;
         outfile << main_y << std::endl;
         outfile << main_w << std::endl;
         outfile << main_h << std::endl;
         outfile << visible << std::endl;
         outfile << keylayout << std::endl;
         outfile << mchannel << std::endl;
         outfile << velocity << std::endl;
         outfile.close();
     }
}

void XJackKeyBoard::nsm_show_ui() {
    widget_show_all(win);
    XFlush(win->app->dpy);
    XMoveWindow(win->app->dpy,win->widget, main_x, main_y);
    nsmsig.trigger_nsm_gui_is_shown();
}

void XJackKeyBoard::nsm_hide_ui() {
    widget_hide(win);
    XFlush(win->app->dpy);
    nsmsig.trigger_nsm_gui_is_hidden();
}

void XJackKeyBoard::show_ui(int present) {
    if(present) {
        widget_show_all(win);
        XMoveWindow(win->app->dpy,win->widget, main_x, main_y);
        if(nsmsig.nsm_session_control)
            nsmsig.trigger_nsm_gui_is_shown();
    }else {
        widget_hide(win);
        if(nsmsig.nsm_session_control)
            nsmsig.trigger_nsm_gui_is_hidden();
    }
}

void XJackKeyBoard::draw_board(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    use_bg_color_scheme(w, NORMAL_);
    cairo_paint (w->crb);
}

void XJackKeyBoard::init_ui(Xputty *app) {
    win = create_window(app, DefaultRootWindow(app->dpy), 0, 0, 700, 240);
    XSelectInput(win->app->dpy, win->widget,StructureNotifyMask|ExposureMask|KeyPressMask 
                    |EnterWindowMask|LeaveWindowMask|ButtonReleaseMask|KeyReleaseMask
                    |ButtonPressMask|Button1MotionMask|PointerMotionMask);
    widget_set_icon_from_png(win,icon,LDVAR(midikeyboard_png));
    widget_set_title(win, client_name.c_str());
    win->flags |= NO_AUTOREPEAT;
    win->parent_struct = this;
    win->func.expose_callback = draw_board;
    win->func.configure_notify_callback = win_configure_callback;
    win->func.map_notify_callback = map_callback;
    win->func.unmap_notify_callback = unmap_callback;
    win->func.key_press_callback = key_press;
    win->func.key_release_callback = key_release;

    XSizeHints* win_size_hints;
    win_size_hints = XAllocSizeHints();
    win_size_hints->flags =  PMinSize|PBaseSize|PMaxSize|PWinGravity;
    win_size_hints->min_width = 700/2;
    win_size_hints->min_height = 240/2;
    win_size_hints->base_width = 700;
    win_size_hints->base_height = 240;
    win_size_hints->max_width = 1875;
    win_size_hints->max_height = 240*2;
    win_size_hints->win_gravity = CenterGravity;
    XSetWMNormalHints(win->app->dpy, win->widget, win_size_hints);
    XFree(win_size_hints);

    XResizeWindow (win->app->dpy, win->widget, main_w, main_h);

    add_label(win,"Channel:",10,5,60,20);
    channel =  add_combobox(win, "Channel", 70, 5, 60, 30);
    channel->flags |= NO_AUTOREPEAT;
    channel->scale.gravity = ASPECT;
    combobox_add_numeric_entrys(channel,1,16);
    combobox_set_active_entry(channel, 0);
    set_adjustment(channel->adj,0.0, 0.0, 0.0, 15.0, 1.0, CL_ENUM);
    channel->func.value_changed_callback = channel_callback;
    channel->func.key_press_callback = key_press;
    channel->func.key_release_callback = key_release;

    add_label(win,"Bank:",140,5,60,20);
    bank =  add_combobox(win, "Bank", 200, 5, 60, 30);
    bank->flags |= NO_AUTOREPEAT;
    bank->scale.gravity = ASPECT;
    combobox_add_numeric_entrys(bank,0,127);
    combobox_set_active_entry(bank, 0);
    set_adjustment(bank->adj,0.0, 0.0, 0.0, 15.0, 1.0, CL_ENUM);
    bank->func.value_changed_callback = bank_callback;
    bank->func.key_press_callback = key_press;
    bank->func.key_release_callback = key_release;

    add_label(win,"Program:",260,5,60,20);
    program =  add_combobox(win, "Program", 320, 5, 60, 30);
    program->flags |= NO_AUTOREPEAT;
    program->scale.gravity = ASPECT;
    combobox_add_numeric_entrys(program,0,127);
    combobox_set_active_entry(program, 0);
    set_adjustment(program->adj,0.0, 0.0, 0.0, 15.0, 1.0, CL_ENUM);
    program->func.value_changed_callback = program_callback;
    program->func.key_press_callback = key_press;
    program->func.key_release_callback = key_release;

    layout = add_combobox(win, "", 390, 5, 130, 30);
    layout->data = LAYOUT;
    layout->flags |= NO_AUTOREPEAT;
    layout->scale.gravity = ASPECT;
    combobox_add_entry(layout,"qwertz");
    combobox_add_entry(layout,"qwerty");
    combobox_add_entry(layout,"azerty");
    combobox_set_active_entry(layout, 0);
    set_adjustment(layout->adj,0.0, 0.0, 0.0, 2.0, 1.0, CL_ENUM);
    layout->func.value_changed_callback = layout_callback;
    layout->func.key_press_callback = key_press;
    layout->func.key_release_callback = key_release;


    keymap = add_hslider(win, "Keyboard mapping", 520, 2, 160, 35);
    keymap->data = KEYMAP;
    keymap->flags |= NO_AUTOREPEAT;
    set_adjustment(keymap->adj,2.0, 2.0, 0.0, 4.0, 1.0, CL_CONTINUOS);
    adj_set_scale(keymap->adj, 0.05);
    keymap->func.value_changed_callback = octave_callback;
    keymap->func.key_press_callback = key_press;
    keymap->func.key_release_callback = key_release;

    w = create_widget(app, win, 0, 40, 700, 200);
    w->flags &= ~USE_TRANSPARENCY;
    add_midi_keyboard(w, "MidiKeyBoard", 0, 0, 700, 200);

    MidiKeyboard *keys = (MidiKeyboard*)w->parent_struct;

    keys->mk_send_note = get_note;
    keys->mk_send_velocity = get_velocity;
    keys->mk_send_volume = get_volume;
    keys->mk_send_mod = get_mod;
    keys->mk_send_detune = get_detune;
    keys->mk_send_all_sound_off = get_all_notes_off;
    keys->mk_send_attack = get_attack;
    keys->mk_send_expression = get_expression;
    keys->mk_send_release = get_release;
    keys->mk_send_pitch = get_pitch;
    keys->mk_send_balance = get_balance;
    keys->mk_send_sustain = get_sustain;
    keys->mk_send_sostenuto = get_sostenuto;

    if (!has_config) {
        Screen *screen = DefaultScreenOfDisplay(win->app->dpy);
        main_x = screen->width/2 - main_w/2;
        main_y = screen->height/2 - main_h/2; 
    }
    combobox_set_active_entry(channel, mchannel);
    combobox_set_active_entry(layout, keylayout);
    adj_set_value(keys->w[6]->adj, velocity);
}

// static
void XJackKeyBoard::win_configure_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    if (attrs.map_state != IsViewable) return;
    int x1, y1;
    Window child;
    XTranslateCoordinates( win->app->dpy, win->widget, DefaultRootWindow(
                    win->app->dpy), 0, 0, &x1, &y1, &child );

    xjmkb->main_x = x1;
    xjmkb->main_y = y1;
    xjmkb->main_w = attrs.width;
    xjmkb->main_h = attrs.height;
}

// static
void XJackKeyBoard::map_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->visible = 1;
}

// static
void XJackKeyBoard::unmap_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->visible = 0;
}

// static
void XJackKeyBoard::get_note(Widget_t *w, int *key, bool on_off) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    if (on_off) {
        xjmkb->mmessage->send_midi_cc(0x90, (*key),xjmkb->velocity, 3);
    } else {
        xjmkb->mmessage->send_midi_cc(0x80, (*key),xjmkb->velocity, 3);
    }
}

// static
void XJackKeyBoard::get_velocity(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;    
    xjmkb->velocity = (*value); 
}

// static
void XJackKeyBoard::get_volume(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 39, (*value), 3);
}

// static
void XJackKeyBoard::get_mod(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 1, (*value), 3);
}

// static
void XJackKeyBoard::get_detune(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 94, (*value), 3);
}

// static
void XJackKeyBoard::get_all_notes_off(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 123, 0, 3);
}

// static
void XJackKeyBoard::get_attack(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 73, (*value), 3);
}

// static
void XJackKeyBoard::get_expression(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 11, (*value), 3);
}

// static
void XJackKeyBoard::get_release(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 72, (*value), 3);
}

// static
void XJackKeyBoard::get_pitch(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    unsigned int change = (unsigned int)(128 * (*value));
    unsigned int low = change & 0x7f;  // Low 7 bits
    unsigned int high = (change >> 7) & 0x7f;  // High 7 bits
    xjmkb->mmessage->send_midi_cc(0xE0,  low, high, 3);
}

// static
void XJackKeyBoard::get_balance(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 8, (*value), 3);
}

// static
void XJackKeyBoard::get_sustain(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 64, (*value)*127, 3);
}

// static
void XJackKeyBoard::get_sostenuto(Widget_t *w,int *value) {
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->send_midi_cc(0xB0, 66, (*value)*127, 3);
}

// static
void XJackKeyBoard::channel_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mmessage->channel = xjmkb->mchannel = (int)adj_get_value(w->adj);
}

// static
void XJackKeyBoard::bank_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mbank = (int)adj_get_value(w->adj);
    xjmkb->mmessage->send_midi_cc(0xB0, 32, xjmkb->mbank, 3);
    xjmkb->mmessage->send_midi_cc(0xC0, xjmkb->mprogram, 0, 2);
}

// static
void XJackKeyBoard::program_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->mprogram = (int)adj_get_value(w->adj);
    xjmkb->mmessage->send_midi_cc(0xB0, 32, xjmkb->mbank, 3);
    xjmkb->mmessage->send_midi_cc(0xC0, xjmkb->mprogram, 0, 2);
}

// static
void XJackKeyBoard::layout_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    MidiKeyboard *keys = (MidiKeyboard*)xjmkb->w->parent_struct;
    keys->layout = xjmkb->keylayout = (int)adj_get_value(w->adj);
}

// static
void XJackKeyBoard::octave_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    MidiKeyboard *keys = (MidiKeyboard*)xjmkb->w->parent_struct;
    keys->octave = (int)12*adj_get_value(w->adj);
    expose_widget(xjmkb->w);
}

// static
void XJackKeyBoard::key_press(void *w_, void *key_, void *user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->w->func.key_press_callback(xjmkb->w, key_, user_data);
}

// static
void XJackKeyBoard::key_release(void *w_, void *key_, void *user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XJackKeyBoard *xjmkb = (XJackKeyBoard*) win->parent_struct;
    xjmkb->w->func.key_release_callback(xjmkb->w, key_, user_data);
}

// static
void XJackKeyBoard::signal_handle (int sig, XJackKeyBoard *xjmkb) {
    if (sig == SIGTERM) quit(xjmkb->win);
    jack_client_close (xjmkb->client);
    fprintf (stderr, "\n%s: signal %i received, exiting ...\n",xjmkb->client_name.c_str(), sig);
    exit (0);
}


/****************************************************************
 ** class PosixSignalHandler
 **
 ** Watch for incomming system signals in a extra thread
 ** 
 */

PosixSignalHandler::PosixSignalHandler(XJackKeyBoard *xjmkb_)
    : waitset(),
      thread(nullptr),
      xjmkb(xjmkb_),
      exit(false) {
    sigemptyset(&waitset);

    sigaddset(&waitset, SIGINT);
    sigaddset(&waitset, SIGQUIT);
    sigaddset(&waitset, SIGTERM);
    sigaddset(&waitset, SIGHUP);
    sigaddset(&waitset, SIGKILL);

    sigprocmask(SIG_BLOCK, &waitset, NULL);
    create_thread();
}

PosixSignalHandler::~PosixSignalHandler() {
    if (thread) {
        exit = true;
        pthread_kill(thread->native_handle(), SIGINT);
        thread->join();
        delete thread;
    }
    sigprocmask(SIG_UNBLOCK, &waitset, NULL);
}

void PosixSignalHandler::create_thread() {
    try {
        thread = new std::thread(
            sigc::mem_fun(*this, &PosixSignalHandler::signal_helper_thread));
    } catch (std::system_error& e) {
        fprintf(stderr,"Thread create failed (signal): %s", e.what());
    }
}

void PosixSignalHandler::signal_helper_thread() {

    pthread_sigmask(SIG_BLOCK, &waitset, NULL);
    while (true) {
        int sig;
        int ret = sigwait(&waitset, &sig);
        if (exit) {
            break;
        }
        if (ret != 0) {
            assert(errno == EINTR);
            continue;
        }
        switch (sig) {
        case SIGTERM:
        case SIGINT:
        case SIGQUIT:
        case SIGHUP:
            xjmkb->signal_handle (sig, xjmkb);
            break;
        default:
            assert(false);
        }
    }
}

} // namespace midikeyboard


/****************************************************************
 ** main
 **
 ** init the classes, create the UI, open jackd-client and start application
 ** 
 */

int main (int argc, char *argv[]) {
    XInitThreads();
    Xputty app;

    midikeyboard::MidiMessenger mmessage;
    nsmhandler::NsmSignalHandler nsmsig;
    midikeyboard::XJackKeyBoard xjmkb(&mmessage, nsmsig);
    midikeyboard::PosixSignalHandler xsig(&xjmkb);
    nsmhandler::NsmWatchDog poll;
    nsmhandler::NsmHandler nsmh(&poll, &nsmsig);

    nsmsig.nsm_session_control = nsmh.check_nsm(argv);

    xjmkb.read_config();

    main_init(&app);
    
    xjmkb.init_ui(&app);
    xjmkb.init_jack();

    xjmkb.show_ui(xjmkb.visible);

    main_run(&app);
   
    main_quit(&app);
    xjmkb.save_config();
    jack_client_close (xjmkb.client);

    exit (0);

} // namespace midikeyboard

