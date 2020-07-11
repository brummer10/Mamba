/*
 *                           0BSD 
 * 
 *                    BSD Zero Clause License
 * 
 *  Copyright (c) 2019 Hermann Meyer
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

#pragma once

#ifndef XMIDI_KEYBOARD_H_
#define XMIDI_KEYBOARD_H_

#include "xwidgets.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CPORTS 11

typedef void (*midikeyfunc)(Widget_t *w,int *key, bool on_off);
typedef void (*midiwheelfunc)(Widget_t *w,int *value);

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

typedef struct {
    Widget_t *w[CPORTS];

    int octave;
    int layout;
    int modwheel;
    int detune;
    int attack;
    int expression;
    int release;
    int volume;
    int velocity;
    int pitchwheel;
    int balance;
    int sustain;
    int sostenuto;
    int prelight_key;
    int new_prelight_key;
    int active_key;
    int new_active_key;
    int send_key;
    int in_motion;
    unsigned long key_matrix[4];

    midikeyfunc mk_send_note;
    midiwheelfunc mk_send_pitch;
    midiwheelfunc mk_send_balance;
    midiwheelfunc mk_send_sustain;
    midiwheelfunc mk_send_sostenuto;
    midiwheelfunc mk_send_mod;
    midiwheelfunc mk_send_detune;
    midiwheelfunc mk_send_attack;
    midiwheelfunc mk_send_expression;
    midiwheelfunc mk_send_release;
    midiwheelfunc mk_send_volume;
    midiwheelfunc mk_send_velocity;
    midiwheelfunc mk_send_all_sound_off;
} MidiKeyboard;

void keysym_azerty_to_midi_key(long inkey, float *midi_key);

void keysym_qwertz_to_midi_key(long inkey, float *midi_key);

void keysym_qwerty_to_midi_key(unsigned int inkey, float *midi_key);

bool is_key_in_matrix(unsigned long *key_matrix, int key);

bool have_key_in_matrix(unsigned long *key_matrix);

void clear_key_matrix(unsigned long *key_matrix);

void add_keyboard(Widget_t *wid);

Widget_t *open_midi_keyboard(Widget_t *w);

void add_midi_keyboard(Widget_t *parent, const char * label,
                            int x, int y, int width, int height);

#ifdef __cplusplus
}
#endif

#endif //XMIDI_KEYBOARD_H_
