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
 *      Configuration defines for use with RSXNT.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifdef ALLEGRO_SRC
   #error RSXNT can only use the DLL, not build it
#endif


/* describe this platform */
#define ALLEGRO_PLATFORM_STR  "RSXNT"
#define ALLEGRO_WINDOWS
#define ALLEGRO_I386
#define ALLEGRO_LITTLE_ENDIAN

#ifdef USE_CONSOLE
   #define ALLEGRO_CONSOLE_OK
   #define ALLEGRO_NO_MAGIC_MAIN
#endif


/* funky stuff to make global variables work with the DLL */
#if (__GNUC__ > 2) || ((__GNUC__ == 2) && (__GNUC_MINOR__ >= 424242))   /* todo: find out what version supports this and test it */

   #define AL_VAR(type, name)             extern type name __attribute__ ((dllimport))
   #define AL_ARRAY(type, name)           extern type name[] __attribute__ ((dllimport))
   #define AL_FUNC(type, name, args)      type name args __attribute__ ((dllimport))
   #define AL_FUNCPTR(type, name, args)   extern type (*name) args __attribute__ ((dllimport))

#else

   #define AL_VAR(type, name)             extern type name
   #define AL_ARRAY(type, name)           extern type *name
   #define AL_FUNCPTR(type, name, args)   extern type (*name) args

   #include "rsxdll.h"

#endif


/* workaround for broken RSXNT implementation of these */
#undef isalpha
#undef isdigit
#undef isalnum
#undef isspace

#define isalpha(c)   ((((c) >= 'a') && ((c) <= 'z')) || (((c) >= 'A') && ((c) <= 'Z')))
#define isdigit(c)   (((c) >= '0') && ((c) <= '9'))
#define isalnum(c)   (isalpha(c) || isdigit(c))
#define isspace(c)   uisspace(c)


/* arrange for other headers to be included later on */
#define ALLEGRO_EXTRA_HEADER     "allegro/alwin.h"
#define ALLEGRO_INTERNAL_HEADER  "allegro/aintwin.h"

