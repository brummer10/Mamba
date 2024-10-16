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
#include "xmkeyboard.h"


/****************************************************************
 ** main
 **
 ** init the classes, create the UI, open jackd-client and start application
 ** 
 */

int main (int argc, char *argv[]) {
    auto t1 = std::chrono::high_resolution_clock::now();

#ifdef ENABLE_NLS
    // set Message type to locale to fetch localisation support
    std::setlocale (LC_MESSAGES, "");
    // set Ctype to C to avoid symbol clashes from different locales
    std::setlocale (LC_CTYPE, "C");
    bindtextdomain(GETTEXT_PACKAGE, LOCAL_DIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);
#endif

    if(0 == XInitThreads()) 
        fprintf(stderr, "Warning: XInitThreads() failed\n");

    signalhandler::PosixSignalHandler xsig;

    Xputty app;

    mamba::MidiMessenger mmessage;
    nsmhandler::NsmSignalHandler nsmsig;
    animatedkeyboard::AnimatedKeyBoard  animidi;

    midimapper::MidiMapper midimap([&mmessage]
        (int _cc, int _pg, int _bgn, int _num, bool have_channel) noexcept
        {mmessage.send_midi_cc( _cc, _pg, _bgn, _num, have_channel);});

    xalsa::XAlsa xalsa([&mmessage]
        (int _cc, int _pg, int _bgn, int _num, bool have_channel) noexcept
        {mmessage.send_midi_cc( _cc, _pg, _bgn, _num, have_channel);},
        [&midimap] (const uint8_t* m ,uint8_t n ) noexcept {midimap.mmapper_input_notify(m,n);});

    xjack::XJack xjack(&mmessage,
        [&xalsa] (const uint8_t* m ,uint8_t n ) noexcept {xalsa.xalsa_output_notify(m,n);},
        [&xalsa] (int p ) {xalsa.xalsa_set_priority(p);},
        [&midimap] (const uint8_t* m ,uint8_t n ) noexcept {midimap.mmapper_input_notify(m,n);},
        [&midimap] (int p ) {midimap.mmapper_set_priority(p);});

    xsynth::XSynth xsynth;
    midikeyboard::XKeyBoard xjmkb(&xjack, &xalsa, &xsynth, &midimap, &mmessage, nsmsig, xsig, &animidi);
    nsmhandler::NsmHandler nsmh(&nsmsig);

    nsmsig.nsm_session_control = nsmh.check_nsm(xjmkb.client_name.c_str(), argv);

    main_init(&app);

    if (xjack.init_jack()) {

        if (!nsmsig.nsm_session_control)
            xjmkb.set_config_file();

        xjmkb.read_config();
        xjmkb.init_ui(&app);
        MambaKeyboard *keys = (MambaKeyboard*)xjmkb.wid->parent_struct;
        midimap.mmapper_start([keys] (int channel, int key, bool set)
            {mamba_set_key_in_matrix(keys->in_key_matrix[channel], key, set);});
        if (xalsa.xalsa_init(xjack.client_name.c_str(), "input", "output") >= 0) {
            xalsa.xalsa_start([keys] (int channel, int key, bool set)
                {mamba_set_key_in_matrix(keys->in_key_matrix[channel], key, set);});
        } else {
            fprintf(stderr, _("Couldn't open a alsa port, is the alsa sequencer running?\n"));
        }

        if (!xjmkb.soundfont.empty()) {
            std::string synth_instance = xjack.client_name;
            std::transform(synth_instance.begin(), synth_instance.end(), synth_instance.begin(), ::tolower);
            xsynth.setup(xjack.SampleRate, synth_instance.c_str());
            xsynth.init_synth();
            xsynth.load_soundfont(xjmkb.soundfont.c_str());
            const char **port_list = NULL;
            port_list = jack_get_ports(xjack.client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
            if (port_list) {
                for (int i = 0; port_list[i] != NULL; i++) {
                    if (strstr(port_list[i], synth_instance.c_str())) {
                        const char *my_port = jack_port_name(xjack.out_port);
                        jack_connect(xjack.client, my_port,port_list[i]);
                        break;
                    }
                }
                jack_free(port_list);
                port_list = NULL;
            }
            for (int i = 0; i<16;i++)
                mmessage.send_midi_cc(0xB0 | i, 7, xjmkb.volume[i], 3, true);
            xjmkb.fs[0]->state = 0;
            xjmkb.fs[1]->state = 0;
            xjmkb.fs[2]->state = 0;
            xjmkb.fs[3]->state = 0;
            xjmkb.rebuild_instrument_list();
            xjmkb.rebuild_soundfont_list();
            xjmkb.init_modulators(&xjmkb);
        }
        
        if (argc > 1) {

#ifdef __XDG_MIME_H__
            if(strstr(xdg_mime_get_mime_type_from_file_name(argv[1]), "midi")) {
#else
            if( access(argv[1], F_OK ) != -1 ) {
#endif
                xjmkb.dialog_load_response(xjmkb.win, (void*) &argv[1]);
            }
        }

        if (xsynth.synth_is_active()) {
            for(std::vector<mamba::MidiEvent>::iterator i = xjack.rec.play[0].begin(); i != xjack.rec.play[0].end(); ++i) {
                if (((*i).buffer[0] & 0xf0) == 0xB0 && ((*i).buffer[1]== 32 || (*i).buffer[1]== 0)) {
                    mmessage.send_midi_cc((*i).buffer[0], (*i).buffer[1], (*i).buffer[2], 3, true);
                } else if (((*i).buffer[0] & 0xf0) == 0xC0 ) {
                    mmessage.send_midi_cc((*i).buffer[0], (*i).buffer[1], 0, 2, true);
                }
            }
        }
        xjmkb.client_name = xjack.client_name;
        std::string tittle = xjmkb.client_name + _(" - Virtual Midi Keyboard");
        widget_set_title(xjmkb.win, tittle.c_str());
        xjmkb.show_ui(xjmkb.visible);
        if (xsynth.synth_is_active()) xjmkb.rebuild_instrument_list();
        auto t2 = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>( t2 - t1 ).count();
        debug_print("%f sec\n",duration/1e+6);

        main_run(&app);
        
        animidi.stop();
        if (xjack.client) jack_client_close (xjack.client);
        xsynth.unload_synth();
        if(!nsmsig.nsm_session_control) xjmkb.save_config();
    }
    main_quit(&app);

    exit (0);

} 
