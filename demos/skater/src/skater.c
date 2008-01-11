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
 *      Demo game.
 *
 *      By Miran Amon, Nick Davies, Elias Pschernig, Thomas Harte,
 *      Jakub Wasilewski.
 *
 *      See readme.txt for copyright information.
 */

#include <allegro.h>
#include "../include/framewk.h"


int main(void)
{
   if (init_framework() != DEMO_OK) {
      return 1;
   }

   run_framework();
   shutdown_framework();

   return 0;
}

END_OF_MAIN()
