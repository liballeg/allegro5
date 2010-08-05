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
*      main() function replacement for MacOS X.
*
*      By Angelo Mottola.
*
*      See readme.txt for copyright information.
*/

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif
#undef main

extern int _al_mangled_main(int, char **);

/* main:
*  Replacement for main function.
*/
int main(int argc, char *argv[])
{
   _al_osx_run_main(argc, argv, _al_mangled_main);
   return 0;
}

/* Local variables:       */
/* c-basic-offset: 3      */
/* indent-tabs-mode: nil  */
/* c-file-style: "linux" */
/* End:                   */
