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
 *      Main header file for the Allegro library.
 *      This should be included by everyone and everything.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_H
#define ALLEGRO_H

#ifdef __cplusplus
   extern "C" {
#endif

#define ALLEGRO_VERSION          3
#define ALLEGRO_SUB_VERSION      9
#define ALLEGRO_WIP_VERSION      35
#define ALLEGRO_VERSION_STR      "3.9.35 (CVS)"
#define ALLEGRO_DATE_STR         "2001"
#define ALLEGRO_DATE             20010124    /* yyyymmdd */

#ifndef ALLEGRO_NO_STD_HEADERS
   #include <stddef.h>
   #include <stdlib.h>
   #include <stdarg.h>
   #include <errno.h>
#endif

#include "allegro/alconfig.h"

#ifdef ALLEGRO_INCLUDE_MATH_H
   #include <math.h>
#endif

/*******************************************/
/************ Some global stuff ************/
/*******************************************/

#ifndef TRUE 
   #define TRUE         -1
   #define FALSE        0
#endif

#undef MIN
#undef MAX
#undef MID

#define MIN(x,y)     (((x) < (y)) ? (x) : (y))
#define MAX(x,y)     (((x) > (y)) ? (x) : (y))
#define MID(x,y,z)   MAX((x), MIN((y), (z)))

#undef ABS
#define ABS(x)       (((x) >= 0) ? (x) : (-(x)))

#undef SGN
#define SGN(x)       (((x) >= 0) ? 1 : -1)

typedef long fixed;

struct RGB;
struct BITMAP;
struct RLE_SPRITE;
struct FONT_GLYPH;
struct GFX_VTABLE;
struct SAMPLE;
struct MIDI;

AL_ARRAY(char, allegro_id);
AL_ARRAY(char, allegro_error);

#define AL_ID(a,b,c,d)     (((a)<<24) | ((b)<<16) | ((c)<<8) | (d))

#define OSTYPE_UNKNOWN     0
#define OSTYPE_WIN3        AL_ID('W','I','N','3')
#define OSTYPE_WIN95       AL_ID('W','9','5',' ')
#define OSTYPE_WIN98       AL_ID('W','9','8',' ')
#define OSTYPE_WINNT       AL_ID('N','T',' ',' ')
#define OSTYPE_OS2         AL_ID('O','S','2',' ')
#define OSTYPE_WARP        AL_ID('W','A','R','P')
#define OSTYPE_DOSEMU      AL_ID('D','E','M','U')
#define OSTYPE_OPENDOS     AL_ID('O','D','O','S')
#define OSTYPE_LINUX       AL_ID('T','U','X',' ')
#define OSTYPE_UNIX        AL_ID('U','N','I','X')
#define OSTYPE_BEOS        AL_ID('B','E','O','S')

AL_VAR(int, os_type);

AL_VAR(int *, allegro_errno);

#define SYSTEM_AUTODETECT  0
#define SYSTEM_NONE        AL_ID('N','O','N','E')

AL_FUNC(int, install_allegro, (int system_id, int *errno_ptr, AL_METHOD(int, atexit_ptr, (AL_METHOD(void, func, (void))))));
AL_FUNC(void, allegro_exit, (void));

AL_FUNC(void, check_cpu, (void));

AL_ARRAY(char, cpu_vendor);
AL_VAR(int, cpu_family);
AL_VAR(int, cpu_model);
AL_VAR(int, cpu_fpu);
AL_VAR(int, cpu_mmx);
AL_VAR(int, cpu_3dnow);
AL_VAR(int, cpu_cpuid);

AL_FUNC(void, lock_bitmap, (struct BITMAP *bmp));
AL_FUNC(void, lock_sample, (struct SAMPLE *spl));
AL_FUNC(void, lock_midi, (struct MIDI *midi));

AL_PRINTFUNC(void, allegro_message, (AL_CONST char *msg, ...), 1, 2);

AL_FUNC(void, al_assert, (AL_CONST char *file, int line));
AL_PRINTFUNC(void, al_trace, (AL_CONST char *msg, ...), 1, 2);

AL_FUNC(void, register_assert_handler, (AL_METHOD(int, handler, (AL_CONST char *msg))));
AL_FUNC(void, register_trace_handler, (AL_METHOD(int, handler, (AL_CONST char *msg))));


#ifdef DEBUGMODE
   #define ASSERT(condition)     { if (!(condition)) al_assert(__FILE__, __LINE__); }
   #define TRACE                 al_trace
#else
   #define ASSERT(condition)
   #define TRACE                 1 ? (void) 0 : al_trace
#endif


typedef struct _DRIVER_INFO         /* info about a hardware driver */
{
   int id;                          /* integer ID */
   void *driver;                    /* the driver structure */
   int autodetect;                  /* set to allow autodetection */
} _DRIVER_INFO;


typedef struct SYSTEM_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(int, init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(void, get_executable_name, (char *output, int size));
   AL_METHOD(int, find_resource, (char *dest, AL_CONST char *resource, int size));
   AL_METHOD(void, set_window_title, (AL_CONST char *name));
   AL_METHOD(void, message, (AL_CONST char *msg));
   AL_METHOD(void, assert, (AL_CONST char *msg));
   AL_METHOD(void, save_console_state, (void));
   AL_METHOD(void, restore_console_state, (void));
   AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height));
   AL_METHOD(void, created_bitmap, (struct BITMAP *bmp));
   AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));
   AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));
   AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap));
   AL_METHOD(void, read_hardware_palette, (void));
   AL_METHOD(void, set_palette_range, (AL_CONST struct RGB *p, int from, int to, int retracesync));
   AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth));
   AL_METHOD(int, set_display_switch_mode, (int mode));
   AL_METHOD(int, set_display_switch_callback, (int dir, AL_METHOD(void, cb, (void))));
   AL_METHOD(void, remove_display_switch_callback, (AL_METHOD(void, cb, (void))));
   AL_METHOD(void, display_switch_lock, (int lock, int foreground));
   AL_METHOD(int, desktop_color_depth, (void));
   AL_METHOD(void, yield_timeslice, (void));
   AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void));
   AL_METHOD(_DRIVER_INFO *, digi_drivers, (void));
   AL_METHOD(_DRIVER_INFO *, midi_drivers, (void));
   AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void));
   AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void));
   AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void));
   AL_METHOD(_DRIVER_INFO *, timer_drivers, (void));
} SYSTEM_DRIVER;


AL_VAR(SYSTEM_DRIVER, system_none);
AL_VAR(SYSTEM_DRIVER *, system_driver);
AL_ARRAY(_DRIVER_INFO, _system_driver_list);



/******************************************/
/************ Unicode routines ************/
/******************************************/

#define U_ASCII         AL_ID('A','S','C','8')
#define U_ASCII_CP      AL_ID('A','S','C','P')
#define U_UNICODE       AL_ID('U','N','I','C')
#define U_UTF8          AL_ID('U','T','F','8')
#define U_CURRENT       AL_ID('c','u','r','.')

AL_FUNC(void, set_uformat, (int type));
AL_FUNC(int, get_uformat, (void));
AL_FUNC(void, register_uformat, (int type, AL_METHOD(int, u_getc, (AL_CONST char *s)), AL_METHOD(int, u_getx, (char **s)), AL_METHOD(int, u_setc, (char *s, int c)), AL_METHOD(int, u_width, (AL_CONST char *s)), AL_METHOD(int, u_cwidth, (int c)), AL_METHOD(int, u_isok, (int c)), int u_width_max));
AL_FUNC(void, set_ucodepage, (AL_CONST unsigned short *table, AL_CONST unsigned short *extras));

AL_FUNC(int, need_uconvert, (AL_CONST char *s, int type, int newtype));
AL_FUNC(int, uconvert_size, (AL_CONST char *s, int type, int newtype));
AL_FUNC(void, do_uconvert, (AL_CONST char *s, int type, char *buf, int newtype, int size));
AL_FUNC(char *, uconvert, (AL_CONST char *s, int type, char *buf, int newtype, int size));
AL_FUNC(int, uwidth_max, (int type));

#define uconvert_ascii(s, buf)      uconvert(s, U_ASCII, buf, U_CURRENT, sizeof(buf))
#define uconvert_toascii(s, buf)    uconvert(s, U_CURRENT, buf, U_ASCII, sizeof(buf))

#define EMPTY_STRING    "\0\0\0"

AL_ARRAY(char, empty_string);

AL_FUNCPTR(int, ugetc, (AL_CONST char *s));
AL_FUNCPTR(int, ugetx, (char **s));
AL_FUNCPTR(int, ugetxc, (AL_CONST char **s));
AL_FUNCPTR(int, usetc, (char *s, int c));
AL_FUNCPTR(int, uwidth, (AL_CONST char *s));
AL_FUNCPTR(int, ucwidth, (int c));
AL_FUNCPTR(int, uisok, (int c));
AL_FUNC(int, uoffset, (AL_CONST char *s, int index));
AL_FUNC(int, ugetat, (AL_CONST char *s, int index));
AL_FUNC(int, usetat, (char *s, int index, int c));
AL_FUNC(int, uinsert, (char *s, int index, int c));
AL_FUNC(int, uremove, (char *s, int index));
AL_FUNC(int, utolower, (int c));
AL_FUNC(int, utoupper, (int c));
AL_FUNC(int, uisspace, (int c));
AL_FUNC(int, uisdigit, (int c));
AL_FUNC(int, ustrsize, (AL_CONST char *s));
AL_FUNC(int, ustrsizez, (AL_CONST char *s));
AL_FUNC(char *, ustrcpy, (char *dest, AL_CONST char *src));
AL_FUNC(char *, ustrcat, (char *dest, AL_CONST char *src));
AL_FUNC(int, ustrlen, (AL_CONST char *s));
AL_FUNC(int, ustrcmp, (AL_CONST char *s1, AL_CONST char *s2));
AL_FUNC(char *, ustrncpy, (char *dest, AL_CONST char *src, int n));
AL_FUNC(char *, ustrncat, (char *dest, AL_CONST char *src, int n));
AL_FUNC(int, ustrncmp, (AL_CONST char *s1, AL_CONST char *s2, int n));
AL_FUNC(int, ustricmp, (AL_CONST char *s1, AL_CONST char *s2));
AL_FUNC(char *, ustrlwr, (char *s));
AL_FUNC(char *, ustrupr, (char *s));
AL_FUNC(char *, ustrchr, (AL_CONST char *s, int c));
AL_FUNC(char *, ustrrchr, (AL_CONST char *s, int c));
AL_FUNC(char *, ustrstr, (AL_CONST char *s1, AL_CONST char *s2));
AL_FUNC(char *, ustrpbrk, (AL_CONST char *s, AL_CONST char *set));
AL_FUNC(char *, ustrtok, (char *s, AL_CONST char *set));
AL_FUNC(double, uatof, (AL_CONST char *s));
AL_FUNC(long, ustrtol, (AL_CONST char *s, char **endp, int base));
AL_FUNC(double, ustrtod, (AL_CONST char *s, char **endp));
AL_FUNC(AL_CONST char *, ustrerror, (int err));
AL_FUNC(int, uvsprintf, (char *buf, AL_CONST char *format, va_list args));
AL_PRINTFUNC(int, usprintf, (char *buf, AL_CONST char *format, ...), 2, 3);

AL_FUNC(char *, _ustrdup, (AL_CONST char *src, AL_METHOD(void *, malloc_func, (size_t))));

#define ustrdup(src)    _ustrdup(src, malloc)


/************************************************/
/************ Configuration routines ************/
/************************************************/

AL_FUNC(void, set_config_file, (AL_CONST char *filename));
AL_FUNC(void, set_config_data, (AL_CONST char *data, int length));
AL_FUNC(void, override_config_file, (AL_CONST char *filename));
AL_FUNC(void, override_config_data, (AL_CONST char *data, int length));

AL_FUNC(void, push_config_state, (void));
AL_FUNC(void, pop_config_state, (void));

AL_FUNC(void, hook_config_section, (AL_CONST char *section, AL_METHOD(int, intgetter, (AL_CONST char *, int)), AL_METHOD(AL_CONST char *, stringgetter, (AL_CONST char *, AL_CONST char *)), AL_METHOD(void, stringsetter, (AL_CONST char *, AL_CONST char *))));
AL_FUNC(int, config_is_hooked, (AL_CONST char *section));

AL_FUNC(AL_CONST char *, get_config_string, (AL_CONST char *section, AL_CONST char *name, AL_CONST char *def));
AL_FUNC(int, get_config_int, (AL_CONST char *section, AL_CONST char *name, int def));
AL_FUNC(int, get_config_hex, (AL_CONST char *section, AL_CONST char *name, int def));
AL_FUNC(float, get_config_float, (AL_CONST char *section, AL_CONST char *name, float def));
AL_FUNC(int, get_config_id, (AL_CONST char *section, AL_CONST char *name, int def));
AL_FUNC(char **, get_config_argv, (AL_CONST char *section, AL_CONST char *name, int *argc));
AL_FUNC(AL_CONST char *, get_config_text, (AL_CONST char *msg));

AL_FUNC(void, set_config_string, (AL_CONST char *section, AL_CONST char *name, AL_CONST char *val));
AL_FUNC(void, set_config_int, (AL_CONST char *section, AL_CONST char *name, int val));
AL_FUNC(void, set_config_hex, (AL_CONST char *section, AL_CONST char *name, int val));
AL_FUNC(void, set_config_float, (AL_CONST char *section, AL_CONST char *name, float val));
AL_FUNC(void, set_config_id, (AL_CONST char *section, AL_CONST char *name, int val));



/****************************************/
/************ Mouse routines ************/
/****************************************/

#define MOUSEDRV_AUTODETECT  -1
#define MOUSEDRV_NONE         0


typedef struct MOUSE_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(int,  init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(void, poll, (void));
   AL_METHOD(void, timer_poll, (void));
   AL_METHOD(void, position, (int x, int y));
   AL_METHOD(void, set_range, (int x1, int y1, int x2, int y2));
   AL_METHOD(void, set_speed, (int xspeed, int yspeed));
   AL_METHOD(void, get_mickeys, (int *mickeyx, int *mickeyy));
   AL_METHOD(int,  analyse_data, (AL_CONST char *buffer, int size));
} MOUSE_DRIVER;


AL_VAR(MOUSE_DRIVER, mousedrv_none);
AL_VAR(MOUSE_DRIVER *, mouse_driver);
AL_VAR(int, mouse_type);

AL_ARRAY(_DRIVER_INFO, _mouse_driver_list);
AL_VAR(int, _mouse_installed);

AL_FUNC(int, install_mouse, (void));
AL_FUNC(void, remove_mouse, (void));

AL_FUNC(int, poll_mouse, (void));
AL_FUNC(int, mouse_needs_poll, (void));

AL_VAR(volatile int, mouse_x);
AL_VAR(volatile int, mouse_y);
AL_VAR(volatile int, mouse_z);
AL_VAR(volatile int, mouse_b);
AL_VAR(volatile int, mouse_pos);

AL_VAR(volatile int, freeze_mouse_flag);

#define MOUSE_FLAG_MOVE             1
#define MOUSE_FLAG_LEFT_DOWN        2
#define MOUSE_FLAG_LEFT_UP          4
#define MOUSE_FLAG_RIGHT_DOWN       8
#define MOUSE_FLAG_RIGHT_UP         16
#define MOUSE_FLAG_MIDDLE_DOWN      32
#define MOUSE_FLAG_MIDDLE_UP        64
#define MOUSE_FLAG_MOVE_Z           128

AL_FUNCPTR(void, mouse_callback, (int flags));

AL_FUNC(void, show_mouse, (struct BITMAP *bmp));
AL_FUNC(void, scare_mouse, (void));
AL_FUNC(void, scare_mouse_area, (int x, int y, int w, int h));
AL_FUNC(void, unscare_mouse, (void));
AL_FUNC(void, position_mouse, (int x, int y));
AL_FUNC(void, position_mouse_z, (int z));
AL_FUNC(void, set_mouse_range, (int x1, int y1, int x2, int y2));
AL_FUNC(void, set_mouse_speed, (int xspeed, int yspeed));
AL_FUNC(void, set_mouse_sprite, (struct BITMAP *sprite));
AL_FUNC(void, set_mouse_sprite_focus, (int x, int y));
AL_FUNC(void, get_mouse_mickeys, (int *mickeyx, int *mickeyy));



/****************************************/
/************ Timer routines ************/
/****************************************/

#define TIMERS_PER_SECOND     1193181L
#define SECS_TO_TIMER(x)      ((long)(x) * TIMERS_PER_SECOND)
#define MSEC_TO_TIMER(x)      ((long)(x) * (TIMERS_PER_SECOND / 1000))
#define BPS_TO_TIMER(x)       (TIMERS_PER_SECOND / (long)(x))
#define BPM_TO_TIMER(x)       ((60 * TIMERS_PER_SECOND) / (long)(x))


typedef struct TIMER_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(int,  init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(int,  install_int, (AL_METHOD(void, proc, (void)), long speed));
   AL_METHOD(void, remove_int, (AL_METHOD(void, proc, (void))));
   AL_METHOD(int,  install_param_int, (AL_METHOD(void, proc, (void *param)), void *param, long speed));
   AL_METHOD(void, remove_param_int, (AL_METHOD(void, proc, (void *param)), void *param));
   AL_METHOD(int,  can_simulate_retrace, (void));
   AL_METHOD(void, simulate_retrace, (int enable));
   AL_METHOD(void, rest, (long time, AL_METHOD(void, callback, (void))));
} TIMER_DRIVER;


AL_VAR(TIMER_DRIVER *, timer_driver);
AL_ARRAY(_DRIVER_INFO, _timer_driver_list);
AL_VAR(int, _timer_installed);

AL_FUNC(int, install_timer, (void));
AL_FUNC(void, remove_timer, (void));

AL_FUNC(int, install_int_ex, (AL_METHOD(void, proc, (void)), long speed));
AL_FUNC(int, install_int, (AL_METHOD(void, proc, (void)), long speed));
AL_FUNC(void, remove_int, (AL_METHOD(void, proc, (void))));

AL_FUNC(int, install_param_int_ex, (AL_METHOD(void, proc, (void *param)), void *param, long speed));
AL_FUNC(int, install_param_int, (AL_METHOD(void, proc, (void *param)), void *param, long speed));
AL_FUNC(void, remove_param_int, (AL_METHOD(void, proc, (void *param)), void *param));

AL_VAR(volatile int, retrace_count);
AL_FUNCPTR(void, retrace_proc, (void));

AL_FUNC(int,  timer_can_simulate_retrace, (void));
AL_FUNC(void, timer_simulate_retrace, (int enable));
AL_FUNC(int,  timer_is_using_retrace, (void));

AL_FUNC(void, rest, (long time));
AL_FUNC(void, rest_callback, (long time, AL_METHOD(void, callback, (void))));



/*******************************************/
/************ Keyboard routines ************/
/*******************************************/

typedef struct KEYBOARD_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   int autorepeat;
   AL_METHOD(int,  init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(void, poll, (void));
   AL_METHOD(void, set_leds, (int leds));
   AL_METHOD(void, set_rate, (int delay, int rate));
   AL_METHOD(void, wait_for_input, (void));
   AL_METHOD(void, stop_waiting_for_input, (void));
   AL_METHOD(int,  scancode_to_ascii, (int scancode));
} KEYBOARD_DRIVER;


AL_VAR(KEYBOARD_DRIVER *, keyboard_driver);
AL_ARRAY(_DRIVER_INFO, _keyboard_driver_list);
AL_VAR(int, _keyboard_installed);

AL_FUNC(int, install_keyboard, (void));
AL_FUNC(void, remove_keyboard, (void));

AL_FUNC(int, poll_keyboard, (void));
AL_FUNC(int, keyboard_needs_poll, (void));

AL_FUNCPTR(int, keyboard_callback, (int key));
AL_FUNCPTR(int, keyboard_ucallback, (int key, int *scancode));
AL_FUNCPTR(void, keyboard_lowlevel_callback, (int scancode));

AL_FUNC(void, install_keyboard_hooks, (AL_METHOD(int, keypressed, (void)), AL_METHOD(int, readkey, (void))));

AL_ARRAY(volatile char, key);
AL_VAR(volatile int, key_shifts);

AL_VAR(int, three_finger_flag);
AL_VAR(int, key_led_flag);

AL_FUNC(int, keypressed, (void));
AL_FUNC(int, readkey, (void));
AL_FUNC(int, ureadkey, (int *scancode));
AL_FUNC(void, simulate_keypress, (int keycode));
AL_FUNC(void, simulate_ukeypress, (int keycode, int scancode));
AL_FUNC(void, clear_keybuf, (void));
AL_FUNC(void, set_leds, (int leds));
AL_FUNC(void, set_keyboard_rate, (int delay, int repeat));
AL_FUNC(int, scancode_to_ascii, (int scancode));

#define KB_SHIFT_FLAG         0x0001
#define KB_CTRL_FLAG          0x0002
#define KB_ALT_FLAG           0x0004
#define KB_LWIN_FLAG          0x0008
#define KB_RWIN_FLAG          0x0010
#define KB_MENU_FLAG          0x0020
#define KB_SCROLOCK_FLAG      0x0100
#define KB_NUMLOCK_FLAG       0x0200
#define KB_CAPSLOCK_FLAG      0x0400
#define KB_INALTSEQ_FLAG      0x0800
#define KB_ACCENT1_FLAG       0x1000
#define KB_ACCENT2_FLAG       0x2000
#define KB_ACCENT3_FLAG       0x4000
#define KB_ACCENT4_FLAG       0x8000

#define KEY_A                 1
#define KEY_B                 2
#define KEY_C                 3
#define KEY_D                 4
#define KEY_E                 5
#define KEY_F                 6
#define KEY_G                 7
#define KEY_H                 8
#define KEY_I                 9
#define KEY_J                 10
#define KEY_K                 11
#define KEY_L                 12
#define KEY_M                 13
#define KEY_N                 14
#define KEY_O                 15
#define KEY_P                 16
#define KEY_Q                 17
#define KEY_R                 18
#define KEY_S                 19
#define KEY_T                 20
#define KEY_U                 21
#define KEY_V                 22
#define KEY_W                 23
#define KEY_X                 24
#define KEY_Y                 25
#define KEY_Z                 26
#define KEY_0                 27
#define KEY_1                 28
#define KEY_2                 29
#define KEY_3                 30
#define KEY_4                 31
#define KEY_5                 32
#define KEY_6                 33
#define KEY_7                 34
#define KEY_8                 35
#define KEY_9                 36
#define KEY_0_PAD             37
#define KEY_1_PAD             38
#define KEY_2_PAD             39
#define KEY_3_PAD             40
#define KEY_4_PAD             41
#define KEY_5_PAD             42
#define KEY_6_PAD             43
#define KEY_7_PAD             44
#define KEY_8_PAD             45
#define KEY_9_PAD             46
#define KEY_F1                47
#define KEY_F2                48
#define KEY_F3                49
#define KEY_F4                50
#define KEY_F5                51
#define KEY_F6                52
#define KEY_F7                53
#define KEY_F8                54
#define KEY_F9                55
#define KEY_F10               56
#define KEY_F11               57
#define KEY_F12               58
#define KEY_ESC               59
#define KEY_TILDE             60
#define KEY_MINUS             61
#define KEY_EQUALS            62
#define KEY_BACKSPACE         63
#define KEY_TAB               64
#define KEY_OPENBRACE         65
#define KEY_CLOSEBRACE        66
#define KEY_ENTER             67
#define KEY_COLON             68
#define KEY_QUOTE             69
#define KEY_BACKSLASH         70
#define KEY_BACKSLASH2        71
#define KEY_COMMA             72
#define KEY_STOP              73
#define KEY_SLASH             74
#define KEY_SPACE             75
#define KEY_INSERT            76
#define KEY_DEL               77
#define KEY_HOME              78
#define KEY_END               79
#define KEY_PGUP              80
#define KEY_PGDN              81
#define KEY_LEFT              82
#define KEY_RIGHT             83
#define KEY_UP                84
#define KEY_DOWN              85
#define KEY_SLASH_PAD         86
#define KEY_ASTERISK          87
#define KEY_MINUS_PAD         88
#define KEY_PLUS_PAD          89
#define KEY_DEL_PAD           90
#define KEY_ENTER_PAD         91
#define KEY_PRTSCR            92
#define KEY_PAUSE             93
#define KEY_ABNT_C1           94
#define KEY_YEN               95
#define KEY_KANA              96
#define KEY_CONVERT           97
#define KEY_NOCONVERT         98
#define KEY_AT                99
#define KEY_CIRCUMFLEX        100
#define KEY_COLON2            101
#define KEY_KANJI             102

#define KEY_MODIFIERS         103

#define KEY_LSHIFT            103
#define KEY_RSHIFT            104
#define KEY_LCONTROL          105
#define KEY_RCONTROL          106
#define KEY_ALT               107
#define KEY_ALTGR             108
#define KEY_LWIN              109
#define KEY_RWIN              110
#define KEY_MENU              111
#define KEY_SCRLOCK           112
#define KEY_NUMLOCK           113
#define KEY_CAPSLOCK          114

#define KEY_MAX               115



/*******************************************/
/************ Joystick routines ************/
/*******************************************/

#define JOY_TYPE_AUTODETECT      -1
#define JOY_TYPE_NONE            0


#define MAX_JOYSTICKS            8
#define MAX_JOYSTICK_AXIS        3
#define MAX_JOYSTICK_STICKS      4
#define MAX_JOYSTICK_BUTTONS     16


/* information about a single joystick axis */
typedef struct JOYSTICK_AXIS_INFO
{
   int pos;
   int d1, d2;
   AL_CONST char *name;
} JOYSTICK_AXIS_INFO;


/* information about one or more axis (a slider or directional control) */
typedef struct JOYSTICK_STICK_INFO
{
   int flags;
   int num_axis;
   JOYSTICK_AXIS_INFO axis[MAX_JOYSTICK_AXIS];
   AL_CONST char *name;
} JOYSTICK_STICK_INFO;


/* information about a joystick button */
typedef struct JOYSTICK_BUTTON_INFO
{
   int b;
   AL_CONST char *name;
} JOYSTICK_BUTTON_INFO;


/* information about an entire joystick */
typedef struct JOYSTICK_INFO
{
   int flags;
   int num_sticks;
   int num_buttons;
   JOYSTICK_STICK_INFO stick[MAX_JOYSTICK_STICKS];
   JOYSTICK_BUTTON_INFO button[MAX_JOYSTICK_BUTTONS];
} JOYSTICK_INFO;


/* joystick status flags */
#define JOYFLAG_DIGITAL             1
#define JOYFLAG_ANALOGUE            2
#define JOYFLAG_CALIB_DIGITAL       4
#define JOYFLAG_CALIB_ANALOGUE      8
#define JOYFLAG_CALIBRATE           16
#define JOYFLAG_SIGNED              32
#define JOYFLAG_UNSIGNED            64


/* alternative spellings */
#define JOYFLAG_ANALOG              JOYFLAG_ANALOGUE
#define JOYFLAG_CALIB_ANALOG        JOYFLAG_CALIB_ANALOGUE


/* global joystick information */
AL_ARRAY(JOYSTICK_INFO, joy);
AL_VAR(int, num_joysticks);


typedef struct JOYSTICK_DRIVER         /* driver for reading joystick input */
{
   int  id; 
   AL_CONST char *name; 
   AL_CONST char *desc; 
   AL_CONST char *ascii_name;
   AL_METHOD(int, init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(int, poll, (void));
   AL_METHOD(int, save_data, (void));
   AL_METHOD(int, load_data, (void));
   AL_METHOD(AL_CONST char *, calibrate_name, (int n));
   AL_METHOD(int, calibrate, (int n));
} JOYSTICK_DRIVER;


AL_VAR(JOYSTICK_DRIVER, joystick_none);
AL_VAR(JOYSTICK_DRIVER *, joystick_driver);
AL_ARRAY(_DRIVER_INFO, _joystick_driver_list);


/* macros for constructing the driver list */
#define BEGIN_JOYSTICK_DRIVER_LIST                             \
   _DRIVER_INFO _joystick_driver_list[] =                      \
   {

#define END_JOYSTICK_DRIVER_LIST                               \
      {  JOY_TYPE_NONE,    &joystick_none,      TRUE  },       \
      {  0,                NULL,                0     }        \
   };


AL_FUNC(int, install_joystick, (int type));
AL_FUNC(void, remove_joystick, (void));

AL_FUNC(int, initialise_joystick, (void));

AL_VAR(int, _joystick_installed);

AL_FUNC(int, poll_joystick, (void));

AL_FUNC(int, save_joystick_data, (AL_CONST char *filename));
AL_FUNC(int, load_joystick_data, (AL_CONST char *filename));

AL_FUNC(AL_CONST char *, calibrate_joystick_name, (int n));
AL_FUNC(int, calibrate_joystick, (int n));

AL_VAR(int, joy_type);



/************************************************/
/************ Screen/bitmap routines ************/
/************************************************/

#define GFX_TEXT                 -1
#define GFX_AUTODETECT           0
#define GFX_SAFE                 AL_ID('S','A','F','E')


typedef struct GFX_DRIVER        /* creates and manages the screen bitmap */
{
   int  id; 
   AL_CONST char *name; 
   AL_CONST char *desc; 
   AL_CONST char *ascii_name;
   AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
   AL_METHOD(void, exit, (struct BITMAP *b));
   AL_METHOD(int, scroll, (int x, int y));
   AL_METHOD(void, vsync, (void));
   AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync));
   AL_METHOD(int, request_scroll, (int x, int y));
   AL_METHOD(int, poll_scroll, (void));
   AL_METHOD(void, enable_triple_buffer, (void));
   AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
   AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
   AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
   AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
   AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
   AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
   AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   AL_METHOD(void, hide_mouse, (void));
   AL_METHOD(void, move_mouse, (int x, int y));
   AL_METHOD(void, drawing_mode, (void));
   AL_METHOD(void, save_video_state, (void));
   AL_METHOD(void, restore_video_state, (void));
   int w, h;                     /* physical (not virtual!) screen size */
   int linear;                   /* true if video memory is linear */
   long bank_size;               /* bank size, in bytes */
   long bank_gran;               /* bank granularity, in bytes */
   long vid_mem;                 /* video memory size, in bytes */
   long vid_phys_base;           /* physical address of video memory */
} GFX_DRIVER;


AL_VAR(GFX_DRIVER *, gfx_driver);
AL_ARRAY(_DRIVER_INFO, _gfx_driver_list);


/* macros for constructing the driver list */
#define BEGIN_GFX_DRIVER_LIST                      \
   _DRIVER_INFO _gfx_driver_list[] =               \
   {

#define END_GFX_DRIVER_LIST                        \
      {  0,          NULL,       0     }           \
   };


#define GFX_CAN_SCROLL                    0x00000001
#define GFX_CAN_TRIPLE_BUFFER             0x00000002
#define GFX_HW_CURSOR                     0x00000004
#define GFX_HW_HLINE                      0x00000008
#define GFX_HW_HLINE_XOR                  0x00000010
#define GFX_HW_HLINE_SOLID_PATTERN        0x00000020
#define GFX_HW_HLINE_COPY_PATTERN         0x00000040
#define GFX_HW_FILL                       0x00000080
#define GFX_HW_FILL_XOR                   0x00000100
#define GFX_HW_FILL_SOLID_PATTERN         0x00000200
#define GFX_HW_FILL_COPY_PATTERN          0x00000400
#define GFX_HW_LINE                       0x00000800
#define GFX_HW_LINE_XOR                   0x00001000
#define GFX_HW_TRIANGLE                   0x00002000
#define GFX_HW_TRIANGLE_XOR               0x00004000
#define GFX_HW_GLYPH                      0x00008000
#define GFX_HW_VRAM_BLIT                  0x00010000
#define GFX_HW_VRAM_BLIT_MASKED           0x00020000
#define GFX_HW_MEM_BLIT                   0x00040000
#define GFX_HW_MEM_BLIT_MASKED            0x00080000
#define GFX_HW_SYS_TO_VRAM_BLIT           0x00100000
#define GFX_HW_SYS_TO_VRAM_BLIT_MASKED    0x00200000


AL_VAR(int, gfx_capabilities);   /* current driver capabilities */


typedef struct GFX_VTABLE        /* functions for drawing onto bitmaps */
{
   int color_depth;
   int mask_color;
   void *unwrite_bank;  /* C function on some machines, asm on i386 */
   AL_METHOD(void, set_clip, (struct BITMAP *bmp));
   AL_METHOD(void, acquire, (struct BITMAP *bmp));
   AL_METHOD(void, release, (struct BITMAP *bmp));
   AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));
   AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));
   AL_METHOD(int,  getpixel, (struct BITMAP *bmp, int x, int y));
   AL_METHOD(void, putpixel, (struct BITMAP *bmp, int x, int y, int color));
   AL_METHOD(void, vline, (struct BITMAP *bmp, int x, int y1, int y2, int color));
   AL_METHOD(void, hline, (struct BITMAP *bmp, int x1, int y, int x2, int color));
   AL_METHOD(void, line, (struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color));
   AL_METHOD(void, rectfill, (struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color));
   AL_METHOD(int,  triangle, (struct BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color));
   AL_METHOD(void, draw_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_256_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_sprite_v_flip, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_sprite_h_flip, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_sprite_vh_flip, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_trans_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_trans_rgba_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y));
   AL_METHOD(void, draw_lit_sprite, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color));
   AL_METHOD(void, draw_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
   AL_METHOD(void, draw_trans_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
   AL_METHOD(void, draw_trans_rgba_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
   AL_METHOD(void, draw_lit_rle_sprite, (struct BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
   AL_METHOD(void, draw_character, (struct BITMAP *bmp, struct BITMAP *sprite, int x, int y, int color));
   AL_METHOD(void, draw_glyph, (struct BITMAP *bmp, AL_CONST struct FONT_GLYPH *glyph, int x, int y, int color));
   AL_METHOD(void, blit_from_memory, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_memory, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_from_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_system, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_self, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_self_forward, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, blit_to_self_backward, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, masked_blit, (struct BITMAP *source, struct BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
   AL_METHOD(void, clear_to_color, (struct BITMAP *bitmap, int color));
   AL_METHOD(void, draw_sprite_end, (void));
   AL_METHOD(void, blit_end, (void));
} GFX_VTABLE;


AL_VAR(GFX_VTABLE, __linear_vtable8);
AL_VAR(GFX_VTABLE, __linear_vtable15);
AL_VAR(GFX_VTABLE, __linear_vtable16);
AL_VAR(GFX_VTABLE, __linear_vtable24);
AL_VAR(GFX_VTABLE, __linear_vtable32);


typedef struct _VTABLE_INFO
{
   int color_depth;
   GFX_VTABLE *vtable;
} _VTABLE_INFO;

AL_ARRAY(_VTABLE_INFO, _vtable_list);


/* macros for constructing the vtable list */
#define BEGIN_COLOR_DEPTH_LIST               \
   _VTABLE_INFO _vtable_list[] =             \
   {

#define END_COLOR_DEPTH_LIST                 \
      {  0,    NULL  }                       \
   };

#define COLOR_DEPTH_8                        \
   {  8,    &__linear_vtable8    },

#define COLOR_DEPTH_15                       \
   {  15,   &__linear_vtable15   },

#define COLOR_DEPTH_16                       \
   {  16,   &__linear_vtable16   },

#define COLOR_DEPTH_24                       \
   {  24,   &__linear_vtable24   },

#define COLOR_DEPTH_32                       \
   {  32,   &__linear_vtable32   },


typedef struct BITMAP            /* a bitmap structure */
{
   int w, h;                     /* width and height in pixels */
   int clip;                     /* flag if clipping is turned on */
   int cl, cr, ct, cb;           /* clip left, right, top and bottom values */
   GFX_VTABLE *vtable;           /* drawing functions */
   void *write_bank;             /* C func on some machines, asm on i386 */
   void *read_bank;              /* C func on some machines, asm on i386 */
   void *dat;                    /* the memory we allocated for the bitmap */
   unsigned long id;             /* for identifying sub-bitmaps */
   void *extra;                  /* points to a structure with more info */
   int x_ofs;                    /* horizontal offset (for sub-bitmaps) */
   int y_ofs;                    /* vertical offset (for sub-bitmaps) */
   int seg;                      /* bitmap segment */
   unsigned char *line[ZERO_SIZE];
} BITMAP;


#define BMP_ID_VIDEO       0x80000000
#define BMP_ID_SYSTEM      0x40000000
#define BMP_ID_SUB         0x20000000
#define BMP_ID_PLANAR      0x10000000
#define BMP_ID_NOBLIT      0x08000000
#define BMP_ID_LOCKED      0x04000000
#define BMP_ID_AUTOLOCK    0x02000000
#define BMP_ID_MASK        0x01FFFFFF


AL_VAR(BITMAP *, screen);

#define SCREEN_W     (gfx_driver ? gfx_driver->w : 0)
#define SCREEN_H     (gfx_driver ? gfx_driver->h : 0)

#define VIRTUAL_W    (screen ? screen->w : 0)
#define VIRTUAL_H    (screen ? screen->h : 0)

#define COLORCONV_NONE              0

#define COLORCONV_8_TO_15           1
#define COLORCONV_8_TO_16           2
#define COLORCONV_8_TO_24           4
#define COLORCONV_8_TO_32           8

#define COLORCONV_15_TO_8           0x10
#define COLORCONV_15_TO_16          0x20
#define COLORCONV_15_TO_24          0x40
#define COLORCONV_15_TO_32          0x80

#define COLORCONV_16_TO_8           0x100
#define COLORCONV_16_TO_15          0x200
#define COLORCONV_16_TO_24          0x400
#define COLORCONV_16_TO_32          0x800

#define COLORCONV_24_TO_8           0x1000
#define COLORCONV_24_TO_15          0x2000
#define COLORCONV_24_TO_16          0x4000
#define COLORCONV_24_TO_32          0x8000

#define COLORCONV_32_TO_8           0x10000
#define COLORCONV_32_TO_15          0x20000
#define COLORCONV_32_TO_16          0x40000
#define COLORCONV_32_TO_24          0x80000

#define COLORCONV_32A_TO_8          0x100000
#define COLORCONV_32A_TO_15         0x200000
#define COLORCONV_32A_TO_16         0x400000
#define COLORCONV_32A_TO_24         0x800000

#define COLORCONV_DITHER_PAL        0x1000000
#define COLORCONV_DITHER_HI         0x2000000

#define COLORCONV_DITHER            (COLORCONV_DITHER_PAL |          \
				     COLORCONV_DITHER_HI)

#define COLORCONV_EXPAND_256        (COLORCONV_8_TO_15 |             \
				     COLORCONV_8_TO_16 |             \
				     COLORCONV_8_TO_24 |             \
				     COLORCONV_8_TO_32)

#define COLORCONV_REDUCE_TO_256     (COLORCONV_15_TO_8 |             \
				     COLORCONV_16_TO_8 |             \
				     COLORCONV_24_TO_8 |             \
				     COLORCONV_32_TO_8 |             \
				     COLORCONV_32A_TO_8)

#define COLORCONV_EXPAND_15_TO_16    COLORCONV_15_TO_16

#define COLORCONV_REDUCE_16_TO_15    COLORCONV_16_TO_15

#define COLORCONV_EXPAND_HI_TO_TRUE (COLORCONV_15_TO_24 |            \
				     COLORCONV_15_TO_32 |            \
				     COLORCONV_16_TO_24 |            \
				     COLORCONV_16_TO_32)

#define COLORCONV_REDUCE_TRUE_TO_HI (COLORCONV_24_TO_15 |            \
				     COLORCONV_24_TO_16 |            \
				     COLORCONV_32_TO_15 |            \
				     COLORCONV_32_TO_16)

#define COLORCONV_24_EQUALS_32      (COLORCONV_24_TO_32 |            \
				     COLORCONV_32_TO_24)

#define COLORCONV_TOTAL             (COLORCONV_EXPAND_256 |          \
				     COLORCONV_REDUCE_TO_256 |       \
				     COLORCONV_EXPAND_15_TO_16 |     \
				     COLORCONV_REDUCE_16_TO_15 |     \
				     COLORCONV_EXPAND_HI_TO_TRUE |   \
				     COLORCONV_REDUCE_TRUE_TO_HI |   \
				     COLORCONV_24_EQUALS_32 |        \
				     COLORCONV_32A_TO_15 |           \
				     COLORCONV_32A_TO_16 |           \
				     COLORCONV_32A_TO_24)

#define COLORCONV_PARTIAL           (COLORCONV_EXPAND_15_TO_16 |     \
				     COLORCONV_REDUCE_16_TO_15 |     \
				     COLORCONV_24_EQUALS_32)

#define COLORCONV_MOST              (COLORCONV_EXPAND_15_TO_16 |     \
				     COLORCONV_REDUCE_16_TO_15 |     \
				     COLORCONV_EXPAND_HI_TO_TRUE |   \
				     COLORCONV_REDUCE_TRUE_TO_HI |   \
				     COLORCONV_24_EQUALS_32)

AL_FUNC(void, set_color_depth, (int depth));
AL_FUNC(void, set_color_conversion, (int mode));
AL_FUNC(void, request_refresh_rate, (int rate));
AL_FUNC(int, get_refresh_rate, (void));
AL_FUNC(int, set_gfx_mode, (int card, int w, int h, int v_w, int v_h));
AL_FUNC(int, scroll_screen, (int x, int y));
AL_FUNC(int, request_scroll, (int x, int y));
AL_FUNC(int, poll_scroll, (void));
AL_FUNC(int, show_video_bitmap, (BITMAP *bitmap));
AL_FUNC(int, request_video_bitmap, (BITMAP *bitmap));
AL_FUNC(int, enable_triple_buffer, (void));
AL_FUNC(BITMAP *, create_bitmap, (int width, int height));
AL_FUNC(BITMAP *, create_bitmap_ex, (int color_depth, int width, int height));
AL_FUNC(BITMAP *, create_sub_bitmap, (BITMAP *parent, int x, int y, int width, int height));
AL_FUNC(BITMAP *, create_video_bitmap, (int width, int height));
AL_FUNC(BITMAP *, create_system_bitmap, (int width, int height));
AL_FUNC(void, destroy_bitmap, (BITMAP *bitmap));

#define SWITCH_NONE           0
#define SWITCH_PAUSE          1
#define SWITCH_AMNESIA        2
#define SWITCH_BACKGROUND     3
#define SWITCH_BACKAMNESIA    4

#define SWITCH_IN             0
#define SWITCH_OUT            1

AL_FUNC(int, set_display_switch_mode, (int mode));
AL_FUNC(int, get_display_switch_mode, (void));
AL_FUNC(int, set_display_switch_callback, (int dir, AL_METHOD(void, cb, (void))));
AL_FUNC(void, remove_display_switch_callback, (AL_METHOD(void, cb, (void))));



/************************************************/
/************ Color/Palette routines ************/
/************************************************/

typedef struct RGB
{
   unsigned char r, g, b;
   unsigned char filler;
} RGB;

#define PAL_SIZE     256

typedef RGB PALETTE[PAL_SIZE];

AL_VAR(RGB, black_rgb);
AL_VAR(PALETTE, black_palette);
AL_VAR(PALETTE, desktop_palette);
AL_VAR(PALETTE, default_palette);
AL_VAR(PALETTE, _current_palette);
AL_VAR(int, _current_palette_changed);

typedef struct {
   unsigned char data[32][32][32];
} RGB_MAP;

typedef struct {
   unsigned char data[PAL_SIZE][PAL_SIZE];
} COLOR_MAP;

AL_VAR(RGB_MAP *, rgb_map);
AL_VAR(COLOR_MAP *, color_map);

typedef AL_METHOD(unsigned long, BLENDER_FUNC, (unsigned long x, unsigned long y, unsigned long n));

AL_VAR(BLENDER_FUNC, _blender_func15);
AL_VAR(BLENDER_FUNC, _blender_func16);
AL_VAR(BLENDER_FUNC, _blender_func24);
AL_VAR(BLENDER_FUNC, _blender_func32);

AL_VAR(BLENDER_FUNC, _blender_func15x);
AL_VAR(BLENDER_FUNC, _blender_func16x);
AL_VAR(BLENDER_FUNC, _blender_func24x);

AL_VAR(int, _blender_col_15);
AL_VAR(int, _blender_col_16);
AL_VAR(int, _blender_col_24);
AL_VAR(int, _blender_col_32);

AL_VAR(int, _blender_alpha);

AL_VAR(int, _color_depth);

AL_VAR(int, _rgb_r_shift_15); 
AL_VAR(int, _rgb_g_shift_15); 
AL_VAR(int, _rgb_b_shift_15);
AL_VAR(int, _rgb_r_shift_16); 
AL_VAR(int, _rgb_g_shift_16); 
AL_VAR(int, _rgb_b_shift_16);
AL_VAR(int, _rgb_r_shift_24); 
AL_VAR(int, _rgb_g_shift_24); 
AL_VAR(int, _rgb_b_shift_24);
AL_VAR(int, _rgb_r_shift_32); 
AL_VAR(int, _rgb_g_shift_32); 
AL_VAR(int, _rgb_b_shift_32);
AL_VAR(int, _rgb_a_shift_32);

AL_ARRAY(int, _rgb_scale_5);
AL_ARRAY(int, _rgb_scale_6);

#define MASK_COLOR_8       0
#define MASK_COLOR_15      0x7C1F
#define MASK_COLOR_16      0xF81F
#define MASK_COLOR_24      0xFF00FF
#define MASK_COLOR_32      0xFF00FF

AL_ARRAY(int, palette_color8);
AL_ARRAY(int, palette_color15);
AL_ARRAY(int, palette_color16);
AL_ARRAY(int, palette_color24);
AL_ARRAY(int, palette_color32);

AL_VAR(int *, palette_color);

AL_FUNC(void, vsync, (void));

AL_FUNC(void, set_color, (int index, AL_CONST RGB *p));
AL_FUNC(void, set_palette, (AL_CONST PALETTE p));
AL_FUNC(void, set_palette_range, (AL_CONST PALETTE p, int from, int to, int retracesync));

AL_FUNC(void, get_color, (int index, RGB *p));
AL_FUNC(void, get_palette, (PALETTE p));
AL_FUNC(void, get_palette_range, (PALETTE p, int from, int to));

AL_FUNC(void, fade_interpolate, (AL_CONST PALETTE source, AL_CONST PALETTE dest, PALETTE output, int pos, int from, int to));
AL_FUNC(void, fade_from_range, (AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed, int from, int to));
AL_FUNC(void, fade_in_range, (AL_CONST PALETTE p, int speed, int from, int to));
AL_FUNC(void, fade_out_range, (int speed, int from, int to));
AL_FUNC(void, fade_from, (AL_CONST PALETTE source, AL_CONST PALETTE dest, int speed));
AL_FUNC(void, fade_in, (AL_CONST PALETTE p, int speed));
AL_FUNC(void, fade_out, (int speed));

AL_FUNC(void, select_palette, (AL_CONST PALETTE p));
AL_FUNC(void, unselect_palette, (void));

AL_FUNC(void, generate_332_palette, (PALETTE pal));
AL_FUNC(int, generate_optimized_palette, (BITMAP *image, PALETTE pal, AL_CONST signed char rsvdcols[256]));

AL_FUNC(void, create_rgb_table, (RGB_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_light_table, (COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_trans_table, (COLOR_MAP *table, AL_CONST PALETTE pal, int r, int g, int b, AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_color_table, (COLOR_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, blend, (AL_CONST PALETTE pal, int x, int y, RGB *rgb)), AL_METHOD(void, callback, (int pos))));
AL_FUNC(void, create_blender_table, (COLOR_MAP *table, AL_CONST PALETTE pal, AL_METHOD(void, callback, (int pos))));

AL_FUNC(void, set_blender_mode, (BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, int r, int g, int b, int a));
AL_FUNC(void, set_blender_mode_ex, (BLENDER_FUNC b15, BLENDER_FUNC b16, BLENDER_FUNC b24, BLENDER_FUNC b32, BLENDER_FUNC b15x, BLENDER_FUNC b16x, BLENDER_FUNC b24x, int r, int g, int b, int a));

AL_FUNC(void, set_alpha_blender, (void));
AL_FUNC(void, set_write_alpha_blender, (void));
AL_FUNC(void, set_trans_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_add_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_burn_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_color_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_difference_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_dissolve_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_dodge_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_hue_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_invert_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_luminance_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_multiply_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_saturation_blender, (int r, int g, int b, int a));
AL_FUNC(void, set_screen_blender, (int r, int g, int b, int a));

AL_FUNC(void, hsv_to_rgb, (float h, float s, float v, int *r, int *g, int *b));
AL_FUNC(void, rgb_to_hsv, (int r, int g, int b, float *h, float *s, float *v));

AL_FUNC(int, bestfit_color, (AL_CONST PALETTE pal, int r, int g, int b));

AL_FUNC(int, makecol, (int r, int g, int b));
AL_FUNC(int, makecol8, (int r, int g, int b));
AL_FUNC(int, makecol_depth, (int color_depth, int r, int g, int b));

AL_FUNC(int, makeacol, (int r, int g, int b, int a));
AL_FUNC(int, makeacol_depth, (int color_depth, int r, int g, int b, int a));

AL_FUNC(int, makecol15_dither, (int r, int g, int b, int x, int y));
AL_FUNC(int, makecol16_dither, (int r, int g, int b, int x, int y));

AL_FUNC(int, getr, (int c));
AL_FUNC(int, getg, (int c));
AL_FUNC(int, getb, (int c));
AL_FUNC(int, geta, (int c));

AL_FUNC(int, getr_depth, (int color_depth, int c));
AL_FUNC(int, getg_depth, (int color_depth, int c));
AL_FUNC(int, getb_depth, (int color_depth, int c));
AL_FUNC(int, geta_depth, (int color_depth, int c));



/******************************************************/
/************ Graphics and sprite routines ************/
/******************************************************/

AL_FUNC(void, set_clip, (BITMAP *bitmap, int x1, int y1, int x2, int y2));

#define DRAW_MODE_SOLID             0        /* flags for drawing_mode() */
#define DRAW_MODE_XOR               1
#define DRAW_MODE_COPY_PATTERN      2
#define DRAW_MODE_SOLID_PATTERN     3
#define DRAW_MODE_MASKED_PATTERN    4
#define DRAW_MODE_TRANS             5

AL_FUNC(void, drawing_mode, (int mode, BITMAP *pattern, int x_anchor, int y_anchor));
AL_FUNC(void, xor_mode, (int on));
AL_FUNC(void, solid_mode, (void));
AL_FUNC(void, do_line, (BITMAP *bmp, int x1, int y1, int x2, int y2, int d, AL_METHOD(void, proc, (BITMAP *, int, int, int))));
AL_FUNC(void, triangle, (BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color));
AL_FUNC(void, polygon, (BITMAP *bmp, int vertices, AL_CONST int *points, int color));
AL_FUNC(void, rect, (BITMAP *bmp, int x1, int y1, int x2, int y2, int color));
AL_FUNC(void, do_circle, (BITMAP *bmp, int x, int y, int radius, int d, AL_METHOD(void, proc, (BITMAP *, int, int, int))));
AL_FUNC(void, circle, (BITMAP *bmp, int x, int y, int radius, int color));
AL_FUNC(void, circlefill, (BITMAP *bmp, int x, int y, int radius, int color));
AL_FUNC(void, do_ellipse, (BITMAP *bmp, int x, int y, int rx, int ry, int d, AL_METHOD(void, proc, (BITMAP *, int, int, int))));
AL_FUNC(void, ellipse, (BITMAP *bmp, int x, int y, int rx, int ry, int color));
AL_FUNC(void, ellipsefill, (BITMAP *bmp, int x, int y, int rx, int ry, int color));
AL_FUNC(void, do_arc, (BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int d, AL_METHOD(void, proc, (BITMAP *, int, int, int))));
AL_FUNC(void, arc, (BITMAP *bmp, int x, int y, fixed ang1, fixed ang2, int r, int color));
AL_FUNC(void, calc_spline, (AL_CONST int points[8], int npts, int *x, int *y));
AL_FUNC(void, spline, (BITMAP *bmp, AL_CONST int points[8], int color));
AL_FUNC(void, floodfill, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, blit, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, masked_blit, (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, stretch_blit, (BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h));
AL_FUNC(void, masked_stretch_blit, (BITMAP *s, BITMAP *d, int s_x, int s_y, int s_w, int s_h, int d_x, int d_y, int d_w, int d_h));
AL_FUNC(void, stretch_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int w, int h));
AL_FUNC(void, rotate_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle));
AL_FUNC(void, rotate_scaled_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale));
AL_FUNC(void, pivot_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle));
AL_FUNC(void, pivot_scaled_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale));
AL_FUNC(void, rotate_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle));
AL_FUNC(void, rotate_scaled_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, fixed angle, fixed scale));
AL_FUNC(void, pivot_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle));
AL_FUNC(void, pivot_scaled_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y, int cx, int cy, fixed angle, fixed scale));
AL_FUNC(void, draw_gouraud_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int c1, int c2, int c3, int c4));
AL_FUNC(void, clear, (BITMAP *bitmap));


typedef struct RLE_SPRITE           /* a RLE compressed sprite */
{
   int w, h;                        /* width and height in pixels */
   int color_depth;                 /* color depth of the image */
   int size;                        /* size of sprite data in bytes */
   signed char dat[ZERO_SIZE];
} RLE_SPRITE;


AL_FUNC(RLE_SPRITE *, get_rle_sprite, (BITMAP *bitmap));
AL_FUNC(void, destroy_rle_sprite, (RLE_SPRITE *sprite));


#if (defined ALLEGRO_I386) && (!defined ALLEGRO_USE_C)

/* compiled sprite structure */
typedef struct COMPILED_SPRITE 
{
   short planar;                    /* set if it's a planar (mode-X) sprite */
   short color_depth;               /* color depth of the image */
   short w, h;                      /* size of the sprite */
   struct {
      void *draw;                   /* routines to draw the image */
      int len;                      /* length of the drawing functions */
   } proc[4];
} COMPILED_SPRITE;

#else

/* emulate compiled sprites using RLE on other platforms */
typedef RLE_SPRITE COMPILED_SPRITE;

#endif


AL_FUNC(COMPILED_SPRITE *, get_compiled_sprite, (BITMAP *bitmap, int planar));
AL_FUNC(void, destroy_compiled_sprite, (COMPILED_SPRITE *sprite));
AL_FUNC(void, draw_compiled_sprite, (BITMAP *bmp, AL_CONST COMPILED_SPRITE *sprite, int x, int y));


typedef struct FONT_GLYPH           /* a single monochrome font character */
{
   short w, h;
   unsigned char dat[ZERO_SIZE];
} FONT_GLYPH;


struct FONT_VTABLE;

typedef struct FONT
{
    void* data;
    int height;

    struct FONT_VTABLE* vtable;
}FONT;

AL_VAR(FONT *, font);

AL_FUNC(int, text_mode, (int mode));
AL_FUNC(void, textout, (BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color));
AL_FUNC(void, textout_centre, (BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color));
AL_FUNC(void, textout_right, (BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color));
AL_FUNC(void, textout_justify, (BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff, int color));
AL_PRINTFUNC(void, textprintf, (BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC(void, textprintf_centre, (BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC(void, textprintf_right, (BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC(void, textprintf_justify, (BITMAP *bmp, AL_CONST FONT *f, int x1, int x2, int y, int diff, int color, AL_CONST char *format, ...), 8, 9);
AL_FUNC(int, text_length, (AL_CONST FONT *f, AL_CONST char *str));
AL_FUNC(int, text_height, (AL_CONST FONT *f));
AL_FUNC(void, destroy_font, (FONT *f));


typedef struct V3D                  /* a 3d point (fixed point version) */
{
   fixed x, y, z;                   /* position */
   fixed u, v;                      /* texture map coordinates */
   int c;                           /* color */
} V3D;


typedef struct V3D_f                /* a 3d point (floating point version) */
{
   float x, y, z;                   /* position */
   float u, v;                      /* texture map coordinates */
   int c;                           /* color */
} V3D_f;


#define POLYTYPE_FLAT               0
#define POLYTYPE_GCOL               1
#define POLYTYPE_GRGB               2
#define POLYTYPE_ATEX               3
#define POLYTYPE_PTEX               4
#define POLYTYPE_ATEX_MASK          5
#define POLYTYPE_PTEX_MASK          6
#define POLYTYPE_ATEX_LIT           7
#define POLYTYPE_PTEX_LIT           8
#define POLYTYPE_ATEX_MASK_LIT      9
#define POLYTYPE_PTEX_MASK_LIT      10
#define POLYTYPE_ATEX_TRANS         11
#define POLYTYPE_PTEX_TRANS         12
#define POLYTYPE_ATEX_MASK_TRANS    13
#define POLYTYPE_PTEX_MASK_TRANS    14
#define POLYTYPE_MAX                15
#define POLYTYPE_ZBUF               16

AL_VAR(float, scene_gap);

AL_FUNC(void, polygon3d, (BITMAP *bmp, int type, BITMAP *texture, int vc, V3D *vtx[]));
AL_FUNC(void, polygon3d_f, (BITMAP *bmp, int type, BITMAP *texture, int vc, V3D_f *vtx[]));
AL_FUNC(void, triangle3d, (BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3));
AL_FUNC(void, triangle3d_f, (BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3));
AL_FUNC(void, quad3d, (BITMAP *bmp, int type, BITMAP *texture, V3D *v1, V3D *v2, V3D *v3, V3D *v4));
AL_FUNC(void, quad3d_f, (BITMAP *bmp, int type, BITMAP *texture, V3D_f *v1, V3D_f *v2, V3D_f *v3, V3D_f *v4));
AL_FUNC(int, clip3d, (int type, fixed min_z, fixed max_z, int vc, AL_CONST V3D *vtx[], V3D *vout[], V3D *vtmp[], int out[]));
AL_FUNC(int, clip3d_f, (int type, float min_z, float max_z, int vc, AL_CONST V3D_f *vtx[], V3D_f *vout[], V3D_f *vtmp[], int out[]));
AL_FUNC(int, create_zbuffer, (BITMAP *bmp));
AL_FUNC(void, clear_zbuffer, (float z));
AL_FUNC(void, destroy_zbuffer, (void));
AL_FUNC(int, scene_start, (BITMAP *bmp, int nedge, int npoly));
AL_FUNC(int, scene_polygon3d, (int type, BITMAP *texture, int vx, V3D *vtx[]));
AL_FUNC(int, scene_polygon3d_f, (int type, BITMAP *texture, int vx, V3D_f *vtx[]));
AL_FUNC(void, scene_render, (void));



/******************************************/
/************ FLI/FLC routines ************/
/******************************************/

#define FLI_OK          0              /* FLI player return values */
#define FLI_EOF         -1
#define FLI_ERROR       -2
#define FLI_NOT_OPEN    -3

AL_FUNC(int, play_fli, (AL_CONST char *filename, BITMAP *bmp, int loop, AL_METHOD(int, callback, (void))));
AL_FUNC(int, play_memory_fli, (void *fli_data, BITMAP *bmp, int loop, AL_METHOD(int, callback, (void))));

AL_FUNC(int, open_fli, (AL_CONST char *filename));
AL_FUNC(int, open_memory_fli, (void *fli_data));
AL_FUNC(void, close_fli, (void));
AL_FUNC(int, next_fli_frame, (int loop));
AL_FUNC(void, reset_fli_variables, (void));

AL_VAR(BITMAP *, fli_bitmap);          /* current frame of the FLI */
AL_VAR(PALETTE, fli_palette);          /* current FLI palette */

AL_VAR(int, fli_bmp_dirty_from);       /* what part of fli_bitmap is dirty */
AL_VAR(int, fli_bmp_dirty_to);
AL_VAR(int, fli_pal_dirty_from);       /* what part of fli_palette is dirty */
AL_VAR(int, fli_pal_dirty_to);

AL_VAR(int, fli_frame);                /* current frame number */

AL_VAR(volatile int, fli_timer);       /* for timing FLI playback */



/****************************************/
/************ Sound routines ************/
/****************************************/

#define DIGI_VOICES           64       /* Theoretical maximums: */
#define MIDI_VOICES           64       /* actual drivers may not be */
#define MIDI_TRACKS           32       /* able to handle this many */


typedef struct SAMPLE                  /* a sample */
{
   int bits;                           /* 8 or 16 */
   int stereo;                         /* sample type flag */
   int freq;                           /* sample frequency */
   int priority;                       /* 0-255 */
   unsigned long len;                  /* length (in samples) */
   unsigned long loop_start;           /* loop start position */
   unsigned long loop_end;             /* loop finish position */
   unsigned long param;                /* for internal use by the driver */
   void *data;                         /* sample data */
} SAMPLE;


typedef struct MIDI                    /* a midi file */
{
   int divisions;                      /* number of ticks per quarter note */
   struct {
      unsigned char *data;             /* MIDI message stream */
      int len;                         /* length of the track data */
   } track[MIDI_TRACKS]; 
} MIDI;


typedef struct AUDIOSTREAM
{
   int voice;                          /* the voice we are playing on */
   SAMPLE *samp;                       /* the sample we are using */
   int len;                            /* buffer length */
   int bufcount;                       /* number of buffers per sample half */
   int bufnum;                         /* current refill buffer */
   int active;                         /* which half is currently playing */
   void *locked;                       /* the locked buffer */
} AUDIOSTREAM;


#define DIGI_AUTODETECT       -1       /* for passing to install_sound() */
#define DIGI_NONE             0

#define MIDI_AUTODETECT       -1 
#define MIDI_NONE             0 
#define MIDI_DIGMID           AL_ID('D','I','G','I')


typedef struct DIGI_DRIVER             /* driver for playing digital sfx */
{
   int  id;                            /* driver ID code */
   AL_CONST char *name;                /* driver name */
   AL_CONST char *desc;                /* description string */
   AL_CONST char *ascii_name;          /* ASCII format name string */
   int  voices;                        /* available voices */
   int  basevoice;                     /* voice number offset */
   int  max_voices;                    /* maximum voices we can support */
   int  def_voices;                    /* default number of voices to use */

   /* setup routines */
   AL_METHOD(int,  detect, (int input)); 
   AL_METHOD(int,  init, (int input, int voices)); 
   AL_METHOD(void, exit, (int input)); 
   AL_METHOD(int,  mixer_volume, (int volume));

   /* for use by the audiostream functions */
   AL_METHOD(void *, lock_voice, (int voice, int start, int end));
   AL_METHOD(void, unlock_voice, (int voice));
   AL_METHOD(int,  buffer_size, (void));

   /* voice control functions */
   AL_METHOD(void, init_voice, (int voice, AL_CONST SAMPLE *sample));
   AL_METHOD(void, release_voice, (int voice));
   AL_METHOD(void, start_voice, (int voice));
   AL_METHOD(void, stop_voice, (int voice));
   AL_METHOD(void, loop_voice, (int voice, int playmode));

   /* position control functions */
   AL_METHOD(int,  get_position, (int voice));
   AL_METHOD(void, set_position, (int voice, int position));

   /* volume control functions */
   AL_METHOD(int,  get_volume, (int voice));
   AL_METHOD(void, set_volume, (int voice, int volume));
   AL_METHOD(void, ramp_volume, (int voice, int time, int endvol));
   AL_METHOD(void, stop_volume_ramp, (int voice));

   /* pitch control functions */
   AL_METHOD(int,  get_frequency, (int voice));
   AL_METHOD(void, set_frequency, (int voice, int frequency));
   AL_METHOD(void, sweep_frequency, (int voice, int time, int endfreq));
   AL_METHOD(void, stop_frequency_sweep, (int voice));

   /* pan control functions */
   AL_METHOD(int,  get_pan, (int voice));
   AL_METHOD(void, set_pan, (int voice, int pan));
   AL_METHOD(void, sweep_pan, (int voice, int time, int endpan));
   AL_METHOD(void, stop_pan_sweep, (int voice));

   /* effect control functions */
   AL_METHOD(void, set_echo, (int voice, int strength, int delay));
   AL_METHOD(void, set_tremolo, (int voice, int rate, int depth));
   AL_METHOD(void, set_vibrato, (int voice, int rate, int depth));

   /* input functions */
   int rec_cap_bits;
   int rec_cap_stereo;
   AL_METHOD(int,  rec_cap_rate, (int bits, int stereo));
   AL_METHOD(int,  rec_cap_parm, (int rate, int bits, int stereo));
   AL_METHOD(int,  rec_source, (int source));
   AL_METHOD(int,  rec_start, (int rate, int bits, int stereo));
   AL_METHOD(void, rec_stop, (void));
   AL_METHOD(int,  rec_read, (void *buf));
} DIGI_DRIVER;


typedef struct MIDI_DRIVER             /* driver for playing midi music */
{
   int  id;                            /* driver ID code */
   AL_CONST char *name;                /* driver name */
   AL_CONST char *desc;                /* description string */
   AL_CONST char *ascii_name;          /* ASCII format name string */
   int  voices;                        /* available voices */
   int  basevoice;                     /* voice number offset */
   int  max_voices;                    /* maximum voices we can support */
   int  def_voices;                    /* default number of voices to use */
   int  xmin, xmax;                    /* reserved voice range */

   /* setup routines */
   AL_METHOD(int,  detect, (int input));
   AL_METHOD(int,  init, (int input, int voices));
   AL_METHOD(void, exit, (int input));
   AL_METHOD(int,  mixer_volume, (int volume));

   /* raw MIDI output to MPU-401, etc. */
   AL_METHOD(void, raw_midi, (int data));

   /* dynamic patch loading routines */
   AL_METHOD(int,  load_patches, (AL_CONST char *patches, AL_CONST char *drums));
   AL_METHOD(void, adjust_patches, (AL_CONST char *patches, AL_CONST char *drums));

   /* note control functions */
   AL_METHOD(void, key_on, (int inst, int note, int bend, int vol, int pan));
   AL_METHOD(void, key_off, (int voice));
   AL_METHOD(void, set_volume, (int voice, int vol));
   AL_METHOD(void, set_pitch, (int voice, int note, int bend));
   AL_METHOD(void, set_pan, (int voice, int pan));
   AL_METHOD(void, set_vibrato, (int voice, int amount));
} MIDI_DRIVER;


AL_VAR(DIGI_DRIVER, digi_none);
AL_VAR(MIDI_DRIVER, midi_none);
AL_VAR(MIDI_DRIVER, midi_digmid);

AL_ARRAY(_DRIVER_INFO, _digi_driver_list);
AL_ARRAY(_DRIVER_INFO, _midi_driver_list);


/* macros for constructing the driver lists */
#define BEGIN_DIGI_DRIVER_LIST                                 \
   _DRIVER_INFO _digi_driver_list[] =                          \
   {

#define END_DIGI_DRIVER_LIST                                   \
      {  DIGI_NONE,        &digi_none,          TRUE  },       \
      {  0,                NULL,                0     }        \
   };

#define BEGIN_MIDI_DRIVER_LIST                                 \
   _DRIVER_INFO _midi_driver_list[] =                          \
   {

#define END_MIDI_DRIVER_LIST                                   \
      {  MIDI_NONE,        &midi_none,          TRUE  },       \
      {  0,                NULL,                0     }        \
   };

#define MIDI_DRIVER_DIGMID                                     \
      {  MIDI_DIGMID,      &midi_digmid,        TRUE  },


AL_VAR(DIGI_DRIVER *, digi_driver);
AL_VAR(MIDI_DRIVER *, midi_driver);

AL_VAR(DIGI_DRIVER *, digi_input_driver);
AL_VAR(MIDI_DRIVER *, midi_input_driver);

AL_VAR(int, digi_card);
AL_VAR(int, midi_card);

AL_VAR(int, digi_input_card);
AL_VAR(int, midi_input_card);

AL_VAR(volatile long, midi_pos);       /* current position in the midi file */

AL_VAR(long, midi_loop_start);         /* where to loop back to at EOF */
AL_VAR(long, midi_loop_end);           /* loop when we hit this position */

AL_FUNC(int, detect_digi_driver, (int driver_id));
AL_FUNC(int, detect_midi_driver, (int driver_id));

AL_FUNC(void, reserve_voices, (int digi_voices, int midi_voices));
AL_FUNC(void, set_volume_per_voice, (int scale));

AL_FUNC(int, install_sound, (int digi, int midi, AL_CONST char *cfg_path));
AL_FUNC(void, remove_sound, (void));

AL_FUNC(int, install_sound_input, (int digi, int midi));
AL_FUNC(void, remove_sound_input, (void));

AL_FUNC(void, set_volume, (int digi_volume, int midi_volume));

AL_VAR(int, _sound_installed);
AL_VAR(int, _sound_input_installed);

AL_FUNC(SAMPLE *, load_sample, (AL_CONST char *filename));
AL_FUNC(SAMPLE *, load_wav, (AL_CONST char *filename));
AL_FUNC(SAMPLE *, load_voc, (AL_CONST char *filename));
AL_FUNC(SAMPLE *, create_sample, (int bits, int stereo, int freq, int len));
AL_FUNC(void, destroy_sample, (SAMPLE *spl));

AL_FUNC(int, play_sample, (AL_CONST SAMPLE *spl, int vol, int pan, int freq, int loop));
AL_FUNC(void, stop_sample, (AL_CONST SAMPLE *spl));
AL_FUNC(void, adjust_sample, (AL_CONST SAMPLE *spl, int vol, int pan, int freq, int loop));

AL_FUNC(int, allocate_voice, (AL_CONST SAMPLE *spl));
AL_FUNC(void, deallocate_voice, (int voice));
AL_FUNC(void, reallocate_voice, (int voice, AL_CONST SAMPLE *spl));
AL_FUNC(void, release_voice, (int voice));
AL_FUNC(void, voice_start, (int voice));
AL_FUNC(void, voice_stop, (int voice));
AL_FUNC(void, voice_set_priority, (int voice, int priority));
AL_FUNC(SAMPLE *, voice_check, (int voice));

#define PLAYMODE_PLAY           0
#define PLAYMODE_LOOP           1
#define PLAYMODE_FORWARD        0
#define PLAYMODE_BACKWARD       2
#define PLAYMODE_BIDIR          4

AL_FUNC(void, voice_set_playmode, (int voice, int playmode));

AL_FUNC(int, voice_get_position, (int voice));
AL_FUNC(void, voice_set_position, (int voice, int position));

AL_FUNC(int, voice_get_volume, (int voice));
AL_FUNC(void, voice_set_volume, (int voice, int volume));
AL_FUNC(void, voice_ramp_volume, (int voice, int time, int endvol));
AL_FUNC(void, voice_stop_volumeramp, (int voice));

AL_FUNC(int, voice_get_frequency, (int voice));
AL_FUNC(void, voice_set_frequency, (int voice, int frequency));
AL_FUNC(void, voice_sweep_frequency, (int voice, int time, int endfreq));
AL_FUNC(void, voice_stop_frequency_sweep, (int voice));

AL_FUNC(int, voice_get_pan, (int voice));
AL_FUNC(void, voice_set_pan, (int voice, int pan));
AL_FUNC(void, voice_sweep_pan, (int voice, int time, int endpan));
AL_FUNC(void, voice_stop_pan_sweep, (int voice));

AL_FUNC(void, voice_set_echo, (int voice, int strength, int delay));
AL_FUNC(void, voice_set_tremolo, (int voice, int rate, int depth));
AL_FUNC(void, voice_set_vibrato, (int voice, int rate, int depth));

#define SOUND_INPUT_MIC    1
#define SOUND_INPUT_LINE   2
#define SOUND_INPUT_CD     3

AL_FUNC(int, get_sound_input_cap_bits, (void));
AL_FUNC(int, get_sound_input_cap_stereo, (void));
AL_FUNC(int, get_sound_input_cap_rate, (int bits, int stereo));
AL_FUNC(int, get_sound_input_cap_parm, (int rate, int bits, int stereo));
AL_FUNC(int, set_sound_input_source, (int source));
AL_FUNC(int, start_sound_input, (int rate, int bits, int stereo));
AL_FUNC(void, stop_sound_input, (void));
AL_FUNC(int, read_sound_input, (void *buffer));

AL_FUNCPTR(void, digi_recorder, (void));

AL_FUNC(MIDI *, load_midi, (AL_CONST char *filename));
AL_FUNC(void, destroy_midi, (MIDI *midi));
AL_FUNC(int, play_midi, (MIDI *midi, int loop));
AL_FUNC(int, play_looped_midi, (MIDI *midi, int loop_start, int loop_end));
AL_FUNC(void, stop_midi, (void));
AL_FUNC(void, midi_pause, (void));
AL_FUNC(void, midi_resume, (void));
AL_FUNC(int, midi_seek, (int target));
AL_FUNC(void, midi_out, (unsigned char *data, int length));
AL_FUNC(int, load_midi_patches, (void));

AL_FUNCPTR(void, midi_msg_callback, (int msg, int byte1, int byte2));
AL_FUNCPTR(void, midi_meta_callback, (int type, AL_CONST unsigned char *data, int length));
AL_FUNCPTR(void, midi_sysex_callback, (AL_CONST unsigned char *data, int length));

AL_FUNCPTR(void, midi_recorder, (unsigned char data));

AL_FUNC(AUDIOSTREAM *, play_audio_stream, (int len, int bits, int stereo, int freq, int vol, int pan));
AL_FUNC(void, stop_audio_stream, (AUDIOSTREAM *stream));
AL_FUNC(void *, get_audio_stream_buffer, (AUDIOSTREAM *stream));
AL_FUNC(void, free_audio_stream_buffer, (AUDIOSTREAM *stream));



/***********************************************************/
/************ File I/O and compression routines ************/
/***********************************************************/

AL_FUNC(char *, fix_filename_case, (char *path));
AL_FUNC(char *, fix_filename_slashes, (char *path));
AL_FUNC(char *, fix_filename_path, (char *dest, AL_CONST char *path, int size));
AL_FUNC(char *, replace_filename, (char *dest, AL_CONST char *path, AL_CONST char *filename, int size));
AL_FUNC(char *, replace_extension, (char *dest, AL_CONST char *filename, AL_CONST char *ext, int size));
AL_FUNC(char *, append_filename, (char *dest, AL_CONST char *path, AL_CONST char *filename, int size));
AL_FUNC(char *, get_filename, (AL_CONST char *path));
AL_FUNC(char *, get_extension, (AL_CONST char *filename));
AL_FUNC(void, put_backslash, (char *filename));
AL_FUNC(int, file_exists, (AL_CONST char *filename, int attrib, int *aret));
AL_FUNC(int, exists, (AL_CONST char *filename));
AL_FUNC(long, file_size, (AL_CONST char *filename));
AL_FUNC(long, file_time, (AL_CONST char *filename));
AL_FUNC(int, delete_file, (AL_CONST char *filename));
AL_FUNC(int, for_each_file, (AL_CONST char *name, int attrib, AL_METHOD(void, callback, (AL_CONST char *filename, int attrib, int param)), int param));
AL_FUNC(int, find_allegro_resource, (char *dest, AL_CONST char *resource, AL_CONST char *ext, AL_CONST char *datafile, AL_CONST char *objectname, AL_CONST char *envvar, AL_CONST char *subdir, int size));

#ifndef EOF 
   #define EOF    (-1)
#endif

#define F_READ          "r"
#define F_WRITE         "w"
#define F_READ_PACKED   "rp"
#define F_WRITE_PACKED  "wp"
#define F_WRITE_NOPACK  "w!"

#define F_BUF_SIZE      4096           /* 4K buffer for caching data */
#define F_PACK_MAGIC    0x736C6821L    /* magic number for packed files */
#define F_NOPACK_MAGIC  0x736C682EL    /* magic number for autodetect */
#define F_EXE_MAGIC     0x736C682BL    /* magic number for appended data */

#define PACKFILE_FLAG_WRITE      1     /* the file is being written */
#define PACKFILE_FLAG_PACK       2     /* data is compressed */
#define PACKFILE_FLAG_CHUNK      4     /* file is a sub-chunk */
#define PACKFILE_FLAG_EOF        8     /* reached the end-of-file */
#define PACKFILE_FLAG_ERROR      16    /* an error has occurred */
#define PACKFILE_FLAG_OLD_CRYPT  32    /* backward compatibility mode */
#define PACKFILE_FLAG_EXEDAT     64    /* reading from our executable */


typedef struct PACKFILE                /* our very own FILE structure... */
{
   int hndl;                           /* DOS file handle */
   int flags;                          /* PACKFILE_FLAG_* constants */
   unsigned char *buf_pos;             /* position in buffer */
   int buf_size;                       /* number of bytes in the buffer */
   long todo;                          /* number of bytes still on the disk */
   struct PACKFILE *parent;            /* nested, parent file */
   void *pack_data;                    /* for LZSS compression */
   char *filename;                     /* name of the file */
   char *passdata;                     /* encryption key data */
   char *passpos;                      /* current key position */
   unsigned char buf[F_BUF_SIZE];      /* the actual data buffer */
} PACKFILE;


AL_FUNC(void, packfile_password, (AL_CONST char *password));
AL_FUNC(PACKFILE *, pack_fopen, (AL_CONST char *filename, AL_CONST char *mode));
AL_FUNC(int, pack_fclose, (PACKFILE *f));
AL_FUNC(int, pack_fseek, (PACKFILE *f, int offset));
AL_FUNC(PACKFILE *, pack_fopen_chunk, (PACKFILE *f, int pack));
AL_FUNC(PACKFILE *, pack_fclose_chunk, (PACKFILE *f));
AL_FUNC(int, pack_igetw, (PACKFILE *f));
AL_FUNC(long, pack_igetl, (PACKFILE *f));
AL_FUNC(int, pack_iputw, (int w, PACKFILE *f));
AL_FUNC(long, pack_iputl, (long l, PACKFILE *f));
AL_FUNC(int, pack_mgetw, (PACKFILE *f));
AL_FUNC(long, pack_mgetl, (PACKFILE *f));
AL_FUNC(int, pack_mputw, (int w, PACKFILE *f));
AL_FUNC(long, pack_mputl, (long l, PACKFILE *f));
AL_FUNC(long, pack_fread, (void *p, long n, PACKFILE *f));
AL_FUNC(long, pack_fwrite, (void *p, long n, PACKFILE *f));
AL_FUNC(char *, pack_fgets, (char *p, int max, PACKFILE *f));
AL_FUNC(int, pack_fputs, (AL_CONST char *p, PACKFILE *f));

AL_FUNC(int, _sort_out_getc, (PACKFILE *f));
AL_FUNC(int, _sort_out_putc, (int c, PACKFILE *f));

#define pack_feof(f)       ((f)->flags & PACKFILE_FLAG_EOF)
#define pack_ferror(f)     ((f)->flags & PACKFILE_FLAG_ERROR)



/*******************************************/
/************ Datafile routines ************/
/*******************************************/

#define DAT_ID(a,b,c,d)    AL_ID(a,b,c,d)

#define DAT_MAGIC          DAT_ID('A','L','L','.')
#define DAT_FILE           DAT_ID('F','I','L','E')
#define DAT_DATA           DAT_ID('D','A','T','A')
#define DAT_FONT           DAT_ID('F','O','N','T')
#define DAT_SAMPLE         DAT_ID('S','A','M','P')
#define DAT_MIDI           DAT_ID('M','I','D','I')
#define DAT_PATCH          DAT_ID('P','A','T',' ')
#define DAT_FLI            DAT_ID('F','L','I','C')
#define DAT_BITMAP         DAT_ID('B','M','P',' ')
#define DAT_RLE_SPRITE     DAT_ID('R','L','E',' ')
#define DAT_C_SPRITE       DAT_ID('C','M','P',' ')
#define DAT_XC_SPRITE      DAT_ID('X','C','M','P')
#define DAT_PALETTE        DAT_ID('P','A','L',' ')
#define DAT_PROPERTY       DAT_ID('p','r','o','p')
#define DAT_NAME           DAT_ID('N','A','M','E')
#define DAT_END            -1


typedef struct DATAFILE_PROPERTY
{
   char *dat;                          /* pointer to the data */
   int type;                           /* property type */
} DATAFILE_PROPERTY;


typedef struct DATAFILE
{
   void *dat;                          /* pointer to the data */
   int type;                           /* object type */
   long size;                          /* size of the object */
   DATAFILE_PROPERTY *prop;            /* object properties */
} DATAFILE;


AL_FUNC(DATAFILE *, load_datafile, (AL_CONST char *filename));
AL_FUNC(DATAFILE *, load_datafile_callback, (AL_CONST char *filename, AL_METHOD(void, callback, (DATAFILE *))));
AL_FUNC(void, unload_datafile, (DATAFILE *dat));

AL_FUNC(DATAFILE *, load_datafile_object, (AL_CONST char *filename, AL_CONST char *objectname));
AL_FUNC(void, unload_datafile_object, (DATAFILE *dat));

AL_FUNC(DATAFILE *, find_datafile_object, (AL_CONST DATAFILE *dat, AL_CONST char *objectname));
AL_FUNC(AL_CONST char *, get_datafile_property, (AL_CONST DATAFILE *dat, int type));
AL_FUNC(void, register_datafile_object, (int id, AL_METHOD(void *, load, (PACKFILE *f, long size)), AL_METHOD(void, destroy, (void *data))));

AL_FUNC(void, fixup_datafile, (DATAFILE *data));

AL_FUNC(BITMAP *, load_bitmap, (AL_CONST char *filename, RGB *pal));
AL_FUNC(BITMAP *, load_bmp, (AL_CONST char *filename, RGB *pal));
AL_FUNC(BITMAP *, load_lbm, (AL_CONST char *filename, RGB *pal));
AL_FUNC(BITMAP *, load_pcx, (AL_CONST char *filename, RGB *pal));
AL_FUNC(BITMAP *, load_tga, (AL_CONST char *filename, RGB *pal));

AL_FUNC(int, save_bitmap, (AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal));
AL_FUNC(int, save_bmp, (AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal));
AL_FUNC(int, save_pcx, (AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal));
AL_FUNC(int, save_tga, (AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal));

AL_FUNC(void, register_bitmap_file_type, (AL_CONST char *ext, AL_METHOD(BITMAP *, load, (AL_CONST char *filename, RGB *pal)), AL_METHOD(int, save, (AL_CONST char *filename, BITMAP *bmp, AL_CONST RGB *pal))));



/***************************************/
/************ Math routines ************/
/***************************************/

AL_FUNC(fixed, fsqrt, (fixed x));
AL_FUNC(fixed, fhypot, (fixed x, fixed y));
AL_FUNC(fixed, fatan, (fixed x));
AL_FUNC(fixed, fatan2, (fixed y, fixed x));

AL_ARRAY(fixed, _cos_tbl);
AL_ARRAY(fixed, _tan_tbl);
AL_ARRAY(fixed, _acos_tbl);


typedef struct MATRIX            /* transformation matrix (fixed point) */
{
   fixed v[3][3];                /* scaling and rotation */
   fixed t[3];                   /* translation */
} MATRIX;


typedef struct MATRIX_f          /* transformation matrix (floating point) */
{
   float v[3][3];                /* scaling and rotation */
   float t[3];                   /* translation */
} MATRIX_f;


AL_VAR(MATRIX, identity_matrix);
AL_VAR(MATRIX_f, identity_matrix_f);

AL_FUNC(void, get_translation_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, get_translation_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, get_scaling_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, get_scaling_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, get_x_rotate_matrix, (MATRIX *m, fixed r));
AL_FUNC(void, get_x_rotate_matrix_f, (MATRIX_f *m, float r));

AL_FUNC(void, get_y_rotate_matrix, (MATRIX *m, fixed r));
AL_FUNC(void, get_y_rotate_matrix_f, (MATRIX_f *m, float r));

AL_FUNC(void, get_z_rotate_matrix, (MATRIX *m, fixed r));
AL_FUNC(void, get_z_rotate_matrix_f, (MATRIX_f *m, float r));

AL_FUNC(void, get_rotation_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, get_rotation_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, get_align_matrix, (MATRIX *m, fixed xfront, fixed yfront, fixed zfront, fixed xup, fixed yup, fixed zup));
AL_FUNC(void, get_align_matrix_f, (MATRIX_f *m, float xfront, float yfront, float zfront, float xup, float yup, float zup));

AL_FUNC(void, get_vector_rotation_matrix, (MATRIX *m, fixed x, fixed y, fixed z, fixed a));
AL_FUNC(void, get_vector_rotation_matrix_f, (MATRIX_f *m, float x, float y, float z, float a));

AL_FUNC(void, get_transformation_matrix, (MATRIX *m, fixed scale, fixed xrot, fixed yrot, fixed zrot, fixed x, fixed y, fixed z));
AL_FUNC(void, get_transformation_matrix_f, (MATRIX_f *m, float scale, float xrot, float yrot, float zrot, float x, float y, float z));

AL_FUNC(void, get_camera_matrix, (MATRIX *m, fixed x, fixed y, fixed z, fixed xfront, fixed yfront, fixed zfront, fixed xup, fixed yup, fixed zup, fixed fov, fixed aspect));
AL_FUNC(void, get_camera_matrix_f, (MATRIX_f *m, float x, float y, float z, float xfront, float yfront, float zfront, float xup, float yup, float zup, float fov, float aspect));

AL_FUNC(void, qtranslate_matrix, (MATRIX *m, fixed x, fixed y, fixed z));
AL_FUNC(void, qtranslate_matrix_f, (MATRIX_f *m, float x, float y, float z));

AL_FUNC(void, qscale_matrix, (MATRIX *m, fixed scale));
AL_FUNC(void, qscale_matrix_f, (MATRIX_f *m, float scale));

AL_FUNC(void, matrix_mul, (AL_CONST MATRIX *m1, AL_CONST MATRIX *m2, MATRIX *out));
AL_FUNC(void, matrix_mul_f, (AL_CONST MATRIX_f *m1, AL_CONST MATRIX_f *m2, MATRIX_f *out));

AL_FUNC(fixed, vector_length, (fixed x, fixed y, fixed z));
AL_FUNC(float, vector_length_f, (float x, float y, float z));

AL_FUNC(void, normalize_vector, (fixed *x, fixed *y, fixed *z));
AL_FUNC(void, normalize_vector_f, (float *x, float *y, float *z));

AL_FUNC(void, cross_product, (fixed x1, fixed y1, fixed z1, fixed x2, fixed y2, fixed z2, fixed *xout, fixed *yout, fixed *zout));
AL_FUNC(void, cross_product_f, (float x1, float y1, float z1, float x2, float y2, float z2, float *xout, float *yout, float *zout));

AL_FUNC(fixed, polygon_z_normal, (AL_CONST V3D *v1, AL_CONST V3D *v2, AL_CONST V3D *v3));
AL_FUNC(float, polygon_z_normal_f, (AL_CONST V3D_f *v1, AL_CONST V3D_f *v2, AL_CONST V3D_f *v3));

AL_FUNC(void, apply_matrix_f, (AL_CONST MATRIX_f *m, float x, float y, float z, float *xout, float *yout, float *zout));

AL_VAR(fixed, _persp_xscale);
AL_VAR(fixed, _persp_yscale);
AL_VAR(fixed, _persp_xoffset);
AL_VAR(fixed, _persp_yoffset);

AL_VAR(float, _persp_xscale_f);
AL_VAR(float, _persp_yscale_f);
AL_VAR(float, _persp_xoffset_f);
AL_VAR(float, _persp_yoffset_f);

AL_FUNC(void, set_projection_viewport, (int x, int y, int w, int h));


typedef struct QUAT
{
   float w, x, y, z;
} QUAT;


AL_VAR(QUAT, identity_quat);

AL_FUNC(void, quat_mul, (AL_CONST QUAT *p, AL_CONST QUAT *q, QUAT *out));
AL_FUNC(void, get_x_rotate_quat, (QUAT *q, float r));
AL_FUNC(void, get_y_rotate_quat, (QUAT *q, float r));
AL_FUNC(void, get_z_rotate_quat, (QUAT *q, float r));
AL_FUNC(void, get_rotation_quat, (QUAT *q, float x, float y, float z));
AL_FUNC(void, get_vector_rotation_quat, (QUAT *q, float x, float y, float z, float a));
AL_FUNC(void, quat_to_matrix, (AL_CONST QUAT *q, MATRIX_f *m));
AL_FUNC(void, matrix_to_quat, (AL_CONST MATRIX_f *m, QUAT *q));
AL_FUNC(void, apply_quat, (AL_CONST QUAT *q, float x, float y, float z, float *xout, float *yout, float *zout));
AL_FUNC(void, quat_slerp, (AL_CONST QUAT *from, AL_CONST QUAT *to, float t, QUAT *out, int how));

#define QUAT_SHORT   0
#define QUAT_LONG    1
#define QUAT_CW      2
#define QUAT_CCW     3
#define QUAT_USER    4

#define quat_interpolate(from, to, t, out)   quat_slerp((from), (to), (t), (out), QUAT_SHORT)



/***************************************/
/************ GUI routines  ************/
/***************************************/

struct DIALOG;

typedef AL_METHOD(int, DIALOG_PROC, (int msg, struct DIALOG *d, int c));

typedef struct DIALOG 
{
   DIALOG_PROC proc;
   int x, y, w, h;               /* position and size of the object */
   int fg, bg;                   /* foreground and background colors */
   int key;                      /* keyboard shortcut (ASCII code) */
   int flags;                    /* flags about the object state */
   int d1, d2;                   /* any data the object might require */
   void *dp, *dp2, *dp3;         /* pointers to more object data */
} DIALOG;


/* a popup menu */
typedef struct MENU
{
   char *text;                   /* menu item text */
   AL_METHOD(int, proc, (void)); /* callback function */
   struct MENU *child;           /* to allow nested menus */
   int flags;                    /* flags about the menu state */
   void *dp;                     /* any data the menu might require */
} MENU;


/* stored information about the state of an active GUI dialog */
typedef struct DIALOG_PLAYER
{
   int obj;
   int res;
   int mouse_obj;
   int focus_obj;
   int joy_on;
   int click_wait;
   int mouse_ox, mouse_oy;
   int mouse_oz;
   int mouse_b;
   DIALOG *dialog;
} DIALOG_PLAYER;


/* bits for the flags field */
#define D_EXIT          1        /* object makes the dialog exit */
#define D_SELECTED      2        /* object is selected */
#define D_GOTFOCUS      4        /* object has the input focus */
#define D_GOTMOUSE      8        /* mouse is on top of object */
#define D_HIDDEN        16       /* object is not visible */
#define D_DISABLED      32       /* object is visible but inactive */
#define D_DIRTY         64       /* object needs to be redrawn */
#define D_INTERNAL      128      /* reserved for internal use */
#define D_USER          256      /* from here on is free for your own use */


/* return values for the dialog procedures */
#define D_O_K           0        /* normal exit status */
#define D_CLOSE         1        /* request to close the dialog */
#define D_REDRAW        2        /* request to redraw the dialog */
#define D_REDRAWME      4        /* request to redraw this object */
#define D_WANTFOCUS     8        /* this object wants the input focus */
#define D_USED_CHAR     16       /* object has used the keypress */


/* messages for the dialog procedures */
#define MSG_START       1        /* start the dialog, initialise */
#define MSG_END         2        /* dialog is finished - cleanup */
#define MSG_DRAW        3        /* draw the object */
#define MSG_CLICK       4        /* mouse click on the object */
#define MSG_DCLICK      5        /* double click on the object */
#define MSG_KEY         6        /* keyboard shortcut */
#define MSG_CHAR        7        /* other keyboard input */
#define MSG_UCHAR       8        /* unicode keyboard input */
#define MSG_XCHAR       9        /* broadcast character to all objects */
#define MSG_WANTFOCUS   10       /* does object want the input focus? */
#define MSG_GOTFOCUS    11       /* got the input focus */
#define MSG_LOSTFOCUS   12       /* lost the input focus */
#define MSG_GOTMOUSE    13       /* mouse on top of object */
#define MSG_LOSTMOUSE   14       /* mouse moved away from object */
#define MSG_IDLE        15       /* update any background stuff */
#define MSG_RADIO       16       /* clear radio buttons */
#define MSG_WHEEL       17       /* mouse wheel moved */
#define MSG_LPRESS      18       /* mouse left button pressed */
#define MSG_LRELEASE    19       /* mouse left button released */
#define MSG_MPRESS      20       /* mouse middle button pressed */
#define MSG_MRELEASE    21       /* mouse middle button released */
#define MSG_RPRESS      22       /* mouse right button pressed */
#define MSG_RRELEASE    23       /* mouse right button released */
#define MSG_USER        24       /* from here on are free... */


/* some dialog procedures */
AL_FUNC(int, d_yield_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_clear_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_box_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_shadow_box_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_bitmap_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_text_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_ctext_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_rtext_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_button_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_check_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_radio_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_icon_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_keyboard_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_edit_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_list_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_text_list_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_textbox_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_slider_proc, (int msg, DIALOG *d, int c));
AL_FUNC(int, d_menu_proc, (int msg, DIALOG *d, int c));
       
AL_VAR(DIALOG_PROC, gui_shadow_box_proc);
AL_VAR(DIALOG_PROC, gui_ctext_proc);
AL_VAR(DIALOG_PROC, gui_button_proc);
AL_VAR(DIALOG_PROC, gui_edit_proc);
AL_VAR(DIALOG_PROC, gui_list_proc);
AL_VAR(DIALOG_PROC, gui_text_list_proc);

AL_VAR(DIALOG *, active_dialog);
AL_VAR(MENU *, active_menu);

AL_VAR(int, gui_mouse_focus);

AL_VAR(int, gui_fg_color);
AL_VAR(int, gui_mg_color);
AL_VAR(int, gui_bg_color);

AL_VAR(int, gui_font_baseline);

AL_FUNCPTR(int, gui_mouse_x, (void));
AL_FUNCPTR(int, gui_mouse_y, (void));
AL_FUNCPTR(int, gui_mouse_z, (void));
AL_FUNCPTR(int, gui_mouse_b, (void));

AL_FUNC(int, gui_textout, (BITMAP *bmp, AL_CONST char *s, int x, int y, int color, int centre));
AL_FUNC(int, gui_strlen, (AL_CONST char *s));
AL_FUNC(void, position_dialog, (DIALOG *dialog, int x, int y));
AL_FUNC(void, centre_dialog, (DIALOG *dialog));
AL_FUNC(void, set_dialog_color, (DIALOG *dialog, int fg, int bg));
AL_FUNC(int, find_dialog_focus, (DIALOG *dialog));
AL_FUNC(int, offer_focus, (DIALOG *d, int obj, int *focus_obj, int force));
AL_FUNC(int, dialog_message, (DIALOG *dialog, int msg, int c, int *obj));
AL_FUNC(int, broadcast_dialog_message, (int msg, int c));
AL_FUNC(int, do_dialog, (DIALOG *dialog, int focus_obj));
AL_FUNC(int, popup_dialog, (DIALOG *dialog, int focus_obj));
AL_FUNC(DIALOG_PLAYER *, init_dialog, (DIALOG *dialog, int focus_obj));
AL_FUNC(int, update_dialog, (DIALOG_PLAYER *player));
AL_FUNC(int, shutdown_dialog, (DIALOG_PLAYER *player));
AL_FUNC(int, do_menu, (MENU *menu, int x, int y));
AL_FUNC(int, alert, (AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, int c1, int c2));
AL_FUNC(int, alert3, (AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, AL_CONST char *b3, int c1, int c2, int c3));
AL_FUNC(int, file_select, (AL_CONST char *message, char *path, AL_CONST char *ext));
AL_FUNC(int, file_select_ex, (AL_CONST char *message, char *path, AL_CONST char *ext, int w, int h));
AL_FUNC(int, gfx_mode_select, (int *card, int *w, int *h));
AL_FUNC(int, gfx_mode_select_ex, (int *card, int *w, int *h, int *color_depth));



/*****************************************/
/************ Helper includes ************/
/*****************************************/

#include "allegro/alinline.h"

#ifdef ALLEGRO_EXTRA_HEADER
   #include ALLEGRO_EXTRA_HEADER
#endif


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_H */


