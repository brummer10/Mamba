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

#include "xmkeyboard.h"
#include <unistd.h>

//long major_chords[12][3] = {{12,13,14,15,16,17,18,19,20,21,22,23},{12,16,19},{13,17,20},{14,18,21},{15,19,22},{16,20,23},{17,21,24},
//                            {18,22,25},{19,23,26},{20,24,27},{21,25,28},{22,26,29},{23,27,30}};

void add_major_chords(long inkey, long *outkey[3]) {
    (*outkey[0]) = inkey;
    (*outkey[1]) = inkey + 4;
    (*outkey[2]) = inkey + 7;
}

void mamba_keysym_azerty_to_midi_key(long inkey, float *midi_key) {
    /* these keys are common to all types of azerty keyboards */
    switch(inkey) {
        case(XK_w) : (*midi_key) = 12.0; /* w = C0 */
        break;
        case(XK_s) : (*midi_key) = 13.0; /* s */
        break;
        case(XK_x) : (*midi_key) = 14.0; /* x */
        break;
        case(XK_d) : (*midi_key) = 15.0; /* d */
        break;
        case(XK_c) : (*midi_key) = 16.0; /* c */
        break;
        case(XK_v) : (*midi_key) = 17.0; /* v */
        break;
        case(XK_g) : (*midi_key) = 18.0; /* g */
        break;
        case(XK_b) : (*midi_key) = 19.0; /* b */
        break;
        case(XK_h) : (*midi_key) = 20.0; /* h */
        break;
        case(XK_n) : (*midi_key) = 21.0; /* n */
        break;
        case(XK_j) : (*midi_key) = 22.0; /* j */
        break;
        case(XK_a) : (*midi_key) = 24.0; /* a = C1 */
        break;
        case(XK_l) : (*midi_key) = 25.0; /* l */
        break;
        case(XK_z) : (*midi_key) = 26.0; /* z */
        break;
        case(XK_m) : (*midi_key) = 27.0; /* m */
        break;
        case(XK_e) : (*midi_key) = 28.0; /* e */
        break;
        case(XK_r) : (*midi_key) = 29.0; /* r */
        break;
        case(XK_t) : (*midi_key) = 31.0; /* t */
        break;
        case(XK_y) : (*midi_key) = 33.0; /* y */
        break;
        case(XK_u) : (*midi_key) = 35.0; /* u */
        break;
        case(XK_i) : (*midi_key) = 36.0; /* i */
        break;
        case(XK_o) : (*midi_key) = 38.0; /* o */
        break;
        case(XK_p) : (*midi_key) = 40.0; /* p */
        break;
    }
}

void mamba_keysym_azerty_fr_to_midi_key(long inkey, float *midi_key) {
    /* common azerty keyboards sell in France */
    switch(inkey) {
        case(XK_less) : (*midi_key) = 11.0; /* "<" = B-1 */
        break;
        case(XK_comma) : (*midi_key) = 23.0; /* , */
        break;
        case(XK_semicolon) : (*midi_key) = 24.0; /* ; = C1 */
        break;
        case(XK_eacute) : (*midi_key) = 25.0; /* é */
        break;
        case(XK_colon) : (*midi_key) = 26.0; /* : */
        break;
        case(XK_quotedbl) : (*midi_key) = 27.0; /* " */
        break;
        case(XK_exclam) : (*midi_key) = 28.0; /* ! */
        break;
        case(XK_ugrave) : (*midi_key) = 29.0; /* ù = F1 */
        break;
        case(XK_asterisk) : (*midi_key) = 30.0; /* * */
        break;
        case(XK_parenleft) : (*midi_key) = 30.0; /* ( */
        break;
        case(XK_minus) : (*midi_key) = 32.0; /* - */
        break;
        case(XK_egrave) : (*midi_key) = 34.0; /* è */
        break;
        case(XK_ccedilla) : (*midi_key) = 37.0; /* ç */
        break;
        case(XK_agrave) : (*midi_key) = 39.0; /* à */
        break;
        case (XK_parenright) : (*midi_key) = 41.0; /* ) = F2 */
        break;
        case (XK_equal) : (*midi_key) = 42.0; /* = */
        break;
        case (XK_dollar) : (*midi_key) = 43.0; /* $ */
        break;
        default: mamba_keysym_azerty_to_midi_key(inkey, midi_key);
    }
}

void mamba_keysym_azerty_be_to_midi_key(long inkey, float *midi_key) {
    /* common azerty keyboards sell in Belgium (Wallonia) */
    switch(inkey) {
        case(XK_less) : (*midi_key) = 11.0; /* "<" = B-1 */
        break;
        case(XK_comma) : (*midi_key) = 23.0; /* , */
        break;
        case(XK_semicolon) : (*midi_key) = 24.0; /* ; = C1 */
        break;
        case(XK_eacute) : (*midi_key) = 25.0; /* é */
        break;
        case(XK_colon) : (*midi_key) = 26.0; /* : */
        break;
        case(XK_quotedbl) : (*midi_key) = 27.0; /* " */
        break;
        case(XK_equal) : (*midi_key) = 28.0; /* = */
        break;
        case(XK_ugrave) : (*midi_key) = 29.0; /* ù = F1 */
        break;
        case(XK_mu) : (*midi_key) = 30.0; /* µ */
        break;
        case(XK_parenleft) : (*midi_key) = 30.0; /* ( */
        break;
        case(XK_section) : (*midi_key) = 32.0; /* § */
        break;
        case(XK_egrave) : (*midi_key) = 34.0; /* è */
        break;
        case(XK_ccedilla) : (*midi_key) = 37.0; /* ç */
        break;
        case(XK_agrave) : (*midi_key) = 39.0; /* à */
        break;
        case (XK_parenright) : (*midi_key) = 41.0; /* ) = F2 */
        break;
        case (XK_minus) : (*midi_key) = 42.0; /* - */
        break;
        case (XK_dollar) : (*midi_key) = 43.0; /* $ */
        break;
        default: mamba_keysym_azerty_to_midi_key(inkey, midi_key);
    }
}

void mamba_keysym_azerty_afnor_to_midi_key(long inkey, float *midi_key) {
    /* probably a future standard for azerty keyboards */
    switch(inkey) {
        case(XK_less) : (*midi_key) = 11.0; /* "<" = B-1 */
        break;
        case(XK_KP_Decimal) : (*midi_key) = 23.0; /* . */
        break;
        case(XK_semicolon) : (*midi_key) = 24.0; /* ; = C1 */
        break;
        case(XK_eacute) : (*midi_key) = 25.0; /* é */
        break;
        case(XK_colon) : (*midi_key) = 26.0; /* : */
        break;
        case(XK_quotedbl) : (*midi_key) = 27.0; /* " */
        break;
        case(XK_exclam) : (*midi_key) = 28.0; /* ! */
        break;
        case(XK_ugrave) : (*midi_key) = 29.0; /* ù = F1 */
        break;
        case(XK_asterisk) : (*midi_key) = 30.0; /* * */
        break;
        case(XK_parenleft) : (*midi_key) = 30.0; /* ( */
        break;
        case(XK_minus) : (*midi_key) = 32.0; /* - */
        break;
        case(XK_egrave) : (*midi_key) = 34.0; /* è */
        break;
        case(XK_ccedilla) : (*midi_key) = 37.0; /* ç */
        break;
        case(XK_agrave) : (*midi_key) = 39.0; /* à */
        break;
        case (XK_parenright) : (*midi_key) = 41.0; /* ) = F2 */
        break;
        case (XK_equal) : (*midi_key) = 42.0; /* = */
        break;
        case (XK_dollar) : (*midi_key) = 43.0; /* $ */
        break;
        default: mamba_keysym_azerty_to_midi_key(inkey, midi_key);
    }
}

void mamba_keysym_qwertz_to_midi_key(long inkey, float *midi_key) {
    switch(inkey) {
        case(XK_y) : (*midi_key) = 12.0; /* y = C0 */
        break;
        case(XK_s) : (*midi_key) = 13.0; /* s */
        break;
        case(XK_x) : (*midi_key) = 14.0; /* x */
        break;
        case(XK_d) : (*midi_key) = 15.0; /* d */
        break;
        case(XK_c) : (*midi_key) = 16.0; /* c */
        break;
        case(XK_v) : (*midi_key) = 17.0; /* v */
        break;
        case(XK_g) : (*midi_key) = 18.0; /* g */
        break;
        case(XK_b) : (*midi_key) = 19.0; /* b */
        break;
        case(XK_h) : (*midi_key) = 20.0; /* h */
        break;
        case(XK_n) : (*midi_key) = 21.0; /* n */
        break;
        case(XK_j) : (*midi_key) = 22.0; /* j */
        break;
        case(XK_m) : (*midi_key) = 23.0; /* m */
        break;
        case(XK_q) : (*midi_key) = 24.0; /* q */
        break;
        case(XK_2) : (*midi_key) = 25.0; /* 2 */
        break;
        case(XK_w) : (*midi_key) = 26.0; /* w */
        break;
        case(XK_3) : (*midi_key) = 27.0; /* 3 */
        break;
        case(XK_e) : (*midi_key) = 28.0; /* e */
        break;
        case(XK_r) : (*midi_key) = 29.0; /* r */
        break;
        case(XK_5) : (*midi_key) = 30.0; /* 5 */
        break;
        case(XK_t) : (*midi_key) = 31.0; /* t */
        break;
        case(XK_6) : (*midi_key) = 32.0; /* 6 */
        break;
        case(XK_z) : (*midi_key) = 33.0; /* z */
        break;
        case(XK_7) : (*midi_key) = 34.0; /* 7 */
        break;
        case(XK_u) : (*midi_key) = 35.0; /* u */
        break;
        case(XK_i) : (*midi_key) = 36.0; /* i */
        break;
        case(XK_9) : (*midi_key) = 37.0; /* 9 */
        break;
        case(XK_o) : (*midi_key) = 38.0; /* o */
        break;
        case(XK_0) : (*midi_key) = 39.0; /* 0 */
        break;
        case(XK_p) : (*midi_key) = 40.0; /* p */
        break;
        case(XK_udiaeresis) : (*midi_key) = 41.0; /* ü */
        break;
        case(XK_plus) : (*midi_key) = 42.0; /* + */
        break;
    }
}

void mamba_keysym_qwerty_to_midi_key(unsigned int inkey, float *midi_key) {
    mamba_keysym_qwertz_to_midi_key(inkey,midi_key);
    if ((*midi_key) == 12) (*midi_key) = 33;
    else if ((*midi_key) == 33) (*midi_key) = 12;
}

void mamba_custom_to_midi_key(long custom_keys[128][2], long inkey, float *midi_key) {
    int i = 0;
    for(;i<129;i++) {
        int j = 0;
        for(;j<2;j++) {
            if (inkey == custom_keys[i][j]) {
                (*midi_key) = (float)i;
                return;
            }
        }
    }
}

void add_major_chord(unsigned long *key_matrix, int inkey, bool set) {
    unsigned long *use_matrix = &key_matrix[0];
    int key = inkey + 4;
    
    if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    if (set) {
        (*use_matrix) |= (1 << key);
    }else {
        (*use_matrix) &= (~(1 << key));
    }

    key = inkey + 7;
    if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    if (set) {
        (*use_matrix) |= (1 << key);
    }else {
        (*use_matrix) &= (~(1 << key));
    }
}

void add_minor_chord(unsigned long *key_matrix, int inkey, bool set) {
    unsigned long *use_matrix = &key_matrix[0];
    int key = inkey + 3;
    
    if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    if (set) {
        (*use_matrix) |= (1 << key);
    }else {
        (*use_matrix) &= (~(1 << key));
    }

    key = inkey + 7;
    if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    if (set) {
        (*use_matrix) |= (1 << key);
    }else {
        (*use_matrix) &= (~(1 << key));
    }
}

void mamba_set_key_in_matrix(unsigned long *key_matrix, int key, bool set) {
    unsigned long *use_matrix = &key_matrix[0];
    if (key > 127) return;

    if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    if (set) {
        (*use_matrix) |= (1 << key);
    }else {
        (*use_matrix) &= (~(1 << key));
    }
}

bool mamba_is_key_in_matrix(unsigned long *key_matrix, int key) {
    unsigned long *use_matrix = &key_matrix[0];
    if (key > 127) return false;

    if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    bool ret = false;
    if((*use_matrix) & (1<<key)) {
        ret = true;
    }
    return ret;
}

int is_key_in_in_matrix(MambaKeyboard *keys, int key) {
    int i = 0;
    for(;i<16;i++) {
        if (mamba_is_key_in_matrix(keys->in_key_matrix[i], key)) {
           return i; 
       }
    }
    return -1;
}

bool mamba_have_key_in_matrix(unsigned long *key_matrix) {
    
    bool ret = false;
    int i = 0;
    int j = 0;
    for(;j<4;j++) {
        for(;i<32;i++) {
            if(key_matrix[j] & (1<<i)) {
                ret = true;
                break;
            }
        }
        i = 0;
    }
    return ret;
}

void mamba_clear_key_matrix(unsigned long *key_matrix) {
    int i = 0;
    int j = 0;
    for(;j<4;j++) {
        for(;i<32;i++) {
            key_matrix[j] &= (~(1 << i));
        }
        i = 0;
    }
}

void use_matrix_color(Widget_t *w, int c) {
    double ci = ((c+1)/100.0)*12.0;
    if (c<4)
        cairo_set_source_rgba(w->crb, ci, 0.2, 0.4, 1.00);
    else if (c<8)
        cairo_set_source_rgba(w->crb, 0.6, 0.2+ci-0.48, 0.4, 1.00);
    else if (c<12)
        cairo_set_source_rgba(w->crb, 0.6-(ci-0.96), 0.68-(ci-1.08), 0.4, 1.00);
    else
        cairo_set_source_rgba(w->crb, 0.12+(ci-1.56), 0.32, 0.4-(ci-1.44), 1.00);
}

void set_key_in_edo_matrix(unsigned long *key_matrix, int key, bool set) {
    unsigned long *use_matrix = &key_matrix[0];
    if (key > 253) return;

    if(key>222) {
        use_matrix = &key_matrix[7];
        key -=222;
    } else if(key>190) {
        use_matrix = &key_matrix[6];
        key -=190;
    } else if(key>158) {
        use_matrix = &key_matrix[5];
        key -=158;
    } else if(key>126) {
        use_matrix = &key_matrix[4];
        key -=126;
    } else if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    if (set) {
        (*use_matrix) |= (1 << key);
    }else {
        (*use_matrix) &= (~(1 << key));
    }
}

bool is_key_in_edo_matrix(unsigned long *key_matrix, int key) {
    unsigned long *use_matrix = &key_matrix[0];
    if (key > 253) return false;

    if(key>222) {
        use_matrix = &key_matrix[7];
        key -=222;
    } else if(key>190) {
        use_matrix = &key_matrix[6];
        key -=190;
    } else if(key>158) {
        use_matrix = &key_matrix[5];
        key -=158;
    } else if(key>126) {
        use_matrix = &key_matrix[4];
        key -=126;
    } else if(key>94) {
        use_matrix = &key_matrix[3];
        key -=94;
    } else if(key>62) {
        use_matrix = &key_matrix[2];
        key -=62;
    } else if(key>31) {
        use_matrix = &key_matrix[1];
        key -= 31;
    }
    bool ret = false;
    if((*use_matrix) & (1<<key)) {
        ret = true;
    }
    return ret;
}

static void setup_edo_matrix(int *matrix, int edo, unsigned long *edo_matrix) {
    int i = 0;
    for(;i<254;i++) {
        if (matrix[i % edo])
            set_key_in_edo_matrix(edo_matrix, i, true);
        else
            set_key_in_edo_matrix(edo_matrix, i, false);
    }
}

void mamba_set_edo(MambaKeyboard *keys, Widget_t *w, int edo) {
    keys->edo = edo;
    keys->octave = edo*2;
    if (edo == 0) { // just intonation
        keys->edo = 12;
        keys->octave = keys->edo*2;
        int matrix_set[12] = {1,0,1,0,1,1,0,1,0,1,0,1};
        setup_edo_matrix(matrix_set, keys->edo, keys->edo_matrix);
    } else if (edo == 5) {
        int matrix_set[5] = {1,1,1,1,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 6) {
        int matrix_set[6] = {1,1,1,1,1,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 7) {
        int matrix_set[7] = {1,1,1,1,1,1,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 8) {
        int matrix_set[8] = {1,1,1,1,1,1,1,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 9) {
        int matrix_set[9] = {1,1,1,0,1,1,1,1,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 10) {
        int matrix_set[10] = {1,0,1,1,0,1,1,0,1,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 11) {
        int matrix_set[11] = {1,0,1,0,1,1,0,1,1,0,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 12) {
        int matrix_set[12] = {1,0,1,0,1,1,0,1,0,1,0,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 13) {
        int matrix_set[13] = {1,0,1,0,1,1,0,1,0,1,1,0,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 14) {
        int matrix_set[14] = {1,0,1,0,1,0,1,0,1,0,1,0,1,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 15) {
        int matrix_set[15] = {1,0,1,0,1,0,1,0,1,0,1,0,0,1,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 16) {
        int matrix_set[16] = {1,0,1,0,1,0,0,1,0,1,0,1,0,1,0,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 17) {
        int matrix_set[17] = {1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 18) {
        int matrix_set[18] = {1,0,0,1,0,0,1,1,0,0,1,0,0,1,1,0,0,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 19) {
        int matrix_set[19] = {1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0,0,1,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 20) {
        int matrix_set[20] = {1,0,0,1,0,0,1,0,0,1,0,0,1,0,1,0,0,1,0,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 21) {
        int matrix_set[21] = {1,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0,1,0,0,1,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 22) {
        int matrix_set[22] = {1,0,0,1,0,0,1,0,0,1,0,0,1,0,1,0,1,0,0,1,0,0};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    } else if (edo == 23) {
        int matrix_set[23] = {1,0,0,1,0,0,1,0,0,1,1,0,0,1,0,0,1,0,0,1,0,0,1};
        setup_edo_matrix(matrix_set, edo, keys->edo_matrix);
    }
    
    expose_widget(w);
}

static void draw_double_key(Widget_t* w, cairo_pattern_t *pat, MambaKeyboard *keys, int *i, int *k, int height) {
    int ik = is_key_in_in_matrix(keys, (*k)+keys->octave);
    int key_size = keys->key_size/2;
    int j = 0;
    int i_ = (*i);
    cairo_set_line_width(w->crb, 1.0);
    for (;j<2;j++) {
        cairo_rectangle(w->crb,i_+keys->key_offset-2,0, key_size-1,height);
        if ( (*k)+keys->octave == keys->active_key || mamba_is_key_in_matrix(keys->key_matrix,(*k)+keys->octave)) {
            //use_base_color_scheme(w, ACTIVE_);
            use_matrix_color(w, keys->channel);
            cairo_set_line_width(w->crb, 1.0);
        } else if (ik > -1) {
            use_matrix_color(w, ik);
            cairo_set_line_width(w->crb, 2.0);
        } else if ( (*k)+keys->octave == keys->prelight_key) {
            use_base_color_scheme(w, PRELIGHT_);
            cairo_set_line_width(w->crb, 2.0);
        } else {
            use_bg_color_scheme(w, NORMAL_);
            cairo_set_line_width(w->crb, 1.0);
        }

        cairo_fill_preserve(w->crb);
        cairo_set_source(w->crb, pat);
        cairo_fill_preserve(w->crb);
        use_base_color_scheme(w, NORMAL_);
        cairo_stroke(w->crb);

        (*k)++;
        i_ += key_size+1;
    }
}

static void draw_keyboard(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    int width_t = attrs.width;
    int height_t = attrs.height;
    if (attrs.map_state != IsViewable) return;
    use_bg_color_scheme(w, NORMAL_);
    cairo_fill_preserve(w->crb);
    cairo_paint(w->crb);
    MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
 
    int i = 0;
    int k = 0;
    int ik = -1;
    int octave_ofset = keys->key_size/6;
    int octave = keys->octave;
    if (keys->key_size<24)
        cairo_set_font_size (w->crb, w->app->small_font);
    else
        cairo_set_font_size (w->crb, w->app->normal_font);

    for(;i<width_t;i++) {
        //fprintf(stderr, " set white k = %i, set = %i, space = %i edo_matrix %i\n", k+keys->octave, set, space, mamba_is_key_in_matrix(keys->edo_matrix[0],k+keys->octave));
        ik = is_key_in_in_matrix(keys, k+keys->octave);
        cairo_rectangle(w->crb,i,0,keys->key_size+1,height_t);
        if ( k+keys->octave == keys->active_key || mamba_is_key_in_matrix(keys->key_matrix,k+keys->octave)) {
            //use_base_color_scheme(w, ACTIVE_);
            use_matrix_color(w, keys->channel);
            cairo_set_line_width(w->crb, 1.0);
        } else if (ik > -1) {
            use_matrix_color(w, ik);
            cairo_set_line_width(w->crb, 2.0);
        } else if ( k+keys->octave == keys->prelight_key) {
            use_base_color_scheme(w, PRELIGHT_);
            cairo_set_line_width(w->crb, 2.0);
        } else {
            use_fg_color_scheme(w, NORMAL_);
            cairo_set_line_width(w->crb, 1.0);
        }

        cairo_fill_preserve(w->crb);
        use_base_color_scheme(w, NORMAL_);
        cairo_stroke(w->crb);

        if(k+octave == 0) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C-1");
        } else if(k+octave == keys->edo) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C0");
        } else if(k+octave == keys->edo *2) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C1");
        } else if(k+octave == keys->edo *3) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C2");
        } else if(k+octave == keys->edo *4) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C3");
        } else if(k+octave == keys->edo *5) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C4");
        } else if(k+octave == keys->edo *6) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C5");
        } else if(k+octave == keys->edo *7) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C6");
        } else if(k+octave == keys->edo *8) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C7");
        } else if(k+octave == keys->edo *9) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C8");
        } else if(k+octave == keys->edo *10) {
            cairo_move_to (w->crb, i+octave_ofset, height_t*0.9);
            use_bg_color_scheme(w, NORMAL_);
            cairo_show_text(w->crb, "C9");
        }

        if (!is_key_in_edo_matrix(keys->edo_matrix,k+1+keys->octave)) {
            k++;
            if (!is_key_in_edo_matrix(keys->edo_matrix,k+1+keys->octave))
                k++;
        }

        if (k>127) break;
        i+=keys->key_size;
        k++;
    }

    k = 1;
    ik = -1;
    i = 0;
    cairo_pattern_t *pat = cairo_pattern_create_linear (0, 0, 0, height_t*0.59);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.85, 0.85, 0.85, 0.4);
    cairo_pattern_add_color_stop_rgba(pat, 0.2, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, 0.0);

    for(;i<width_t;i++) {

        if ((!is_key_in_edo_matrix(keys->edo_matrix,k+keys->octave))) {
            if (!is_key_in_edo_matrix(keys->edo_matrix,k+1+keys->octave)) {
                draw_double_key(w, pat, keys, &i, &k, height_t*0.59);
            } else {
                ik = is_key_in_in_matrix(keys, k+keys->octave);
                cairo_set_line_width(w->crb, 1.0);
                cairo_rectangle(w->crb,i+keys->key_offset,0,keys->key_size-4,height_t*0.59);
                if ( k+keys->octave == keys->active_key || mamba_is_key_in_matrix(keys->key_matrix,k+keys->octave)) {
                    //use_base_color_scheme(w, ACTIVE_);
                    use_matrix_color(w, keys->channel);
                    cairo_set_line_width(w->crb, 1.0);
                } else if (ik > -1) {
                    use_matrix_color(w, ik);
                    cairo_set_line_width(w->crb, 2.0);
                } else if ( k+keys->octave == keys->prelight_key) {
                    use_base_color_scheme(w, PRELIGHT_);
                    cairo_set_line_width(w->crb, 2.0);
                } else {
                    use_bg_color_scheme(w, NORMAL_);
                    cairo_set_line_width(w->crb, 1.0);
                }

                cairo_fill_preserve(w->crb);
                cairo_set_source(w->crb, pat);
                cairo_fill_preserve(w->crb);
                use_base_color_scheme(w, NORMAL_);
                cairo_stroke(w->crb);

                k++;
            }
        }

        i+=keys->key_size;
        k++;
        if(k>127)break;
    }
    cairo_pattern_destroy (pat);
    pat = cairo_pattern_create_linear (0, 0, 0, height_t);
    cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, 0.4);
    cairo_pattern_add_color_stop_rgba(pat, 0.8, 0.0, 0.0, 0.0, 0.0);
    cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.0, 0.0, 0.0, 0.0);
    cairo_set_source(w->crb, pat);
    cairo_rectangle(w->crb,0,0,width_t,height_t);
    cairo_fill(w->crb);
    cairo_pattern_destroy (pat);
}

static void check_double_key(Widget_t *p, MambaKeyboard *keys, XMotionEvent *xmotion, bool *catchit, int *i, int *set_key, int height) {
    int key_size = keys->key_size/2;
    int i_ = (*i);
    int j = 0;
    for (;j<2;j++) {
        if(xmotion->x > i_+keys->key_offset-2 && xmotion->x < i_+key_size+keys->key_offset) {
            keys->prelight_key = (*set_key)+keys->octave;
            if(xmotion->state & Button1Mask) {
                if (keys->active_key != keys->prelight_key) {
                    keys->send_key = keys->active_key;
                    if (keys->send_key>=0 && keys->send_key<128) {
                        if (mamba_is_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key)) 
                            mamba_set_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key,false);
                        keys->mk_send_note(p, &keys->send_key,false);
                    }
                    keys->active_key = keys->prelight_key;
                    keys->send_key = keys->active_key;
                    keys->last_active_key = keys->active_key;
                    if (keys->send_key>=0 && keys->send_key<128)
                        keys->mk_send_note(p, &keys->send_key,true);
                }
            }
            (*catchit) = true;
            if (keys->prelight_key != keys->new_prelight_key ||
                    keys->active_key != keys->new_active_key ) {
                //expose_widget(w);
                keys->new_prelight_key = keys->prelight_key;
                keys->new_active_key = keys->active_key;
            }
            break;
        }
            
        (*set_key)++;
        i_ += key_size-2;
    }
}

static void keyboard_motion(void *w_, void* xmotion_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t *)w->parent;
    MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
    XMotionEvent *xmotion = (XMotionEvent*)xmotion_;
    XWindowAttributes attrs;
    XGetWindowAttributes(w->app->dpy, (Window)w->widget, &attrs);
    if (attrs.map_state != IsViewable) return;
    int width = attrs.width;
    int height = attrs.height;

    bool catchit = false;

    if(xmotion->y < height*0.59) {
        keys->in_motion = 1;
        int set_key = 1;
        int i = 0;
        for(;i<width;i++) {
            if ((!is_key_in_edo_matrix(keys->edo_matrix,set_key+keys->octave))) {
                if ((!is_key_in_edo_matrix(keys->edo_matrix,set_key+1+keys->octave)) ||
                        (!mamba_is_key_in_matrix(keys->edo_matrix,set_key-1+keys->octave) &&
                        !mamba_is_key_in_matrix(keys->edo_matrix,set_key+keys->octave))) {
                    check_double_key(p, keys, xmotion, &catchit, &i, &set_key, height*0.59);
                } else {
                    if(xmotion->x > i+keys->key_offset && xmotion->x < i+keys->key_size+keys->key_offset-3) {
                        keys->prelight_key = set_key+keys->octave;
                        if(xmotion->state & Button1Mask) {
                            if (keys->active_key != keys->prelight_key) {
                                keys->send_key = keys->active_key;
                                if (keys->send_key>=0 && keys->send_key<128) {
                                    if (mamba_is_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key)) 
                                        mamba_set_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key,false);
                                    keys->mk_send_note(p, &keys->send_key,false);
                                }
                                keys->active_key = keys->prelight_key;
                                keys->send_key = keys->active_key;
                                keys->last_active_key = keys->active_key;
                                if (keys->send_key>=0 && keys->send_key<128)
                                    keys->mk_send_note(p, &keys->send_key,true);
                            }
                        }
                        catchit = true;
                        if (keys->prelight_key != keys->new_prelight_key ||
                                keys->active_key != keys->new_active_key ) {
                            //expose_widget(w);
                            keys->new_prelight_key = keys->prelight_key;
                            keys->new_active_key = keys->active_key;
                        }
                        break;
                    }
                    set_key++;
                }
            }
            i+=keys->key_size;
            set_key++;
        }        
    }

    if (!catchit) {
        int i = 0;
        int k = 0;

        for(;i<width;i++) {
           // if ((mamba_is_key_in_matrix(keys->edo_matrix,k+keys->octave))) {
                if(xmotion->x > i && xmotion->x < i+keys->key_size) {
                    keys->prelight_key = k+keys->octave;
                    if(xmotion->state & Button1Mask) {
                        if (keys->active_key != keys->prelight_key) {
                            keys->send_key = keys->active_key;
                            if (keys->send_key>=0 && keys->send_key<128) {
                                if (mamba_is_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key)) 
                                    mamba_set_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key,false);
                                keys->mk_send_note(p, &keys->send_key,false);
                            }
                            keys->active_key = keys->prelight_key;
                            keys->send_key = keys->active_key;
                            keys->last_active_key = keys->active_key;
                            if (keys->send_key>=0 && keys->send_key<128)
                                keys->mk_send_note(p, &keys->send_key,true);
                        }
                    }
                    if (keys->prelight_key != keys->new_prelight_key ||
                            keys->active_key != keys->new_active_key ) {
                        //expose_widget(w);
                        keys->new_prelight_key = keys->prelight_key;
                        keys->new_active_key = keys->active_key;
                    }
                    break;
                }
                if (!(is_key_in_edo_matrix(keys->edo_matrix,k+1+keys->octave))) {
                    k++;
                    if (!(is_key_in_edo_matrix(keys->edo_matrix,k+1+keys->octave)))
                    k++;
                }
           // }
            i+=keys->key_size;
            k++;
        }
    }
}

static void get_outkey(MambaKeyboard *keys, KeySym sym, float *outkey) {
    switch(keys->layout) {
        case(0):mamba_keysym_qwertz_to_midi_key(sym, outkey);
        break;
        case(1):mamba_keysym_qwerty_to_midi_key(sym, outkey);
        break;
        case(2):mamba_keysym_azerty_fr_to_midi_key(sym, outkey);
        break;
        case(3):mamba_keysym_azerty_be_to_midi_key(sym, outkey);
        break;
        case(4):mamba_custom_to_midi_key(keys->custom_keys, sym, outkey);
        break;
        default:mamba_keysym_qwertz_to_midi_key(sym, outkey);
        break;
    }
}

static void key_press(void *w_, void *key_, void *user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t *)w->parent;
    if (!w) return;
    MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
    XKeyEvent *key = (XKeyEvent*)key_;
    if (!key) return;
    if (key->state & ControlMask) {
        p->func.key_press_callback(p, key_, user_data);
    } else {
        float outkey = 0.0;
        KeySym sym = XLookupKeysym (key, 0);
        get_outkey(keys, sym, &outkey);

        if ((int)outkey && !mamba_is_key_in_matrix(keys->key_matrix, (int)outkey+keys->octave)) {
            mamba_set_key_in_matrix(keys->key_matrix,(int)outkey+keys->octave,true);
            keys->send_key = (int)outkey+keys->octave;
            if (keys->send_key>=0 && keys->send_key<128)
                keys->mk_send_note(p, &keys->send_key,true);
            //expose_widget(w);
        } 
        if (sym == XK_space) {
            mamba_clear_key_matrix(&keys->key_matrix[0]);
            int i = 0;
            for (;i<16;i++) mamba_clear_key_matrix(&keys->in_key_matrix[i][0]);
            keys->mk_send_all_sound_off(p, NULL);
            //expose_widget(w);
        }
    }
}

static void key_release(void *w_, void *key_, void *user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t *)w->parent;
    if (!w) return;
    MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
    XKeyEvent *key = (XKeyEvent*)key_;
    if (!key) return;
    float outkey = 0.0;
    KeySym sym = XLookupKeysym (key, 0);
    get_outkey(keys, sym, &outkey);
    if ((int)outkey && mamba_is_key_in_matrix(keys->key_matrix, (int)outkey+keys->octave)) {
        mamba_set_key_in_matrix(keys->key_matrix,(int)outkey+keys->octave,false);
        keys->send_key = (int)outkey+keys->octave;
        if (keys->send_key>=0 && keys->send_key<128)
            keys->mk_send_note(p,&keys->send_key,false);
        //expose_widget(w);
    }
}

static void leave_keyboard(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
    keys->prelight_key = -1;
    keys->active_key = -1;
    keys->in_motion = 0;
    //expose_widget(w);
}

static void button_pressed_keyboard(void *w_, void* button_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t *)w->parent;
    if (w->flags & HAS_POINTER) {
        MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
        XButtonEvent *xbutton = (XButtonEvent*)button_;
        if(xbutton->button == Button1) {
            keys->active_key = keys->prelight_key;
            keys->send_key = keys->active_key;
            keys->last_active_key = keys->active_key;
            if (keys->send_key>=0 && keys->send_key<128)
                keys->mk_send_note(p,&keys->send_key,true);
            //expose_widget(w);
        } else if (xbutton->button == Button3) {
            keys->send_key = keys->prelight_key;
            if (keys->send_key>=0 && keys->send_key<128) {
                if (mamba_is_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key)) {
                    mamba_set_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key,false);
                    keys->mk_send_note(p,&keys->send_key,false);
                } else {
                    mamba_set_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key,true);
                    keys->mk_send_note(p,&keys->send_key,true);
                }
            }
        }
    }
}

static void button_released_keyboard(void *w_, void* button_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    Widget_t *p = (Widget_t *)w->parent;
    MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
    XButtonEvent *xbutton = (XButtonEvent*)button_;
    if (w->flags & HAS_POINTER) {
        if(xbutton->button == Button1) {
            keys->send_key = keys->active_key;
            if (keys->send_key>=0 && keys->send_key<128) {
                keys->mk_send_note(p,&keys->send_key,false);
                if (mamba_is_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key)) 
                    mamba_set_key_in_matrix(keys->in_key_matrix[keys->channel], keys->send_key,false);
            }
            keys->active_key = -1;
            //expose_widget(w);
        }
    } else {
        if(xbutton->button == Button1) {
            keys->send_key = keys->last_active_key;
            if (keys->send_key>=0 && keys->send_key<128) {
                keys->mk_send_note(p,&keys->send_key,false);
            }
            keys->last_active_key = -1;
        }
    }
}

static void keyboard_mem_free(void *w_, void* user_data) {
    Widget_t *w = (Widget_t*)w_;
    MambaKeyboard *keys = (MambaKeyboard*)w->parent_struct;
    free(keys);
}

bool mamba_need_redraw(MambaKeyboard *keys) {
    bool have = false;
    int i = 0;
    for(;i<16;i++) {
        if (mamba_have_key_in_matrix(keys->in_key_matrix[i])) {
            have = true;
            break;
        }
    }

    return (keys->active_key > 0 ? 1 : 0) |
    (keys->prelight_key  > 0 ? 1 : 0)  |
    mamba_have_key_in_matrix(keys->key_matrix) |
    have;
}

const char* dir_name(const char* path) {
    char* p = NULL;
    if (!asprintf(&p, "%s", path)) return NULL;
    static char buffer[PATH_MAX];
    char* n = strrchr(p, '/');
    if (n) {
        strncpy(buffer, p, n-p);
    }
    free(p);
    return buffer;
}

void mamba_read_keymap(MambaKeyboard *keys, const char* keymapfile, long custom_keys[128][2]) {
    if( access(keymapfile, F_OK ) != -1 ) {
        FILE *fp;
        if((fp=fopen(keymapfile, "rb"))==NULL) {
            fprintf(stderr,"Cannot open %s.\n", keymapfile);
            return;
        }

        if(fread(custom_keys, sizeof(long), 128*2, fp) != 128*2) {
            if(feof(fp))
            fprintf(stderr,"Premature end of file.");
            else
            fprintf(stderr,"File read error.");
        }
        fclose(fp);
    }
}


void mamba_add_midi_keyboard(Widget_t *parent, const char * label,
                            int x, int y, int width, int height) {
    XSelectInput(parent->app->dpy, parent->widget,StructureNotifyMask|ExposureMask|KeyPressMask 
                    |EnterWindowMask|LeaveWindowMask|ButtonReleaseMask|KeyReleaseMask
                    |ButtonPressMask|Button1MotionMask|PointerMotionMask);
    mamba_add_keyboard(parent, label);
}

Widget_t *mamba_open_midi_keyboard(Widget_t *w, const char * label) {
    Widget_t *wid = create_window(w->app, DefaultRootWindow(w->app->dpy), 0, 0, 700, 200);
    XSelectInput(wid->app->dpy, wid->widget,StructureNotifyMask|ExposureMask|KeyPressMask 
                    |EnterWindowMask|LeaveWindowMask|ButtonReleaseMask|KeyReleaseMask
                    |ButtonPressMask|Button1MotionMask|PointerMotionMask);
    mamba_add_keyboard(wid, label);
    wid->parent = w;
    return wid;
}

void mamba_add_keyboard(Widget_t *wid, const char * label) {
    MambaKeyboard *keys = (MambaKeyboard*)malloc(sizeof(MambaKeyboard));
    wid->parent_struct = keys;
    wid->flags |= HAS_MEM | NO_AUTOREPEAT;
    keys->prelight_key = -1;
    keys->active_key = -1;
    keys->new_prelight_key = -1;
    keys->new_active_key = -1;
    keys->last_active_key = -1;
    keys->in_motion = 0;
    keys->send_key = -1;
    keys->octave = 12*2;
    keys->layout = 0;
    keys->channel = 0;
    keys->key_size = 24;
    keys->key_offset = 15;
    keys->edo = 12;
    memset(keys->custom_keys, 0, 128*2*sizeof keys->custom_keys[0][0]);
    int j = 0;
    for(;j<4;j++) {
        keys->key_matrix[j] = 0;
    }
    int i = 0;
    for(;i<16;i++) {
        j = 0;
        for(;j<4;j++) {
            keys->in_key_matrix[i][j] = 0;
        }
    }
    j = 0;
    for(;j<8;j++) {
        keys->edo_matrix[j] = 0;
    }
    mamba_set_edo(keys, wid, 12);
    mamba_read_keymap(keys, label, keys->custom_keys);

    wid->func.expose_callback = draw_keyboard;
    wid->func.motion_callback = keyboard_motion;
    wid->func.leave_callback = leave_keyboard;
    wid->func.button_press_callback = button_pressed_keyboard;
    wid->func.button_release_callback = button_released_keyboard;
    wid->func.double_click_callback = button_released_keyboard;
    wid->func.key_press_callback = key_press;
    wid->func.key_release_callback = key_release;
    wid->func.mem_free_callback = keyboard_mem_free;

}
