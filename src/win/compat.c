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
 *      Contains dummy symbols for maintaining DLL compatibility.
 *      Definitions go into include/allegro/compat.h
 *
 *      By Henrik Stokseth.
 */

#include "allegro.h"
#include "allegro/aintern.h"

static void dll_error();

static void dll_error()
{
   allegro_message("FATAL ERROR: You tried to use a function that has been removed from Allegro!\n");
   allegro_exit();
   exit(-666);
}

/* dummy functions */

void _load_config_text()
{ dll_error(); }
