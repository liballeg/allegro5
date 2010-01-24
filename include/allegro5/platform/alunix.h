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
 *      Unix-specific header defines.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_UNIX
   #error bad include
#endif


/* magic to capture name of executable file */
extern int    __crt0_argc;
extern char **__crt0_argv;

#ifdef ALLEGRO_WITH_MAGIC_MAIN


   #ifndef ALLEGRO_NO_MAGIC_MAIN
      #define ALLEGRO_MAGIC_MAIN
      #define main _al_mangled_main
      #ifdef __cplusplus
         extern "C" int _al_mangled_main(int, char **);
      #endif
   #endif

#endif

