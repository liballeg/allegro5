/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Header file for X specific functionality.
 * 
 *      By Robert MacGregor.
 * 
 */

#ifndef __al_included_allegro5_allegro_x_h
#define __al_included_allegro5_allegro_x_h

#include <X11/Xlib.h>

#include "allegro5/base.h"
#include "allegro5/display.h"

#ifdef __cplusplus
   extern "C" {
#endif

/*
 *  Public X-related API
 */
AL_FUNC(XID, al_get_x_window_id, (A5O_DISPLAY *display));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_SRC)
AL_FUNC(bool, al_x_set_initial_icon, (A5O_BITMAP *bitmap));
#endif

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
