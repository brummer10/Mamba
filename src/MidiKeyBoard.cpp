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
#include "xcustommap.h"


namespace midikeyboard {


/****************************************************************
 ** class AnimatedKeyBoard
 **
 ** animate midi input from jack on the keyboard in a extra thread
 ** 
 */

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


/****************************************************************
 ** class XKeyBoard
 **
 ** Create Keyboard layout and send events to MidiMessenger
 ** 
 */

XKeyBoard::XKeyBoard(xjack::XJack *xjack_, xalsa::XAlsa *xalsa_, xsynth::XSynth *xsynth_,
        mamba::MidiMessenger *mmessage_, nsmhandler::NsmSignalHandler& nsmsig_,
        PosixSignalHandler& xsig_, AnimatedKeyBoard * animidi_)
    : xjack(xjack_),
    xalsa(xalsa_),
    xsynth(xsynth_),
    save(),
    load(),
    mmessage(mmessage_),
    animidi(animidi_),
    nsmsig(nsmsig_),
    xsig(xsig_) {
    client_name = "Mamba";
    if (getenv("XDG_CONFIG_HOME")) {
        path = getenv("XDG_CONFIG_HOME");
        config_file = path +"/Mamba.conf";
        keymap_file =  path +"/Mamba.keymap";
        multikeymap_file =  path +"/Mamba.multikeymap";
    } else {
        path = getenv("HOME");
        config_file = path +"/.config/Mamba.conf";
        keymap_file =  path +"/.config/Mamba.keymap";
        multikeymap_file =  path +"/.config/Mamba.multikeymap";
    }
    fs_instruments = NULL;
    soundfontpath = getenv("HOME");
    has_config = false;
    main_x = 0;
    main_y = 0;
    main_w = 700;
    main_h = 240;
    visible = 1;
    velocity = 127;
    volume = 63;
    mbank = 0;
    mprogram = 0;
    mbpm = 120;
    song_bpm = 120;
    keylayout = 0;
    octave = 2;
    mchannel = 0;
    freewheel = 0;
    lchannels = 0;
    run_one_more = 0;
    need_save = false;
    pitch_scroll = false;
    view_has_changed = false;
    key_size_changed = false;
    view_controls = 1;
    view_program = 1;
    width_inc = 25;
    key_size = 0;
    is_inited.store(false, std::memory_order_release);
    filepath = getenv("HOME") ? getenv("HOME") : "/";

    nsmsig.signal_trigger_nsm_show_gui().connect(
        sigc::mem_fun(this, &XKeyBoard::nsm_show_ui));

    nsmsig.signal_trigger_nsm_hide_gui().connect(
        sigc::mem_fun(this, &XKeyBoard::nsm_hide_ui));

    nsmsig.signal_trigger_nsm_save_gui().connect(
        sigc::mem_fun(this, &XKeyBoard::save_config));

    nsmsig.signal_trigger_nsm_gui_open().connect(
        sigc::mem_fun(this, &XKeyBoard::set_config));

    xsig.signal_trigger_quit_by_posix().connect(
        sigc::mem_fun(this, &XKeyBoard::signal_handle));

    xsig.signal_trigger_kill_by_posix().connect(
        sigc::mem_fun(this, &XKeyBoard::exit_handle));

    xjack->signal_trigger_get_midi_in().connect(
        sigc::mem_fun(this, &XKeyBoard::get_midi_in));

    xjack->signal_trigger_quit_by_jack().connect(
        sigc::mem_fun(this, &XKeyBoard::quit_by_jack));
}

XKeyBoard::~XKeyBoard() {}

//static
XKeyBoard* XKeyBoard::get_instance(void *w_) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    return (XKeyBoard*) win->parent_struct;
}

std::string XKeyBoard::remove_sub(std::string a, std::string b) {
    std::string::size_type fpos = a.find(b);
    if (fpos != std::string::npos )
        a.erase(a.begin() + fpos, a.begin() + fpos + b.length());
    return (a);
}

// GUI stuff starts here
void XKeyBoard::set_config(const char *name, const char *client_id, bool op_gui) {
    client_name = client_id;
    xjack->client_name = client_name;
    std::string try_path = name;
    try_path += ".config";
    if (access(try_path.c_str(), F_OK) == -1 ) {
        read_config();
    }
    path = name;
    config_file = path + ".config";
    if (op_gui) {
        visible = 0;
    } else {
        visible = 1;
    }
}

void XKeyBoard::read_config() {
    std::ifstream infile(config_file);
    std::string line;
    std::string key;
    std::string value;
    if (infile.is_open()) {
        while (std::getline(infile, line)) {
            std::istringstream buf(line);
            buf >> key;
            buf >> value;
            if (key.compare("[main_x]") == 0) main_x = std::stoi(value);
            else if (key.compare("[main_y]") == 0) main_y = std::stoi(value);
            else if (key.compare("[main_w]") == 0) main_w = std::stoi(value);
            else if (key.compare("[main_h]") == 0) main_h = std::stoi(value);
            else if (key.compare("[visible]") == 0) visible = std::stoi(value);
            else if (key.compare("[hide_controlls]") == 0) view_controls = std::stoi(value);
            else if (key.compare("[hide_proc]") == 0) view_program = std::stoi(value);
            else if (key.compare("[key_size]") == 0) key_size = std::stoi(value);
            else if (key.compare("[keylayout]") == 0) keylayout = std::stoi(value);
            else if (key.compare("[midi_through]") == 0) xjack->midi_through = std::stoi(value);
            else if (key.compare("[mchannel]") == 0) mchannel = std::stoi(value);
            else if (key.compare("[velocity]") == 0) velocity = std::stoi(value);
            else if (key.compare("[filepath]") == 0) filepath = remove_sub(line, "[filepath] ");
            else if (key.compare("[octave]") == 0) octave = std::stoi(value);
            else if (key.compare("[volume]") == 0) volume = std::stoi(value);
            else if (key.compare("[freewheel]") == 0) freewheel = std::stoi(value);
            else if (key.compare("[lchannels]") == 0) lchannels = std::stoi(value);
            else if (key.compare("[soundfontpath]") == 0) soundfontpath = remove_sub(line, "[soundfontpath] ");
            else if (key.compare("[soundfont]") == 0) soundfont = remove_sub(line, "[soundfont] ");
            else if (key.compare("[reverb_on]") == 0) xsynth->reverb_on = std::stoi(value);
            else if (key.compare("[reverb_level]") == 0) xsynth->reverb_level = std::stof(value);
            else if (key.compare("[reverb_width]") == 0) xsynth->reverb_width = std::stof(value);
            else if (key.compare("[reverb_damp]") == 0) xsynth->reverb_damp = std::stof(value);
            else if (key.compare("[reverb_roomsize]") == 0) xsynth->reverb_roomsize = std::stof(value);
            else if (key.compare("[chorus_on]") == 0) xsynth->chorus_on = std::stoi(value);
            else if (key.compare("[chorus_type]") == 0) xsynth->chorus_type = std::stoi(value);
            else if (key.compare("[chorus_depth]") == 0) xsynth->chorus_depth = std::stof(value);
            else if (key.compare("[chorus_speed]") == 0) xsynth->chorus_speed = std::stof(value);
            else if (key.compare("[chorus_level]") == 0) xsynth->chorus_level = std::stof(value);
            else if (key.compare("[chorus_voices]") == 0) xsynth->chorus_voices = std::stoi(value);
            else if (key.compare("[synth_volume]") == 0) xsynth->volume_level = std::stof(value);
            else if (key.compare("[channel_instruments]") == 0) {
                for (int i = 0; i < 15; i++) {
                    xsynth->channel_instrument[i] = std::stoi(value);
                    buf >> value;
                }
                xsynth->channel_instrument[15] = std::stoi(value);
            } else if (key.compare("[recent_files]") == 0) recent_files.push_back(remove_sub(line, "[recent_files] "));
            else if (key.compare("[recent_sfonts]") == 0) recent_sfonts.push_back(remove_sub(line, "[recent_sfonts] "));
            key.clear();
            value.clear();
        }
        infile.close();
        has_config = true;
    }

    std::ifstream vinfile(config_file+"vec");
    if (vinfile.is_open()) {
        mamba::MidiEvent ev;
        int word = 0;
        double time = 0;
        std::getline(vinfile, line);
        for (int j = 0; j < 16; j++) {
            while (std::getline(vinfile, line)) {
                std::istringstream buf(line);
                if(line.find("CHANNEL") != std::string::npos) break;
                buf >> word;
                ev.buffer[0] = word;
                buf >> word;
                ev.buffer[1] = word;
                buf >> word;
                ev.buffer[2] = word;
                buf >> word;
                ev.num = word;
                buf >> time;
                ev.deltaTime = time;
                buf >> time;
                ev.absoluteTime = time;
                xjack->rec.play[j].push_back(ev);
            }
        }
        vinfile.close();
    }
    // convert old custom keymap to new format when needed
    if( access(keymap_file.data(), F_OK ) != -1 ) {
        fprintf(stderr, "old keymap file found %s\n", keymap_file.data());
        long temp[128];
        memset(temp, 0, 128 * sizeof(long));
        FILE *fp;
        if((fp=fopen(keymap_file.data(), "rb"))==NULL) {
            fprintf(stderr, "cant open keymap file\n");
            remove(keymap_file.data());
            return;
        }
        if(fread(temp, sizeof(long), 128, fp) != 128) {
            fclose(fp);
            fprintf(stderr, "cant read keymap file\n");
            remove(keymap_file.data());
            return;
        }
        fclose(fp);
        long temp2[128][2];
        memset(temp2, 0, 128 * 2 * sizeof temp2[0][0]);
        int i = 0;        
        for(;i<128;i++) {
            temp2[i][0] = temp[i];
        }
        FILE *fp2;
        if((fp2=fopen(multikeymap_file.data(), "wb"))==NULL) {
            fprintf(stderr, "fail to convert custom keymap to new format\n");
            remove(keymap_file.data());
            return;
        }
        if(fwrite(temp2, sizeof(long), 128*2, fp2) != 128*2) {
            fprintf(stderr, "fail to convert custom keymap to new format\n");
            remove(keymap_file.data());
            fclose(fp2);
            return;
        }
        remove(keymap_file.data());
        fprintf(stderr, "convert custom keymap to new format\n");
        fclose(fp2);
    }
}

void XKeyBoard::save_config() {
    if(nsmsig.nsm_session_control)
        XLockDisplay(win->app->dpy);
    std::ofstream outfile(config_file);
    if (outfile.is_open()) {
         outfile << "[main_x] "<< main_x << std::endl;
         outfile << "[main_y] " << main_y << std::endl;
         outfile << "[main_w] " << main_w << std::endl;
         outfile << "[main_h] " << main_h << std::endl;
         outfile << "[visible] " << visible << std::endl;
         outfile << "[hide_controlls] " << view_controls << std::endl;
         outfile << "[hide_proc] " << view_program << std::endl;
         outfile << "[key_size] " << key_size << std::endl;
         outfile << "[keylayout] " << keylayout << std::endl;
         outfile << "[midi_through] " << xjack->midi_through << std::endl;
         outfile << "[mchannel] " << mchannel << std::endl;
         outfile << "[velocity] " << velocity << std::endl;
         outfile << "[filepath] " << filepath << std::endl;
         outfile << "[octave] " << octave << std::endl;
         outfile << "[volume] " << volume << std::endl;
         outfile << "[freewheel] " << freewheel << std::endl;
         outfile << "[lchannels] " << lchannels << std::endl;
         outfile << "[soundfontpath] " << soundfontpath << std::endl;
         outfile << "[soundfont] " << soundfont << std::endl;
         outfile << "[reverb_on] " << xsynth->reverb_on << std::endl;
         outfile << "[reverb_level] " << xsynth->reverb_level << std::endl;
         outfile << "[reverb_width] " << xsynth->reverb_width << std::endl;
         outfile << "[reverb_damp] " << xsynth->reverb_damp << std::endl;
         outfile << "[reverb_roomsize] " << xsynth->reverb_roomsize << std::endl;
         outfile << "[chorus_on] " << xsynth->chorus_on << std::endl;
         outfile << "[chorus_type] " << xsynth->chorus_type << std::endl;
         outfile << "[chorus_depth] " << xsynth->chorus_depth << std::endl;
         outfile << "[chorus_speed] " << xsynth->chorus_speed << std::endl;
         outfile << "[chorus_level] " << xsynth->chorus_level << std::endl;
         outfile << "[chorus_voices] " << xsynth->chorus_voices << std::endl;
         outfile << "[synth_volume] " << xsynth->volume_level << std::endl;
         outfile << "[channel_instruments] ";
         for (int i = 0; i < 16; i++) {
             outfile << " " << xsynth->channel_instrument[i];
         }
         outfile << std::endl;
         for (auto i : recent_files) {
             outfile << "[recent_files] "  << i << std::endl;
         }
         for (auto i : recent_sfonts) {
             outfile << "[recent_sfonts] "  << i << std::endl;
         }
         outfile.close();
    }
    if (need_save ) {
        std::ofstream outfile(config_file+"vec");
        if (outfile.is_open()) {
            for (int j = 0; j < 16; j++) {
                outfile << "[CHANNEL" << j << "]"  << std::endl;
                for(std::vector<mamba::MidiEvent>::const_iterator i = xjack->rec.play[j].begin(); i != xjack->rec.play[j].end(); ++i) {
                    outfile << (int)(*i).buffer[0] << " " << (int)(*i).buffer[1] << " " 
                        << (int)(*i).buffer[2] << " " << (*i).num << " " << (*i).deltaTime << " " << (*i).absoluteTime << std::endl;
                }
            }
            outfile.close();
        }
    }
    if(nsmsig.nsm_session_control)
        XUnlockDisplay(win->app->dpy);
}

// temporary disable adj_callback
inline void dummy_callback(void *w_, void* user_data) {

}

void XKeyBoard::nsm_show_ui() {
    if (is_inited.load(std::memory_order_acquire)) {
        XLockDisplay(win->app->dpy);
        widget_show_all(win);
        XFlush(win->app->dpy);
        XMoveWindow(win->app->dpy,win->widget, main_x, main_y);
        nsmsig.trigger_nsm_gui_is_shown();
        XUnlockDisplay(win->app->dpy);
    } else {
        visible = 1;
    }
}

void XKeyBoard::nsm_hide_ui() {
    XLockDisplay(win->app->dpy);
    widget_hide(win);
    XFlush(win->app->dpy);
    nsmsig.trigger_nsm_gui_is_hidden();
    XUnlockDisplay(win->app->dpy);
}

void XKeyBoard::show_ui(int present) {
    if(present) {
        widget_show_all(win);
        XMoveWindow(win->app->dpy,win->widget, main_x, main_y);
        if(nsmsig.nsm_session_control)
            nsmsig.trigger_nsm_gui_is_shown();
    } else {
        widget_hide(win);
        map_callback(win,NULL);
        visible = 0;
        if(nsmsig.nsm_session_control)
            nsmsig.trigger_nsm_gui_is_hidden();
    }
}

void XKeyBoard::get_midi_in(int c, int n, bool on) {
    MidiKeyboard *keys = (MidiKeyboard*)wid->parent_struct;
    set_key_in_matrix(keys->in_key_matrix[c], n, on);
}

void XKeyBoard::quit_by_jack() {
    fprintf (stderr, "Quit by jack \n");
    XLockDisplay(win->app->dpy);
    quit(win);
    XFlush(win->app->dpy);
    XUnlockDisplay(win->app->dpy);
}

// static
void XKeyBoard::draw_my_combobox_entrys(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    if (attrs.map_state != IsViewable) return;
    int width = attrs.width;
    int height = attrs.height;
    ComboBox_t *comboboxlist = (ComboBox_t*)w->parent_struct;

    use_base_color_scheme(w, NORMAL_);
    cairo_rectangle(w->crb, 0, 0, width, height);
    cairo_fill (w->crb);

    int i = (int)max(0,adj_get_value(w->adj));
    int a = 0;
    int j = comboboxlist->list_size < static_cast<unsigned int>(comboboxlist->show_items+i+1) ? 
      comboboxlist->list_size : comboboxlist->show_items+i+1;
    for(;i<j;i++) {
        double ci = ((i+1)/100.0)*12.0;
        if (i<4)
            cairo_set_source_rgba(w->crb, ci, 0.2, 0.4, 1.00);
        else if (i<8)
            cairo_set_source_rgba(w->crb, 0.6, 0.2+ci-0.48, 0.4, 1.00);
        else if (i<12)
            cairo_set_source_rgba(w->crb, 0.6-(ci-0.96), 0.68-(ci-1.08), 0.4, 1.00);
        else
            cairo_set_source_rgba(w->crb, 0.12+(ci-1.56), 0.32, 0.4-(ci-1.44), 1.00);
        if(i == comboboxlist->prelight_item && i == comboboxlist->active_item)
            use_base_color_scheme(w, ACTIVE_);
        else if(i == comboboxlist->prelight_item)
            use_base_color_scheme(w, PRELIGHT_);
        cairo_rectangle(w->crb, 0, a*25, width, 25);
        cairo_fill_preserve(w->crb);
        cairo_set_line_width(w->crb, 1.0);
        use_frame_color_scheme(w, PRELIGHT_);
        cairo_stroke(w->crb); 
        cairo_text_extents_t extents;
        /** show label **/
        if(i == comboboxlist->prelight_item && i == comboboxlist->active_item)
            use_text_color_scheme(w, ACTIVE_);
        else if(i == comboboxlist->prelight_item)
            use_text_color_scheme(w, PRELIGHT_);
        else if (i == comboboxlist->active_item)
            use_text_color_scheme(w, SELECTED_);
        else
            use_text_color_scheme(w,NORMAL_ );

        cairo_set_font_size (w->crb, 12);
        cairo_text_extents(w->crb,"Ay", &extents);
        double h = extents.height;
        cairo_text_extents(w->crb,comboboxlist->list_names[i] , &extents);

        cairo_move_to (w->crb, 15, (25*(a+1)) - h +2);
        cairo_show_text(w->crb, comboboxlist->list_names[i]);
        cairo_new_path (w->crb);
        if (i == comboboxlist->prelight_item && extents.width > (float)width-20) {
            tooltip_set_text(w,comboboxlist->list_names[i]);
            w->flags |= HAS_TOOLTIP;
            show_tooltip(w);
        } else {
            w->flags &= ~HAS_TOOLTIP;
            hide_tooltip(w);
        }
        a++;
    }
}

void XKeyBoard::rounded_rectangle(cairo_t *cr,float x, float y, float width, float height) {
    cairo_new_path (cr);
    cairo_move_to  (cr, x, (y + height)/2);
    cairo_curve_to (cr, x ,y, x, y, (x + width)/2, y);
    cairo_curve_to (cr, width, y, width, y, width, (y + height)/2);
    cairo_curve_to (cr, width, height, width, height, (width + x)/2, height);
    cairo_curve_to (cr, x, height, x, height, x, (y + height)/2);
    cairo_close_path (cr);
}

void XKeyBoard::pattern_out(Widget_t *w, int height) {
    cairo_pattern_t *pat = cairo_pattern_create_linear (0, -4, 0, height-4);
    cairo_pattern_add_color_stop_rgba (pat, 0,  0.3, 0.3, 0.3, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.25,  0.2, 0.2, 0.2, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.15, 0.15, 0.15, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.75,  0.1, 0.1, 0.1, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 1,  0.05, 0.05, 0.05, 1.0);
    cairo_set_source(w->crb, pat);
    cairo_pattern_destroy (pat);
}

void XKeyBoard::pattern_in(Widget_t *w, Color_state st, int height) {
    Colors *c = get_color_scheme(w->app,st);
    if (!c) return;
    cairo_pattern_t *pat = cairo_pattern_create_linear (2, 2, 2, height);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 0.5, c->light[0],  c->light[1], c->light[2],  c->light[3]);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(w->crb, pat);
    cairo_pattern_destroy (pat);
}

int XKeyBoard::remove_low_dash(char *str)  noexcept{

    char *src;
    char *dst;
    int i = 0;
    int r = 0;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != '_') {
            dst++;
        } else {
            r = i;
        }
        i++;
    }
    *dst = '\0';
    return r;
}

// static
void XKeyBoard::draw_button(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    if (!w) return;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    int width = attrs.width-2;
    int height = attrs.height-2;
    if (attrs.map_state != IsViewable) return;
    if (!w->state && (int)w->adj_y->value) {
        w->state = 3;
    } else if (w->state == 3 && !(int)w->adj_y->value) {
        w->state = 0;
    }

    xjmkb->rounded_rectangle(w->crb,2.0, 2.0, width, height);

    if(w->state==0) {
        cairo_set_line_width(w->crb, 1.0);
        xjmkb->pattern_out(w, height);
        cairo_fill_preserve(w->crb);
        use_frame_color_scheme(w, PRELIGHT_);
    } else if(w->state==1) {
        xjmkb->pattern_out(w, height);
        cairo_fill_preserve(w->crb);
        cairo_set_line_width(w->crb, 1.5);
        use_frame_color_scheme(w, PRELIGHT_);
    } else if(w->state==2) {
        xjmkb->pattern_in(w, SELECTED_, height);
        cairo_fill_preserve(w->crb);
        cairo_set_line_width(w->crb, 1.0);
        use_frame_color_scheme(w, PRELIGHT_);
    } else if(w->state==3) {
        xjmkb->pattern_in(w, ACTIVE_, height);
        cairo_fill_preserve(w->crb);
        cairo_set_line_width(w->crb, 1.0);
        use_frame_color_scheme(w, PRELIGHT_);
    }
    cairo_stroke(w->crb);

    if(w->state==2) {
        xjmkb->rounded_rectangle(w->crb,4.0, 4.0, width, height);
        cairo_stroke(w->crb);
        xjmkb->rounded_rectangle(w->crb,3.0, 3.0, width, height);
        cairo_stroke(w->crb);
    } else if (w->state==3) {
        xjmkb->rounded_rectangle(w->crb,3.0, 3.0, width, height);
        cairo_stroke(w->crb);
    }

    float offset = 0.0;
    if(w->state==1 && ! (int)w->adj_y->value) {
        offset = 1.0;
    } else if(w->state==1) {
        offset = 2.0;
    } else if(w->state==2) {
        offset = 2.0;
    } else if(w->state==3) {
        offset = 1.0;
    }

    cairo_text_extents_t extents;
    use_text_color_scheme(w, get_color_state(w));
    cairo_set_font_size (w->crb, w->app->normal_font/w->scale.ascale);

    if (strstr(w->label, "_")) {
        cairo_text_extents(w->crb, "--", &extents);
        double underline = extents.width;
        strncpy(w->input_label,w->label, sizeof(w->input_label)-1);
        int pos = xjmkb->remove_low_dash(w->input_label);
        int len = strlen(w->input_label);
        cairo_text_extents(w->crb,w->input_label , &extents);
        int set_line = (extents.width/len) * pos;
        cairo_move_to (w->crb, (width-extents.width)*0.5 +offset, (height+extents.height)*0.5 +offset);
        cairo_show_text(w->crb, w->input_label);
        cairo_set_line_width(w->crb, 1.0);
        cairo_move_to (w->crb, (width-extents.width)*0.5 +offset + set_line, (height+extents.height)*0.55 +offset);
        cairo_line_to(w->crb,(width-extents.width)*0.5 +offset + set_line + underline, (height+extents.height)*0.55 +offset);
        cairo_stroke(w->crb);
    } else {
        cairo_text_extents(w->crb,w->label , &extents);
        cairo_move_to (w->crb, (width-extents.width)*0.5 +offset, (height+extents.height)*0.5 +offset);
        cairo_show_text(w->crb, w->label);
    }
}

// static
void XKeyBoard::mk_draw_knob(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    int width = attrs.width-2;
    int height = attrs.height-2;

    const double scale_zero = 20 * (M_PI/180); // defines "dead zone" for knobs
    int arc_offset = 2;
    int knob_x = 0;
    int knob_y = 0;

    int grow = (width > height-15) ? height-15:width;
    knob_x = grow-1;
    knob_y = grow-1;
    /** get values for the knob **/

    int knobx = (width - knob_x) * 0.5;
    int knobx1 = width* 0.5;

    int knoby = (height - knob_y) * 0.5;
    int knoby1 = height * 0.5;

    double knobstate = adj_get_state(w->adj_y);
    double angle = scale_zero + knobstate * 2 * (M_PI - scale_zero);

    double pointer_off =knob_x/3.5;
    double radius = min(knob_x-pointer_off, knob_y-pointer_off) / 2;
    double lengh_x = (knobx+radius+pointer_off/2) - radius * sin(angle);
    double lengh_y = (knoby+radius+pointer_off/2) + radius * cos(angle);
    double radius_x = (knobx+radius+pointer_off/2) - radius/ 1.18 * sin(angle);
    double radius_y = (knoby+radius+pointer_off/2) + radius/ 1.18 * cos(angle);
    cairo_pattern_t* pat;
    cairo_new_path (w->crb);

    pat = cairo_pattern_create_linear (0, 0, 0, knob_y);
    cairo_pattern_add_color_stop_rgba (pat, 1,  0.3, 0.3, 0.3, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.75,  0.2, 0.2, 0.2, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.15, 0.15, 0.15, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.25,  0.1, 0.1, 0.1, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0,  0.05, 0.05, 0.05, 1.0);

    cairo_scale (w->crb, 0.95, 1.05);
    cairo_arc(w->crb,knobx1+arc_offset/2, knoby1-arc_offset, knob_x/2.2, 0, 2 * M_PI );
    cairo_set_source (w->crb, pat);
    cairo_fill_preserve (w->crb);
    cairo_set_source_rgb (w->crb, 0.1, 0.1, 0.1); 
    cairo_set_line_width(w->crb,1);
    cairo_stroke(w->crb);
    cairo_scale (w->crb, 1.05, 0.95);
    cairo_new_path (w->crb);
    cairo_pattern_destroy (pat);
    pat = NULL;

    pat = cairo_pattern_create_linear (0, 0, 0, knob_y);
    cairo_pattern_add_color_stop_rgba (pat, 0,  0.3, 0.3, 0.3, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.25,  0.2, 0.2, 0.2, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.15, 0.15, 0.15, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 0.75,  0.1, 0.1, 0.1, 1.0);
    cairo_pattern_add_color_stop_rgba (pat, 1,  0.05, 0.05, 0.05, 1.0);

    cairo_arc(w->crb,knobx1, knoby1, knob_x/2.6, 0, 2 * M_PI );
    cairo_set_source (w->crb, pat);
    cairo_fill_preserve (w->crb);
    cairo_set_source_rgb (w->crb, 0.1, 0.1, 0.1); 
    cairo_set_line_width(w->crb,1);
    cairo_stroke(w->crb);
    cairo_new_path (w->crb);
    cairo_pattern_destroy (pat);

    /** create a rotating pointer on the kob**/
    cairo_set_line_cap(w->crb, CAIRO_LINE_CAP_ROUND); 
    cairo_set_line_join(w->crb, CAIRO_LINE_JOIN_BEVEL);
    cairo_move_to(w->crb, radius_x, radius_y);
    cairo_line_to(w->crb,lengh_x,lengh_y);
    cairo_set_line_width(w->crb,3);
    cairo_set_source_rgb (w->crb,0.63,0.63,0.63);
    cairo_stroke(w->crb);
    cairo_new_path (w->crb);

    cairo_text_extents_t extents;
    /** show value on the kob**/
    if (w->state) {
        char s[64];
        const char* format[] = {"%.1f", "%.2f", "%.3f"};
        float value = adj_get_value(w->adj);
        if (fabs(w->adj->step)>0.99) {
            snprintf(s, 63,"%d",  (int) value);
        } else if (fabs(w->adj->step)>0.09) {
            snprintf(s, 63, format[1-1], value);
        } else {
            snprintf(s, 63, format[2-1], value);
        }
        cairo_set_source_rgb (w->crb, 0.6, 0.6, 0.6);
        cairo_set_font_size (w->crb, knobx1/3);
        cairo_text_extents(w->crb, s, &extents);
        cairo_move_to (w->crb, knobx1-extents.width/2, knoby1+extents.height/2);
        cairo_show_text(w->crb, s);
        cairo_new_path (w->crb);
    }

    /** show label below the knob**/
    use_text_color_scheme(w, get_color_state(w));
    cairo_set_font_size (w->crb,  (w->app->normal_font-1)/w->scale.ascale);
    cairo_text_extents(w->crb,w->label , &extents);

    cairo_move_to (w->crb, knobx1-extents.width/2, height );
    cairo_show_text(w->crb, w->label);
    cairo_new_path (w->crb);
}

// static
void XKeyBoard::draw_menubar(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    int width = attrs.width;
    int height = attrs.height;
    use_bg_color_scheme(w, NORMAL_);
    cairo_rectangle(w->crb,0,0,width,height);
    cairo_fill (w->crb);
    use_bg_color_scheme(w, ACTIVE_);
    cairo_rectangle(w->crb,0,height-2,width,2);
    cairo_fill(w->crb);
}

// static
void XKeyBoard::draw_topbox(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    int width = attrs.width;
    int height = attrs.height;
    use_bg_color_scheme(w, NORMAL_);
    cairo_rectangle(w->crb,0,0,width,height);
    cairo_fill (w->crb);
    use_bg_color_scheme(w, ACTIVE_);
    cairo_rectangle(w->crb,0,height-2,width,2);
    cairo_fill(w->crb);
}

// static
void XKeyBoard::draw_middlebox(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    int height = attrs.height;
    set_pattern(w,&w->app->color_scheme->selected,&w->app->color_scheme->normal,BACKGROUND_);
    cairo_paint (w->crb);

    cairo_pattern_t *pat = cairo_pattern_create_linear (0, 0, 0, height);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 0.8, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, 0.6);
    cairo_set_source(w->crb, pat);
    cairo_paint (w->crb);
    cairo_pattern_destroy (pat);
}

// static
void XKeyBoard::set_std_value(void *w_, void* button, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    adj_set_value(w->adj, w->adj->std_value);
}

Widget_t *XKeyBoard::add_keyboard_knob(Widget_t *parent, const char * label,
                                int x, int y, int width, int height) {
    Widget_t *wid = add_knob(parent,label, x, y, width, height);
    wid->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    set_adjustment(wid->adj,64.0, 64.0, 0.0, 127.0, 1.0, CL_CONTINUOS);
    wid->func.expose_callback = mk_draw_knob;
    wid->func.key_press_callback = key_press;
    wid->func.key_release_callback = key_release;
    wid->func.double_click_callback = set_std_value;
    return wid;
}

Widget_t *XKeyBoard::add_keyboard_button(Widget_t *parent, const char * label,
                                int x, int y, int width, int height) {
    Widget_t *wid = add_toggle_button(parent,label, x, y, width, height);   
    wid->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    wid->func.expose_callback = draw_button;
    wid->func.key_press_callback = key_press;
    wid->func.key_release_callback = key_release;
    wid->func.double_click_callback = set_std_value;
    return wid;
}

void XKeyBoard::init_ui(Xputty *app) {
    win = create_window(app, DefaultRootWindow(app->dpy), 0, 0, 700, 265);
    XSelectInput(win->app->dpy, win->widget,StructureNotifyMask|ExposureMask|KeyPressMask 
                    |EnterWindowMask|LeaveWindowMask|ButtonReleaseMask|KeyReleaseMask
                    |ButtonPressMask|Button1MotionMask|PointerMotionMask);
    widget_set_icon_from_png(win,LDVAR(midikeyboard_png));
    std::string tittle = client_name + _(" - Virtual Midi Keyboard");
    widget_set_title(win, tittle.c_str());
    widget_set_dnd_aware(win);
    win->flags |= NO_AUTOREPEAT;
    win->scale.gravity = NORTHEAST;
    win->parent_struct = this;
    win->func.dnd_notify_callback = dnd_load_response;
    win->func.configure_notify_callback = win_configure_callback;
    win->func.map_notify_callback = map_callback;
    win->func.unmap_notify_callback = unmap_callback;
    win->func.key_press_callback = key_press;
    win->func.key_release_callback = key_release;

    XSizeHints* win_size_hints;
    win_size_hints = XAllocSizeHints();
    win_size_hints->flags =  PMinSize|PBaseSize|PMaxSize|PWinGravity|PResizeInc;
    win_size_hints->min_width = 700;
    win_size_hints->min_height = 225;
    win_size_hints->base_width = 700;
    win_size_hints->base_height = 265;
    win_size_hints->max_width = 1875;
    win_size_hints->max_height = 266; //need to be 1 more then min to avoid flicker in the UI!!
    win_size_hints->width_inc = width_inc;
    win_size_hints->height_inc = 10;
    win_size_hints->win_gravity = CenterGravity;
    XSetWMNormalHints(win->app->dpy, win->widget, win_size_hints);
    XFree(win_size_hints);

    // menu
    menubar = add_menubar(win,"",0, 0, 700, 25);
    menubar->func.expose_callback = draw_menubar;
    menubar->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    menubar->func.key_press_callback = key_press;
    menubar->func.key_release_callback = key_release;

    filemenu = menubar_add_menu(menubar,_("_File"));
    load_midi = menu_add_submenu(filemenu, _("_Load MIDI"));
    menu_add_entry(load_midi,_("_Load New"));
    add_midi = menu_add_submenu(filemenu, _("Add MIDI"));
    menu_add_entry(add_midi,_("Add New"));
    file_remove_menu = menu_add_submenu(filemenu, _("Remove MIDI"));
    menu_add_entry(filemenu,_("_Save MIDI as"));
    if (nsmsig.nsm_session_control)
        menu_add_entry(filemenu,_("Hide"));
    else
        menu_add_entry(filemenu,_("_Quit"));
    filemenu->func.value_changed_callback = file_callback;
    filemenu->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    filemenu->func.key_press_callback = key_press;
    filemenu->func.key_release_callback = key_release;
    load_midi->func.key_press_callback = key_press;
    load_midi->func.key_release_callback = key_release;
    load_midi->func.value_changed_callback = load_midi_callback;
    add_midi->func.key_press_callback = key_press;
    add_midi->func.key_release_callback = key_release;
    add_midi->func.value_changed_callback = add_midi_callback;
    file_remove_menu->func.key_press_callback = key_press;
    file_remove_menu->func.key_release_callback = key_release;
    file_remove_menu->func.value_changed_callback = file_remove_callback;

    view_menu = menubar_add_menu(menubar,_("_View"));
    view_menu->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    view_menu->func.key_press_callback = key_press;
    view_menu->func.key_release_callback = key_release;
    view_proc = menu_add_check_entry(view_menu, _("Channel/Bank/Instrument"));
    view_controller = menu_add_check_entry(view_menu, _("Controls"));
    key_size_menu = menu_add_submenu(view_menu,_("Keysize"));
    menu_add_radio_entry(key_size_menu,_("Big"));
    menu_add_radio_entry(key_size_menu,_("Normal"));
    menu_add_radio_entry(key_size_menu,_("Small"));
    
    adj_set_value(view_proc->adj, 1.0);
    adj_set_value(view_controller->adj, 1.0);
    view_controller->func.value_changed_callback = view_callback;
    view_proc->func.value_changed_callback = view_callback;
    key_size_menu->func.value_changed_callback = key_size_callback;

    mapping = menubar_add_menu(menubar,_("_Mapping"));
    keymap = menu_add_submenu(mapping,_("Keyboard"));
    menu_add_radio_entry(keymap,_("qwertz"));
    menu_add_radio_entry(keymap,_("qwerty"));
    menu_add_radio_entry(keymap,_("azerty (fr)"));
    menu_add_radio_entry(keymap,_("azerty (be)"));
    menu_add_radio_entry(keymap,_("custom"));
    mapping->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    mapping->func.key_press_callback = key_press;
    mapping->func.key_release_callback = key_release;
    keymap->func.value_changed_callback = layout_callback;

    octavemap = menu_add_submenu(mapping,_("Octave"));
    menu_add_radio_entry(octavemap,_("C 0"));
    menu_add_radio_entry(octavemap,_("C 1"));
    menu_add_radio_entry(octavemap,_("C 2"));
    menu_add_radio_entry(octavemap,_("C 3"));
    menu_add_radio_entry(octavemap,_("C 4"));
    adj_set_value(octavemap->adj, 2.0);
    octavemap->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    octavemap->func.key_press_callback = key_press;
    octavemap->func.key_release_callback = key_release;
    octavemap->func.value_changed_callback = octave_callback;

    menu_add_entry(mapping,_("_Keymap Editor"));
    mapping->func.value_changed_callback = keymap_callback;

    grab_keyboard = menu_add_accel_check_entry(mapping,_("_Grab Keyboard"));
    grab_keyboard->func.value_changed_callback = grab_callback;

    midi_through = menu_add_accel_check_entry(mapping,_("Midi _Through"));
    adj_set_value(midi_through->adj, static_cast<float>(xjack->midi_through));
    midi_through->func.value_changed_callback = through_callback;


    connection = menubar_add_menu(menubar,_("C_onnect"));
    inputs = menu_add_submenu(connection,_("Jack input"));
    outputs = menu_add_submenu(connection,_("Jack output"));
    alsa_inputs = menu_add_submenu(connection,_("ALSA input"));
    alsa_outputs = menu_add_submenu(connection,_("ALSA output"));
    connection->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    connection->func.key_press_callback = key_press;
    connection->func.key_release_callback = key_release;
    connection->func.button_press_callback = make_connection_menu;
    inputs->func.value_changed_callback = connection_in_callback;
    outputs->func.value_changed_callback = connection_out_callback;
    alsa_inputs->func.value_changed_callback = alsa_connection_callback;
    alsa_outputs->func.value_changed_callback = alsa_oconnection_callback;

    synth = menubar_add_menu(menubar,_("Fl_uidsynth"));
    sfont_menu = menu_add_submenu(synth,_("Load Soun_dFont"));
    menu_add_entry(sfont_menu,_("Loa_d New"));
    fs[0] = menu_add_entry(synth,_("Fluidsy_nth Settings"));
    fs[0]->state = 4;
    fs[1] = menu_add_entry(synth,_("Fluids_ynth Panic"));
    fs[1]->state = 4;
    fs[2] = menu_add_entry(synth,_("E_xit Fluidsynth"));
    fs[2]->state = 4;
    synth->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    synth->func.key_press_callback = key_press;
    synth->func.key_release_callback = key_release;
    synth->func.value_changed_callback = synth_callback;
    sfont_menu->func.value_changed_callback = sfont_callback;

    looper = menubar_add_menu(menubar,_("Looper"));
    view_channels = menu_add_submenu(looper,_("Show"));
    menu_add_radio_entry(view_channels,_("All Channels"));
    menu_add_radio_entry(view_channels,_("Only selected Channel"));
    adj_set_value(view_channels->adj, 0.0);
    view_channels->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    view_channels->func.key_press_callback = key_press;
    view_channels->func.key_release_callback = key_release;
    view_channels->func.value_changed_callback = view_channels_callback;
    
    looper->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    free_wheel = menu_add_check_entry(looper,_("Freewheel"));
    free_wheel->func.value_changed_callback = freewheel_callback;
    menu_add_entry(looper,_("Clear All Channels"));
    menu_add_entry(looper,_("Clear Current Channel"));
    looper->func.value_changed_callback = clear_loops_callback;
    looper->func.key_press_callback = key_press;
    looper->func.key_release_callback = key_release;

    info = menubar_add_menu(menubar,_("_Info"));
    menu_add_entry(info,_("_About"));
    info->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    info->func.key_press_callback = key_press;
    info->func.key_release_callback = key_release;
    info->func.value_changed_callback = info_callback;

    // top box
    proc_box = create_widget(win->app, win, 0, 25, 700, 45);
    proc_box->func.expose_callback = draw_topbox;
    proc_box->flags |= NO_AUTOREPEAT;
    proc_box->flags  &= ~USE_TRANSPARENCY;
    proc_box->scale.gravity = NORTHEAST;
    proc_box->func.key_press_callback = key_press;
    proc_box->func.key_release_callback = key_release;

    Widget_t *tmp = add_label(proc_box,_("Channel:"),10,10,60,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;
    channel =  add_combobox(proc_box, _("Channel"), 70, 7, 60, 30);
    channel->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    channel->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    channel->scale.gravity = ASPECT;
    combobox_add_numeric_entrys(channel,1,16);
    combobox_add_entry(channel,"--");
    combobox_set_active_entry(channel, 0);
    set_adjustment(channel->adj,0.0, 0.0, 0.0, 16.0, 1.0, CL_ENUM);
    channel->func.value_changed_callback = channel_callback;
    channel->func.key_press_callback = key_press;
    channel->func.key_release_callback = key_release;
    Widget_t *menu = channel->childlist->childs[1];
    Widget_t *view_port = menu->childlist->childs[0];
    view_port->func.expose_callback = draw_my_combobox_entrys;
    tmp = channel->childlist->childs[0];
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_label(proc_box,_("Bank:"),130,10,60,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;
    bank =  add_combobox(proc_box, _("Bank"), 190, 7, 60, 30);
    bank->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    bank->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    bank->scale.gravity = ASPECT;
    combobox_add_numeric_entrys(bank,0,128);
    combobox_set_active_entry(bank, 0);
    set_adjustment(bank->adj,0.0, 0.0, 0.0, 128.0, 1.0, CL_ENUM);
    bank->func.value_changed_callback = bank_callback;
    bank->func.key_press_callback = key_press;
    bank->func.key_release_callback = key_release;
    tmp = bank->childlist->childs[0];
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_label(proc_box,_("Program:"),250,10,80,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;
    program =  add_combobox(proc_box, _("Program"), 330, 7, 60, 30);
    program->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    program->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    program->scale.gravity = ASPECT;
    combobox_add_numeric_entrys(program,0,127);
    combobox_set_active_entry(program, 0);
    set_adjustment(program->adj,0.0, 0.0, 0.0, 127.0, 1.0, CL_ENUM);
    program->func.value_changed_callback = program_callback;
    program->func.key_press_callback = key_press;
    program->func.key_release_callback = key_release;
    tmp = program->childlist->childs[0];
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_label(proc_box,_("BPM:"),390,10,60,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;
    bpm = add_valuedisplay(proc_box, _("BPM"), 450, 7, 60, 30);
    bpm->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    bpm->scale.gravity = ASPECT;
    set_adjustment(bpm->adj,120.0, mbpm, 24.0, 360.0, 1.0, CL_CONTINUOS);
    bpm->func.value_changed_callback = bpm_callback;
    bpm->func.key_press_callback = key_press;
    bpm->func.key_release_callback = key_release;

    songbpm = add_label(proc_box,_("File BPM:"),510,10,100,20);
    songbpm->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    snprintf(songbpm->input_label, 31,_("File BPM: %d"),  (int) song_bpm);
    songbpm->label = songbpm->input_label;
    songbpm->func.key_press_callback = key_press;
    songbpm->func.key_release_callback = key_release;

    time_line = add_label(proc_box,_("--"),610,10,100,20);
    time_line->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    snprintf(time_line->input_label, 31,"%.2f sec", xjack->get_max_loop_time());
    time_line->label = time_line->input_label;
    time_line->func.key_press_callback = key_press;
    time_line->func.key_release_callback = key_release;

    // middle box
    int bpos = 70;
    if (!view_program) bpos -= 45;
    knob_box = create_widget(win->app, win, 0, bpos, 700, 77);
    knob_box->func.expose_callback = draw_middlebox;
    knob_box->flags |= NO_AUTOREPEAT;
    knob_box->flags &= ~USE_TRANSPARENCY;
    knob_box->scale.gravity = NORTHEAST;
    knob_box->func.key_press_callback = key_press;
    knob_box->func.key_release_callback = key_release;
    
    w[0] = add_keyboard_knob(knob_box, _("PitchBend"), 5, 0, 60, 75);
    w[0]->data = PITCHBEND;
    w[0]->func.value_changed_callback = pitchwheel_callback;
    w[0]->func.button_release_callback = pitchwheel_release_callback;
    w[0]->func.button_press_callback = pitchwheel_press_callback;

    w[9] = add_keyboard_knob(knob_box, _("Balance"), 65, 0, 60, 75);
    w[9]->data = BALANCE;
    w[9]->func.value_changed_callback = balance_callback;

    w[1] = add_keyboard_knob(knob_box, _("ModWheel"), 125, 0, 60, 75);
    w[1]->data = MODULATION;
    set_adjustment(w[1]->adj, 0.0, 0.0, 0.0, 127.0, 1.0, CL_CONTINUOS);
    w[1]->func.value_changed_callback = modwheel_callback;

    w[2] = add_keyboard_knob(knob_box, _("Detune"), 185, 0, 60, 75);
    w[2]->data = CELESTE;
    w[2]->func.value_changed_callback = detune_callback;

    w[10] = add_keyboard_knob(knob_box, _("Expression"), 245, 0, 60, 75);
    w[10]->data = EXPRESSION;
    set_adjustment(w[10]->adj, 127.0, 127.0, 0.0, 127.0, 1.0, CL_CONTINUOS);
    w[10]->func.value_changed_callback = expression_callback;

    w[3] = add_keyboard_knob(knob_box, _("Attack"), 305, 0, 60, 75);
    w[3]->data = ATTACK_TIME;
    w[3]->func.value_changed_callback = attack_callback;

    w[4] = add_keyboard_knob(knob_box, _("Release"), 365, 0, 60, 75);
    w[4]->data = RELEASE_TIME;
    w[4]->func.value_changed_callback = release_callback;

    w[5] = add_keyboard_knob(knob_box, _("Volume"), 425, 0, 60, 75);
    w[5]->data = VOLUME;
    w[5]->func.value_changed_callback = volume_callback;

    w[6] = add_keyboard_knob(knob_box, _("Velocity"), 485, 0, 60, 75);
    w[6]->data = VELOCITY;
    set_adjustment(w[6]->adj, 127.0, 127.0, 0.0, 127.0, 1.0, CL_CONTINUOS);
    w[6]->func.value_changed_callback = velocity_callback;

    w[7] = add_keyboard_button(knob_box, _("Sustain"), 550, 5, 75, 30);
    w[7]->data = SUSTAIN;
    w[7]->func.value_changed_callback = sustain_callback;

    w[8] = add_keyboard_button(knob_box, _("Sostenuto"), 550, 45, 75, 30);
    w[8]->data = SOSTENUTO;
    w[8]->func.value_changed_callback = sostenuto_callback;

    record = add_keyboard_button(knob_box, _("_Record"), 635, 5, 55, 30);
    record->func.value_changed_callback = record_callback;

    play = add_keyboard_button(knob_box, _("_Play"), 635, 45, 55, 30);
    play->func.adj_callback = set_play_label;
    play->func.value_changed_callback = play_callback;

    // open a widget for the keyboard layout
    int wpos = view_controls ? 147 : 70;
    if (!view_program) wpos -= 45;
    wid = create_widget(app, win, 0, wpos, 700, 118);
    wid->flags &= ~USE_TRANSPARENCY;
    wid->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    wid->scale.gravity = NORTHWEST;
    add_midi_keyboard(wid, multikeymap_file.c_str(), 0, 0, 700, 118);

    MidiKeyboard *keys = (MidiKeyboard*)wid->parent_struct;

    keys->mk_send_note = get_note;
    keys->mk_send_all_sound_off = get_all_notes_off;

    // when no config file exsist, center window on sreen
    if (!has_config) {
        Screen *screen = DefaultScreenOfDisplay(win->app->dpy);
        main_x = screen->width/2 - main_w/2;
        main_y = screen->height/2 - main_h/2; 
    }

    // set controllers to saved values
    combobox_set_active_entry(channel, mchannel);
    //combobox_set_active_entry(layout, keylayout);
    adj_set_value(keymap->adj,keylayout);
    adj_set_value(octavemap->adj,octave);
    adj_set_value(w[6]->adj, velocity);
    adj_set_value(w[5]->adj, volume);
    adj_set_value(free_wheel->adj, freewheel);

    // set window to saved size
    XResizeWindow (win->app->dpy, win->widget, main_w, main_h);

    // build the recent files menu
    build_recent_menu();
    // build the recent sfont menu
    build_sfont_menu();

    init_synth_ui(win);
    // start the timeout thread for keyboard animation
    animidi->start(30, std::bind(animate_midi_keyboard,(void*)wid));
    is_inited.store(true, std::memory_order_release);
}

// static
void XKeyBoard::animate_midi_keyboard(void *w_) {
    Widget_t *w = (Widget_t*)w_;
    MidiKeyboard *keys = (MidiKeyboard*)w->parent_struct;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);

    if (xjmkb->xjack->transport_state_changed.load(std::memory_order_acquire)) {
        xjmkb->xjack->transport_state_changed.store(false, std::memory_order_release);
        XLockDisplay(w->app->dpy);
        xjmkb->play->func.adj_callback = dummy_callback;
        adj_set_value(xjmkb->play->adj,
            (float)xjmkb->xjack->transport_set.load(std::memory_order_acquire));
        expose_widget(xjmkb->play);
        XFlush(w->app->dpy);
        xjmkb->play->func.adj_callback = set_play_label;
        XUnlockDisplay(w->app->dpy);
    }

    if (xjmkb->xjack->bpm_changed.load(std::memory_order_acquire)) {
        xjmkb->xjack->bpm_changed.store(false, std::memory_order_release);
        XLockDisplay(w->app->dpy);
        xjmkb->bpm->func.adj_callback = dummy_callback;
        adj_set_value(xjmkb->bpm->adj,
            (float)xjmkb->xjack->bpm_set.load(std::memory_order_acquire));
        expose_widget(xjmkb->bpm);
        XFlush(w->app->dpy);
        xjmkb->bpm->func.adj_callback = transparent_draw;
        XUnlockDisplay(w->app->dpy);
    }

    if ((xjmkb->xjack->record || xjmkb->xjack->play) && !xjmkb->xjack->freewheel) {
        static int scip = 8;
        if (scip >= 8) {
            XLockDisplay(w->app->dpy);
            if ( xjmkb->xjack->play && xjmkb->xjack->get_max_loop_time() > 0.0) {
                snprintf(xjmkb->time_line->input_label, 31,"%.2f sec", 
                    xjmkb->xjack->get_max_loop_time() -
                    (double)((xjmkb->xjack->stPlay - xjmkb->xjack->stStart)/(double)xjmkb->xjack->SampleRate));
            } else if (xjmkb->xjack->record && xjmkb->xjack->play) {
                snprintf(xjmkb->time_line->input_label, 31, "%.2f sec",
                    (double)((xjmkb->xjack->stPlay - xjmkb->xjack->rcStart)/(double)xjmkb->xjack->SampleRate));
            } else {
                snprintf(xjmkb->time_line->input_label, 31, "%.2f sec", xjmkb->xjack->get_max_loop_time());
            }
            xjmkb->time_line->label = xjmkb->time_line->input_label;
            expose_widget(xjmkb->time_line);
            XFlush(w->app->dpy);
            XUnlockDisplay(w->app->dpy);
            scip = 0;
        }
        scip++;
        if (xjmkb->xjack->record_off.load(std::memory_order_acquire)) {
            xjmkb->xjack->record_off.store(false, std::memory_order_release);
            XLockDisplay(w->app->dpy);
            xjmkb->record->func.adj_callback = dummy_callback;
            adj_set_value(xjmkb->record->adj, 0.0);
            expose_widget(xjmkb->record);
            XFlush(w->app->dpy);
            xjmkb->record->func.adj_callback = transparent_draw;
            XUnlockDisplay(w->app->dpy);
        }
    }

    bool repeat = need_redraw(keys);
    if ((repeat || xjmkb->run_one_more) && xjmkb->xjack->client) {
        XLockDisplay(w->app->dpy);
        expose_widget(w);
        XFlush(w->app->dpy);
        XUnlockDisplay(w->app->dpy);
        if (repeat)
            xjmkb->run_one_more = 10;
    }
    xjmkb->run_one_more = max(0,xjmkb->run_one_more-1);
}

// static
void XKeyBoard::win_configure_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    if (attrs.map_state != IsViewable) return;
    int width = attrs.width;
    int height = attrs.height;
    if (xjmkb->view_has_changed) {
        int h = 147;
        int h2 = 70;
        if (xjmkb->view_program<1.0) h -= 45;
        if (xjmkb->view_controls<1.0) h -= 77;
        if (xjmkb->view_controls) {
            if (xjmkb->view_program<1.0) h2 -= 45;
            XMoveWindow(xjmkb->win->app->dpy,xjmkb->knob_box->widget, 0, h2);
        }
        if (!xjmkb->key_size_changed) {
            xjmkb->wid->scale.init_y = h;
            XMoveWindow(xjmkb->win->app->dpy,xjmkb->wid->widget, 0, h);
        }
        xjmkb->key_size_changed = false;
        xjmkb->view_has_changed = false;
    }

    Window parent;
    Window root;
    Window * children;
    unsigned int num_children;
    XQueryTree(xjmkb->win->app->dpy, xjmkb->win->widget,
        &root, &parent, &children, &num_children);
    if (children) XFree(children);
    if (xjmkb->win->widget == root || parent == root) parent = xjmkb->win->widget;
    XGetWindowAttributes(w->app->dpy, parent, &attrs);

    int x1, y1;
    Window child;
    XTranslateCoordinates( xjmkb->win->app->dpy, parent, DefaultRootWindow(
                    xjmkb->win->app->dpy), 0, 0, &x1, &y1, &child );

    xjmkb->main_x = x1 - min(1,(width - attrs.width))/2;
    xjmkb->main_y = y1 - min(1,(height - attrs.height))/2;
    xjmkb->main_w = width;
    xjmkb->main_h = height;

    XGetWindowAttributes(w->app->dpy, (Window)xjmkb->synth_ui->widget, &attrs);
    if (attrs.map_state != IsViewable) return;
    int y = xjmkb->main_y-226;
    if (xjmkb->main_y < 230) y = xjmkb->main_y + xjmkb->main_h+21;
    XMoveWindow(xjmkb->win->app->dpy,xjmkb->synth_ui->widget, xjmkb->main_x, y);
}

//static
void XKeyBoard::view_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->view_controls = (int)adj_get_value(xjmkb->view_controller->adj);
    xjmkb->view_program = (int)adj_get_value(xjmkb->view_proc->adj);
    xjmkb->view_has_changed = true;
    
    int h = 265;
    int h1 = 265;
    if (xjmkb->view_controls<1.0) {
        h -= 77;
        h1 -= 77;
        widget_hide(xjmkb->knob_box);
    } else {
        widget_show_all(xjmkb->knob_box);
    }
    if (xjmkb->view_program<1.0) {
        h -= 45;
        h1 -= 40;
        widget_hide(xjmkb->proc_box);
    } else {
        widget_show_all(xjmkb->proc_box);
    }
    if (xjmkb->key_size == 1) {
        h1 -= 20;
    } else if (xjmkb->key_size == 2) {
        h1 -= 40;
    }
    
    xjmkb->win->scale.init_height = h;
    XSizeHints* win_size_hints;
    win_size_hints = XAllocSizeHints();
    win_size_hints->flags =  PMinSize|PBaseSize|PMaxSize|PWinGravity|PResizeInc;
    win_size_hints->min_width = 28*xjmkb->width_inc;
    win_size_hints->min_height = h1;
    win_size_hints->base_width = 28*xjmkb->width_inc;
    win_size_hints->base_height = h1;
    win_size_hints->max_width = 1875;
    win_size_hints->max_height = h1+1; //need to be 1 more then min to avoid flicker in the UI!!
    win_size_hints->width_inc = xjmkb->width_inc;
    win_size_hints->height_inc = 10;
    win_size_hints->win_gravity = CenterGravity;
    XSetWMNormalHints(xjmkb->win->app->dpy, xjmkb->win->widget, win_size_hints);
    XFree(win_size_hints);
}

//static
void XKeyBoard::key_size_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
    xjmkb->key_size = (int)adj_get_value(w->adj);
    static bool first_set = true;
    if (xjmkb->key_size == 0) {
        keys->key_size = 24;
        keys->key_offset = 15;
        xjmkb->width_inc = 25;
    } else if (xjmkb->key_size == 1) {
        keys->key_size = 21;
        keys->key_offset = 13;
        xjmkb->width_inc = 22;
    } else if (xjmkb->key_size == 2) {
        keys->key_size = 18;
        keys->key_offset = 12;
        xjmkb->width_inc = 19;
    }
    if (!first_set) {
        xjmkb->key_size_changed = true;
        view_callback(xjmkb->view_controller,NULL);
    }
    first_set = false;
}


void XKeyBoard::get_port_entrys(Widget_t *parent, jack_port_t *my_port, JackPortFlags type) {
    Widget_t *menu = parent->childlist->childs[0];
    Widget_t *view_port = menu->childlist->childs[0];
    
    int i = view_port->childlist->elem;
    for(;i>-1;i--) {
        menu_remove_item(menu,view_port->childlist->childs[i]);
    }

    bool new_port = true;
    const char **port_list = NULL;
    port_list = jack_get_ports(xjack->client, NULL, JACK_DEFAULT_MIDI_TYPE, type);
    if (port_list) {
        for (int i = 0; port_list[i] != NULL; i++) {
            jack_port_t *port = jack_port_by_name(xjack->client,port_list[i]);
            if (!port || jack_port_is_mine(xjack->client, port)) {
                new_port = false;
            }
            if (new_port) {
                Widget_t *entry = menu_add_check_entry(parent,port_list[i]);
                if (jack_port_connected_to(my_port, port_list[i])) {
                    adj_set_value(entry->adj,1.0);
                } else {
                    adj_set_value(entry->adj,0.0);
                }
            }
            new_port = true;
        }
        jack_free(port_list);
        port_list = NULL;
    }
} 

void XKeyBoard::get_alsa_port_menu() {
    xalsa->xalsa_get_ports(&alsa_ports,&alsa_oports);
    xalsa->xalsa_get_iconnections(&alsa_connections);
    xalsa->xalsa_get_oconnections(&alsa_oconnections);

    Widget_t *menu = alsa_inputs->childlist->childs[0];
    Widget_t *view_port = menu->childlist->childs[0];
    
    int i = view_port->childlist->elem;
    for(;i>-1;i--) {
        menu_remove_item(menu,view_port->childlist->childs[i]);
    }

    for(std::vector<std::string>::const_iterator i = alsa_ports.begin(); i != alsa_ports.end(); ++i) {
        Widget_t *entry = menu_add_check_entry(alsa_inputs,(*i).c_str());
        for(std::vector<std::string>::const_iterator j = alsa_connections.begin(); j != alsa_connections.end(); ++j) {
            if ((*i).find((*j)) != std::string::npos) {
                adj_set_value(entry->adj,1.0);
            } 
        }
    }
    menu = alsa_outputs->childlist->childs[0];
    view_port = menu->childlist->childs[0];
    
    i = view_port->childlist->elem;
    for(;i>-1;i--) {
        menu_remove_item(menu,view_port->childlist->childs[i]);
    }

    for(std::vector<std::string>::const_iterator i = alsa_oports.begin(); i != alsa_oports.end(); ++i) {
        Widget_t *entry = menu_add_check_entry(alsa_outputs,(*i).c_str());
        for(std::vector<std::string>::const_iterator j = alsa_oconnections.begin(); j != alsa_oconnections.end(); ++j) {
            if ((*i).find((*j)) != std::string::npos) {
                adj_set_value(entry->adj,1.0);
            } 
        }
    }
}

// static
void XKeyBoard::make_connection_menu(void *w_, void* button, void* user_data) {
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    xjmkb->get_port_entrys(xjmkb->outputs, xjmkb->xjack->out_port, JackPortIsInput);
    xjmkb->get_port_entrys(xjmkb->inputs, xjmkb->xjack->in_port, JackPortIsOutput);
    if (xjmkb->xalsa->is_running()) xjmkb->get_alsa_port_menu();
}

// static
void XKeyBoard::connection_in_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    Widget_t *menu = w->childlist->childs[0];
    Widget_t *view_port =  menu->childlist->childs[0];
    int i = (int)adj_get_value(w->adj);
    Widget_t *entry = view_port->childlist->childs[i];
    const char *my_port = jack_port_name(xjmkb->xjack->in_port);
    if (adj_get_value(entry->adj)) {
        jack_connect(xjmkb->xjack->client, entry->label, my_port);
    } else {
        jack_disconnect(xjmkb->xjack->client, entry->label, my_port);
    }
}

// static
void XKeyBoard::connection_out_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    Widget_t *menu = w->childlist->childs[0];
    Widget_t *view_port =  menu->childlist->childs[0];
    int i = (int)adj_get_value(w->adj);
    Widget_t *entry = view_port->childlist->childs[i];
    const char *my_port = jack_port_name(xjmkb->xjack->out_port);
    if (adj_get_value(entry->adj)) {
        jack_connect(xjmkb->xjack->client, my_port,entry->label);
    } else {
        jack_disconnect(xjmkb->xjack->client, my_port,entry->label);
    }
}

// static
void XKeyBoard::alsa_connection_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *menu = w->childlist->childs[0];
    Widget_t *view_port =  menu->childlist->childs[0];
    int i = (int)adj_get_value(w->adj);
    Widget_t *entry = view_port->childlist->childs[i];
    int client = -1;
    int port = -1;
    std::istringstream buf(entry->label);
        buf >> client;
        buf >> port;
    if (client == -1 || port == -1) return;
    if (adj_get_value(entry->adj)) {
        XKeyBoard::get_instance(w)->xalsa->xalsa_connect(client, port);
    } else {
        XKeyBoard::get_instance(w)->xalsa->xalsa_disconnect(client, port);
    }
}

// static
void XKeyBoard::alsa_oconnection_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *menu = w->childlist->childs[0];
    Widget_t *view_port =  menu->childlist->childs[0];
    int i = (int)adj_get_value(w->adj);
    Widget_t *entry = view_port->childlist->childs[i];
    int client = -1;
    int port = -1;
    std::istringstream buf(entry->label);
        buf >> client;
        buf >> port;
    if (client == -1 || port == -1) return;
    if (adj_get_value(entry->adj)) {
        XKeyBoard::get_instance(w)->xalsa->xalsa_oconnect(client, port);
    } else {
        XKeyBoard::get_instance(w)->xalsa->xalsa_odisconnect(client, port);
    }
}

// static
void XKeyBoard::map_callback(void *w_, void* user_data) {
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    xjmkb->visible = 1;
    if(!xjmkb->nsmsig.nsm_session_control)
        make_connection_menu(xjmkb->connection, NULL, NULL);
    xevfunc store = xjmkb->view_proc->func.value_changed_callback;
    xjmkb->view_proc->func.value_changed_callback = dummy_callback;
    adj_set_value(xjmkb->view_proc->adj, (float)xjmkb->view_program);
    xjmkb->view_proc->func.value_changed_callback = store;
    store = xjmkb->view_controller->func.value_changed_callback;
    xjmkb->view_controller->func.value_changed_callback = dummy_callback;
    adj_set_value(xjmkb->view_controller->adj, (float)xjmkb->view_controls);
    xjmkb->view_controller->func.value_changed_callback = store;
    adj_set_value(xjmkb->key_size_menu->adj, xjmkb->key_size);
    view_callback(xjmkb->view_controller,NULL);
}

// static
void XKeyBoard::unmap_callback(void *w_, void* user_data) noexcept{
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    xjmkb->visible = 0;
}

// static
void XKeyBoard::get_note(Widget_t *w, const int *key, const bool on_off) noexcept{
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    if (on_off) {
        xjmkb->mmessage->send_midi_cc(0x90, (*key),xjmkb->velocity, 3, false);
    } else {
        xjmkb->mmessage->send_midi_cc(0x80, (*key),xjmkb->velocity, 3, false);
    }
}

// static
void XKeyBoard::get_all_notes_off(Widget_t *w, const int *value) noexcept{
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->mmessage->send_midi_cc(0xB0, 120, 0, 3, false);
}

// static
void XKeyBoard::dnd_load_response(void *w_, void* user_data) {
    if(user_data !=NULL) {
        char* dndfile = NULL;
        bool midi_done = false;
        bool sf2_done = false;
        dndfile = strtok(*(char**)user_data, "\r\n");
        while (dndfile != NULL) {
            if (strstr(dndfile, ".mid") && !midi_done) {
                dialog_load_response(w_, (void*)&dndfile);
                midi_done = true;
            } else if (strstr(dndfile, ".sf") && !sf2_done) {
                synth_load_response(w_, (void*)&dndfile);
                sf2_done = true;
            }
            dndfile = strtok(NULL, "\r\n");
        }
    }
}

void XKeyBoard::recent_file_manager(const char* file_) {
    bool add = true;
    for (auto i : recent_files) {
        if (strcmp(i.data(),file_) == 0) {
            add = false;
        }
    }
    if (add) {
        recent_files.push_back(file_);
        if (recent_files.size()>10) recent_files.erase(recent_files.begin());
        build_recent_menu();
    }
}

// static
void XKeyBoard::dialog_load_response(void *w_, void* user_data) {
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    if(user_data !=NULL) {

#ifdef __XDG_MIME_H__
        if(!strstr(xdg_mime_get_mime_type_from_file_name(*(const char**)user_data), "midi")) {
            Widget_t *dia = open_message_dialog(xjmkb->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't load file, is that a MIDI file?"),NULL);
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
            return;
        }
#endif
        float play = adj_get_value(xjmkb->play->adj);
        adj_set_value(xjmkb->play->adj,0.0);
        adj_set_value(xjmkb->record->adj,0.0);
        if (!xjmkb->load.load_from_file(&xjmkb->xjack->rec.play[0], &xjmkb->song_bpm, *(const char**)user_data)) {
            Widget_t *dia = open_message_dialog(xjmkb->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't load file, is that a MIDI file?"),NULL);
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
        } else {
            xjmkb->recent_file_manager(*(char**)user_data);
            std::string file(basename(*(char**)user_data));
            for(int i = 1;i<16;i++)
                xjmkb->xjack->rec.play[i].clear();
            xjmkb->file_names.clear();
            xjmkb->file_names.push_back(file);
            xjmkb->filepath = dirname(*(char**)user_data);
            xjmkb->build_remove_menu();
            std::string tittle = xjmkb->client_name + _(" - Virtual Midi Keyboard") + " - " + file;
            widget_set_title(xjmkb->win, tittle.c_str());
            adj_set_value(xjmkb->bpm->adj, xjmkb->song_bpm);
            snprintf(xjmkb->songbpm->input_label, 31,_("File BPM: %d"),  (int) xjmkb->song_bpm);
            xjmkb->songbpm->label = xjmkb->songbpm->input_label;
            expose_widget(xjmkb->songbpm);
            snprintf(xjmkb->time_line->input_label, 31,"%.2f sec", xjmkb->xjack->get_max_loop_time());
            xjmkb->time_line->label = xjmkb->time_line->input_label;
            expose_widget(xjmkb->time_line);
            adj_set_value(xjmkb->play->adj, play);
        }
    }
}

// static
void XKeyBoard::dialog_add_response(void *w_, void* user_data) {
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    if(user_data !=NULL) {

#ifdef __XDG_MIME_H__
        if(!strstr(xdg_mime_get_mime_type_from_file_name(*(const char**)user_data), "midi")) {
            Widget_t *dia = open_message_dialog(xjmkb->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't load file, is that a MIDI file?"),NULL);
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
            return;
        }
#endif
        //float play = adj_get_value(xjmkb->play->adj);
        //adj_set_value(xjmkb->play->adj,0.0);
        adj_set_value(xjmkb->record->adj,0.0);
        if (!xjmkb->load.add_from_file(&xjmkb->xjack->rec.play[0], &xjmkb->song_bpm, *(const char**)user_data)) {
            Widget_t *dia = open_message_dialog(xjmkb->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't load file, is that a MIDI file?"),NULL);
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
        } else {
            xjmkb->recent_file_manager(*(char**)user_data);
            std::string file(basename(*(char**)user_data));
            xjmkb->file_names.push_back(file);
            xjmkb->filepath = dirname(*(char**)user_data);
            xjmkb->build_remove_menu();
            std::string tittle = xjmkb->client_name + _(" - Virtual Midi Keyboard") + " - " + "Multifile";
            widget_set_title(xjmkb->win, tittle.c_str());
            adj_set_value(xjmkb->bpm->adj, xjmkb->song_bpm);
            snprintf(xjmkb->songbpm->input_label, 31,_("File BPM: %d"),  (int) xjmkb->song_bpm);
            xjmkb->songbpm->label = xjmkb->songbpm->input_label;
            expose_widget(xjmkb->songbpm);
            snprintf(xjmkb->time_line->input_label, 31,"%.2f sec", xjmkb->xjack->get_max_loop_time());
            xjmkb->time_line->label = xjmkb->time_line->input_label;
            expose_widget(xjmkb->time_line);
            //adj_set_value(xjmkb->play->adj, play);
        }
    }
}

// static
void XKeyBoard::dialog_save_response(void *w_, void* user_data) {
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    if(user_data !=NULL) {
        std::string filename = *(const char**)user_data;
        std::string::size_type idx;
        idx = filename.rfind('.');
        if(idx != std::string::npos) {
            std::string extension = filename.substr(idx+1);
            if (extension.find("mid") == std::string::npos) {
                filename += ".midi";
            }
        } else {
            filename += ".midi";
        }
        const char* fn = filename.data();
        adj_set_value(xjmkb->play->adj,0.0);
        adj_set_value(xjmkb->record->adj,0.0);
        xjmkb->save.save_to_file(xjmkb->xjack->rec.play, fn);
    }
}

//static
void XKeyBoard::rebuild_remove_menu(void *w_, void* button, void* user_data) {
    XButtonEvent *xbutton = (XButtonEvent*)button;
    if (xbutton->button == Button4 || xbutton->button == Button5) return;
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->build_remove_menu();
}

void XKeyBoard::build_remove_menu() {
    Widget_t *menu = file_remove_menu->childlist->childs[0];
    Widget_t *view_port =  menu->childlist->childs[0];
    int i = view_port->childlist->elem-1;
    for(;i>-1;i--) {
        menu_remove_item(menu,view_port->childlist->childs[i]);
    }

    for(std::vector<std::string>::const_iterator i = file_names.begin(); i != file_names.end(); ++i) {
        Widget_t *entry = menu_add_entry(file_remove_menu,(*i).c_str());
        entry->func.button_release_callback = rebuild_remove_menu;
    }
}

//static
void XKeyBoard::file_remove_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    xjmkb->file_names.erase(xjmkb->file_names.begin()+value);
    
    //float play = adj_get_value(xjmkb->play->adj);
    //adj_set_value(xjmkb->play->adj,0.0);
    adj_set_value(xjmkb->record->adj,0.0);
    xjmkb->load.remove_file(&xjmkb->xjack->rec.play[0], value);
    snprintf(xjmkb->time_line->input_label, 31,"%.2f sec", xjmkb->xjack->get_max_loop_time());
    xjmkb->time_line->label = xjmkb->time_line->input_label;
    expose_widget(xjmkb->time_line);
    //if ((int)xjmkb->xjack->get_max_loop_time())
    //    adj_set_value(xjmkb->play->adj, play);    
}

void XKeyBoard::build_recent_menu() {
    Widget_t *menu = load_midi->childlist->childs[0];
    Widget_t *view_port =  menu->childlist->childs[0];
    int i = view_port->childlist->elem-1;
    for(;i>-1;i--) {
        menu_remove_item(menu,view_port->childlist->childs[i]);
    }
    menu_add_entry(load_midi,_("_Load New"));

    menu = add_midi->childlist->childs[0];
    view_port =  menu->childlist->childs[0];
    i = view_port->childlist->elem-1;
    for(;i>-1;i--) {
        menu_remove_item(menu,view_port->childlist->childs[i]);
    }
    menu_add_entry(add_midi,_("Add New"));

    for(std::vector<std::string>::const_iterator i = recent_files.begin(); i != recent_files.end(); ++i) {
        const char *cstr = basename((char*)(*i).c_str());
        menu_add_entry(load_midi,cstr);
        menu_add_entry(add_midi,cstr);
    }
}

//static
void XKeyBoard::load_midi_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    if (value == 0) {
        Widget_t *dia = open_file_dialog(xjmkb->win, xjmkb->filepath.c_str(), "midi");
        XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
        xjmkb->win->func.dialog_callback = dialog_load_response;
    } else {
        std::string recent = xjmkb->recent_files[value-1];
        const char *cstr = &recent[0];
        dialog_load_response(w, (void*)&cstr);
    }
}

//static
void XKeyBoard::add_midi_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    if (value == 0) {
        Widget_t *dia = open_file_dialog(xjmkb->win, xjmkb->filepath.c_str(), "midi");
        XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
        xjmkb->win->func.dialog_callback = dialog_add_response;
    } else {
        std::string recent = xjmkb->recent_files[value-1];
        const char *cstr = &recent[0];
        dialog_add_response(w, (void*)&cstr);
    }
}

//static
void XKeyBoard::file_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    switch (value) {
        case(0):
        break;
        case(1):
        break;
        case(2):
        break;
        case(3):
        {
            Widget_t *dia = save_file_dialog(xjmkb->win, xjmkb->filepath.c_str(), "midi");
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
            xjmkb->win->func.dialog_callback = dialog_save_response;
        }
        break;
        case(4):
        {
            if(xjmkb->nsmsig.nsm_session_control) {
                widget_hide(xjmkb->win);
                xjmkb->nsmsig.trigger_nsm_gui_is_hidden();
            } else {
                quit(xjmkb->win);
            }
        }
        break;
        default:
        break;
    }
}

void XKeyBoard::build_sfont_menu() {
    Widget_t *menu = sfont_menu->childlist->childs[0];
    Widget_t *view_port =  menu->childlist->childs[0];
    int i = view_port->childlist->elem-1;
    for(;i>-1;i--) {
        menu_remove_item(menu,view_port->childlist->childs[i]);
    }
    menu_add_entry(sfont_menu,_("Loa_d New"));

    for(std::vector<std::string>::const_iterator i = recent_sfonts.begin(); i != recent_sfonts.end(); ++i) {
        const char *cstr = basename((char*)(*i).c_str());
        Widget_t *entry = menu_add_radio_entry(sfont_menu,cstr);
        if (strcmp(soundfont.data(),(*i).c_str()) == 0) {
            radio_item_set_active(entry);
        }
    }
}

void XKeyBoard::recent_sfont_manager(const char* file_) {
    bool add = true;
    for (auto i : recent_sfonts) {
        if (strcmp(i.data(),file_) == 0) {
            add = false;
        }
    }
    if (add) {
        recent_sfonts.push_back(file_);
        if (recent_sfonts.size()>10) recent_sfonts.erase(recent_sfonts.begin());
        build_sfont_menu();
    }
}

// static
void XKeyBoard::synth_load_response(void *w_, void* user_data) {
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    if(user_data !=NULL) {
        float play = adj_get_value(xjmkb->play->adj);
        //adj_set_value(xjmkb->play->adj,0.0);
        if ((int)play &&  xjmkb->xsynth->synth_is_active()) {
            xjmkb->xsynth->unload_synth();
        }
         
        if( access(*(const char**)user_data, F_OK ) == -1 ) {
            Widget_t *dia = open_message_dialog(xjmkb->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't access file, sorry"),NULL);
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
            return;
        }
        if (strstr(*(const char**)user_data, ".sfz")) {
            Widget_t *dia = open_message_dialog(xjmkb->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't load file in sfz format, sorry"),NULL);
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
            return;
        }
        if(xjmkb->fs_instruments) {
            combobox_delete_entrys(xjmkb->fs_instruments);
        }
        if (!xjmkb->xsynth->synth_is_active()) {
            xjmkb->xsynth->setup(xjmkb->xjack->SampleRate);
            xjmkb->xsynth->init_synth();
        }
        if (xjmkb->xsynth->load_soundfont( *(const char**)user_data)) {
            Widget_t *dia = open_message_dialog(xjmkb->win, ERROR_BOX, *(const char**)user_data, 
            _("Couldn't load file, is that a soundfont file?"),NULL);
            XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
            return;
        }
        xjmkb->recent_sfont_manager(*(const char**)user_data);
        xjmkb->rebuild_instrument_list();
        xjmkb->soundfont =  *(const char**)user_data;
        xjmkb->soundfontname =  basename(*(char**)user_data);
        xjmkb->soundfontpath = dirname(*(char**)user_data);
        std::string title = _("Fluidsynth - ");
        title += xjmkb->soundfontname;
        widget_set_title(xjmkb->synth_ui, title.c_str());
        expose_widget(xjmkb->fs_instruments);
        expose_widget(xjmkb->fs_soundfont);
        const char **port_list = NULL;
        port_list = jack_get_ports(xjmkb->xjack->client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
        std::string synth_instance = jack_get_client_name(xjmkb->xjack->client);
        std::transform(synth_instance.begin(), synth_instance.end(), synth_instance.begin(), ::tolower);
        if (port_list) {
            for (int i = 0; port_list[i] != NULL; i++) {
                if (strstr(port_list[i], synth_instance.c_str())) {
                    if (!jack_port_connected_to(xjmkb->xjack->out_port, port_list[i])) {
                        const char *my_port = jack_port_name(xjmkb->xjack->out_port);
                        jack_connect(xjmkb->xjack->client, my_port,port_list[i]);
                    }
                    break;
                }
            }
            jack_free(port_list);
            port_list = NULL;
        }
        XWindowAttributes attrs;
        XGetWindowAttributes(xjmkb->win->app->dpy, (Window)xjmkb->synth_ui->widget, &attrs);
        if (attrs.map_state == IsViewable) {
            widget_show_all(xjmkb->fs_instruments);
        }
        xjmkb->mmessage->send_midi_cc(0xB0, 7, xjmkb->volume, 3, false);
        xjmkb->fs[0]->state = 0;
        xjmkb->fs[1]->state = 0;
        xjmkb->fs[2]->state = 0;
        xjmkb->rebuild_soundfont_list();
        xjmkb->build_sfont_menu();
    }
}

//static
void XKeyBoard::sfont_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    if (value == 0) {
        Widget_t *dia = open_file_dialog(xjmkb->win, xjmkb->soundfontpath.c_str(), ".sf");
        XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
        xjmkb->win->func.dialog_callback = synth_load_response;
    } else {
        std::string recent = xjmkb->recent_sfonts[value-1];
        const char *cstr = &recent[0];
        synth_load_response(w, (void*)&cstr);
    }
}

//static
void XKeyBoard::synth_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    switch (value) {
        case(0):
        {
        }
        break;
        case(1):
        {
            xjmkb->show_synth_ui(1);
        }
        break;
        case(2):
        {
            xjmkb->xsynth->panic();
        }
        break;
        case(3):
        {
            xjmkb->show_synth_ui(0);
            xjmkb->xsynth->unload_synth();
            xjmkb->soundfont = "";
            xjmkb->fs[0]->state = 4;
            xjmkb->fs[1]->state = 4;
            xjmkb->fs[2]->state = 4;
            xjmkb->build_sfont_menu();
        }
        break;
        default:
        break;
    }
}

// static
void XKeyBoard::info_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    std::string info = _("Mamba ");
    info += VERSION;
    info += _(" is written by Hermann Meyer|released under the BSD Zero Clause License|");
    info += "https://github.com/brummer10/Mamba";
    info += _("|For MIDI file handling it uses libsmf|a BSD-licensed C library|written by Edward Tomasz Napierala|");
    info += "https://github.com/stump/libsmf";
    info += _("|");
    Widget_t *dia = open_message_dialog(win, INFO_BOX, _("Mamba"), info.data(), NULL);
    XSetTransientForHint(win->app->dpy, dia->widget, win->widget);
}

// static
void XKeyBoard::channel_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
    if (xjmkb->xjack->play>0) {
        for (int i = 0; i<16;i++) 
            clear_key_matrix(keys->in_key_matrix[i]);
    }
    xjmkb->xjack->rec.channel = xjmkb->mmessage->channel = keys->channel = xjmkb->mchannel = (int)adj_get_value(w->adj);
    if(xjmkb->xsynth->synth_is_active()) {
        int i = xjmkb->xsynth->get_instrument_for_channel(xjmkb->mchannel);
        if ( i >-1)
            combobox_set_active_entry(xjmkb->fs_instruments, i);
    }
}

// static
void XKeyBoard::bank_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    if(!xjmkb->xsynth->synth_is_active()) {
        xjmkb->mbank = (int)adj_get_value(w->adj);
        xjmkb->mmessage->send_midi_cc(0xB0, 32, xjmkb->mbank, 3, false);
        xjmkb->mmessage->send_midi_cc(0xC0, xjmkb->mprogram, 0, 2, false);
    }
}

// static
void XKeyBoard::program_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->mprogram = xjmkb->xjack->program = (int)adj_get_value(w->adj);
    xjmkb->mbank = xjmkb->xjack->bank = (int)adj_get_value(xjmkb->bank->adj);
    if(xjmkb->xsynth->synth_is_active()) {
        int ret = 0;
        int bank = 0;
        int program = 0;
        for(std::vector<std::string>::const_iterator i = xjmkb->xsynth->instruments.begin();
                                                i != xjmkb->xsynth->instruments.end(); ++i) {
            std::istringstream buf((*i));
            buf >> bank;
            buf >> program;
            if (bank == xjmkb->mbank && program == xjmkb->mprogram) {
                adj_set_value(xjmkb->fs_instruments->adj, ret);
                xjmkb->xsynth->set_instrument_on_channel(xjmkb->mchannel,ret);
                break;
            }
            ret++;
        }
    } else {
        xjmkb->mmessage->send_midi_cc(0xB0, 32, xjmkb->mbank, 3, false);
        xjmkb->mmessage->send_midi_cc(0xC0, xjmkb->mprogram, 0, 2, false);
    }
}

// static
void XKeyBoard::bpm_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->mbpm = (int)adj_get_value(w->adj);
    xjmkb->xjack->bpm_ratio = (double)xjmkb->song_bpm/(double)xjmkb->mbpm;
}

// static
void XKeyBoard::modwheel_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 1, value, 3, false);
}

// static
void XKeyBoard::detune_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 94, value, 3, false);
}

// static
void XKeyBoard::attack_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 73, value, 3, false);
}

// static
void XKeyBoard::expression_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 11, value, 3, false);
}

// static 
void XKeyBoard::release_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 72, value, 3, false);
}

// static 
void XKeyBoard::volume_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->volume = (int)adj_get_value(w->adj);
    xjmkb->mmessage->send_midi_cc(0xB0, 7, xjmkb->volume, 3, false);
}

// static 
void XKeyBoard::velocity_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->velocity = value; 
}

// static
void XKeyBoard::pitchwheel_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    unsigned int change = (unsigned int)(128 * value);
    unsigned int low = change & 0x7f;  // Low 7 bits
    unsigned int high = (change >> 7) & 0x7f;  // High 7 bits
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xE0,  low, high, 3, false);
}

// static
void XKeyBoard::pitchwheel_press_callback(void *w_, void* button, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XButtonEvent *xbutton = (XButtonEvent*)button;
    if (xbutton->button == Button2) {
        XKeyBoard::get_instance(w)->pitch_scroll = true;
    }
}

// static
void XKeyBoard::pitchwheel_release_callback(void *w_, void* button, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    XButtonEvent *xbutton = (XButtonEvent*)button;
    if (xbutton->button == Button4 && xjmkb->pitch_scroll) {
        float value = min(w->adj->max_value,max(w->adj->min_value, 
                                w->adj->value + (w->adj->step * 10)));
        adj_set_value(w->adj,value);
    } else if (xbutton->button == Button5 && xjmkb->pitch_scroll) {
        float value = min(w->adj->max_value,max(w->adj->min_value, 
                                w->adj->value + (w->adj->step * -10)));
        adj_set_value(w->adj,value);
    } else if (xbutton->button == Button2) {
        xjmkb->pitch_scroll = false;
        adj_set_value(w->adj,64);
    } else {
        adj_set_value(w->adj,64);
    }
    int value = (int)adj_get_value(w->adj);
    unsigned int change = (unsigned int)(128 * value);
    unsigned int low = change & 0x7f;  // Low 7 bits
    unsigned int high = (change >> 7) & 0x7f;  // High 7 bits
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xE0,  low, high, 3, false);
}

// static
void XKeyBoard::balance_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 8, value, 3, false);
}

// static
void XKeyBoard::sustain_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 64, value*127, 3, false);
}

// static
void XKeyBoard::sostenuto_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    XKeyBoard::get_instance(w)->mmessage->send_midi_cc(0xB0, 66, value*127, 3, false);
}

void XKeyBoard::find_next_beat_time(double *absoluteTime) {
    double beat = 60.0/(double)song_bpm;
    int beats = std::round(((*absoluteTime)/beat));
    (*absoluteTime) = (double)beats*beat;
}

// static
void XKeyBoard::record_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    xjmkb->xjack->record = value;
    if (value > 0) {
        std::string tittle = xjmkb->client_name + _(" - Virtual Midi Keyboard");
        widget_set_title(xjmkb->win, tittle.c_str());
        xjmkb->song_bpm = adj_get_value(xjmkb->bpm->adj);
        snprintf(xjmkb->songbpm->input_label, 31,_("File BPM: %d"),  (int) xjmkb->song_bpm);
        xjmkb->songbpm->label = xjmkb->songbpm->input_label;
        expose_widget(xjmkb->songbpm);
        xjmkb->xjack->store1.clear();
        xjmkb->xjack->store2.clear();
        int c = xjmkb->mchannel;
        if (xjmkb->mchannel>15) {
            xjmkb->xjack->rec.channel = xjmkb->mmessage->channel = c = 0;
        }
        xjmkb->xjack->rec.play[c].clear();
        xjmkb->xjack->fresh_take = true;
        xjmkb->xjack->rec.start();
        xjmkb->need_save = true;
    } else if ( xjmkb->xjack->rec.is_running()) {
        if (xjmkb->xjack->rec.st == &xjmkb->xjack->store1 && xjmkb->xjack->store2.size()) {
            xjmkb->xjack->rec.st = &xjmkb->xjack->store2;
        } else {
            xjmkb->xjack->rec.st = &xjmkb->xjack->store1;
        }
        jack_nframes_t stop = jack_last_frame_time(xjmkb->xjack->client);
        double deltaTime = (double)(((stop) - xjmkb->xjack->start)/(double)xjmkb->xjack->SampleRate); // seconds
        double absoluteTime = (double)(((stop) - xjmkb->xjack->absoluteStart)/(double)xjmkb->xjack->SampleRate); // seconds
        if(!xjmkb->xjack->get_max_loop_time() && !xjmkb->freewheel)
            xjmkb->find_next_beat_time(&absoluteTime);
        else if (xjmkb->xjack->get_max_loop_time() && !xjmkb->freewheel)
            absoluteTime = xjmkb->xjack->get_max_loop_time();
        mamba::MidiEvent ev = {{0x80, 0, 0}, 3, deltaTime, absoluteTime};
        xjmkb->xjack->rec.st->push_back(ev);
        xjmkb->xjack->rec.stop();
        xjmkb->xjack->record_finished = 1;
        snprintf(xjmkb->time_line->input_label, 31,"%.2f sec", xjmkb->xjack->get_max_loop_time());
        xjmkb->time_line->label = xjmkb->time_line->input_label;
        expose_widget(xjmkb->time_line);
    }
}

// static
void XKeyBoard::play_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    xjmkb->xjack->play = value;
    if (value < 1) {
        MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
        for (int i = 0; i<16;i++) 
            clear_key_matrix(keys->in_key_matrix[i]);
        xjmkb->mmessage->send_midi_cc(0xB0, 123, 0, 3, false);
        if (xjmkb->xsynth->synth_is_active()) xjmkb->xsynth->panic();
        xjmkb->xjack->first_play = true;
    }
    snprintf(xjmkb->time_line->input_label, 31,"%.2f sec", xjmkb->xjack->get_max_loop_time());
    xjmkb->time_line->label = xjmkb->time_line->input_label;
    expose_widget(xjmkb->time_line);
}

// static
void XKeyBoard::set_play_label(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    if (value) {
        w->label = _("Sto_p");
    } else {
        w->label = _("_Play");
    }
    expose_widget(w);
}

// static
void XKeyBoard::freewheel_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    int value = (int)adj_get_value(w->adj);
    xjmkb->xjack->freewheel = xjmkb->freewheel = xjmkb->save.freewheel = value;
}

// static
void XKeyBoard::clear_loops_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
    if ((int)adj_get_value(w->adj) == 2) {
        xjmkb->xjack->play = 0.0;
        //adj_set_value(xjmkb->play->adj, 0.0);
        //set_play_label(xjmkb->play,NULL);
        //adj_set_value(xjmkb->record->adj, 0.0);
        for (int i = 0; i<16;i++) 
            xjmkb->xjack->rec.play[i].clear();
        for (int i = 0; i<16;i++) 
            clear_key_matrix(keys->in_key_matrix[i]);
        xjmkb->file_names.clear();
        xjmkb->build_remove_menu();
        xjmkb->load.positions.clear();
        std::string tittle = xjmkb->client_name + _(" - Virtual Midi Keyboard");
        widget_set_title(xjmkb->win, tittle.c_str());
        adj_set_value(xjmkb->bpm->adj,120.0);
        xjmkb->song_bpm = adj_get_value(xjmkb->bpm->adj);
        xjmkb->mbpm = (int)adj_get_value(xjmkb->bpm->adj);
        snprintf(xjmkb->songbpm->input_label, 31,_("File BPM: %d"),  (int) xjmkb->song_bpm);
        xjmkb->songbpm->label = xjmkb->songbpm->input_label;
        xjmkb->xjack->bpm_ratio = (double)xjmkb->song_bpm/(double)xjmkb->mbpm;
        expose_widget(xjmkb->songbpm);
        snprintf(xjmkb->time_line->input_label, 31,"%.2f sec", xjmkb->xjack->get_max_loop_time());
        xjmkb->time_line->label = xjmkb->time_line->input_label;
        expose_widget(xjmkb->time_line);
        xjmkb->need_save = true;
    } else if ((int)adj_get_value(w->adj) == 3) {
        if (xjmkb->xjack->rec.channel == 0) {
            xjmkb->file_names.clear();
            xjmkb->build_remove_menu();
            xjmkb->load.positions.clear();
        }
        xjmkb->xjack->rec.play[xjmkb->xjack->rec.channel].clear();
        clear_key_matrix(keys->in_key_matrix[xjmkb->xjack->rec.channel]);
        xjmkb->mmessage->send_midi_cc(0xB0 | xjmkb->xjack->rec.channel, 123, 0, 3, true);
        xjmkb->need_save = true;
    }
}

// static
void XKeyBoard::layout_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
    keys->layout = xjmkb->keylayout = (int)adj_get_value(w->adj);
    
    if ((int)adj_get_value(w->adj) == 3) {
        if( access(xjmkb->multikeymap_file.c_str(), F_OK ) == -1 ) {
            open_custom_keymap(xjmkb->wid, xjmkb->win, xjmkb->multikeymap_file.c_str());
        }
    }
}

// static
void XKeyBoard::octave_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
    keys->octave = (int)12*adj_get_value(w->adj);
    xjmkb->octave = (int)adj_get_value(w->adj);
    expose_widget(xjmkb->wid);
}

// static
void XKeyBoard::view_channels_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xjack->view_channels = xjmkb->lchannels = (int)adj_get_value(w->adj);
    if (xjmkb->lchannels) {
        MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
        for (int i = 0; i<16;i++) 
            clear_key_matrix(keys->in_key_matrix[i]);
    }
}

// static
void XKeyBoard::keymap_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    if ((int)adj_get_value(w->adj) == 2)
        open_custom_keymap(xjmkb->wid, xjmkb->win, xjmkb->multikeymap_file.c_str());
}

// static
void XKeyBoard::grab_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    if (adj_get_value(w->adj)) {
        XGrabKeyboard(xjmkb->win->app->dpy, xjmkb->wid->widget, true, 
                    GrabModeAsync, GrabModeAsync, CurrentTime); 
    } else {
        XUngrabKeyboard(xjmkb->win->app->dpy, CurrentTime); 
    }
}

// static
void XKeyBoard::through_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    if (adj_get_value(w->adj)) {
        xjmkb->xjack->midi_through = 1;
    } else {
        xjmkb->xjack->midi_through = 0;
    }
}

// static
void XKeyBoard::key_press(void *w_, void *key_, void *user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    XKeyEvent *key = (XKeyEvent*)key_;
    if (!key) return;
    if (key->state & ControlMask) {
        KeySym sym = XLookupKeysym (key, 0);
        if (!sym) return;
        switch (sym) {
            case (XK_a):
            {
                xjmkb->info_callback(xjmkb->info,NULL);
            }
            break;
            case (XK_c):
            {
                xjmkb->signal_handle (2);
            }
            break;
            case (XK_d):
            {
                Widget_t *dia = open_file_dialog(xjmkb->win, xjmkb->soundfontpath.c_str(), ".sf");
                XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
                xjmkb->win->func.dialog_callback = synth_load_response;
            }
            break;
            case (XK_f):
            {
                Widget_t *menu = xjmkb->filemenu->childlist->childs[0];
                XWindowAttributes attrs;
                XGetWindowAttributes(w->app->dpy, (Window)menu->widget, &attrs);
                if (attrs.map_state != IsViewable) {
                    pop_menu_show(xjmkb->filemenu, menu, 6, true);
                } else {
                    widget_hide(menu);
                }
            }
            break;
            case (XK_g):
            {
                float value = adj_get_value(xjmkb->grab_keyboard->adj);
                adj_set_value(xjmkb->grab_keyboard->adj, 1.0-value);
                if (adj_get_value(xjmkb->grab_keyboard->adj)) {
                    XGrabKeyboard(xjmkb->win->app->dpy, xjmkb->wid->widget, true, 
                                GrabModeAsync, GrabModeAsync, CurrentTime); 
                } else {
                    XUngrabKeyboard(xjmkb->win->app->dpy, CurrentTime); 
                }
            }
            break;
            case (XK_i):
            {
                Widget_t *menu = xjmkb->info->childlist->childs[0];
                XWindowAttributes attrs;
                XGetWindowAttributes(w->app->dpy, (Window)menu->widget, &attrs);
                if (attrs.map_state != IsViewable) {
                    pop_menu_show(xjmkb->info, menu, 6, true);
                } else {
                    widget_hide(menu);
                }
            }
            break;
            case (XK_k):
            {
                open_custom_keymap(xjmkb->wid, xjmkb->win, xjmkb->multikeymap_file.c_str());
            }
            break;
            case (XK_l):
            {
                Widget_t *dia = open_file_dialog(xjmkb->win, xjmkb->filepath.c_str(), "midi");
                XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
                xjmkb->win->func.dialog_callback = dialog_load_response;
            }
            break;
            case (XK_m):
            {
                Widget_t *menu = xjmkb->mapping->childlist->childs[0];
                XWindowAttributes attrs;
                XGetWindowAttributes(w->app->dpy, (Window)menu->widget, &attrs);
                if (attrs.map_state != IsViewable) {
                    pop_menu_show(xjmkb->mapping, menu, 6, true);
                } else {
                    widget_hide(menu);
                }
            }
            break;
            case (XK_n):
            {
                if(!xjmkb->xsynth->synth_is_active() || !xjmkb->fs_instruments) break;
                XWindowAttributes attrs;
                XGetWindowAttributes(w->app->dpy, (Window)xjmkb->synth_ui->widget, &attrs);
                if (attrs.map_state != IsViewable) {
                    xjmkb->show_synth_ui(1);
                } else {
                    xjmkb->show_synth_ui(0);
                }
            }
            break;
            case (XK_o):
            {
                Widget_t *menu = xjmkb->connection->childlist->childs[0];
                XWindowAttributes attrs;
                XGetWindowAttributes(w->app->dpy, (Window)menu->widget, &attrs);
                if (attrs.map_state != IsViewable) {
                    make_connection_menu(xjmkb->connection, NULL, NULL);
                    pop_menu_show(xjmkb->connection, menu, 6, true);
                } else {
                    widget_hide(menu);
                }
            }
            break;
            case (XK_p):
            {
                int value = (int)adj_get_value(xjmkb->play->adj);
                if (value) adj_set_value(xjmkb->play->adj,0.0);
                else adj_set_value(xjmkb->play->adj,1.0);
            }
            break;
            case (XK_q):
            {
                if(xjmkb->nsmsig.nsm_session_control) {
                    widget_hide(xjmkb->win);
                    xjmkb->nsmsig.trigger_nsm_gui_is_hidden();
                } else {
                    fprintf(stderr, "ctrl +q received, quit now\n");
                    quit(xjmkb->win);
                }
            }
            break;
            case (XK_r):
            { 
                int value = (int)adj_get_value(xjmkb->record->adj);
                if (value) adj_set_value(xjmkb->record->adj,0.0);
                else adj_set_value(xjmkb->record->adj,1.0);
            }
            break;
            case (XK_s):
            {
                Widget_t *dia = save_file_dialog(xjmkb->win, xjmkb->filepath.c_str(), "midi");
                XSetTransientForHint(xjmkb->win->app->dpy, dia->widget, xjmkb->win->widget);
                xjmkb->win->func.dialog_callback = dialog_save_response;
            }
            break;
            case (XK_t):
            {
                float value = adj_get_value(xjmkb->midi_through->adj);
                adj_set_value(xjmkb->midi_through->adj, 1.0-value);
            }
            break;
            case (XK_u):
            {
                Widget_t *menu = xjmkb->synth->childlist->childs[0];
                XWindowAttributes attrs;
                XGetWindowAttributes(w->app->dpy, (Window)menu->widget, &attrs);
                if (attrs.map_state != IsViewable) {
                    pop_menu_show(xjmkb->synth, menu, 6, true);
                } else {
                    widget_hide(menu);
                }
            }
            break;
            case (XK_x):
            {
                if(!xjmkb->xsynth->synth_is_active()) break;
                xjmkb->show_synth_ui(0);
                xjmkb->xsynth->unload_synth();
                xjmkb->soundfont = "";
                xjmkb->fs[0]->state = 4;
                xjmkb->fs[1]->state = 4;
                xjmkb->fs[2]->state = 4;
            }
            break;
            case (XK_y):
            {
                if(!xjmkb->xsynth->synth_is_active()) break;
                xjmkb->xsynth->panic();
            }
            break;
            case (XK_0):
            {
                MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
                adj_set_value(xjmkb->octavemap->adj,0.0);
                keys->octave = 0;
                xjmkb->octave = 0;
                expose_widget(xjmkb->wid);
            }
            break;
            case (XK_1):
            {
                MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
                adj_set_value(xjmkb->octavemap->adj,1.0);
                keys->octave = 12;
                xjmkb->octave = 12;
                expose_widget(xjmkb->wid);
            }
            break;
            case (XK_2):
            {
                MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
                adj_set_value(xjmkb->octavemap->adj,2.0);
                keys->octave = 24;
                xjmkb->octave = 24;
                expose_widget(xjmkb->wid);
            }
            break;
            case (XK_3):
            {
                MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
                adj_set_value(xjmkb->octavemap->adj,3.0);
                keys->octave = 36;
                xjmkb->octave = 36;
                expose_widget(xjmkb->wid);
            }
            break;
            case (XK_4):
            {
                MidiKeyboard *keys = (MidiKeyboard*)xjmkb->wid->parent_struct;
                adj_set_value(xjmkb->octavemap->adj,4.0);
                keys->octave = 48;
                xjmkb->octave = 48;
                expose_widget(xjmkb->wid);
            }
            break;
            default:
            break;
        }
    } else {
        xjmkb->wid->func.key_press_callback(xjmkb->wid, key_, user_data);
    }
}

// static
void XKeyBoard::key_release(void *w_, void *key_, void *user_data) {
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w_);
    xjmkb->wid->func.key_release_callback(xjmkb->wid, key_, user_data);
}

/**************** Fluidsynth settings window *****************/

//static 
void XKeyBoard::draw_synth_ui(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    set_pattern(w,&w->app->color_scheme->selected,&w->app->color_scheme->normal,BACKGROUND_);
    cairo_paint (w->crb);
}

//static 
void XKeyBoard::synth_ui_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    if (w->flags & HAS_POINTER && !*(int*)user_data){
        XKeyBoard::get_instance(w)->show_synth_ui(0);
    }
    adj_set_value(w->adj, 0.0);
}

//static 
void XKeyBoard::reverb_level_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->reverb_level = adj_get_value(w->adj);
    xjmkb->xsynth->set_reverb_levels();
}

//static 
void XKeyBoard::reverb_width_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->reverb_width = adj_get_value(w->adj);
    xjmkb->xsynth->set_reverb_levels();
}

//static 
void XKeyBoard::reverb_damp_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->reverb_damp = adj_get_value(w->adj);
    xjmkb->xsynth->set_reverb_levels();
}

//static 
void XKeyBoard::reverb_roomsize_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->reverb_roomsize = adj_get_value(w->adj);
    xjmkb->xsynth->set_reverb_levels();
}

//static 
void XKeyBoard::reverb_on_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->reverb_on = (int)adj_get_value(w->adj);
    xjmkb->xsynth->set_reverb_on(xjmkb->xsynth->reverb_on);
}

//static 
void XKeyBoard::chorus_type_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->chorus_type = (int)adj_get_value(w->adj);
    xjmkb->xsynth->set_chorus_levels();
}

//static 
void XKeyBoard::chorus_depth_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->chorus_depth = adj_get_value(w->adj);
    xjmkb->xsynth->set_chorus_levels();
}

//static 
void XKeyBoard::chorus_speed_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->chorus_speed = adj_get_value(w->adj);
    xjmkb->xsynth->set_chorus_levels();
}

//static 
void XKeyBoard::chorus_level_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->chorus_level = adj_get_value(w->adj);
    xjmkb->xsynth->set_chorus_levels();
}

//static 
void XKeyBoard::chorus_voices_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->chorus_voices = (int)adj_get_value(w->adj);
    xjmkb->xsynth->set_chorus_levels();
}

//static 
void XKeyBoard::chorus_on_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->chorus_on = (int)adj_get_value(w->adj);
    xjmkb->xsynth->set_chorus_on(xjmkb->xsynth->chorus_on);
}

// static
void XKeyBoard::channel_pressure_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->set_channel_pressure(xjmkb->mchannel, (int)adj_get_value(w->adj));
}
// static
void XKeyBoard::synth_volume_callback(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    XKeyBoard *xjmkb = XKeyBoard::get_instance(w);
    xjmkb->xsynth->volume_level = adj_get_value(w->adj);
    xjmkb->xsynth->set_gain();
}

// static
void XKeyBoard::set_on_off_label(void *w_, void* user_data) noexcept{
    Widget_t *w = (Widget_t*)w_;
    int value = (int)adj_get_value(w->adj);
    if (value) {
        w->label = _("Off");
    } else {
        w->label = _("On");
    }
    expose_widget(w);
}

//static
void XKeyBoard::instrument_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XKeyBoard *xjmkb = (XKeyBoard*) win->parent_struct;
    int i = (int)adj_get_value(xjmkb->fs_instruments->adj);
    xjmkb->xsynth->channel_instrument[xjmkb->mchannel] = i;
    std::istringstream buf(xjmkb->xsynth->instruments[i]);
    buf >> xjmkb->mbank;
    buf >> xjmkb->mprogram;
    adj_set_value(xjmkb->bank->adj,xjmkb->mbank);
    adj_set_value(xjmkb->program->adj,xjmkb->mprogram);
}

void XKeyBoard::rebuild_instrument_list() {
    if(fs_instruments) {
        combobox_delete_entrys(fs_instruments);
    }

    for(std::vector<std::string>::const_iterator
            i = xsynth->instruments.begin();i != xsynth->instruments.end(); ++i) {
        combobox_add_entry(fs_instruments,(*i).c_str());
    }

    int i = xsynth->get_instrument_for_channel(mchannel);
    if ( i >-1)
        combobox_set_active_entry(fs_instruments, i);
}

//static
void XKeyBoard::soundfont_callback(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *win = get_toplevel_widget(w->app);
    XKeyBoard *xjmkb = (XKeyBoard*) win->parent_struct;
    if (!strstr(xjmkb->soundfontname.data(), w->label)) {
        std::string sf = xjmkb->soundfontpath;
        sf +="/";
        sf +=w->label;
        const char*sf_load = sf.data();
        xjmkb->synth_load_response(w, (void*)&sf_load);
    }
}

void XKeyBoard::rebuild_soundfont_list() {
    if(fs_soundfont) {
        combobox_delete_entrys(fs_soundfont);
    }
    int active = 0;
    // get all soundfonts from choosen directory
    FilePicker *fp = (FilePicker*)malloc(sizeof(FilePicker));
    fp_init(fp, soundfontpath.c_str());
    std::string f = ".sf";
    // filter will be free by FilePicker!!
    char *filter = (char*)malloc(sizeof(char) * (f.length() + 1));
    strcpy(filter, f.c_str());
    fp->filter = filter;
    fp->use_filter = 1;
    char *path = &soundfontpath[0];
    fp_get_files(fp,path,0, 1);
    for(unsigned int i = 0; i<fp->file_counter; i++) {
        combobox_add_entry(fs_soundfont, fp->file_names[i]);
        if (strstr(soundfontname.data(), fp->file_names[i]))
            active = i;
    }
    fp_free(fp);
    free(fp);
    combobox_set_active_entry(fs_soundfont, active);
}

void XKeyBoard::show_synth_ui(int present) {
    if(present) {
        widget_show_all(synth_ui);
        int y = main_y-226;
        if (main_y < 230) y = main_y + main_h+21;
        XMoveWindow(win->app->dpy,synth_ui->widget, main_x, y);
    }else {
        widget_hide(synth_ui);
    }
}

void XKeyBoard::init_synth_ui(Widget_t *parent) {
    synth_ui = create_window(parent->app, DefaultRootWindow(parent->app->dpy), 0, 0, 650, 200);
    XSelectInput(parent->app->dpy, synth_ui->widget,StructureNotifyMask|ExposureMask|KeyPressMask 
                    |EnterWindowMask|LeaveWindowMask|ButtonReleaseMask|KeyReleaseMask
                    |ButtonPressMask|Button1MotionMask|PointerMotionMask);
    XSetTransientForHint(parent->app->dpy, synth_ui->widget, parent->widget);
    soundfontname =  basename((char*)soundfont.c_str());
    std::string title = _("Fluidsynth - ");
    title += soundfontname;
    widget_set_title(synth_ui, title.c_str());
    widget_set_dnd_aware(synth_ui);
    synth_ui->flags &= ~USE_TRANSPARENCY;
    synth_ui->flags |= NO_AUTOREPEAT | NO_PROPAGATE | HIDE_ON_DELETE;
    synth_ui->func.expose_callback = draw_synth_ui;
    synth_ui->scale.gravity = CENTER;
    synth_ui->parent = parent;
    synth_ui->parent_struct = this;
    synth_ui->func.key_press_callback = key_press;
    synth_ui->func.key_release_callback = key_release;
    synth_ui->func.dnd_notify_callback = dnd_load_response;

    fs_instruments = add_combobox(synth_ui, _("Instruments"), 20, 10, 260, 30);
    fs_instruments->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    fs_instruments->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    fs_instruments->func.value_changed_callback = instrument_callback;
    fs_instruments->func.key_press_callback = key_press;
    fs_instruments->func.key_release_callback = key_release;
    Widget_t *tmp = fs_instruments->childlist->childs[0];
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    fs_soundfont = add_combobox(synth_ui, _("Soundfonts"), 290, 10, 260, 30);
    fs_soundfont->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    fs_soundfont->childlist->childs[0]->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    fs_soundfont->func.value_changed_callback = soundfont_callback;
    fs_soundfont->func.key_press_callback = key_press;
    fs_soundfont->func.key_release_callback = key_release;
    tmp = fs_soundfont->childlist->childs[0];
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    // reverb
    tmp = add_keyboard_button(synth_ui, _("On"), 20,  150, 60, 30);
    tmp->func.adj_callback = set_on_off_label;
    tmp->func.value_changed_callback = reverb_on_callback;
    adj_set_value(tmp->adj,(float)xsynth->reverb_on);

    tmp = add_label(synth_ui,_("Reverb"),15,50,80,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_knob(synth_ui, _("Roomsize"), 20, 70, 60, 75);
    set_adjustment(tmp->adj, 0.2, 0.2, 0.0, 1.2, 0.01, CL_CONTINUOS);
    adj_set_value(tmp->adj, xsynth->reverb_roomsize);
    tmp->func.value_changed_callback = reverb_roomsize_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_knob(synth_ui, _("Damp"), 80, 70, 65, 75);
    set_adjustment(tmp->adj, 0.0, 0.0, 0.0, 1.0, 0.01, CL_CONTINUOS);
    adj_set_value(tmp->adj, xsynth->reverb_damp);
    tmp->func.value_changed_callback = reverb_damp_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_knob(synth_ui, _("Width"), 145, 70, 65, 75);
    set_adjustment(tmp->adj, 0.5, 0.5, 0.0, 100.0, 0.5, CL_CONTINUOS);
    adj_set_value(tmp->adj, xsynth->reverb_width);
    tmp->func.value_changed_callback = reverb_width_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_knob(synth_ui, _("Level"), 210, 70, 65, 75);
    set_adjustment(tmp->adj, 0.5, 0.5, 0.0, 1.0, 0.01, CL_CONTINUOS);
    adj_set_value(tmp->adj, xsynth->reverb_level);
    tmp->func.value_changed_callback = reverb_level_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    // chorus
    tmp = add_label(synth_ui,_("Chorus"),290,50,80,20);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_button(synth_ui, _("On"), 290,  150, 60, 30);
    tmp->func.adj_callback = set_on_off_label;
    tmp->func.value_changed_callback = chorus_on_callback;
    adj_set_value(tmp->adj, (float)xsynth->chorus_on);

    tmp = add_keyboard_knob(synth_ui, _("voices"), 290, 70, 65, 75);
    set_adjustment(tmp->adj, 0.0, 0.0, 0.0, 99.0, 1.0, CL_CONTINUOS);
    adj_set_value(tmp->adj, (float)xsynth->chorus_voices);
    tmp->func.value_changed_callback = chorus_voices_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_knob(synth_ui, _("Level"), 355, 70, 65, 75);
    set_adjustment(tmp->adj, 0.0, 0.0, 0.0, 10.0, 0.1, CL_CONTINUOS);
    adj_set_value(tmp->adj, xsynth->chorus_level);
    tmp->func.value_changed_callback = chorus_level_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_knob(synth_ui, _("Speed"), 420, 70, 65, 75);
    set_adjustment(tmp->adj, 0.1, 0.1, 0.1, 5.0, 0.05, CL_CONTINUOS);
    adj_set_value(tmp->adj, xsynth->chorus_speed);
    tmp->func.value_changed_callback = chorus_speed_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_keyboard_knob(synth_ui, _("Depth"), 485, 70, 65, 75);
    set_adjustment(tmp->adj, 0.0, 0.0, 0.0, 21.0, 0.1, CL_CONTINUOS);
    adj_set_value(tmp->adj, xsynth->chorus_depth);
    tmp->func.value_changed_callback = chorus_depth_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_combobox(synth_ui, _("MODE"), 370, 150, 100, 30);
    combobox_add_entry(tmp, _("SINE"));
    combobox_add_entry(tmp, _("TRIANGLE"));
    combobox_set_active_entry(tmp, (float)xsynth->chorus_type);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.value_changed_callback = chorus_type_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    tmp = add_hslider(synth_ui, _("Channel Pressure"), 80, 150, 210, 30);
    set_adjustment(tmp->adj, 0.0, 0.0, 0.0, 127.0, 1.0, CL_CONTINUOS);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.value_changed_callback = channel_pressure_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;

    // general
    tmp = add_keyboard_button(synth_ui, _("Close"), 570, 160, 60, 30);
    tmp->func.value_changed_callback = synth_ui_callback;

    tmp = add_keyboard_knob(synth_ui, _("Volume"), 555, 40, 95, 105);
    set_adjustment(tmp->adj, 0.0, 0.0, 0.0, 1.2, 0.0005, CL_LOGSCALE);
    adj_set_value(tmp->adj, xsynth->volume_level);
    tmp->flags |= NO_AUTOREPEAT | NO_PROPAGATE;
    tmp->func.value_changed_callback = synth_volume_callback;
    tmp->func.key_press_callback = key_press;
    tmp->func.key_release_callback = key_release;
    
}

/******************* Exit handlers ********************/

void XKeyBoard::signal_handle (int sig) {
    if(xjack->client) jack_client_close (xjack->client);
    xjack->client = NULL;
    XLockDisplay(win->app->dpy);
    quit(win);
    XFlush(win->app->dpy);
    XUnlockDisplay(win->app->dpy);
    fprintf (stderr, "\n%s: signal %i received, bye bye ...\n",client_name.c_str(), sig);
}

void XKeyBoard::exit_handle (int sig) {
    if(xjack->client) jack_client_close (xjack->client);
    xjack->client = NULL;
    fprintf (stderr, "\n%s: signal %i received, exiting ...\n",client_name.c_str(), sig);
    exit (0);
}

/****************************************************************
 ** class PosixSignalHandler
 **
 ** Watch for incomming system signals in a extra thread
 ** 
 */

PosixSignalHandler::PosixSignalHandler()
    : sigc::trackable(),
      waitset(),
      thread(nullptr),
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
            case SIGINT:
            case SIGTERM:
            case SIGQUIT:
                trigger_quit_by_posix(sig);
            break;
            case SIGHUP:
            case SIGKILL:
                trigger_kill_by_posix(sig);
            break;
            default:
            break;
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

    midikeyboard::PosixSignalHandler xsig;

    Xputty app;

    mamba::MidiMessenger mmessage;
    nsmhandler::NsmSignalHandler nsmsig;
    midikeyboard::AnimatedKeyBoard  animidi;

    xalsa::XAlsa xalsa([&mmessage]
        (int _cc, int _pg, int _bgn, int _num, bool have_channel) noexcept
        {mmessage.send_midi_cc( _cc, _pg, _bgn, _num, have_channel);});

    xjack::XJack xjack(&mmessage,
        [&xalsa] (const uint8_t* m ,uint8_t n ) noexcept {xalsa.xalsa_output_notify(m,n);},
        [&xalsa] (int p ) {xalsa.xalsa_set_priority(p);});

    xsynth::XSynth xsynth;
    midikeyboard::XKeyBoard xjmkb(&xjack, &xalsa, &xsynth, &mmessage, nsmsig, xsig, &animidi);
    nsmhandler::NsmHandler nsmh(&nsmsig);

    nsmsig.nsm_session_control = nsmh.check_nsm(xjmkb.client_name.c_str(), argv);

    xjmkb.read_config();

    main_init(&app);
    
    xjmkb.init_ui(&app);
    if (xalsa.xalsa_init("Mamba", "input", "output") >= 0) {
        MidiKeyboard *keys = (MidiKeyboard*)xjmkb.wid->parent_struct;
        xalsa.xalsa_start([keys] (int channel, int key, bool set)
            {set_key_in_matrix(keys->in_key_matrix[channel], key, set);});
    } else {
        fprintf(stderr, _("Couldn't open a alsa port, is the alsa sequencer running?\n"));
    }
    if (xjack.init_jack()) {
        if (!xjmkb.soundfont.empty()) {
            xsynth.setup(xjack.SampleRate);
            xsynth.init_synth();
            xsynth.load_soundfont(xjmkb.soundfont.c_str());
            const char **port_list = NULL;
            port_list = jack_get_ports(xjack.client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
            std::string synth_instance = jack_get_client_name(xjack.client);
            std::transform(synth_instance.begin(), synth_instance.end(), synth_instance.begin(), ::tolower);
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
            mmessage.send_midi_cc(0xB0, 7, xjmkb.volume, 3, false);
            xjmkb.fs[0]->state = 0;
            xjmkb.fs[1]->state = 0;
            xjmkb.fs[2]->state = 0;
            xjmkb.rebuild_instrument_list();
            xjmkb.rebuild_soundfont_list();
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

        xjmkb.show_ui(xjmkb.visible);
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



