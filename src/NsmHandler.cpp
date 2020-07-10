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


#include "NsmHandler.h"


namespace nsmhandler {


/****************************************************************
 ** class NsmWatchDog
 **
 ** Watch for incomming messages from NSM server in a extra timeout thread
 ** 
 */

NsmWatchDog::NsmWatchDog() 
    :_execute(false) {
}

NsmWatchDog::~NsmWatchDog() {
    if( _execute.load(std::memory_order_acquire) ) {
        stop();
    };
}

void NsmWatchDog::stop() {
    _execute.store(false, std::memory_order_release);
    if (_thd.joinable()) {
        _thd.join();
    }
}

void NsmWatchDog::start(int interval, std::function<void(void)> func) {
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

bool NsmWatchDog::is_running() const noexcept {
    return ( _execute.load(std::memory_order_acquire) && 
             _thd.joinable() );
}


/****************************************************************
 ** class NsmHandler
 **
 ** Check if NSM server response, if so, connect the handlers
 ** and signal the UI thread to set up the variables
 */

NsmHandler::NsmHandler(NsmWatchDog *poll_, NsmSignalHandler *nsmsig_)
    : poll(poll_),
    nsmsig(nsmsig_),
    nsm(0),
    wait_id(true),
    optional_gui(false) {
        
    nsmsig->signal_trigger_nsm_gui_is_shown().connect(
        sigc::mem_fun(this, &NsmHandler::_on_nsm_is_shown));

    nsmsig->signal_trigger_nsm_gui_is_hidden().connect(
        sigc::mem_fun(this, &NsmHandler::_on_nsm_is_hidden));
}

NsmHandler::~NsmHandler() {
    if (nsm) {
        nsm_free(nsm);
        nsm = 0;
    }
}

bool NsmHandler::check_nsm(char *argv[]) {

    const char *nsm_url = getenv( "NSM_URL" );

    if (nsm_url) {
        nsm = nsm_new();
        nsm_set_open_callback( nsm, nsm_open, static_cast<void*>(this));
        nsm_set_save_callback( nsm, nsm_save, static_cast<void*>(this));
        nsm_set_show_callback( nsm, nsm_show, static_cast<void*>(this));
        nsm_set_hide_callback( nsm, nsm_hide, static_cast<void*>(this));
        if ( 0 == nsm_init( nsm, nsm_url)) {
            nsm_send_announce( nsm, "MidiKeyBoard", ":optional-gui:", argv[0]);
            int wait_count = 0;
            while(wait_id) {
                nsm_check_wait(nsm,500);
                wait_count +=1;
                if (wait_count > 200) {
                        fprintf (stderr, "\n NSM didn't response, exiting ...\n");
                        exit (1);
                }
            }
            return true;
        } else {
            nsm_free(nsm);
            nsm = 0;
        }
    }
    return false;
}

void NsmHandler::_poll_nsm(void* arg) {
    NsmHandler *nsmh = static_cast<NsmHandler*>(arg);
    nsm_check_nowait(nsmh->nsm);
}

void NsmHandler::_nsm_start_poll() {
    poll->start(200, std::bind(_poll_nsm,this));
}

int NsmHandler::_nsm_open (const char *name, const char *display_name,
                            const char *client_id, char **out_msg ) {

    if (strstr(nsm_get_session_manager_features(nsm), ":optional-gui:")) {
        optional_gui = true;
    }
    nsmsig->trigger_nsm_gui_open(name, client_id, optional_gui);
    wait_id = false;

    _nsm_start_poll();

    return ERR_OK;
}
 
int NsmHandler::_nsm_save ( char **out_msg) {
    nsmsig->trigger_nsm_save_gui();
    return ERR_OK;
}
 
void NsmHandler::_nsm_show () {
    nsmsig->trigger_nsm_show_gui();
}

void NsmHandler::_nsm_hide () {
    nsmsig->trigger_nsm_hide_gui();
}
  
void NsmHandler::_on_nsm_is_shown () {
    nsm_send_is_shown(nsm);
}
  
void NsmHandler::_on_nsm_is_hidden () {
    nsm_send_is_hidden(nsm);
}
 
//static
int NsmHandler::nsm_open (const char *name, const char *display_name,
            const char *client_id, char **out_msg, void *userdata ) {
    NsmHandler * nsmhandler = static_cast<NsmHandler*>(userdata);
    return nsmhandler->_nsm_open (name, display_name, client_id, out_msg);
}
            
//static            
int NsmHandler::nsm_save ( char **out_msg, void *userdata ) {
    NsmHandler * nsmhandler = static_cast<NsmHandler*>(userdata);
    return nsmhandler->_nsm_save(out_msg);
}
            
//static            
void NsmHandler::nsm_show (void *userdata ) {
    NsmHandler * nsmhandler = static_cast<NsmHandler*>(userdata);
    return nsmhandler->_nsm_show();
}
            
//static            
void NsmHandler::nsm_hide (void *userdata ) {
    NsmHandler * nsmhandler = static_cast<NsmHandler*>(userdata);
    return nsmhandler->_nsm_hide();
}

} // namespace nsmhandler

