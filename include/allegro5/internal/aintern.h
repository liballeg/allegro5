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
    
#define _ALLEGRO_MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define _ALLEGRO_MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define _ALLEGRO_CLAMP(x,y,z) _ALLEGRO_MAX((x), _ALLEGRO_MIN((y), (z)))

AL_FUNC(void, _al_rotate_scale_flip_coordinates, (al_fixed w, al_fixed h,
   al_fixed x, al_fixed y, al_fixed cx, al_fixed cy, al_fixed angle,
   al_fixed scale_x, al_fixed scale_y, int h_flip, int v_flip,
   al_fixed xs[4], al_fixed ys[4]));

#if (defined allegro_windows)

   al_func(int, _al_win_open, (const char *filename, int mode, int perm));
   al_func(int, _al_win_remove, (const char *filename));


   #define _al_open(filename, mode, perm)   _al_win_open(filename, mode, perm)
   #define _al_remove(filename)             _al_win_remove(filename)

#else

   #define _al_open(filename, mode, perm)   open(filename, mode, perm)
   #define _al_remove(filename)             unlink(filename)

#endif

/* length in bytes of the cpu_vendor string */
#define _AL_CPU_VENDOR_SIZE 32


AL_FUNCPTR(int, _al_trace_handler, (AL_CONST char *msg));


/* list of functions to call at program cleanup */
AL_FUNC(void, _al_add_exit_func, (AL_METHOD(void, func, (void)), AL_CONST char *desc));
AL_FUNC(void, _al_remove_exit_func, (AL_METHOD(void, func, (void))));
AL_FUNC(void, _al_run_exit_funcs, (void));


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

/* message stuff */
#define ALLEGRO_MESSAGE_SIZE  4096

#if 0
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
#endif

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
