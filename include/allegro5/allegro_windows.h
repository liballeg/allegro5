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
 *      Header file for Windows specific API.
 *
 *      By Trent Gamblin.
 *
 */

#ifndef ALLEGRO_WINDOWS_H
#define ALLEGRO_WINDOWS_H

#ifndef SCAN_DEPEND
#include <windows.h>
#endif

#ifdef __cplusplus
   extern "C" {
#endif

/*
 *  Public Windows-related API
 */

AL_FUNC(HWND,               al_get_win_window_handle,           (ALLEGRO_DISPLAY *));

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
