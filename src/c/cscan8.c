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
 *      256 color polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifdef ALLEGRO_COLOR8

#include "allegro/aintern.h"
#include "cdefs8.h"
#include "cscan.h"

#endif

