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


#include "xwidgets.h"

#pragma once

#ifndef CUSTOMDRAWINGS_H
#define CUSTOMDRAWINGS_H
namespace midikeyboard {
class CustomDrawings {
public:

    static void draw_my_combobox_entrys(void *w_, void* user_data) noexcept{
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

    static void rounded_rectangle(cairo_t *cr,float x, float y, float width, float height) {
        cairo_new_path (cr);
        float r = height * 0.33334;
        cairo_new_path (cr);
        cairo_arc(cr, x+r, y+r, r, M_PI, 3*M_PI/2);
        cairo_arc(cr, x+width-1-r, y+r, r, 3*M_PI/2, 0);
        cairo_arc(cr, x+width-1-r, y+height-1-r, r, 0, M_PI/2);
        cairo_arc(cr, x+r, y+height-1-r, r, M_PI/2, M_PI);
        cairo_close_path(cr);
        cairo_close_path (cr);
    }

    static void pattern_out(Widget_t *w, int height) {
        cairo_pattern_t *pat = cairo_pattern_create_linear (0, -4, 0, height-4);
        cairo_pattern_add_color_stop_rgba (pat, 0,  0.3, 0.3, 0.3, 1.0);
        cairo_pattern_add_color_stop_rgba (pat, 0.25,  0.2, 0.2, 0.2, 1.0);
        cairo_pattern_add_color_stop_rgba (pat, 0.5,  0.15, 0.15, 0.15, 1.0);
        cairo_pattern_add_color_stop_rgba (pat, 0.75,  0.1, 0.1, 0.1, 1.0);
        cairo_pattern_add_color_stop_rgba (pat, 1,  0.05, 0.05, 0.05, 1.0);
        cairo_set_source(w->crb, pat);
        cairo_pattern_destroy (pat);
    }

    static void pattern_in(Widget_t *w, Color_state st, int height) {
        Colors *c = get_color_scheme(w,st);
        if (!c) return;
        cairo_pattern_t *pat = cairo_pattern_create_linear (2, 2, 2, height);
        cairo_pattern_add_color_stop_rgba(pat, 0.0, 0.0, 0.0, 0.0, 0.0);
        cairo_pattern_add_color_stop_rgba(pat, 0.5, c->light[0],  c->light[1], c->light[2],  c->light[3]);
        cairo_pattern_add_color_stop_rgba(pat, 1.0, 0.0, 0.0, 0.0, 0.0);
        cairo_set_source(w->crb, pat);
        cairo_pattern_destroy (pat);
    }

    static int remove_low_dash(char *str)  noexcept{

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

    static void knobShadowOutset(cairo_t* const cr, int width, int height, int x, int y) {
        cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x + width, y + height);
        cairo_pattern_add_color_stop_rgba
            (pat, 0, 0.33, 0.33, 0.33, 1);
        cairo_pattern_add_color_stop_rgba
            (pat, 0.45, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.4);
        cairo_pattern_add_color_stop_rgba
            (pat, 0.65, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.4);
        cairo_pattern_add_color_stop_rgba 
            (pat, 1, 0.05, 0.05, 0.05, 1);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_NONE);
        cairo_set_source(cr, pat);
        cairo_fill_preserve (cr);
        cairo_pattern_destroy (pat);
    }

    static void knobShadowInset(cairo_t* const cr, int width, int height, int x, int y) {
        cairo_pattern_t* pat = cairo_pattern_create_linear (x, y, x + width, y + height);
        cairo_pattern_add_color_stop_rgba
            (pat, 1, 0.33, 0.33, 0.33, 1);
        cairo_pattern_add_color_stop_rgba
            (pat, 0.65, 0.33 * 0.6, 0.33 * 0.6, 0.33 * 0.6, 0.4);
        cairo_pattern_add_color_stop_rgba
            (pat, 0.55, 0.05 * 2.0, 0.05 * 2.0, 0.05 * 2.0, 0.4);
        cairo_pattern_add_color_stop_rgba
            (pat, 0, 0.05, 0.05, 0.05, 1);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_NONE);
        cairo_set_source(cr, pat);
        cairo_fill (cr);
        cairo_pattern_destroy (pat);
    }

    static void roundrec(cairo_t *cr, double x, double y, double width, double height, double r) {
        cairo_arc(cr, x+r, y+r, r, M_PI, 3*M_PI/2);
        cairo_arc(cr, x+width-r, y+r, r, 3*M_PI/2, 0);
        cairo_arc(cr, x+width-r, y+height-r, r, 0, M_PI/2);
        cairo_arc(cr, x+r, y+height-r, r, M_PI/2, M_PI);
        cairo_close_path(cr);
    }

    static void switchLight(cairo_t *cr, int x, int y, int h) {
        cairo_pattern_t *pat = cairo_pattern_create_linear (x, y, x, y + h);
        cairo_pattern_add_color_stop_rgba
            (pat, 1, 0.6, 0.6, 0.6, 1.0 * 0.8);
        cairo_pattern_add_color_stop_rgba
            (pat, 0.5, 0.6, 0.6, 0.6, 1.0 * 0.4);
        cairo_pattern_add_color_stop_rgba
            (pat, 0, 0.6, 0.6, 0.6, 1.0 * 0.2);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_NONE);
        cairo_set_source(cr, pat);
        cairo_fill_preserve (cr);
        cairo_pattern_destroy (pat);

    }

    static void draw_my_switch(void *w_, void* user_data) {
        Widget_t *wid = (Widget_t*)w_;    
        const int w = wid->width - 4;
        const int h = wid->height * 0.75;
        const int state = (int)adj_get_state(wid->adj);

        const int centerW = w * 0.5;
        const int centerH = state ? centerW : h - centerW ;
        const int offset = w * 0.2;

        cairo_push_group (wid->crb);
        
        roundrec(wid->crb, 2, 1, w-2, h-2, centerW);
        knobShadowInset(wid->crb, w  , h, 0, 0);
        cairo_stroke_preserve (wid->crb);

        cairo_new_path(wid->crb);
        roundrec(wid->crb, offset, offset, w - (offset * 2), h - (offset * 2), centerW-offset);
        cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
        cairo_fill_preserve(wid->crb);
        if (state) {
            switchLight(wid->crb, offset, offset, h - (offset * 2));
        }
        cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
        cairo_set_line_width(wid->crb,1);
        cairo_stroke_preserve (wid->crb);

        cairo_new_path(wid->crb);
        cairo_arc(wid->crb,centerW, centerH, w/2.8, 0, 2 * M_PI );
        use_bg_color_scheme(wid, PRELIGHT_);
        cairo_fill_preserve(wid->crb);
        knobShadowOutset(wid->crb, w * 0.5 , h, 0, centerH - centerW);
        cairo_set_source_rgba(wid->crb, 0.05, 0.05, 0.05, 1);
        cairo_set_line_width(wid->crb,1);
        cairo_stroke_preserve (wid->crb);

        cairo_new_path(wid->crb);
        cairo_arc(wid->crb,centerW, centerH, w/3.6, 0, 2 * M_PI );
        if(wid->state==1) use_bg_color_scheme(wid, PRELIGHT_);
        else use_bg_color_scheme(wid, NORMAL_);
        cairo_fill_preserve(wid->crb);
        knobShadowInset(wid->crb, w * 0.5 , h, 0, centerH - centerW);
        cairo_stroke (wid->crb);

        /** show label below the switch **/
        cairo_text_extents_t extents;
        if(wid->state==1) use_text_color_scheme(wid, PRELIGHT_);
        else use_text_color_scheme(wid, NORMAL_);
        cairo_set_font_size (wid->crb, (wid->app->normal_font*0.8)/wid->scale.ascale);
        cairo_text_extents(wid->crb,wid->label , &extents);
        cairo_move_to (wid->crb, (w*0.5)-(extents.width/2) + 2, h*1.29 -(extents.height*0.4));
        cairo_show_text(wid->crb, wid->label);
        cairo_new_path (wid->crb);

        cairo_pop_group_to_source (wid->crb);
        cairo_paint (wid->crb);
    }

    static void draw_image_button(Widget_t *w, int width_t, int height_t, float offset) {
        int width, height;
        os_get_surface_size(w->image, &width, &height);
        double half_width = (width/height >=2) ? width*0.5 : width;
        double x = (double)width_t/(double)(half_width);
        double y = (double)height_t/(double)height;
        double x1 = (double)height/(double)height_t;
        double y1 = (double)(half_width)/(double)width_t;
        double off_set = offset*x1;
        double buttonstate = adj_get_state(w->adj);
        int findex = (int)(((width/height)-1) * buttonstate) * (width/height >=2);
        cairo_scale(w->crb, x,y);
        cairo_set_source_surface (w->crb, w->image, -height*findex+off_set, off_set);
        cairo_rectangle(w->crb,0, 0, height, height);
        cairo_fill(w->crb);
        cairo_scale(w->crb, x1,y1);
    }

    static void draw_button(void *w_, void* user_data) noexcept{
        Widget_t *w = (Widget_t*)w_;
        if (!w) return;
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
        if (w->image) {
            draw_image_button(w, width, height, offset);
        } else {

            rounded_rectangle(w->crb,2.0, 2.0, width, height);

            if(w->state==0) {
                cairo_set_line_width(w->crb, 1.0);
                pattern_out(w, height);
                cairo_fill_preserve(w->crb);
                use_frame_color_scheme(w, PRELIGHT_);
            } else if(w->state==1) {
                pattern_out(w, height);
                cairo_fill_preserve(w->crb);
                cairo_set_line_width(w->crb, 1.5);
                use_frame_color_scheme(w, PRELIGHT_);
            } else if(w->state==2) {
                pattern_in(w, SELECTED_, height);
                cairo_fill_preserve(w->crb);
                cairo_set_line_width(w->crb, 1.0);
                use_frame_color_scheme(w, PRELIGHT_);
            } else if(w->state==3) {
                pattern_in(w, ACTIVE_, height);
                cairo_fill_preserve(w->crb);
                cairo_set_line_width(w->crb, 1.0);
                use_frame_color_scheme(w, PRELIGHT_);
            }
            cairo_stroke(w->crb);

            if(w->state==2) {
                rounded_rectangle(w->crb,4.0, 4.0, width, height);
                cairo_stroke(w->crb);
                rounded_rectangle(w->crb,3.0, 3.0, width, height);
                cairo_stroke(w->crb);
            } else if (w->state==3) {
                rounded_rectangle(w->crb,3.0, 3.0, width, height);
                cairo_stroke(w->crb);
            }

            cairo_text_extents_t extents;
            use_text_color_scheme(w, get_color_state(w));
            cairo_set_font_size (w->crb, w->app->normal_font/w->scale.ascale);

            if (strstr(w->label, "_")) {
                cairo_text_extents(w->crb, "--", &extents);
                double underline = extents.width;
                strncpy(w->input_label,w->label, sizeof(w->input_label)-1);
                int pos = remove_low_dash(w->input_label);
                int len = strlen(w->input_label);
                cairo_text_extents(w->crb,w->input_label , &extents);
                int set_line = (extents.width/len) * pos;
                cairo_move_to (w->crb, (width-extents.width)*0.5 +offset, (height+extents.height)*0.5 +offset);
                cairo_show_text(w->crb, w->input_label);
                cairo_set_line_width(w->crb, 1.0);
                cairo_move_to (w->crb, (width-extents.width)*0.5 +offset + set_line, (height+extents.height)*0.55 +offset);
                cairo_line_to(w->crb,(width-extents.width)*0.5 +offset + set_line + underline, (height+extents.height)*0.55 +offset);
                cairo_stroke(w->crb);
            } else if (strstr(w->label, "-") || strstr(w->label, "+" )) {
                cairo_text_extents(w->crb,w->label , &extents);
                cairo_move_to (w->crb, (width-extents.width)*0.55 +offset, (height+extents.height)*0.65 +offset);
                cairo_show_text(w->crb, w->label);
                
            } else {
                cairo_text_extents(w->crb,w->label , &extents);
                cairo_move_to (w->crb, (width-extents.width)*0.5 +offset, (height+extents.height)*0.5 +offset);
                cairo_show_text(w->crb, w->label);
            }
        }
    }

    static void mk_draw_knob(void *w_, void* user_data) noexcept{
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

    static void draw_menubar(void *w_, void* user_data) noexcept{
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

    static void draw_topbox(void *w_, void* user_data) noexcept{
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

    static void draw_middlebox(void *w_, void* user_data) noexcept{
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

    static void draw_synth_ui(void *w_, void* user_data) noexcept{
        Widget_t *w = (Widget_t*)w_;
        set_pattern(w,&w->app->color_scheme->selected,&w->app->color_scheme->normal,BACKGROUND_);
        cairo_paint (w->crb);
    }

};

} // namespace midikeyboard

#endif //CUSTOMDRAWINGS_H

