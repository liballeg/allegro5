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


/* for backward compatibility */
#ifdef USE_CONSOLE
   #define ALLEGRO_NO_MAGIC_MAIN
   #define ALLEGRO_USE_CONSOLE
#endif


/* include platform-specific stuff */

#include "allegro5/platform/alplatf.h"



#if defined ALLEGRO_DJGPP
   #include "allegro5/platform/aldjgpp.h"
#elif defined ALLEGRO_WATCOM
   #include "allegro5/platform/alwatcom.h"
#elif defined ALLEGRO_MINGW32
   #include "allegro5/platform/almngw32.h"
#elif defined ALLEGRO_DMC
   #include "allegro5/platform/aldmc.h"
#elif defined ALLEGRO_BCC32
   #include "allegro5/platform/albcc32.h"
#elif defined ALLEGRO_MSVC
   #include "allegro5/platform/almsvc.h"
#elif defined ALLEGRO_BEOS
   #include "allegro5/platform/albecfg.h"
#elif defined ALLEGRO_IPHONE
   #include "allegro5/platform/aliphonecfg.h"
#elif defined ALLEGRO_MACOSX
   #include "allegro5/platform/alosxcfg.h"
#elif defined ALLEGRO_QNX
   #include "allegro5/platform/alqnxcfg.h"
#elif defined ALLEGRO_GP2XWIZ
   #include "allegro5/platform/alwizcfg.h"
#elif defined ALLEGRO_UNIX
   #include "allegro5/platform/alucfg.h"
#else
   #error platform not supported
#endif

  
#include "allegro5/platform/astdint.h"
#include "allegro5/platform/astdbool.h"



/* special definitions for the GCC compiler */
#ifdef __GNUC__
   #define ALLEGRO_GCC

   #ifndef AL_INLINE
      #ifdef __cplusplus
         #define AL_INLINE(type, name, args, code)    \
            static inline type name args;             \
            static inline type name args code
      /* Needed if this header is included by C99 code, as
       * "extern __inline__" in C99 exports a new global function.
       */
      #elif __GNUC_STDC_INLINE__
         #define AL_INLINE(type, name, args, code)    \
            extern __inline__ __attribute__((__gnu_inline__)) type name args;         \
            extern __inline__ __attribute__((__gnu_inline__)) type name args code
      #else
         #define AL_INLINE(type, name, args, code)    \
            extern __inline__ type name args;         \
            extern __inline__ type name args code
      #endif
   #endif

   #ifndef AL_INLINE_STATIC
      #ifdef __cplusplus
         #define AL_INLINE_STATIC(type, name, args, code)    \
                 AL_INLINE(type, name, args, code)
      #else
         #define AL_INLINE_STATIC(type, name, args, code)    \
            static __inline__ type name args;         \
            static __inline__ type name args code
      #endif
   #endif

   #define AL_PRINTFUNC(type, name, args, a, b)    AL_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))

   #ifndef INLINE
      #define INLINE          __inline__
   #endif

   #if __GNUC__ >= 3
      /* SET: According to gcc volatile is ignored for a return type.
       * I think the code should just ensure that inportb is declared as an
       * __asm__ __volatile__ macro. If that's the case the extra volatile
       * doesn't have any sense.
       */
      #define RET_VOLATILE
   #else
      #define RET_VOLATILE    volatile
   #endif

   #ifndef ZERO_SIZE_ARRAY
      #if __GNUC__ < 3
         #define ZERO_SIZE_ARRAY(type, name)  __extension__ type name[0]
      #else
         #define ZERO_SIZE_ARRAY(type, name)  type name[] /* ISO C99 flexible array members */
      #endif
   #endif
   
   #ifndef LONG_LONG
      #define LONG_LONG       long long
      #ifdef ALLEGRO_GUESS_INTTYPES_OK
         #define int64_t      signed long long
         #define uint64_t     unsigned long long
      #endif
   #endif

   #ifdef __i386__
      #define ALLEGRO_I386
      #ifndef ALLEGRO_NO_ASM
         #define _AL_SINCOS(x, s, c)  __asm__ ("fsincos" : "=t" (c), "=u" (s) : "0" (x))
      #endif
   #endif

   #ifdef __amd64__
      #define ALLEGRO_AMD64
      #ifndef ALLEGRO_NO_ASM
         #define _AL_SINCOS(x, s, c)  __asm__ ("fsincos" : "=t" (c), "=u" (s) : "0" (x))
      #endif
   #endif
   
   #ifdef __arm__
      #define ALLEGRO_ARM
   #endif

   #ifndef AL_FUNC_DEPRECATED
      #if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
         #define AL_FUNC_DEPRECATED(type, name, args)              AL_FUNC(__attribute__ ((deprecated)) type, name, args)
         #define AL_PRINTFUNC_DEPRECATED(type, name, args, a, b)   AL_PRINTFUNC(__attribute__ ((deprecated)) type, name, args, a, b)
         #define AL_INLINE_DEPRECATED(type, name, args, code)      AL_INLINE(__attribute__ ((deprecated)) type, name, args, code)
      #endif
   #endif

   #ifndef AL_ALIAS
      #define AL_ALIAS(DECL, CALL)                      \
      static __attribute__((unused)) __inline__ DECL    \
      {                                                 \
         return CALL;                                   \
      }
   #endif

   #ifndef AL_ALIAS_VOID_RET
      #define AL_ALIAS_VOID_RET(DECL, CALL)                  \
      static __attribute__((unused)) __inline__ void DECL    \
      {                                                      \
         CALL;                                               \
      }
   #endif
#endif


/* use constructor functions, if supported */
#ifdef ALLEGRO_USE_CONSTRUCTOR
   #define CONSTRUCTOR_FUNCTION(func)              func __attribute__ ((constructor))
   #define DESTRUCTOR_FUNCTION(func)               func __attribute__ ((destructor))
#endif


/* the rest of this file fills in some default definitions of language
 * features and helper functions, which are conditionalised so they will
 * only be included if none of the above headers defined custom versions.
 */

#ifndef _AL_SINCOS
   #define _AL_SINCOS(x, s, c)  do { (c) = cos(x); (s) = sin(x); } while (0)
#endif

#ifndef INLINE
   #define INLINE
#endif

#ifndef RET_VOLATILE
   #define RET_VOLATILE   volatile
#endif

#ifndef ZERO_SIZE_ARRAY
   #define ZERO_SIZE_ARRAY(type, name)             type name[]
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

#ifndef AL_FUNCPTRARRAY
   #define AL_FUNCPTRARRAY(type, name, args)       extern type (*name[]) args
#endif

#ifndef AL_INLINE
   #define AL_INLINE(type, name, args, code)       type name args;
#endif

#ifndef AL_FUNC_DEPRECATED
   #define AL_FUNC_DEPRECATED(type, name, args)              AL_FUNC(type, name, args)
   #define AL_PRINTFUNC_DEPRECATED(type, name, args, a, b)   AL_PRINTFUNC(type, name, args, a, b)
   #define AL_INLINE_DEPRECATED(type, name, args, code)      AL_INLINE(type, name, args, code)
#endif

#ifndef AL_ALIAS
   #define AL_ALIAS(DECL, CALL)              \
   static INLINE DECL                        \
   {                                         \
      return CALL;                           \
   }
#endif

#ifndef AL_ALIAS_VOID_RET
   #define AL_ALIAS_VOID_RET(DECL, CALL)     \
   static INLINE void DECL                   \
   {                                         \
      CALL;                                  \
   }
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
   #define FA_NONE         0
   #define FA_ALL          (~FA_NONE)


#ifdef __cplusplus
   extern "C" {
#endif


/* endian-independent 3-byte accessor macros */
#ifdef ALLEGRO_LITTLE_ENDIAN

   #define READ3BYTES(p)  ((*(unsigned char *)(p))               \
                           | (*((unsigned char *)(p) + 1) << 8)  \
                           | (*((unsigned char *)(p) + 2) << 16))

   #define WRITE3BYTES(p,c)  ((*(unsigned char *)(p) = (c)),             \
                              (*((unsigned char *)(p) + 1) = (c) >> 8),  \
                              (*((unsigned char *)(p) + 2) = (c) >> 16))

#elif defined ALLEGRO_BIG_ENDIAN

   #define READ3BYTES(p)  ((*(unsigned char *)(p) << 16)         \
                           | (*((unsigned char *)(p) + 1) << 8)  \
                           | (*((unsigned char *)(p) + 2)))

   #define WRITE3BYTES(p,c)  ((*(unsigned char *)(p) = (c) >> 16),       \
                              (*((unsigned char *)(p) + 1) = (c) >> 8),  \
                              (*((unsigned char *)(p) + 2) = (c)))

#else
   #error endianess not defined
#endif


/* generic versions of the video memory access helpers */
/* FIXME: why do we need macros for this? */
#define bmp_write16(addr, c)        (*((uint16_t *)(addr)) = (c))
#define bmp_write32(addr, c)        (*((uint32_t *)(addr)) = (c))

#define bmp_read16(addr)            (*((uint16_t *)(addr)))
#define bmp_read32(addr)            (*((uint32_t *)(addr)))



/* default random function definition */
#ifndef AL_RAND
   #define AL_RAND()       (rand())
#endif

#ifdef __cplusplus
   }
#endif


/* parameters for the color conversion code */
#if (defined ALLEGRO_WINDOWS) || (defined ALLEGRO_QNX)
   #define ALLEGRO_COLORCONV_ALIGNED_WIDTH
   #define ALLEGRO_NO_COLORCOPY
#endif

