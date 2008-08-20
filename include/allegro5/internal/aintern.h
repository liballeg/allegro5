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
 *      Some definitions for internal use by the library code.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTERN_H
#define AINTERN_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifdef __cplusplus
   extern "C" {
#endif

//AL_FUNC(void, _rotate_scale_flip_coordinates, (fixed w, fixed h, fixed x, fixed y, fixed cx, fixed cy, fixed angle, fixed scale_x, fixed scale_y, int h_flip, int v_flip, fixed xs[4], fixed ys[4]));

/* default truecolor pixel format */
#define DEFAULT_RGB_R_SHIFT_15  0
#define DEFAULT_RGB_G_SHIFT_15  5
#define DEFAULT_RGB_B_SHIFT_15  10
#define DEFAULT_RGB_R_SHIFT_16  0
#define DEFAULT_RGB_G_SHIFT_16  5
#define DEFAULT_RGB_B_SHIFT_16  11
#define DEFAULT_RGB_R_SHIFT_24  0
#define DEFAULT_RGB_G_SHIFT_24  8
#define DEFAULT_RGB_B_SHIFT_24  16
#define DEFAULT_RGB_R_SHIFT_32  0
#define DEFAULT_RGB_G_SHIFT_32  8
#define DEFAULT_RGB_B_SHIFT_32  16
#define DEFAULT_RGB_A_SHIFT_32  24

#if (defined allegro_windows)

   al_func(int, _al_win_open, (const char *filename, int mode, int perm));
   al_func(int, _al_win_unlink, (const char *filename));


   #define _al_open(filename, mode, perm)   _al_win_open(filename, mode, perm)
   #define _al_unlink(filename)             _al_win_unlink(filename)

#else

   #define _al_open(filename, mode, perm)   open(filename, mode, perm)
   #define _al_unlink(filename)             unlink(filename)

#endif

/* length in bytes of the cpu_vendor string */
#define _AL_CPU_VENDOR_SIZE 32


AL_FUNCPTR(int, _al_trace_handler, (AL_CONST char *msg));


/* malloc wrappers */
AL_VAR(void *, _al_memory_opaque);
AL_FUNCPTR(void *, _al_malloc, (void *opaque, size_t size));
AL_FUNCPTR(void *, _al_malloc_atomic, (void *opaque, size_t size));
AL_FUNCPTR(void, _al_free, (void *opaque, void *ptr));
AL_FUNCPTR(void *, _al_realloc, (void *opaque, void *ptr, size_t size));
AL_FUNCPTR(void *, _al_debug_malloc, (int line, const char *file, const char *func,
   void *opaque, size_t size));
AL_FUNCPTR(void *, _al_debug_malloc_atomic, (int line, const char *file, const char *func,
   void *opaque, size_t size));
AL_FUNCPTR(void, _al_debug_free, (int line, const char *file, const char *func,
   void *opaque, void *ptr));
AL_FUNCPTR(void *, _al_debug_realloc, (int line, const char *file, const char *func,
   void *opaque, void *ptr, size_t size));

#ifdef DEBUGMODE
   #define _AL_MALLOC(SIZE)         (_al_debug_malloc(__LINE__, __FILE__, __func__, _al_memory_opaque, SIZE))
   #define _AL_MALLOC_ATOMIC(SIZE)  (_al_debug_malloc_atomic(__LINE__, __FILE__, __func__, _al_memory_opaque, SIZE))
   #define _AL_FREE(PTR)            (_al_debug_free(__LINE__, __FILE__, __func__, _al_memory_opaque, PTR))
   #define _AL_REALLOC(PTR, SIZE)   (_al_debug_realloc(__LINE__, __FILE__, __func__, _al_memory_opaque, PTR, SIZE))
#else
   #define _AL_MALLOC(SIZE)         (_al_malloc(_al_memory_opaque, SIZE))
   #define _AL_MALLOC_ATOMIC(SIZE)  (_al_malloc_atomic(_al_memory_opaque, SIZE))
   #define _AL_FREE(PTR)            (_al_free(_al_memory_opaque, PTR))
   #define _AL_REALLOC(PTR, SIZE)   (_al_realloc(_al_memory_opaque, PTR, SIZE))
#endif

/* list of functions to call at program cleanup */
AL_FUNC(void, _add_exit_func, (AL_METHOD(void, func, (void)), AL_CONST char *desc));
AL_FUNC(void, _remove_exit_func, (AL_METHOD(void, func, (void))));


/* helper structure for talking to Unicode strings */
typedef struct UTYPE_INFO
{
   int id;
   AL_METHOD(int, u_getc, (AL_CONST char *s));
   AL_METHOD(int, u_getx, (char **s));
   AL_METHOD(int, u_setc, (char *s, int c));
   AL_METHOD(int, u_width, (AL_CONST char *s));
   AL_METHOD(int, u_cwidth, (int c));
   AL_METHOD(int, u_isok, (int c));
   int u_width_max;
} UTYPE_INFO;

AL_FUNC(UTYPE_INFO *, _find_utype, (int type));


/* message stuff */
#define ALLEGRO_MESSAGE_SIZE  4096


/* wrappers for implementing disk I/O on different platforms */
AL_FUNC(int, _al_file_isok, (AL_CONST char *filename));
AL_FUNC(uint64_t, _al_file_size_ex, (AL_CONST char *filename));
AL_FUNC(time_t, _al_file_time, (AL_CONST char *filename));
AL_FUNC(int, _al_drive_exists, (int drive));
AL_FUNC(int, _al_getdrive, (void));
AL_FUNC(void, _al_getdcwd, (int drive, char *buf, int size));

AL_FUNC(void, _al_detect_filename_encoding, (void));

/* obsolete; only exists for binary compatibility with 4.2.0 */
AL_FUNC(long, _al_file_size, (AL_CONST char *filename));


/* packfile stuff */
AL_VAR(int, _packfile_filesize);
AL_VAR(int, _packfile_datasize);
AL_VAR(int, _packfile_type);
AL_FUNC(PACKFILE *, _pack_fdopen, (int fd, AL_CONST char *mode));

AL_FUNC(int, _al_lzss_incomplete_state, (AL_CONST LZSS_UNPACK_DATA *dat));


/* config stuff */
void _reload_config(void);



/* various libc stuff */
AL_FUNC(void *, _al_sane_realloc, (void *ptr, size_t size));
AL_FUNC(char *, _al_sane_strncpy, (char *dest, const char *src, size_t n));


#define _AL_RAND_MAX  0xFFFF
AL_FUNC(void, _al_srand, (int seed));
AL_FUNC(int, _al_rand, (void));



#ifdef __cplusplus
   }
#endif

#endif          /* ifndef AINTERN_H */
