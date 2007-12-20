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
 *      (separate modules can be included from the allegro/ directory)
 *
 *      By Shawn Hargreaves.
 *
 *      Vincent Penquerc'h split the original allegro.h into separate headers.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_H
#define ALLEGRO_H


#include "allegro5/base.h"

#include "allegro5/system.h"
#include "allegro5/system_new.h"
#include "allegro5/memory.h"
#include "allegro5/debug.h"

#include "allegro5/unicode.h"

#include "allegro5/altime.h"

#include "allegro5/events.h"

#include "allegro5/mouse.h"
#include "allegro5/timer.h"
#include "allegro5/keyboard.h"
#include "allegro5/joystick.h"

#include "allegro5/palette.h"
#include "allegro5/gfx.h"
#include "allegro5/display_new.h"
#include "allegro5/bitmap_new.h"
#include "allegro5/color.h"
#include "allegro5/draw.h"
#include "allegro5/rle.h"
#include "allegro5/compiled.h"
#include "allegro5/text.h"
#include "allegro5/font.h"

#include "allegro5/fli.h"
#include "allegro5/config.h"
#include "allegro5/gui.h"

#include "allegro5/sound.h"

#include "allegro5/file.h"
#include "allegro5/lzss.h"
#include "allegro5/datafile.h"

#include "allegro5/fixed.h"
#include "allegro5/fmaths.h"
#include "allegro5/matrix.h"
#include "allegro5/quat.h"

#include "allegro5/3d.h"
#include "allegro5/3dmaths.h"


#ifndef ALLEGRO_NO_COMPATIBILITY
   #include "allegro5/alcompat.h"
#endif

#ifndef ALLEGRO_NO_FIX_CLASS
   #ifdef __cplusplus
      #include "allegro5/fix.h"
   #endif
#endif


#ifdef ALLEGRO_EXTRA_HEADER
   #include ALLEGRO_EXTRA_HEADER
#endif


#endif          /* ifndef ALLEGRO_H */


