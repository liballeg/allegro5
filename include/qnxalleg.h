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
 *      Header file for the QNX Allegro library port.
 *
 *      You dont need this file; it is used internally to prototype some library
 *      functions, macros and similar stuff.
 */

#ifndef __QNX__
#error Bad include!
#endif


#ifndef QNX_ALLEGRO_H
#define QNX_ALLEGRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "allegro.h"
#include "allegro/aintern.h"
#include "allegro/aintqnx.h"

#include <Ph.h>
#include <Pt.h>

#define EVENT_SIZE   sizeof(PhEvent_t) + 1000


/* Globals */
PtWidget_t       *ph_window;
PhEvent_t        *ph_event;


typedef void RETSIGTYPE;

#ifdef __cplusplus
}
#endif

#endif