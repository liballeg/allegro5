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
 *      Main header file for the entire Allegro library.
 *      Separate modules can be included from the allegro/ directory.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_H
#define ALLEGRO_H

#ifdef __cplusplus
   extern "C" {
#endif

#include "allegro/base.h"

#include "allegro/system.h"
#include "allegro/debug.h"

#include "allegro/unicode.h"

#include "allegro/mouse.h"
#include "allegro/timer.h"
#include "allegro/keyboard.h"
#include "allegro/joystick.h"

#include "allegro/palette.h"
#include "allegro/gfx.h"
#include "allegro/color.h"
#include "allegro/draw.h"
#include "allegro/rle.h"
#include "allegro/compiled.h"
#include "allegro/text.h"

#include "allegro/fli.h"
#include "allegro/config.h"
#include "allegro/gui.h"

#include "allegro/sound.h"

#include "allegro/file.h"
#include "allegro/datafile.h"

#include "allegro/fixed.h"
#include "allegro/fmaths.h"
#include "allegro/matrix.h"
#include "allegro/quat.h"

#include "allegro/3d.h"
#include "allegro/3dmaths.h"

#include "allegro/alcompat.h"


#ifdef ALLEGRO_EXTRA_HEADER
   #include ALLEGRO_EXTRA_HEADER
#endif


#ifdef __cplusplus
   }
#endif

#ifdef __cplusplus
#include "allegro/fix.h"
#endif

#endif          /* ifndef ALLEGRO_H */


