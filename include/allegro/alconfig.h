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
 *      Configuration defines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


/* which color depths to include? */
#define ALLEGRO_COLOR8
#define ALLEGRO_COLOR16
#define ALLEGRO_COLOR24
#define ALLEGRO_COLOR32


/* include platform-specific stuff */
#if defined SCAN_EXPORT
   #include "alscanex.h"
#elif defined __RSXNT__
   #include "alrsxnt.h"
#elif defined __MINGW32__
   #include "almngw32.h"
#elif defined __BORLANDC__
   #include "albcc32.h"
#elif defined _MSC_VER
   #include "almsvc.h"
#elif defined __WATCOMC__
   #include "alwatcom.h"
#elif defined __BEOS__
   #include "albecfg.h"
#elif defined __MRC__
   #include "almaccfg.h"
#elif defined DJGPP
   #include "aldjgpp.h"
#elif defined __unix__
   #include "alucfg.h"
#else
   #error platform not supported
#endif


/* special definitions for the GCC compiler */
#ifdef __GNUC__
   #define ALLEGRO_GCC

   #ifndef AL_INLINE
      #define AL_INLINE(type, name, args, code)    extern inline type name args code
   #endif

   #define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))

   #define INLINE          inline

   #ifndef ZERO_SIZE
      #define ZERO_SIZE    0
   #endif

   #define LONG_LONG       long long

   #ifdef __i386__
      #define ALLEGRO_I386
   #endif

   #ifndef AL_CONST
      #define AL_CONST     const
   #endif

#endif


/* use constructor functions, if supported */
#ifdef ALLEGRO_USE_CONSTRUCTOR
   #define CONSTRUCTOR_FUNCTION(func)              func __attribute__ ((constructor))
#endif


/* the rest of this file fills in some default definitions of language
 * features and helper functions, which are conditionalised so they will
 * only be included if none of the above headers defined custom versions.
 */

#ifndef INLINE
   #define INLINE
#endif

#ifndef ZERO_SIZE
   #define ZERO_SIZE
#endif

#ifndef AL_CONST
   #define AL_CONST
#endif

#ifndef AL_VAR
   #define AL_VAR(type, name)                      extern type name
#endif

#ifndef AL_ARRAY
   #define AL_ARRAY(type, name)                    extern type name[]
#endif

#ifndef AL_FUNC
   #define AL_FUNC(type, name, args)               type name args
#endif

#ifndef AL_PRINTFUNC
   #define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args)
#endif

#ifndef AL_METHOD
   #define AL_METHOD(type, name, args)             type (*name) args
#endif

#ifndef AL_FUNCPTR
   #define AL_FUNCPTR(type, name, args)            extern type (*name) args
#endif

#ifndef AL_INLINE
   #define AL_INLINE(type, name, args, code)       type name args;
#endif

#ifndef END_OF_MAIN
   #define END_OF_MAIN()
#endif


/* fill in default memory locking macros */
#ifndef END_OF_FUNCTION
   #define END_OF_FUNCTION(x)
   #define END_OF_STATIC_FUNCTION(x)
   #define LOCK_DATA(d, s)
   #define LOCK_CODE(c, s)
   #define UNLOCK_DATA(d, s)
   #define LOCK_VARIABLE(x)
   #define LOCK_FUNCTION(x)
#endif


/* fill in default filename behaviour */
#ifndef ALLEGRO_LFN
   #define ALLEGRO_LFN  1
#endif

#if (defined ALLEGRO_DOS) || (defined ALLEGRO_WINDOWS)
   #define OTHER_PATH_SEPARATOR  '\\'
   #define DEVICE_SEPARATOR      ':'
#else
   #define OTHER_PATH_SEPARATOR  '/'
   #define DEVICE_SEPARATOR      '\0'
#endif


/* emulate the FA_* flags for platforms that don't already have them */
#ifndef FA_RDONLY
   #define FA_RDONLY       1
   #define FA_HIDDEN       2
   #define FA_SYSTEM       4
   #define FA_LABEL        8
   #define FA_DIREC        16
   #define FA_ARCH         32
#endif


/* emulate missing library functions */
#ifdef ALLEGRO_NO_STRICMP
   AL_FUNC(int, _alemu_stricmp, (AL_CONST char *s1, AL_CONST char *s2));
   #define stricmp(s1, s2) _alemu_stricmp(s1, s2)
#endif

#ifdef ALLEGRO_NO_STRLWR
   AL_FUNC(char *, _alemu_strlwr, (char *string));
   #define strlwr(string) _alemu_strlwr(string)
#endif

#ifdef ALLEGRO_NO_STRUPR
   AL_FUNC(char *, _alemu_strupr, (char *string));
   #define strupr(string) _alemu_strupr(string)
#endif

#ifdef ALLEGRO_NO_MEMCMP
   AL_FUNC(int, _alemu_memcmp, (AL_CONST void *s1, AL_CONST void *s2, size_t num));
   #define memcmp(s1, s2, num) _alemu_memcmp(s1, s2, num)
#endif

#ifdef ALLEGRO_NO_FINDFIRST
   struct ffblk
   {
      unsigned char ff_attrib;
      unsigned short ff_ftime;
      unsigned short ff_fdate;
      unsigned long ff_fsize;
      char ff_name[1024];
      void *ff_info;
   };

   AL_FUNC(int, _alemu_findfirst, (AL_CONST char *pattern, struct ffblk *ffblk, int attrib));
   AL_FUNC(int, _alemu_findnext, (struct ffblk *ffblk));
   AL_FUNC(void, _alemu_findclose, (struct ffblk *ffblk));

   #define findfirst(pattern, ffblk, attrib) _alemu_findfirst(pattern, ffblk, attrib)
   #define findnext(ffblk) _alemu_findnext(ffblk)
   #define findclose(ffblk) _alemu_findclose(ffblk)
#endif


/* if nobody put them elsewhere, video bitmaps go in regular memory */
#ifndef _video_ds
   #define _video_ds()  _default_ds()
#endif


/* not many places actually use these, but still worth emulating */
#ifndef ALLEGRO_DJGPP
   #define _farsetsel(seg)
   #define _farnspokeb(addr, val)   (*((unsigned char  *)(addr)) = (val))
   #define _farnspokew(addr, val)   (*((unsigned short *)(addr)) = (val))
   #define _farnspokel(addr, val)   (*((unsigned long  *)(addr)) = (val))
   #define _farnspeekb(addr)        (*((unsigned char  *)(addr)))
   #define _farnspeekw(addr)        (*((unsigned short *)(addr)))
   #define _farnspeekl(addr)        (*((unsigned long  *)(addr)))
#endif


/* generic versions of the video memory access helpers */
#ifndef bmp_select
   #define bmp_select(bmp)
#endif

#ifndef bmp_write8
   #define bmp_write8(addr, c)         (*((unsigned char  *)(addr)) = (c))
   #define bmp_write15(addr, c)        (*((unsigned short *)(addr)) = (c))
   #define bmp_write16(addr, c)        (*((unsigned short *)(addr)) = (c))
   #define bmp_write32(addr, c)        (*((unsigned long  *)(addr)) = (c))

   #define bmp_read8(addr)             (*((unsigned char  *)(addr)))
   #define bmp_read15(addr)            (*((unsigned short *)(addr)))
   #define bmp_read16(addr)            (*((unsigned short *)(addr)))
   #define bmp_read32(addr)            (*((unsigned long  *)(addr)))

   #ifdef ALLEGRO_LITTLE_ENDIAN

      AL_INLINE(void, bmp_write24, (unsigned long addr, int c),
      {
	 unsigned char *p = (unsigned char *)addr;

	 p[0] = c & 0xFF;
	 p[1] = (c>>8) & 0xFF;
	 p[2] = (c>>16) & 0xFF;
      })

      AL_INLINE(int, bmp_read24, (unsigned long addr),
      {
	 unsigned char *p = (unsigned char *)addr;
	 int c;

	 c = p[0];
	 c |= (int)p[1] << 8;
	 c |= (int)p[2] << 16;

	 return c;
      })

   #elif defined ALLEGRO_BIG_ENDIAN

      AL_INLINE(void, bmp_write24, (unsigned long addr, int c),
      {
	 unsigned char *p = (unsigned char *)addr;

	 p[0] = (c>>16) & 0xFF;
	 p[1] = (c>>8) & 0xFF;
	 p[2] = c & 0xFF;
      })

      AL_INLINE(int, bmp_read24, (unsigned long addr),
      {
	 unsigned char *p = (unsigned char *)addr;
	 int c;

	 c = (int)p[0] << 16;
	 c |= (int)p[1] << 8;
	 c |= p[2];

	 return c;
      })

   #else
      #error endianess not defined
   #endif

#endif

