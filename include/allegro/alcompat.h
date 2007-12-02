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
 *      Backward compatibility stuff.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_COMPAT_H
#define ALLEGRO_COMPAT_H

#ifdef __cplusplus
   extern "C" {
#endif


#ifndef ALLEGRO_LIB_BUILD

   #ifndef ALLEGRO_NO_CLEAR_BITMAP_ALIAS
      #if (defined ALLEGRO_GCC)
         static __attribute__((unused)) __inline__ void clear(BITMAP *bmp)
         {
            clear_bitmap(bmp);
         }
      #else
         static INLINE void clear(BITMAP *bmp)
         {
            clear_bitmap(bmp);
         }
      #endif
   #endif

   #ifndef ALLEGRO_NO_FIX_ALIASES
      AL_ALIAS(fixed fadd(fixed x, fixed y), fixadd(x, y))
      AL_ALIAS(fixed fsub(fixed x, fixed y), fixsub(x, y))
      AL_ALIAS(fixed fmul(fixed x, fixed y), fixmul(x, y))
      AL_ALIAS(fixed fdiv(fixed x, fixed y), fixdiv(x, y))
      AL_ALIAS(int fceil(fixed x), fixceil(x))
      AL_ALIAS(int ffloor(fixed x), fixfloor(x))
      AL_ALIAS(fixed fcos(fixed x), fixcos(x))
      AL_ALIAS(fixed fsin(fixed x), fixsin(x))
      AL_ALIAS(fixed ftan(fixed x), fixtan(x))
      AL_ALIAS(fixed facos(fixed x), fixacos(x))
      AL_ALIAS(fixed fasin(fixed x), fixasin(x))
      AL_ALIAS(fixed fatan(fixed x), fixatan(x))
      AL_ALIAS(fixed fatan2(fixed y, fixed x), fixatan2(y, x))
      AL_ALIAS(fixed fsqrt(fixed x), fixsqrt(x))
      AL_ALIAS(fixed fhypot(fixed x, fixed y), fixhypot(x, y))
   #endif

#endif  /* !defined ALLEGRO_LIB_BUILD */


#define KB_NORMAL       1
#define KB_EXTENDED     2

#define SEND_MESSAGE    object_message

#define cpu_fpu         (cpu_capabilities & CPU_FPU)
#define cpu_mmx         (cpu_capabilities & CPU_MMX)
#define cpu_3dnow       (cpu_capabilities & CPU_3DNOW)
#define cpu_cpuid       (cpu_capabilities & CPU_ID)

#define joy_x           (joy[0].stick[0].axis[0].pos)
#define joy_y           (joy[0].stick[0].axis[1].pos)
#define joy_left        (joy[0].stick[0].axis[0].d1)
#define joy_right       (joy[0].stick[0].axis[0].d2)
#define joy_up          (joy[0].stick[0].axis[1].d1)
#define joy_down        (joy[0].stick[0].axis[1].d2)
#define joy_b1          (joy[0].button[0].b)
#define joy_b2          (joy[0].button[1].b)
#define joy_b3          (joy[0].button[2].b)
#define joy_b4          (joy[0].button[3].b)
#define joy_b5          (joy[0].button[4].b)
#define joy_b6          (joy[0].button[5].b)
#define joy_b7          (joy[0].button[6].b)
#define joy_b8          (joy[0].button[7].b)

#define joy2_x          (joy[1].stick[0].axis[0].pos)
#define joy2_y          (joy[1].stick[0].axis[1].pos)
#define joy2_left       (joy[1].stick[0].axis[0].d1)
#define joy2_right      (joy[1].stick[0].axis[0].d2)
#define joy2_up         (joy[1].stick[0].axis[1].d1)
#define joy2_down       (joy[1].stick[0].axis[1].d2)
#define joy2_b1         (joy[1].button[0].b)
#define joy2_b2         (joy[1].button[1].b)

#define joy_throttle    (joy[0].stick[2].axis[0].pos)

#define joy_hat         ((joy[0].stick[1].axis[0].d1) ? 1 :             \
                           ((joy[0].stick[1].axis[0].d2) ? 3 :          \
                              ((joy[0].stick[1].axis[1].d1) ? 4 :       \
                                 ((joy[0].stick[1].axis[1].d2) ? 2 :    \
                                    0))))

#define JOY_HAT_CENTRE        0
#define JOY_HAT_CENTER        0
#define JOY_HAT_LEFT          1
#define JOY_HAT_DOWN          2
#define JOY_HAT_RIGHT         3
#define JOY_HAT_UP            4

AL_FUNC_DEPRECATED(int, initialise_joystick, (void));


/* in case you want to spell 'palette' as 'pallete' */
#define PALLETE                        PALETTE
#define black_pallete                  black_palette
#define desktop_pallete                desktop_palette
#define set_pallete                    set_palette
#define get_pallete                    get_palette
#define set_pallete_range              set_palette_range
#define get_pallete_range              get_palette_range
#define fli_pallete                    fli_palette
#define pallete_color                  palette_color
#define DAT_PALLETE                    DAT_PALETTE
#define select_pallete                 select_palette
#define unselect_pallete               unselect_palette
#define generate_332_pallete           generate_332_palette
#define generate_optimised_pallete     generate_optimised_palette


/* a pretty vague name */
#define fix_filename_path              canonicalize_filename


/* the good old file selector */
#define OLD_FILESEL_WIDTH   -1
#define OLD_FILESEL_HEIGHT  -1

AL_INLINE_DEPRECATED(int, file_select, (AL_CONST char *message, char *path, AL_CONST char *ext),
{
   return file_select_ex(message, path, ext, 1024, OLD_FILESEL_WIDTH, OLD_FILESEL_HEIGHT);
})


/* the old (and broken!) file enumeration function */
AL_FUNC_DEPRECATED(int, for_each_file, (AL_CONST char *name, int attrib, AL_METHOD(void, callback, (AL_CONST char *filename, int attrib, int param)), int param));
/* long is 32-bit only on some systems, and we want to list DVDs! */
AL_FUNC_DEPRECATED(long, file_size, (AL_CONST char *filename));


/* the old state-based textout functions */
AL_VAR(int, _textmode);
AL_FUNC_DEPRECATED(int, text_mode, (int mode));

AL_INLINE_DEPRECATED(void, textout, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color),
{
   textout_ex(bmp, f, str, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(void, textout_centre, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color),
{
   textout_centre_ex(bmp, f, str, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(void, textout_right, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x, int y, int color),
{
   textout_right_ex(bmp, f, str, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(void, textout_justify, (struct BITMAP *bmp, AL_CONST FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff, int color),
{
   textout_justify_ex(bmp, f, str, x1, x2, y, diff, color, _textmode);
})

AL_PRINTFUNC_DEPRECATED(void, textprintf, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC_DEPRECATED(void, textprintf_centre, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC_DEPRECATED(void, textprintf_right, (struct BITMAP *bmp, AL_CONST FONT *f, int x, int y, int color, AL_CONST char *format, ...), 6, 7);
AL_PRINTFUNC_DEPRECATED(void, textprintf_justify, (struct BITMAP *bmp, AL_CONST FONT *f, int x1, int x2, int y, int diff, int color, AL_CONST char *format, ...), 8, 9);

AL_INLINE_DEPRECATED(void, draw_character, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color),
{
   draw_character_ex(bmp, sprite, x, y, color, _textmode);
})

AL_INLINE_DEPRECATED(int, gui_textout, (struct BITMAP *bmp, AL_CONST char *s, int x, int y, int color, int centre),
{
   return gui_textout_ex(bmp, s, x, y, color, _textmode, centre);
})


/* the old close button functions */
AL_INLINE_DEPRECATED(int, set_window_close_button, (int enable),
{
   (void)enable;
   return 0;
})

AL_INLINE_DEPRECATED(void, set_window_close_hook, (void (*proc)(void)),
{
   set_close_button_callback(proc);
})


/* the weird old clipping API */
AL_FUNC_DEPRECATED(void, set_clip, (BITMAP *bitmap, int x1, int y_1, int x2, int y2));


/* unnecessary, can use rest(0) */
AL_INLINE_DEPRECATED(void, yield_timeslice, (void),
{
   ASSERT(system_driver);

   if (system_driver->yield_timeslice)
      system_driver->yield_timeslice();
})



/*****************************************************************************
 * The old timer API
 *****************************************************************************
 *
 * Note: TIMERS_PER_SECOND was changed to a nicer value (was 1193181L).
 * The old timer API is emulated using the new timer API, which works with
 * milliseconds.  With the previous value of TIMERS_PER_SECOND, the two-step
 * conversion to "timers" then to milliseconds would introduce rounding
 * errors, e.g. causing the three extimer counters to go badly out of sync.
 */

#define TIMERS_PER_SECOND     1000000L
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
} TIMER_DRIVER;

AL_VAR(TIMER_DRIVER *, timer_driver);

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

#ifdef ALLEGRO_LIB_BUILD
   AL_FUNC(int,  timer_can_simulate_retrace, (void));
   AL_FUNC(void, timer_simulate_retrace, (int enable));
#else
   /* deprecated since 4.1.16 */
   AL_FUNC_DEPRECATED(int,  timer_can_simulate_retrace, (void));
   AL_FUNC_DEPRECATED(void, timer_simulate_retrace, (int enable));
#endif
AL_FUNC(int,  timer_is_using_retrace, (void));

AL_FUNC(void, rest, (unsigned int tyme));
AL_FUNC(void, rest_callback, (unsigned int tyme, AL_METHOD(void, callback, (void))));



/*****************************************************************************
 * The old joystick API
 *****************************************************************************/

#define JOY_TYPE_AUTODETECT      -1
#define JOY_TYPE_NONE            0


#define MAX_JOYSTICKS            8
#define MAX_JOYSTICK_AXIS        _AL_MAX_JOYSTICK_AXES
#define MAX_JOYSTICK_STICKS      _AL_MAX_JOYSTICK_STICKS
#define MAX_JOYSTICK_BUTTONS     _AL_MAX_JOYSTICK_BUTTONS


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
} JOYSTICK_DRIVER;


/* AL_VAR(JOYSTICK_DRIVER, joystick_none); */
AL_VAR(JOYSTICK_DRIVER *, joystick_driver);


/* these were internals; I dunno where to put them --pw */
AL_VAR(int, _joy_type);
AL_VAR(int, _joystick_installed);


AL_FUNC(int, install_joystick, (int type));
AL_FUNC(void, remove_joystick, (void));

AL_FUNC(int, poll_joystick, (void));

AL_FUNC(int, save_joystick_data, (AL_CONST char *filename));
AL_FUNC(int, load_joystick_data, (AL_CONST char *filename));

AL_FUNC(AL_CONST char *, calibrate_joystick_name, (int n));
AL_FUNC(int, calibrate_joystick, (int n));



/*****************************************************************************
 * The old keyboard API
 *****************************************************************************/

typedef struct KEYBOARD_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
} KEYBOARD_DRIVER;


AL_VAR(KEYBOARD_DRIVER *, keyboard_driver);

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

#define three_finger_flag  _al_three_finger_flag
#define key_led_flag	   _al_key_led_flag

AL_FUNC(int, keypressed, (void));
AL_FUNC(int, readkey, (void));
AL_FUNC(int, ureadkey, (int *scancode));
AL_FUNC(void, simulate_keypress, (int keycode));
AL_FUNC(void, simulate_ukeypress, (int keycode, int scancode));
AL_FUNC(void, clear_keybuf, (void));
AL_FUNC(void, set_leds, (int leds));
AL_FUNC(void, set_keyboard_rate, (int delay, int repeat));
AL_FUNC(int, scancode_to_ascii, (int scancode));
AL_FUNC(AL_CONST char *, scancode_to_name, (int scancode));

#ifndef ALLEGRO_NO_KEY_DEFINES

#define KB_SHIFT_FLAG         ALLEGRO_KEYMOD_SHIFT
#define KB_CTRL_FLAG          ALLEGRO_KEYMOD_CTRL
#define KB_ALT_FLAG           (ALLEGRO_KEYMOD_ALT | ALLEGRO_KEYMOD_ALTGR)
#define KB_LWIN_FLAG          ALLEGRO_KEYMOD_LWIN
#define KB_RWIN_FLAG          ALLEGRO_KEYMOD_RWIN
#define KB_MENU_FLAG          ALLEGRO_KEYMOD_MENU
#define KB_COMMAND_FLAG       ALLEGRO_KEYMOD_COMMAND
#define KB_SCROLOCK_FLAG      ALLEGRO_KEYMOD_SCROLLLOCK
#define KB_NUMLOCK_FLAG       ALLEGRO_KEYMOD_NUMLOCK
#define KB_CAPSLOCK_FLAG      ALLEGRO_KEYMOD_CAPSLOCK
#define KB_INALTSEQ_FLAG      ALLEGRO_KEYMOD_INALTSEQ
#define KB_ACCENT1_FLAG       ALLEGRO_KEYMOD_ACCENT1
#define KB_ACCENT2_FLAG       ALLEGRO_KEYMOD_ACCENT2
#define KB_ACCENT3_FLAG       ALLEGRO_KEYMOD_ACCENT3
#define KB_ACCENT4_FLAG       ALLEGRO_KEYMOD_ACCENT4

#define KEY_A                 ALLEGRO_KEY_A
#define KEY_B                 ALLEGRO_KEY_B
#define KEY_C                 ALLEGRO_KEY_C
#define KEY_D                 ALLEGRO_KEY_D
#define KEY_E                 ALLEGRO_KEY_E
#define KEY_F                 ALLEGRO_KEY_F
#define KEY_G                 ALLEGRO_KEY_G
#define KEY_H                 ALLEGRO_KEY_H
#define KEY_I                 ALLEGRO_KEY_I
#define KEY_J                 ALLEGRO_KEY_J
#define KEY_K                 ALLEGRO_KEY_K
#define KEY_L                 ALLEGRO_KEY_L
#define KEY_M                 ALLEGRO_KEY_M
#define KEY_N                 ALLEGRO_KEY_N
#define KEY_O                 ALLEGRO_KEY_O
#define KEY_P                 ALLEGRO_KEY_P
#define KEY_Q                 ALLEGRO_KEY_Q
#define KEY_R                 ALLEGRO_KEY_R
#define KEY_S                 ALLEGRO_KEY_S
#define KEY_T                 ALLEGRO_KEY_T
#define KEY_U                 ALLEGRO_KEY_U
#define KEY_V                 ALLEGRO_KEY_V
#define KEY_W                 ALLEGRO_KEY_W
#define KEY_X                 ALLEGRO_KEY_X
#define KEY_Y                 ALLEGRO_KEY_Y
#define KEY_Z                 ALLEGRO_KEY_Z
#define KEY_0                 ALLEGRO_KEY_0
#define KEY_1                 ALLEGRO_KEY_1
#define KEY_2                 ALLEGRO_KEY_2
#define KEY_3                 ALLEGRO_KEY_3
#define KEY_4                 ALLEGRO_KEY_4
#define KEY_5                 ALLEGRO_KEY_5
#define KEY_6                 ALLEGRO_KEY_6
#define KEY_7                 ALLEGRO_KEY_7
#define KEY_8                 ALLEGRO_KEY_8
#define KEY_9                 ALLEGRO_KEY_9
#define KEY_0_PAD             ALLEGRO_KEY_PAD_0
#define KEY_1_PAD             ALLEGRO_KEY_PAD_1
#define KEY_2_PAD             ALLEGRO_KEY_PAD_2
#define KEY_3_PAD             ALLEGRO_KEY_PAD_3
#define KEY_4_PAD             ALLEGRO_KEY_PAD_4
#define KEY_5_PAD             ALLEGRO_KEY_PAD_5
#define KEY_6_PAD             ALLEGRO_KEY_PAD_6
#define KEY_7_PAD             ALLEGRO_KEY_PAD_7
#define KEY_8_PAD             ALLEGRO_KEY_PAD_8
#define KEY_9_PAD             ALLEGRO_KEY_PAD_9
#define KEY_F1                ALLEGRO_KEY_F1
#define KEY_F2                ALLEGRO_KEY_F2
#define KEY_F3                ALLEGRO_KEY_F3
#define KEY_F4                ALLEGRO_KEY_F4
#define KEY_F5                ALLEGRO_KEY_F5
#define KEY_F6                ALLEGRO_KEY_F6
#define KEY_F7                ALLEGRO_KEY_F7
#define KEY_F8                ALLEGRO_KEY_F8
#define KEY_F9                ALLEGRO_KEY_F9
#define KEY_F10               ALLEGRO_KEY_F10
#define KEY_F11               ALLEGRO_KEY_F11
#define KEY_F12               ALLEGRO_KEY_F12
#define KEY_ESC               ALLEGRO_KEY_ESCAPE
#define KEY_TILDE             ALLEGRO_KEY_TILDE
#define KEY_MINUS             ALLEGRO_KEY_MINUS
#define KEY_EQUALS            ALLEGRO_KEY_EQUALS
#define KEY_BACKSPACE         ALLEGRO_KEY_BACKSPACE
#define KEY_TAB               ALLEGRO_KEY_TAB
#define KEY_OPENBRACE         ALLEGRO_KEY_OPENBRACE
#define KEY_CLOSEBRACE        ALLEGRO_KEY_CLOSEBRACE
#define KEY_ENTER             ALLEGRO_KEY_ENTER
#define KEY_COLON             ALLEGRO_KEY_SEMICOLON
#define KEY_QUOTE             ALLEGRO_KEY_QUOTE
#define KEY_BACKSLASH         ALLEGRO_KEY_BACKSLASH
#define KEY_BACKSLASH2        ALLEGRO_KEY_BACKSLASH2
#define KEY_COMMA             ALLEGRO_KEY_COMMA
#define KEY_STOP              ALLEGRO_KEY_FULLSTOP
#define KEY_SLASH             ALLEGRO_KEY_SLASH
#define KEY_SPACE             ALLEGRO_KEY_SPACE
#define KEY_INSERT            ALLEGRO_KEY_INSERT
#define KEY_DEL               ALLEGRO_KEY_DELETE
#define KEY_HOME              ALLEGRO_KEY_HOME
#define KEY_END               ALLEGRO_KEY_END
#define KEY_PGUP              ALLEGRO_KEY_PGUP
#define KEY_PGDN              ALLEGRO_KEY_PGDN
#define KEY_LEFT              ALLEGRO_KEY_LEFT
#define KEY_RIGHT             ALLEGRO_KEY_RIGHT
#define KEY_UP                ALLEGRO_KEY_UP
#define KEY_DOWN              ALLEGRO_KEY_DOWN
#define KEY_SLASH_PAD         ALLEGRO_KEY_PAD_SLASH
#define KEY_ASTERISK          ALLEGRO_KEY_PAD_ASTERISK
#define KEY_MINUS_PAD         ALLEGRO_KEY_PAD_MINUS
#define KEY_PLUS_PAD          ALLEGRO_KEY_PAD_PLUS
#define KEY_DEL_PAD           ALLEGRO_KEY_PAD_DELETE
#define KEY_ENTER_PAD         ALLEGRO_KEY_PAD_ENTER
#define KEY_PRTSCR            ALLEGRO_KEY_PRINTSCREEN
#define KEY_PAUSE             ALLEGRO_KEY_PAUSE
#define KEY_ABNT_C1           ALLEGRO_KEY_ABNT_C1
#define KEY_YEN               ALLEGRO_KEY_YEN
#define KEY_KANA              ALLEGRO_KEY_KANA
#define KEY_CONVERT           ALLEGRO_KEY_CONVERT
#define KEY_NOCONVERT         ALLEGRO_KEY_NOCONVERT
#define KEY_AT                ALLEGRO_KEY_AT
#define KEY_CIRCUMFLEX        ALLEGRO_KEY_CIRCUMFLEX
#define KEY_COLON2            ALLEGRO_KEY_COLON2
#define KEY_KANJI             ALLEGRO_KEY_KANJI
#define KEY_EQUALS_PAD        ALLEGRO_KEY_EQUALS_PAD
#define KEY_BACKQUOTE         ALLEGRO_KEY_BACKQUOTE
#define KEY_SEMICOLON         ALLEGRO_KEY_SEMICOLON2
#define KEY_COMMAND           ALLEGRO_KEY_COMMAND

#define KEY_MODIFIERS         ALLEGRO_KEY_MODIFIERS

#define KEY_LSHIFT            ALLEGRO_KEY_LSHIFT
#define KEY_RSHIFT            ALLEGRO_KEY_RSHIFT
#define KEY_LCONTROL          ALLEGRO_KEY_LCTRL
#define KEY_RCONTROL          ALLEGRO_KEY_RCTRL
#define KEY_ALT               ALLEGRO_KEY_ALT
#define KEY_ALTGR             ALLEGRO_KEY_ALTGR
#define KEY_LWIN              ALLEGRO_KEY_LWIN
#define KEY_RWIN              ALLEGRO_KEY_RWIN
#define KEY_MENU              ALLEGRO_KEY_MENU
#define KEY_SCRLOCK           ALLEGRO_KEY_SCROLLLOCK
#define KEY_NUMLOCK           ALLEGRO_KEY_NUMLOCK
#define KEY_CAPSLOCK          ALLEGRO_KEY_CAPSLOCK

#define KEY_MAX               ALLEGRO_KEY_MAX

#endif /* ALLEGRO_NO_KEY_DEFINES */



/*****************************************************************************
 * The old mouse API
 *****************************************************************************/

typedef struct MOUSE_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
} MOUSE_DRIVER;


/* AL_VAR(MOUSE_DRIVER, mousedrv_none); */
AL_VAR(MOUSE_DRIVER *, mouse_driver);

AL_FUNC(int, install_mouse, (void));
AL_FUNC(void, remove_mouse, (void));

AL_FUNC(int, poll_mouse, (void));
AL_FUNC(int, mouse_needs_poll, (void));

AL_FUNC(void, enable_hardware_cursor, (void));
AL_FUNC(void, disable_hardware_cursor, (void));

/* Mouse cursors */
#define MOUSE_CURSOR_NONE        0
#define MOUSE_CURSOR_ALLEGRO     1
#define MOUSE_CURSOR_ARROW       2
#define MOUSE_CURSOR_BUSY        3
#define MOUSE_CURSOR_QUESTION    4
#define MOUSE_CURSOR_EDIT        5
#define NUM_MOUSE_CURSORS        6

AL_VAR(struct BITMAP *, mouse_sprite);
AL_VAR(int, mouse_x_focus);
AL_VAR(int, mouse_y_focus);

AL_VAR(volatile int, mouse_x);
AL_VAR(volatile int, mouse_y);
AL_VAR(volatile int, mouse_z);
AL_VAR(volatile int, mouse_w);
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
#define MOUSE_FLAG_MOVE_W           256

AL_FUNCPTR(void, mouse_callback, (int flags));

AL_FUNC(void, show_mouse, (struct BITMAP *bmp));
AL_FUNC(void, scare_mouse, (void));
AL_FUNC(void, scare_mouse_area, (int x, int y, int w, int h));
AL_FUNC(void, unscare_mouse, (void));
AL_FUNC(void, position_mouse, (int x, int y));
AL_FUNC(void, position_mouse_z, (int z));
AL_FUNC(void, set_mouse_range, (int x1, int y_1, int x2, int y2));
AL_FUNC(void, set_mouse_speed, (int xspeed, int yspeed));
AL_FUNC(void, select_mouse_cursor, (int cursor));
AL_FUNC(void, set_mouse_cursor_bitmap, (int cursor, struct BITMAP *bmp));
AL_FUNC(void, set_mouse_sprite_focus, (int x, int y));
AL_FUNC(void, get_mouse_mickeys, (int *mickeyx, int *mickeyy));
AL_FUNC(void, set_mouse_sprite, (struct BITMAP *sprite));


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_COMPAT_H */

