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
 *      Replacement for main to capture name of executable file.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro5/internal/alconfig.h"

#ifdef ALLEGRO_WITH_MAGIC_MAIN

#undef main


extern int    __crt0_argc;
extern char **__crt0_argv;
extern int _al_mangled_main(int, char **);



/* main:
 *  Replacement for main function (capture arguments and call real main).
 */
int main(int argc, char *argv[])
{
   __crt0_argc = argc;
   __crt0_argv = argv;
 	return _al_mangled_main(__crt0_argc, __crt0_argv);
}

#endif
