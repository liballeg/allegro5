/*
 * Ryan Dickie
 */
#ifndef A5_AUDIO_COMMON_H
#define A5_AUDIO_COMMON_H

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_AUDIO_SRC
         #define _A5_AUDIO_DLL __declspec(dllexport)
      #else
         #define _A5_AUDIO_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_AUDIO_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_AUDIO_VAR(type, name)             extern _A5_AUDIO_DLL type name
   #define A5_AUDIO_ARRAY(type, name)           extern _A5_AUDIO_DLL type name[]
   #define A5_AUDIO_FUNC(type, name, args)      _A5_AUDIO_DLL type __cdecl name args
   #define A5_AUDIO_METHOD(type, name, args)    type (__cdecl *name) args
   #define A5_AUDIO_FUNCPTR(type, name, args)   extern _A5_AUDIO_DLL type (__cdecl *name) args
   #define A5_AUDIO_PRINTFUNC(type, name, args, a, b)  A5_AUDIO_FUNC(type, name, args)
#elif defined ALLEGRO_MINGW32
   #define A5_AUDIO_VAR(type, name)                   extern _A5_AUDIO_DLL type name
   #define A5_AUDIO_ARRAY(type, name)                 extern _A5_AUDIO_DLL type name[]
   #define A5_AUDIO_FUNC(type, name, args)            extern type name args
   #define A5_AUDIO_METHOD(type, name, args)          type (*name) args
   #define A5_AUDIO_FUNCPTR(type, name, args)         extern _A5_AUDIO_DLL type (*name) args
   #define A5_AUDIO_PRINTFUNC(type, name, args, a, b) A5_AUDIO_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))
#else
   #define A5_AUDIO_VAR       AL_VAR
   #define A5_AUDIO_ARRAY     AL_ARRAY
   #define A5_AUDIO_FUNC      AL_FUNC
   #define A5_AUDIO_METHOD    AL_METHOD
   #define A5_AUDIO_FUNCPTR   AL_FUNCPTR
   #define A5_AUDIO_PRINTFUNC AL_PRINTFUNC
#endif


/* TODO: make typedefs for each parameter, eg: enum, etc. and move those into the respective
 * header files */
typedef enum ALLEGRO_AUDIO_ENUM {
   /* Sample depth and type, and signedness. Mixers only use 32-bit signed
      float (-1..+1). The unsigned value is a bit-flag applied to the depth
      value */
   ALLEGRO_AUDIO_8_BIT_UINT   = 0x00,
   ALLEGRO_AUDIO_16_BIT_INT   = 0x01,
   ALLEGRO_AUDIO_24_BIT_INT   = 0x02,
   ALLEGRO_AUDIO_32_BIT_FLOAT = 0x03,

   /* Speaker configuration (mono, stereo, 2.1, 3, etc). With regards to
      behavior, most of this code makes no distinction between, say, 4.1 and
      5 speaker setups.. they both have 5 "channels". However, users would
      like the distinction, and later when the higher-level stuff is added,
      the differences will become more important. (v>>4)+(v&0xF) should yield
      the total channel count */
   ALLEGRO_AUDIO_1_CH   = 0x10,
   ALLEGRO_AUDIO_2_CH   = 0x20,
   ALLEGRO_AUDIO_3_CH   = 0x30,
   ALLEGRO_AUDIO_4_CH   = 0x40,
   ALLEGRO_AUDIO_5_1_CH = 0x51,
   ALLEGRO_AUDIO_6_1_CH = 0x61,
   ALLEGRO_AUDIO_7_1_CH = 0x71,

   /* Sample looping mode */
   ALLEGRO_AUDIO_PLAY_ONCE = 0x100,
   ALLEGRO_AUDIO_ONE_DIR   = 0x101,
   ALLEGRO_AUDIO_BI_DIR    = 0x102,

   /* various driver modes */
   ALLEGRO_AUDIO_DRIVER_AUTODETECT = 0x20000,
   ALLEGRO_AUDIO_DRIVER_OPENAL     = 0x20001,
   ALLEGRO_AUDIO_DRIVER_ALSA       = 0x20002,
   ALLEGRO_AUDIO_DRIVER_DSOUND     = 0x20003

} ALLEGRO_AUDIO_ENUM;


/* Misc. audio functions */
A5_AUDIO_FUNC(int,  al_audio_init, (ALLEGRO_AUDIO_ENUM mode));
A5_AUDIO_FUNC(void, al_audio_deinit, (void));

A5_AUDIO_FUNC(int,  al_audio_channel_count, (ALLEGRO_AUDIO_ENUM conf));
A5_AUDIO_FUNC(int,  al_audio_depth_size, (ALLEGRO_AUDIO_ENUM conf));

#endif
