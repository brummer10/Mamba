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

#ifndef XMAMBA_KEYBOARD_H_
#define XMAMBA_KEYBOARD_H_

#include "xwidgets.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*mambakeyfunc)(Widget_t *w, const int *key, const bool on_off);
typedef void (*mambawheelfunc)(Widget_t *w, const int *value);

typedef struct {

    int channel;
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
    int last_active_key;
    int send_key;
    int in_motion;
    int key_size;
    int key_offset;
    int edo;
    unsigned long key_matrix[4];
    unsigned long in_key_matrix[16][4];
    unsigned long edo_matrix[8];
    long custom_keys[128][2];

    mambakeyfunc mk_send_note;
    mambawheelfunc mk_send_all_sound_off;
} MambaKeyboard;

void mamba_keysym_azerty_to_midi_key(long inkey, float *midi_key);

void mamba_keysym_azerty_fr_to_midi_key(long inkey, float *midi_key);

void mamba_keysym_azerty_be_to_midi_key(long inkey, float *midi_key);

void mamba_keysym_azerty_afnor_to_midi_key(long inkey, float *midi_key);

void mamba_keysym_qwertz_to_midi_key(long inkey, float *midi_key);

void mamba_keysym_qwerty_to_midi_key(unsigned int inkey, float *midi_key);

void mamba_custom_to_midi_key(long custom_keys[128], long inkey, float *midi_key);

void mamba_read_keymap(MambaKeyboard *keys, const char* keymapfile, long custom_keys[128][2]);

void mamba_set_key_in_matrix(unsigned long *key_matrix, int key, bool set);

bool mamba_is_key_in_matrix(unsigned long *key_matrix, int key);

bool mamba_have_key_in_matrix(unsigned long *key_matrix);

void mamba_clear_key_matrix(unsigned long *key_matrix);

void mamba_set_edo(MambaKeyboard *keys, Widget_t *w, int edo);

void mamba_add_keyboard(Widget_t *wid, const char * label);

Widget_t *mamba_open_midi_keyboard(Widget_t *w, const char * label);

void mamba_add_midi_keyboard(Widget_t *parent, const char * label,
                            int x, int y, int width, int height);

bool mamba_need_redraw(MambaKeyboard *keys);

#ifdef __cplusplus
}
#endif

#endif //XMAMBA_KEYBOARD_H_
