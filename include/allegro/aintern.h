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


/* flag for how many times we have been initialised */
AL_VAR(int, _allegro_count);


/* some Allegro functions need a block of scratch memory */
AL_VAR(void *, _scratch_mem);
AL_VAR(int, _scratch_mem_size);


AL_INLINE(void, _grow_scratch_mem, (int size),
{
   if (size > _scratch_mem_size) {
      size = (size+1023) & 0xFFFFFC00;
      _scratch_mem = realloc(_scratch_mem, size);
      _scratch_mem_size = size;
   }
})


/* malloc wrappers for DLL <-> application shared memory */
AL_FUNC(void *, _al_malloc, (int size));
AL_FUNC(void, _al_free, (void *mem));
AL_FUNC(void *, _al_realloc, (void *mem, int size));


/* list of functions to call at program cleanup */
AL_FUNC(void, _add_exit_func, (void (*func)(void)));
AL_FUNC(void, _remove_exit_func, (void (*func)(void)));


/* helper structure for talking to Unicode strings */
typedef struct UTYPE_INFO
{
   int id;
   AL_METHOD(int, u_getc, (AL_CONST char *s));
   AL_METHOD(int, u_getx, (AL_CONST char **s));
   AL_METHOD(int, u_setc, (char *s, int c));
   AL_METHOD(int, u_width, (AL_CONST char *s));
   AL_METHOD(int, u_cwidth, (int c));
   AL_METHOD(int, u_isok, (int c));
   int u_width_max;
} UTYPE_INFO;

AL_FUNC(UTYPE_INFO *, _find_utype, (int type));


/* reads a translation file into memory */
AL_FUNC(void, _load_config_text, (void));


/* wrappers for implementing disk I/O on different platforms */
AL_FUNC(int, _al_file_isok, (AL_CONST char *filename));
AL_FUNC(int, _al_file_exists, (AL_CONST char *filename, int attrib, int *aret));
AL_FUNC(long, _al_file_size, (AL_CONST char *filename));
AL_FUNC(long, _al_file_time, (AL_CONST char *filename));
AL_FUNC(void *, _al_findfirst, (AL_CONST char *name, int attrib, char *nameret, int *aret));
AL_FUNC(int, _al_findnext, (void *dta, char *nameret, int *aret));
AL_FUNC(void, _al_findclose, (void *dta));
AL_FUNC(int, _al_getdrive, (void));
AL_FUNC(void, _al_getdcwd, (int drive, char *buf, int size));

AL_VAR(int, _packfile_filesize);
AL_VAR(int, _packfile_datasize);

AL_VAR(int, _packfile_type);


/* various bits of mouse stuff */
AL_FUNC(void, _handle_mouse_input, (void));
AL_FUNC(void, _set_mouse_range, (void));

AL_VAR(int, _mouse_x);
AL_VAR(int, _mouse_y);
AL_VAR(int, _mouse_z);
AL_VAR(int, _mouse_b);
AL_VAR(int, _mouse_on);

AL_VAR(BITMAP *, _mouse_screen);
AL_VAR(AL_CONST BITMAP *, _mouse_sprite);
AL_VAR(BITMAP *, _mouse_pointer);

AL_VAR(int, _mouse_x_focus);
AL_VAR(int, _mouse_y_focus);


/* various bits of timer stuff */
AL_FUNC(long, _handle_timer_tick, (int interval));

#define MAX_TIMERS      16

/* list of active timer handlers */
typedef struct TIMER_QUEUE
{
   AL_METHOD(void, proc, (void));      /* timer handler functions */
   AL_METHOD(void, param_proc, (void *param));
   void *param;                        /* param for param_proc if used */
   long speed;                         /* timer speed */
   long counter;                       /* counts down to zero=blastoff */
} TIMER_QUEUE;

AL_ARRAY(TIMER_QUEUE, _timer_queue);

AL_VAR(int, _timer_use_retrace);
AL_VAR(volatile int, _retrace_hpp_value);


/* various bits of keyboard stuff */
AL_FUNC(void, _handle_key_press, (int keycode, int scancode));
AL_FUNC(void, _handle_key_release, (int scancode));

AL_ARRAY(volatile char, _key);
AL_VAR(volatile int, _key_shifts);

AL_FUNC(void, _pckeys_init, (void));
AL_FUNC(void, _handle_pckey, (int code));
AL_FUNC(int,  _pckey_scancode_to_ascii, (int scancode));

AL_VAR(unsigned short *, _key_ascii_table);
AL_VAR(unsigned short *, _key_capslock_table);
AL_VAR(unsigned short *, _key_shift_table);
AL_VAR(unsigned short *, _key_control_table);
AL_VAR(unsigned short *, _key_altgr_table);
AL_VAR(unsigned short *, _key_accent1_lower_table);
AL_VAR(unsigned short *, _key_accent1_upper_table);
AL_VAR(unsigned short *, _key_accent2_lower_table);
AL_VAR(unsigned short *, _key_accent2_upper_table);
AL_VAR(unsigned short *, _key_accent3_lower_table);
AL_VAR(unsigned short *, _key_accent3_upper_table);
AL_VAR(unsigned short *, _key_accent4_lower_table);
AL_VAR(unsigned short *, _key_accent4_upper_table);

AL_VAR(int, _key_accent1);
AL_VAR(int, _key_accent2);
AL_VAR(int, _key_accent3);
AL_VAR(int, _key_accent4);
AL_VAR(int, _key_accent1_flag);
AL_VAR(int, _key_accent2_flag);
AL_VAR(int, _key_accent3_flag);
AL_VAR(int, _key_accent4_flag);

AL_VAR(int, _key_standard_kb);


/* some GUI innards that other people need to use */
AL_FUNC(void, _handle_scrollable_scroll_click, (DIALOG *d, int listsize, int *offset));
AL_FUNC(void, _handle_scrollable_scroll, (DIALOG *d, int listsize, int *index, int *offset));
AL_FUNC(void, _handle_listbox_click, (DIALOG *d););
AL_FUNC(void, _draw_scrollable_frame, (DIALOG *d, int listsize, int offset, int height, int fg_color, int bg));
AL_FUNC(void, _draw_listbox, (DIALOG *d));
AL_FUNC(void, _draw_textbox, (char *thetext, int *listsize, int draw, int offset, int wword, int tabsize, int x, int y, int w, int h, int disabled, int fore, int deselect, int disable));

AL_VAR(FONT, _default_font);


/* caches and tables for svga bank switching */
AL_VAR(int, _last_bank_1);
AL_VAR(int, _last_bank_2); 

AL_VAR(int *, _gfx_bank);


/* bank switching routines (these use a non-C calling convention on i386!) */
AL_FUNC(unsigned long, _stub_bank_switch, (BITMAP *bmp, int line));
AL_FUNC(void, _stub_unbank_switch, (BITMAP *bmp));
AL_FUNC(void, _stub_bank_switch_end, (void));

#ifdef GFX_MODEX

AL_FUNC(unsigned long, _x_bank_switch, (BITMAP *bmp, int line));
AL_FUNC(void, _x_unbank_switch, (BITMAP *bmp));
AL_FUNC(void, _x_bank_switch_end, (void));

#endif

#ifdef GFX_VBEAF

AL_FUNC(void, _accel_bank_stub, (void));
AL_FUNC(void, _accel_bank_stub_end, (void));
AL_FUNC(void, _accel_bank_switch, (void));
AL_FUNC(void, _accel_bank_switch_end, (void));

AL_VAR(void *, _accel_driver);

AL_VAR(int, _accel_active);

AL_VAR(void *, _accel_set_bank);
AL_VAR(void *, _accel_idle);

AL_FUNC(void, _fill_vbeaf_libc_exports, (void *ptr));
AL_FUNC(void, _fill_vbeaf_pmode_exports, (void *ptr));

#endif


/* stuff for setting up bitmaps */
AL_FUNC(BITMAP *, _make_bitmap, (int w, int h, unsigned long addr, GFX_DRIVER *driver, int color_depth, int bpl));
AL_FUNC(void, _sort_out_virtual_width, (int *width, GFX_DRIVER *driver));

AL_FUNC(GFX_VTABLE *, _get_vtable, (int color_depth));

AL_VAR(GFX_VTABLE, _screen_vtable);

AL_VAR(int, _gfx_mode_set_count);

AL_VAR(int, _refresh_rate_request);
AL_VAR(int, _current_refresh_rate);

AL_VAR(int, _sub_bitmap_id_count);

AL_VAR(int, _screen_split_position);

#ifdef ALLEGRO_I386
   #define BYTES_PER_PIXEL(bpp)     (((int)(bpp) + 7) / 8)
#else
   #define BYTES_PER_PIXEL(bpp)     (((bpp) <= 8) ? 1                                    \
				     : (((bpp) <= 16) ? sizeof (unsigned short)          \
					: (((bpp) <= 24) ? 3 : sizeof (unsigned long))))
#endif

AL_FUNC(int, _color_load_depth, (int depth, int hasalpha));

AL_VAR(int, _color_conv);

AL_FUNC(BITMAP *, _fixup_loaded_bitmap, (BITMAP *bmp, PALETTE pal, int bpp));


/* console switching support */
AL_FUNC(void, _register_switch_bitmap, (BITMAP *bmp, BITMAP *parent));
AL_FUNC(void, _unregister_switch_bitmap, (BITMAP *bmp));
AL_FUNC(void, _save_switch_state, (int switch_mode));
AL_FUNC(void, _restore_switch_state, ());

AL_VAR(int, _dispsw_status);


/* current drawing mode */
AL_VAR(int, _drawing_mode);
AL_VAR(AL_CONST BITMAP *, _drawing_pattern);
AL_VAR(int, _drawing_x_anchor);
AL_VAR(int, _drawing_y_anchor);
AL_VAR(unsigned int, _drawing_x_mask);
AL_VAR(unsigned int, _drawing_y_mask);

AL_VAR(int, _textmode);

AL_FUNCPTR(int *, _palette_expansion_table, (int bpp));

AL_VAR(int, _got_prev_current_palette);
AL_VAR(PALETTE, _prev_current_palette);


/* truecolor blending functions */
AL_FUNC(unsigned long, _blender_black, (unsigned long x, unsigned long y, unsigned long n));

#ifdef ALLEGRO_COLOR16

AL_FUNC(unsigned long, _blender_trans15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen15, (unsigned long x, unsigned long y, unsigned long n));

AL_FUNC(unsigned long, _blender_trans16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen16, (unsigned long x, unsigned long y, unsigned long n));

#endif

#if (defined ALLEGRO_COLOR24) || (defined ALLEGRO_COLOR32)

AL_FUNC(unsigned long, _blender_trans24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_add24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_burn24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_color24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_difference24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dissolve24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_dodge24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_hue24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_invert24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_luminance24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_multiply24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_saturation24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_screen24, (unsigned long x, unsigned long y, unsigned long n));

#endif

AL_FUNC(unsigned long, _blender_alpha15, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha16, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha24, (unsigned long x, unsigned long y, unsigned long n));
AL_FUNC(unsigned long, _blender_alpha32, (unsigned long x, unsigned long y, unsigned long n));

AL_FUNC(unsigned long, _blender_write_alpha, (unsigned long x, unsigned long y, unsigned long n));


/* graphics drawing routines */
AL_FUNC(void, _normal_line, (BITMAP *bmp, int x1, int y1, int x2, int y2, int color));
AL_FUNC(void, _normal_rectfill, (BITMAP *bmp, int x1, int y1, int x2, int y2, int color));

#ifdef ALLEGRO_COLOR8

AL_FUNC(int,  _linear_getpixel8, (AL_CONST BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel8, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline8, (BITMAP *bmp, int x, int y1, int y2, int color));
AL_FUNC(void, _linear_hline8, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite8, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip8, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip8, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip8, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite8, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite8, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite8, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character8, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_glyph8, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color));
AL_FUNC(void, _linear_blit8, (AL_CONST BITMAP *source,BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward8, (AL_CONST BITMAP *source,BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit8, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color8, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR16

AL_FUNC(void, _linear_putpixel15, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline15, (BITMAP *bmp, int x, int y1, int y2, int color));
AL_FUNC(void, _linear_hline15, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_trans_sprite15, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite15, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite15, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite15, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));

AL_FUNC(int,  _linear_getpixel16, (AL_CONST BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel16, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline16, (BITMAP *bmp, int x, int y1, int y2, int color));
AL_FUNC(void, _linear_hline16, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_256_sprite16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite16, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character16, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_glyph16, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color));
AL_FUNC(void, _linear_blit16, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward16, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit16, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color16, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR24

AL_FUNC(int,  _linear_getpixel24, (AL_CONST BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel24, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline24, (BITMAP *bmp, int x, int y1, int y2, int color));
AL_FUNC(void, _linear_hline24, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_256_sprite24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_sprite24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rgba_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite24, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character24, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_glyph24, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color));
AL_FUNC(void, _linear_blit24, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward24, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit24, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color24, (BITMAP *bitmap, int color));

#endif

#ifdef ALLEGRO_COLOR32

AL_FUNC(int,  _linear_getpixel32, (AL_CONST BITMAP *bmp, int x, int y));
AL_FUNC(void, _linear_putpixel32, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _linear_vline32, (BITMAP *bmp, int x, int y1, int y2, int color));
AL_FUNC(void, _linear_hline32, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _linear_draw_sprite32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_256_sprite32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_v_flip32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_h_flip32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_sprite_vh_flip32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_sprite32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_sprite32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_trans_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _linear_draw_lit_rle_sprite32, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_character32, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _linear_draw_glyph32, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color));
AL_FUNC(void, _linear_blit32, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_blit_backward32, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_masked_blit32, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _linear_clear_to_color32, (BITMAP *bitmap, int color));

#endif

#ifdef GFX_MODEX

AL_FUNC(int,  _x_getpixel, (AL_CONST BITMAP *bmp, int x, int y));
AL_FUNC(void, _x_putpixel, (BITMAP *bmp, int x, int y, int color));
AL_FUNC(void, _x_vline, (BITMAP *bmp, int x, int y1, int y2, int color));
AL_FUNC(void, _x_hline, (BITMAP *bmp, int x1, int y, int x2, int color));
AL_FUNC(void, _x_draw_sprite, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_sprite_v_flip, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_sprite_h_flip, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_sprite_vh_flip, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_trans_sprite, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y));
AL_FUNC(void, _x_draw_lit_sprite, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _x_draw_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _x_draw_trans_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y));
AL_FUNC(void, _x_draw_lit_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color));
AL_FUNC(void, _x_draw_character, (BITMAP *bmp, AL_CONST BITMAP *sprite, int x, int y, int color));
AL_FUNC(void, _x_draw_glyph, (BITMAP *bmp, AL_CONST FONT_GLYPH *glyph, int x, int y, int color));
AL_FUNC(void, _x_blit_from_memory, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit_to_memory, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit_forward, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_blit_backward, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_masked_blit, (AL_CONST BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height));
AL_FUNC(void, _x_clear_to_color, (BITMAP *bitmap, int color));

#endif


/* asm helper for stretch_blit() */
AL_FUNC(void, _do_stretch, (AL_CONST BITMAP *source, BITMAP *dest, void *drawer, int sx, fixed sy, fixed syd, int dx, int dy, int dh, int color_depth));


/* number of fractional bits used by the polygon rasteriser */
#define POLYGON_FIX_SHIFT     18


/* bitfield specifying which polygon attributes need interpolating */
#define INTERP_FLAT           1
#define INTERP_1COL           2
#define INTERP_3COL           4
#define INTERP_FIX_UV         8
#define INTERP_Z              16
#define INTERP_FLOAT_UV       32
#define OPT_FLOAT_UV_TO_FIX   64
#define COLOR_TO_RGB          128


/* information for polygon scanline fillers */
typedef struct POLYGON_SEGMENT
{
   fixed u, v, du, dv;              /* fixed point u/v coordinates */
   fixed c, dc;                     /* single color gouraud shade values */
   fixed r, g, b, dr, dg, db;       /* RGB gouraud shade values */
   float z, dz;                     /* polygon depth (1/z) */
   float fu, fv, dfu, dfv;          /* floating point u/v coordinates */
   unsigned char *texture;          /* the texture map */
   int umask, vmask, vshift;        /* texture map size information */
   int seg;                         /* destination bitmap selector */
} POLYGON_SEGMENT;


/* an active polygon edge */
typedef struct POLYGON_EDGE 
{
   int top;                         /* top y position */
   int bottom;                      /* bottom y position */
   fixed x, dx;                     /* fixed point x position and gradient */
   fixed w;                         /* width of line segment */
   POLYGON_SEGMENT dat;             /* texture/gouraud information */
   struct POLYGON_EDGE *prev;       /* doubly linked list */
   struct POLYGON_EDGE *next;
} POLYGON_EDGE;


/* prototype for the scanline filler functions */
typedef AL_METHOD(void, SCANLINE_FILLER, (unsigned long addr, int w, POLYGON_SEGMENT *info));


/* polygon helper functions */
AL_VAR(SCANLINE_FILLER, _optim_alternative_drawer);
AL_FUNC(POLYGON_EDGE *, _add_edge, (POLYGON_EDGE *list, POLYGON_EDGE *edge, int sort_by_x));
AL_FUNC(POLYGON_EDGE *, _remove_edge, (POLYGON_EDGE *list, POLYGON_EDGE *edge));
AL_FUNC(void, _fill_3d_edge_structure, (POLYGON_EDGE *edge, AL_CONST V3D *v1, AL_CONST V3D *v2, int flags, BITMAP *bmp));
AL_FUNC(void, _fill_3d_edge_structure_f, (POLYGON_EDGE *edge, AL_CONST V3D_f *v1, AL_CONST V3D_f *v2, int flags, BITMAP *bmp));
AL_FUNC(SCANLINE_FILLER, _get_scanline_filler, (int type, int *flags, POLYGON_SEGMENT *info, BITMAP *texture, BITMAP *bmp));
AL_FUNC(void, _clip_polygon_segment, (POLYGON_SEGMENT *info, int gap, int flags));


/* polygon scanline filler functions */
#ifdef ALLEGRO_COLOR8

AL_FUNC(void, _poly_scanline_gcol8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_grgb8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit8, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit8, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_grgb8x, (unsigned long addr, int w, POLYGON_SEGMENT *info));

#endif

#ifdef ALLEGRO_COLOR16

AL_FUNC(void, _poly_scanline_grgb15, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask15, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask15, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit15, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit15, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit15, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit15, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_grgb15x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit15x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit15x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit15x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit15x, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit15d, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit15d, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_grgb16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit16, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit16, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_grgb16x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit16x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit16x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit16x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit16x, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit16d, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit16d, (unsigned long addr, int w, POLYGON_SEGMENT *info));

#endif

#ifdef ALLEGRO_COLOR24

AL_FUNC(void, _poly_scanline_grgb24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit24, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit24, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_grgb24x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit24x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit24x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit24x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit24x, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit24d, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit24d, (unsigned long addr, int w, POLYGON_SEGMENT *info));

#endif

#ifdef ALLEGRO_COLOR32

AL_FUNC(void, _poly_scanline_grgb32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit32, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit32, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_grgb32x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_lit32x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_lit32x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_atex_mask_lit32x, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit32x, (unsigned long addr, int w, POLYGON_SEGMENT *info));

AL_FUNC(void, _poly_scanline_ptex_lit32d, (unsigned long addr, int w, POLYGON_SEGMENT *info));
AL_FUNC(void, _poly_scanline_ptex_mask_lit32d, (unsigned long addr, int w, POLYGON_SEGMENT *info));

#endif


/* sound lib stuff */
AL_VAR(int, _digi_volume);
AL_VAR(int, _midi_volume);
AL_VAR(int, _sound_flip_pan); 
AL_VAR(int, _sound_hq);
AL_VAR(int, _sound_stereo);
AL_VAR(int, _sound_bits);
AL_VAR(int, _sound_freq);
AL_VAR(int, _sound_port);
AL_VAR(int, _sound_dma);
AL_VAR(int, _sound_irq);


AL_FUNCPTR(int, _midi_init, (void));
AL_FUNCPTR(void, _midi_exit, (void));

AL_FUNC(int, _midi_allocate_voice, (int min, int max));

AL_VAR(volatile long, _midi_tick);

AL_FUNC(int, _digmid_find_patches, (char *dir, char *file));

#define VIRTUAL_VOICES  256


typedef struct          /* a virtual (as seen by the user) soundcard voice */
{
   AL_CONST SAMPLE *sample;      /* which sample are we playing? (NULL = free) */
   int num;             /* physical voice number (-1 = been killed off) */
   int autokill;        /* set to free the voice when the sample finishes */
   long time;           /* when we were started (for voice allocation) */
   int priority;        /* how important are we? */
} VOICE;

AL_ARRAY(VOICE, _voice);


typedef struct          /* a physical (as used by hardware) soundcard voice */
{
   int num;             /* the virtual voice currently using me (-1 = free) */
   int playmode;        /* are we looping? */
   int vol;             /* current volume (fixed point .12) */
   int dvol;            /* volume delta, for ramping */
   int target_vol;      /* target volume, for ramping */
   int pan;             /* current pan (fixed point .12) */
   int dpan;            /* pan delta, for sweeps */
   int target_pan;      /* target pan, for sweeps */
   int freq;            /* current frequency (fixed point .12) */
   int dfreq;           /* frequency delta, for sweeps */
   int target_freq;     /* target frequency, for sweeps */
} PHYS_VOICE;

AL_ARRAY(PHYS_VOICE, _phys_voice);


#define MIXER_DEF_SFX               8
#define MIXER_MAX_SFX               64

AL_FUNC(int,  _mixer_init, (int bufsize, int freq, int stereo, int is16bit, int *voices));
AL_FUNC(void, _mixer_exit, (void));
AL_FUNC(void, _mix_some_samples, (unsigned long buf, unsigned short seg, int issigned));
AL_FUNC(void, _mixer_init_voice, (int voice, AL_CONST SAMPLE *sample));
AL_FUNC(void, _mixer_release_voice, (int voice));
AL_FUNC(void, _mixer_start_voice, (int voice));
AL_FUNC(void, _mixer_stop_voice, (int voice));
AL_FUNC(void, _mixer_loop_voice, (int voice, int loopmode));
AL_FUNC(int,  _mixer_get_position, (int voice));
AL_FUNC(void, _mixer_set_position, (int voice, int position));
AL_FUNC(int,  _mixer_get_volume, (int voice));
AL_FUNC(void, _mixer_set_volume, (int voice, int volume));
AL_FUNC(void, _mixer_ramp_volume, (int voice, int time, int endvol));
AL_FUNC(void, _mixer_stop_volume_ramp, (int voice));
AL_FUNC(int,  _mixer_get_frequency, (int voice));
AL_FUNC(void, _mixer_set_frequency, (int voice, int frequency));
AL_FUNC(void, _mixer_sweep_frequency, (int voice, int time, int endfreq));
AL_FUNC(void, _mixer_stop_frequency_sweep, (int voice));
AL_FUNC(int,  _mixer_get_pan, (int voice));
AL_FUNC(void, _mixer_set_pan, (int voice, int pan));
AL_FUNC(void, _mixer_sweep_pan, (int voice, int time, int endpan));
AL_FUNC(void, _mixer_stop_pan_sweep, (int voice));
AL_FUNC(void, _mixer_set_echo, (int voice, int strength, int delay));
AL_FUNC(void, _mixer_set_tremolo, (int voice, int rate, int depth));
AL_FUNC(void, _mixer_set_vibrato, (int voice, int rate, int depth));

AL_FUNC(void, _dummy_noop1, (int p));
AL_FUNC(void, _dummy_noop2, (int p1, int p2));
AL_FUNC(void, _dummy_noop3, (int p1, int p2, int p3));
AL_FUNC(int,  _dummy_load_patches, (AL_CONST char *patches, AL_CONST char *drums));
AL_FUNC(void, _dummy_adjust_patches, (AL_CONST char *patches, AL_CONST char *drums));
AL_FUNC(void, _dummy_key_on, (int inst, int note, int bend, int vol, int pan));


/* datafile ID's for compatibility with the old datafile format */
#define V1_DAT_MAGIC             0x616C6C2EL

#define V1_DAT_DATA              0
#define V1_DAT_FONT              1
#define V1_DAT_BITMAP_16         2 
#define V1_DAT_BITMAP_256        3
#define V1_DAT_SPRITE_16         4
#define V1_DAT_SPRITE_256        5
#define V1_DAT_PALETTE_16        6
#define V1_DAT_PALETTE_256       7
#define V1_DAT_FONT_8x8          8
#define V1_DAT_FONT_PROP         9
#define V1_DAT_BITMAP            10
#define V1_DAT_PALETTE           11
#define V1_DAT_SAMPLE            12
#define V1_DAT_MIDI              13
#define V1_DAT_RLE_SPRITE        14
#define V1_DAT_FLI               15
#define V1_DAT_C_SPRITE          16
#define V1_DAT_XC_SPRITE         17

#define OLD_FONT_SIZE            95
#define LESS_OLD_FONT_SIZE       224


/* datafile object loading functions */
AL_FUNC(void *, load_data_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_file_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_font_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_sample_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_midi_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_bitmap_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_rle_sprite_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_compiled_sprite_object, (PACKFILE *f, long size));
AL_FUNC(void *, load_xcompiled_sprite_object, (PACKFILE *f, long size));

AL_FUNC(void, _unload_datafile_object, (DATAFILE *dat));
AL_FUNC(void, unload_sample, (SAMPLE *s));
AL_FUNC(void, unload_midi, (MIDI *m));


/* information about a datafile object */
typedef struct DATAFILE_TYPE
{
   int type;
   AL_METHOD(void *, load, (PACKFILE *f, long size));
   AL_METHOD(void, destroy, ());
} DATAFILE_TYPE;


#define MAX_DATAFILE_TYPES    32

AL_ARRAY(DATAFILE_TYPE, datafile_type);

AL_VAR(int, _compile_sprites);

AL_FUNC(void, _construct_datafile, (DATAFILE *data));


/* for readbmp.c */
AL_FUNC(void, register_bitmap_file_type_init, (void));


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef AINTERN_H */
