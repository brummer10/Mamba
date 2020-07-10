#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <math.h>
#include <atomic>
#include <sigc++/sigc++.h>

#include <jack/jack.h>
#include <jack/midiport.h>

#include "xwidgets.h"
#include "xmidi_keyboard.h"

//   gcc -Wall MidiKeyBoard.c  -L. ../libxputty/libxputty/libxputty.a -o midikeyboard -I../libxputty/libxputty/include/ `pkg-config --cflags --libs jack` `pkg-config --cflags --libs cairo x11` -lm 

class MidiCC {
private:
    static const int max_midi_cc_cnt = 25;
    std::atomic<bool> send_cc[max_midi_cc_cnt];
    int cc_num[max_midi_cc_cnt];
    int pg_num[max_midi_cc_cnt];
    int bg_num[max_midi_cc_cnt];
    int me_num[max_midi_cc_cnt];
public:
    MidiCC();
    bool send_midi_cc(int _cc, int _pg, int _bgn, int _num);
    inline int next(int i = -1) const;
    inline int size(int i)  const { return me_num[i]; }
    inline void fill(unsigned char *midi_send, int i);
};

inline int MidiCC::next(int i) const {
    while (++i < max_midi_cc_cnt) {
        if (send_cc[i].load(std::memory_order_acquire)) {
            return i;
        }
    }
    return -1;
}

inline void MidiCC::fill(unsigned char *midi_send, int i) {
    if (size(i) == 3) {
        midi_send[2] =  bg_num[i];
    }
    midi_send[1] = pg_num[i];    // program value
    midi_send[0] = cc_num[i];    // controller+ channel
    send_cc[i].store(false, std::memory_order_release);
}

MidiCC::MidiCC() {
    for (int i = 0; i < max_midi_cc_cnt; i++) {
        send_cc[i] = false;
    }
}

bool MidiCC::send_midi_cc(int _cc, int _pg, int _bgn, int _num) {
    //_cc |=c-1;
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


class Xjack {
private:
    MidiCC *mmessage:
    void jack_shutdown (void *arg);
    int jack_xrun_callback(void *arg);
    int jack_srate_callback(jack_nframes_t samplerate, void* arg);
    int jack_buffersize_callback(jack_nframes_t nframes, void* arg);
    int jack_process(jack_nframes_t nframes, void *arg);
    
public:
    Xjack(MidiCC *mmessage);
    jack_client_t *client;
    jack_port_t *in_port;
    jack_port_t *out_port;
};

Xjack::Xjack(MidiCC *mmessage_)
    : mmessage(mmessage_) {
}

class XUi {
private:
    
public:
    MidiCC *mmessage:
    Xjack *xjack;
    Xputty app;
    Widget_t *w;
    Widget_t *win;
    XUi(MidiCC *mmessage);
    void signal_handler (int sig);
    void process_midi_cc(void *buf, jack_nframes_t nframes);
    void get_note(Widget_t *w, int *key, bool on_off);
}

XUi::XUi (MidiCC *mmessage_, Xjack *xjack_)
    : mmessage(mmessage_),
    xjack(xjack_),
    app(0),
    w(0) {
}

void Xjack::jack_shutdown (void *arg) {
    quit(w);
    exit (1);
}

int Xjack::jack_xrun_callback(void *arg) {
    fprintf (stderr, "Xrun \r");
    return 0;
}

int Xjack::jack_srate_callback(jack_nframes_t samplerate, void* arg) {
    fprintf (stderr, "Samplerate %iHz \n", samplerate);
    return 0;
}

int Xjack::jack_buffersize_callback(jack_nframes_t nframes, void* arg) {
    fprintf (stderr, "Buffersize is %i samples \n", nframes);
    return 0;
}

void Xjack::process_midi_cc(void *buf, jack_nframes_t nframes) {
    // midi CC output processing
    for (int i = mmessage.next(); i >= 0; i = mmessage.next(i)) {
        unsigned char* midi_send = jack_midi_event_reserve(buf, i, mmessage.size(i));
        if (midi_send) {
            mmessage.fill(midi_send, i);
        }
    }
}

int Xjack::jack_process(jack_nframes_t nframes, void *arg) {
    void *in = jack_port_get_buffer (in_port, nframes);
    void *out = jack_port_get_buffer (out_port, nframes);
    memcpy (out, in,
        sizeof (unsigned char) * nframes);
    process_midi_cc(out,nframes);
    return 0;
}

void XUi::signal_handler (int sig) {
    jack_client_close (client);
    if (sig == SIGTERM) quit(w);
    fprintf (stderr, "\n signal %i received, exiting ...\n", sig);
    exit (0);
}

void XUi::get_note(Widget_t *w, int *key, bool on_off) {
    Widget *win = get_toplevel_widget(w->app.main);
    XUi *xui = (XUi*) win->parent_struct;
    if (on_off) {
        xui->mmessage.send_midi_cc(0x90, (*key),63, 3);
    } else {
        xui->mmessage.send_midi_cc(0x80, (*key),63, 3);
    }
}

int
main (int argc, char *argv[])
{
    MidiCC mmessage;
    Xjack xjack(&mmessage_);
    XUi xui(&mmessage_, &xjack);



    main_init(&xui.app);
    
    xui.win = create_window(&xui.app, DefaultRootWindow(xui.app.dpy), 0, 0, 700, 200);
    xui.w = create_widget(&xui.app, win, 0, 0, 700, 200);
    widget_set_title(xui.w, "MidiKeyBoard");
    add_midi_keyboard(xui.w, "MidiKeyBoard", 0, 0, 700, 200);
    widget_show_all(xui.win);
    xui.win->parent_struct = &xui;
    MidiKeyboard *keys = (MidiKeyboard*)xui.w->parent_struct;

    keys->mk_send_note = get_note;

    if ((xjack.client = jack_client_open ("MidiKeyBoard", JackNullOption, NULL)) == 0) {
        fprintf (stderr, "jack server not running?\n");
        quit(xui.win);
        return 1;
    }

    xjack.in_port = jack_port_register(
                  xjack.client, "in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);
    xjack.out_port = jack_port_register(
                   xjack.client, "out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    signal (SIGQUIT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGHUP, signal_handler);
    signal (SIGINT, signal_handler);

    jack_set_xrun_callback(xjack.client, xjack.jack_xrun_callback, 0);
    jack_set_sample_rate_callback(xjack.client, xjack.jack_srate_callback, 0);
    jack_set_buffer_size_callback(xjack.client, xjack.jack_buffersize_callback, 0);
    jack_set_process_callback(xjack.client, xjack.jack_process, 0);
    jack_on_shutdown (xjack.client, xjack.jack_shutdown, 0);

    if (jack_activate (xjack.client)) {
        fprintf (stderr, "cannot activate client");
        quit(xui.win);
        return 1;
    }

    if (!jack_is_realtime(xjack.client)) {
        fprintf (stderr, "jack isn't running with realtime priority\n");
    } else {
        fprintf (stderr, "jack running with realtime priority\n");
    }

    main_run(&xui.app);
   
    main_quit(&xui.app);

    jack_client_close (xjack.client);
    exit (0);
}
