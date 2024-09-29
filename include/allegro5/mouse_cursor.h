#ifndef __al_included_allegro5_mouse_cursor_h
#define __al_included_allegro5_mouse_cursor_h

#include "allegro5/base.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct A5O_MOUSE_CURSOR A5O_MOUSE_CURSOR;

typedef enum A5O_SYSTEM_MOUSE_CURSOR
{
   A5O_SYSTEM_MOUSE_CURSOR_NONE        =  0,
   A5O_SYSTEM_MOUSE_CURSOR_DEFAULT     =  1,
   A5O_SYSTEM_MOUSE_CURSOR_ARROW       =  2,
   A5O_SYSTEM_MOUSE_CURSOR_BUSY        =  3,
   A5O_SYSTEM_MOUSE_CURSOR_QUESTION    =  4,
   A5O_SYSTEM_MOUSE_CURSOR_EDIT        =  5,
   A5O_SYSTEM_MOUSE_CURSOR_MOVE        =  6,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_N    =  7,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_W    =  8,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_S    =  9,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_E    = 10,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_NW   = 11,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_SW   = 12,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_SE   = 13,
   A5O_SYSTEM_MOUSE_CURSOR_RESIZE_NE   = 14,
   A5O_SYSTEM_MOUSE_CURSOR_PROGRESS    = 15,
   A5O_SYSTEM_MOUSE_CURSOR_PRECISION   = 16,
   A5O_SYSTEM_MOUSE_CURSOR_LINK        = 17,
   A5O_SYSTEM_MOUSE_CURSOR_ALT_SELECT  = 18,
   A5O_SYSTEM_MOUSE_CURSOR_UNAVAILABLE = 19,
   A5O_NUM_SYSTEM_MOUSE_CURSORS
} A5O_SYSTEM_MOUSE_CURSOR;

struct A5O_BITMAP;
struct A5O_DISPLAY;


AL_FUNC(A5O_MOUSE_CURSOR *, al_create_mouse_cursor, (
        struct A5O_BITMAP *sprite, int xfocus, int yfocus));
AL_FUNC(void, al_destroy_mouse_cursor, (A5O_MOUSE_CURSOR *));
AL_FUNC(bool, al_set_mouse_cursor, (struct A5O_DISPLAY *display,
                                    A5O_MOUSE_CURSOR *cursor));
AL_FUNC(bool, al_set_system_mouse_cursor, (struct A5O_DISPLAY *display,
                                           A5O_SYSTEM_MOUSE_CURSOR cursor_id));
AL_FUNC(bool, al_show_mouse_cursor, (struct A5O_DISPLAY *display));
AL_FUNC(bool, al_hide_mouse_cursor, (struct A5O_DISPLAY *display));


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
