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
 *      Inline functions (generic C).
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_NO_ASM

#if (defined ALLEGRO_GCC) && (defined ALLEGRO_I386)

   /* use i386 asm, GCC syntax */
   #include "al386gcc.h"

#elif (defined ALLEGRO_MSVC) && (defined ALLEGRO_I386)

   /* use i386 asm, MSVC syntax */
   #include "al386vc.h"

#elif (defined ALLEGRO_WATCOM) && (defined ALLEGRO_I386)

   /* use i386 asm, Watcom syntax */
   #include "al386wat.h"

#else

   /* asm not supported */
   #define ALLEGRO_NO_ASM

#endif

#endif



/*****************************************/
/************ System routines ************/
/*****************************************/


AL_INLINE(void, get_executable_name, (char *output, int size),
{
   ASSERT(system_driver);

   if (system_driver->get_executable_name) {
      system_driver->get_executable_name(output, size);
   }
   else {
      output += usetc(output, '.');
      output += usetc(output, '/');
      usetc(output, 0);
   }
})


AL_INLINE(void, set_window_title, (char *name),
{
   ASSERT(system_driver);

   if (system_driver->set_window_title)
      system_driver->set_window_title(name);
})


#define ALLEGRO_WINDOW_CLOSE_MESSAGE                                         \
   "Warning: forcing program shutdown may lead to data loss and unexpected " \
   "results. It is preferable to use the exit command inside the window. "   \
   "Proceed anyway?"


AL_INLINE(int, set_window_close_button, (int enable),
{
   ASSERT(system_driver);

   if (system_driver->set_window_close_button)
      return system_driver->set_window_close_button(enable);

   return -1;
})


AL_INLINE(void, set_window_close_hook, (AL_METHOD(void, proc, (void))),
{
   ASSERT(system_driver);

   if (system_driver->set_window_close_hook)
      system_driver->set_window_close_hook(proc);
})


AL_INLINE(int, desktop_color_depth, (void),
{
   ASSERT(system_driver);

   if (system_driver->desktop_color_depth)
      return system_driver->desktop_color_depth();
   else
      return 0;
})


AL_INLINE(int, get_desktop_resolution, (int *width, int *height),
{
   ASSERT(system_driver);

   if (system_driver->get_desktop_resolution)
      return system_driver->get_desktop_resolution(width, height);
   else
      return -1;
})


AL_INLINE(void, yield_timeslice, (void),
{
   ASSERT(system_driver);

   if (system_driver->yield_timeslice)
      system_driver->yield_timeslice();
})



/*******************************************/
/************ Graphics routines ************/
/*******************************************/


#ifdef ALLEGRO_NO_ASM

   /* use generic C versions */

AL_INLINE(int, _default_ds, (void),
{
   return 0;
})


typedef AL_METHOD(unsigned long, _BMP_BANK_SWITCHER, (BITMAP *bmp, int line));
typedef AL_METHOD(void, _BMP_UNBANK_SWITCHER, (BITMAP *bmp));


AL_INLINE(unsigned long, bmp_write_line, (BITMAP *bmp, int line),
{
   _BMP_BANK_SWITCHER switcher = (_BMP_BANK_SWITCHER)bmp->write_bank;
   return switcher(bmp, line);
})


AL_INLINE(unsigned long, bmp_read_line, (BITMAP *bmp, int line),
{
   _BMP_BANK_SWITCHER switcher = (_BMP_BANK_SWITCHER)bmp->read_bank;
   return switcher(bmp, line);
})


AL_INLINE(void, bmp_unwrite_line, (BITMAP *bmp),
{
   _BMP_UNBANK_SWITCHER switcher = (_BMP_UNBANK_SWITCHER)bmp->vtable->unwrite_bank;
   switcher(bmp);
})

#endif      /* C vs. inline asm */


AL_INLINE(int, bitmap_color_depth, (BITMAP *bmp),
{
   ASSERT(bmp);

   return bmp->vtable->color_depth;
})


AL_INLINE(int, bitmap_mask_color, (BITMAP *bmp),
{
   ASSERT(bmp);

   return bmp->vtable->mask_color;
})


AL_INLINE(int, is_same_bitmap, (BITMAP *bmp1, BITMAP *bmp2),
{
   unsigned long m1;
   unsigned long m2;

   if ((!bmp1) || (!bmp2))
      return FALSE;

   if (bmp1 == bmp2)
      return TRUE;

   m1 = bmp1->id & BMP_ID_MASK;
   m2 = bmp2->id & BMP_ID_MASK;

   return ((m1) && (m1 == m2));
})


AL_INLINE(int, is_linear_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_PLANAR) == 0;
})


AL_INLINE(int, is_planar_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_PLANAR) != 0;
})


AL_INLINE(int, is_memory_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & (BMP_ID_VIDEO | BMP_ID_SYSTEM)) == 0;
})


AL_INLINE(int, is_screen_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return is_same_bitmap(bmp, screen);
})


AL_INLINE(int, is_video_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_VIDEO) != 0;
})


AL_INLINE(int, is_system_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_SYSTEM) != 0;
})


AL_INLINE(int, is_sub_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   return (bmp->id & BMP_ID_SUB) != 0;
})


AL_INLINE(int, makecol15, (int r, int g, int b),
{
   return (((r >> 3) << _rgb_r_shift_15) |
	   ((g >> 3) << _rgb_g_shift_15) |
	   ((b >> 3) << _rgb_b_shift_15));
})


AL_INLINE(int, makecol16, (int r, int g, int b),
{
   return (((r >> 3) << _rgb_r_shift_16) |
	   ((g >> 2) << _rgb_g_shift_16) |
	   ((b >> 3) << _rgb_b_shift_16));
})


AL_INLINE(int, makecol24, (int r, int g, int b),
{
   return ((r << _rgb_r_shift_24) |
	   (g << _rgb_g_shift_24) |
	   (b << _rgb_b_shift_24));
})


AL_INLINE(int, makecol32, (int r, int g, int b),
{
   return ((r << _rgb_r_shift_32) |
	   (g << _rgb_g_shift_32) |
	   (b << _rgb_b_shift_32));
})


AL_INLINE(int, makeacol32, (int r, int g, int b, int a),
{
   return ((r << _rgb_r_shift_32) |
	   (g << _rgb_g_shift_32) |
	   (b << _rgb_b_shift_32) |
	   (a << _rgb_a_shift_32));
})


AL_INLINE(int, getr8, (int c),
{
   return _rgb_scale_6[(int)_current_palette[c].r];
})


AL_INLINE(int, getg8, (int c),
{
   return _rgb_scale_6[(int)_current_palette[c].g];
})


AL_INLINE(int, getb8, (int c),
{
   return _rgb_scale_6[(int)_current_palette[c].b];
})


AL_INLINE(int, getr15, (int c),
{
   return _rgb_scale_5[(c >> _rgb_r_shift_15) & 0x1F];
})


AL_INLINE(int, getg15, (int c),
{
   return _rgb_scale_5[(c >> _rgb_g_shift_15) & 0x1F];
})


AL_INLINE(int, getb15, (int c),
{
   return _rgb_scale_5[(c >> _rgb_b_shift_15) & 0x1F];
})


AL_INLINE(int, getr16, (int c),
{
   return _rgb_scale_5[(c >> _rgb_r_shift_16) & 0x1F];
})


AL_INLINE(int, getg16, (int c),
{
   return _rgb_scale_6[(c >> _rgb_g_shift_16) & 0x3F];
})


AL_INLINE(int, getb16, (int c),
{
   return _rgb_scale_5[(c >> _rgb_b_shift_16) & 0x1F];
})


AL_INLINE(int, getr24, (int c),
{
   return ((c >> _rgb_r_shift_24) & 0xFF);
})


AL_INLINE(int, getg24, (int c),
{
   return ((c >> _rgb_g_shift_24) & 0xFF);
})


AL_INLINE(int, getb24, (int c),
{
   return ((c >> _rgb_b_shift_24) & 0xFF);
})


AL_INLINE(int, getr32, (int c),
{
   return ((c >> _rgb_r_shift_32) & 0xFF);
})


AL_INLINE(int, getg32, (int c),
{
   return ((c >> _rgb_g_shift_32) & 0xFF);
})


AL_INLINE(int, getb32, (int c),
{
   return ((c >> _rgb_b_shift_32) & 0xFF);
})


AL_INLINE(int, geta32, (int c),
{
   return ((c >> _rgb_a_shift_32) & 0xFF);
})


#ifdef ALLEGRO_MPW

   #define acquire_bitmap(bmp)
   #define release_bitmap(bmp)
   #define acquire_screen()
   #define release_screen()

#else

AL_INLINE(void, acquire_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   if (bmp->vtable->acquire)
      bmp->vtable->acquire(bmp);
})


AL_INLINE(void, release_bitmap, (BITMAP *bmp),
{
   ASSERT(bmp);

   if (bmp->vtable->release)
      bmp->vtable->release(bmp);
})


AL_INLINE(void, acquire_screen, (void),
{
   acquire_bitmap(screen);
})


AL_INLINE(void, release_screen, (void),
{
   release_bitmap(screen);
})

#endif

AL_INLINE(int, getpixel, (BITMAP *bmp, int x, int y),
{ 
   ASSERT(bmp);

   return bmp->vtable->getpixel(bmp, x, y);
})


AL_INLINE(void, putpixel, (BITMAP *bmp, int x, int y, int color),
{ 
   ASSERT(bmp);

   bmp->vtable->putpixel(bmp, x, y, color);
})


AL_INLINE(void, vline, (BITMAP *bmp, int x, int y1, int y2, int color),
{ 
   ASSERT(bmp);

   bmp->vtable->vline(bmp, x, y1, y2, color); 
})


AL_INLINE(void, hline, (BITMAP *bmp, int x1, int y, int x2, int color),
{ 
   ASSERT(bmp);

   bmp->vtable->hline(bmp, x1, y, x2, color); 
})


AL_INLINE(void, line, (BITMAP *bmp, int x1, int y1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->line(bmp, x1, y1, x2, y2, color);
})


AL_INLINE(void, rectfill, (BITMAP *bmp, int x1, int y1, int x2, int y2, int color),
{
   ASSERT(bmp);

   bmp->vtable->rectfill(bmp, x1, y1, x2, y2, color);
})


AL_INLINE(void, draw_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{ 
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->vtable->color_depth == 8) {
      bmp->vtable->draw_256_sprite(bmp, sprite, x, y); 
   }
   else {
      ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);
      bmp->vtable->draw_sprite(bmp, sprite, x, y); 
   }
})


AL_INLINE(void, draw_sprite_v_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{ 
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_v_flip(bmp, sprite, x, y); 
})


AL_INLINE(void, draw_sprite_h_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{ 
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_h_flip(bmp, sprite, x, y); 
})


AL_INLINE(void, draw_sprite_vh_flip, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{ 
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_sprite_vh_flip(bmp, sprite, x, y); 
})


AL_INLINE(void, draw_trans_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y),
{ 
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->vtable->color_depth == 32) {
      ASSERT(bmp->vtable->draw_trans_rgba_sprite);
      bmp->vtable->draw_trans_rgba_sprite(bmp, sprite, x, y); 
   }
   else {
      ASSERT((bmp->vtable->color_depth == sprite->vtable->color_depth) ||
	     ((bmp->vtable->color_depth == 32) &&
	      (sprite->vtable->color_depth == 8)));
      bmp->vtable->draw_trans_sprite(bmp, sprite, x, y); 
   }
})


AL_INLINE(void, draw_lit_sprite, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color),
{ 
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->vtable->color_depth);

   bmp->vtable->draw_lit_sprite(bmp, sprite, x, y, color); 
})


AL_INLINE(void, draw_character, (BITMAP *bmp, BITMAP *sprite, int x, int y, int color),
{ 
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(sprite->vtable->color_depth == 8);

   bmp->vtable->draw_character(bmp, sprite, x, y, color); 
})


AL_INLINE(void, draw_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->color_depth);

   bmp->vtable->draw_rle_sprite(bmp, sprite, x, y);
})


AL_INLINE(void, draw_trans_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y),
{
   ASSERT(bmp);
   ASSERT(sprite);

   if (sprite->color_depth == 32) {
      ASSERT(bmp->vtable->draw_trans_rgba_rle_sprite);
      bmp->vtable->draw_trans_rgba_rle_sprite(bmp, sprite, x, y);
   }
   else {
      ASSERT(bmp->vtable->color_depth == sprite->color_depth);
      bmp->vtable->draw_trans_rle_sprite(bmp, sprite, x, y);
   }
})


AL_INLINE(void, draw_lit_rle_sprite, (BITMAP *bmp, AL_CONST struct RLE_SPRITE *sprite, int x, int y, int color),
{
   ASSERT(bmp);
   ASSERT(sprite);
   ASSERT(bmp->vtable->color_depth == sprite->color_depth);

   bmp->vtable->draw_lit_rle_sprite(bmp, sprite, x, y, color);
})


AL_INLINE(void, clear_to_color, (BITMAP *bitmap, int color),
{ 
   ASSERT(bitmap);

   bitmap->vtable->clear_to_color(bitmap, color); 
})


#ifndef ALLEGRO_DOS

AL_INLINE(void, _set_color, (int index, AL_CONST RGB *p),
{
   set_color(index, p);
})

#endif


AL_INLINE(void, _putpixel, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write8(addr+x, color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read8(addr+x);
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel15, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write15(addr+x*sizeof(short), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel15, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read15(addr+x*sizeof(short));
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel16, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write16(addr+x*sizeof(short), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel16, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read16(addr+x*sizeof(short));
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel24, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write24(addr+x*3, color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel24, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read24(addr+x*3);
   bmp_unwrite_line(bmp);

   return c;
})


AL_INLINE(void, _putpixel32, (BITMAP *bmp, int x, int y, int color),
{
   unsigned long addr;

   bmp_select(bmp);
   addr = bmp_write_line(bmp, y);
   bmp_write32(addr+x*sizeof(long), color);
   bmp_unwrite_line(bmp);
})


AL_INLINE(int, _getpixel32, (BITMAP *bmp, int x, int y),
{
   unsigned long addr;
   int c;

   bmp_select(bmp);
   addr = bmp_read_line(bmp, y);
   c = bmp_read32(addr+x*sizeof(long));
   bmp_unwrite_line(bmp);

   return c;
})



/***********************************************************/
/************ File I/O and compression routines ************/
/***********************************************************/


AL_INLINE(int, pack_getc, (PACKFILE *f),
{
   f->buf_size--;
   if (f->buf_size > 0)
      return *(f->buf_pos++);
   else
      return _sort_out_getc(f);
})


AL_INLINE(int, pack_putc, (int c, PACKFILE *f),
{
   f->buf_size++;
   if (f->buf_size >= F_BUF_SIZE)
      return _sort_out_putc(c, f);
   else
      return (*(f->buf_pos++) = c);
})


AL_INLINE(int, pack_feof, (PACKFILE *f),
{
   return (f->flags & PACKFILE_FLAG_EOF);
})


AL_INLINE(int, pack_ferror, (PACKFILE *f),
{
   return (f->flags & PACKFILE_FLAG_ERROR);
})



/***************************************/
/************ Math routines ************/
/***************************************/


/* ftofix and fixtof are used in generic C versions of fmul and fdiv */
AL_INLINE(fixed, ftofix, (double x),
{ 
   if (x > 32767.0) {
      *allegro_errno = ERANGE;
      return 0x7FFFFFFF;
   }

   if (x < -32767.0) {
      *allegro_errno = ERANGE;
      return -0x7FFFFFFF;
   }

   return (long)(x * 65536.0 + (x < 0 ? -0.5 : 0.5)); 
})


AL_INLINE(double, fixtof, (fixed x),
{ 
   return (double)x / 65536.0; 
})


#ifdef ALLEGRO_NO_ASM

   /* use generic C versions */

AL_INLINE(fixed, fadd, (fixed x, fixed y),
{
   fixed result = x + y;

   if (result >= 0) {
      if ((x < 0) && (y < 0)) {
	 *allegro_errno = ERANGE;
	 return -0x7FFFFFFF;
      }
      else
	 return result;
   }
   else {
      if ((x > 0) && (y > 0)) {
	 *allegro_errno = ERANGE;
	 return 0x7FFFFFFF;
      }
      else
	 return result;
   }
})


AL_INLINE(fixed, fsub, (fixed x, fixed y),
{
   fixed result = x - y;

   if (result >= 0) {
      if ((x < 0) && (y > 0)) {
	 *allegro_errno = ERANGE;
	 return -0x7FFFFFFF;
      }
      else
	 return result;
   }
   else {
      if ((x > 0) && (y < 0)) {
	 *allegro_errno = ERANGE;
	 return 0x7FFFFFFF;
      }
      else
	 return result;
   }
})


AL_INLINE(fixed, fmul, (fixed x, fixed y),
{
   return ftofix(fixtof(x) * fixtof(y));
})


AL_INLINE(fixed, fdiv, (fixed x, fixed y),
{
   if (y == 0) {
      *allegro_errno = ERANGE;
      return (x < 0) ? -0x7FFFFFFF : 0x7FFFFFFF;
   }
   else
      return ftofix(fixtof(x) / fixtof(y));
})


AL_INLINE(int, fceil, (fixed x),
{
   x += 0xFFFF;
   if (x >= 0x80000000) {
      *allegro_errno = ERANGE;
      return 0x7FFF;
   }

   return (x >> 16);
})

#endif      /* C vs. inline asm */


AL_INLINE(fixed, itofix, (int x),
{ 
   return x << 16;
})


AL_INLINE(int, fixtoi, (fixed x),
{ 
   return (x >> 16) + ((x & 0x8000) >> 15);
})


AL_INLINE(fixed, fcos, (fixed x),
{
   return _cos_tbl[((x + 0x4000) >> 15) & 0x1FF];
})


AL_INLINE(fixed, fsin, (fixed x),
{ 
   return _cos_tbl[((x - 0x400000 + 0x4000) >> 15) & 0x1FF];
})


AL_INLINE(fixed, ftan, (fixed x),
{ 
   return _tan_tbl[((x + 0x4000) >> 15) & 0xFF];
})


AL_INLINE(fixed, facos, (fixed x),
{
   if ((x < -65536) || (x > 65536)) {
      *allegro_errno = EDOM;
      return 0;
   }

   return _acos_tbl[(x+65536+127)>>8];
})


AL_INLINE(fixed, fasin, (fixed x),
{ 
   if ((x < -65536) || (x > 65536)) {
      *allegro_errno = EDOM;
      return 0;
   }

   return 0x00400000 - _acos_tbl[(x+65536+127)>>8];
})


#ifdef __cplusplus

}  /* end of extern "C", starting C++ wrapper functions */


class fix      /* C++ wrapper for the fixed point routines */
{
public:
   fixed v;

   fix()                                     { }
   fix(const fix &x)                         { v = x.v; }
   fix(const int x)                          { v = itofix(x); }
   fix(const long x)                         { v = itofix(x); }
   fix(const unsigned int x)                 { v = itofix(x); }
   fix(const unsigned long x)                { v = itofix(x); }
   fix(const float x)                        { v = ftofix(x); }
   fix(const double x)                       { v = ftofix(x); }

   operator int() const                      { return fixtoi(v); }
   operator long() const                     { return fixtoi(v); }
   operator unsigned int() const             { return fixtoi(v); }
   operator unsigned long() const            { return fixtoi(v); }
   operator float() const                    { return fixtof(v); }
   operator double() const                   { return fixtof(v); }

   fix& operator = (const fix &x)            { v = x.v;           return *this; }
   fix& operator = (const int x)             { v = itofix(x);     return *this; }
   fix& operator = (const long x)            { v = itofix(x);     return *this; }
   fix& operator = (const unsigned int x)    { v = itofix(x);     return *this; }
   fix& operator = (const unsigned long x)   { v = itofix(x);     return *this; }
   fix& operator = (const float x)           { v = ftofix(x);     return *this; }
   fix& operator = (const double x)          { v = ftofix(x);     return *this; }

   fix& operator +=  (const fix x)           { v += x.v;          return *this; }
   fix& operator +=  (const int x)           { v += itofix(x);    return *this; }
   fix& operator +=  (const long x)          { v += itofix(x);    return *this; }
   fix& operator +=  (const float x)         { v += ftofix(x);    return *this; }
   fix& operator +=  (const double x)        { v += ftofix(x);    return *this; }

   fix& operator -=  (const fix x)           { v -= x.v;          return *this; }
   fix& operator -=  (const int x)           { v -= itofix(x);    return *this; }
   fix& operator -=  (const long x)          { v -= itofix(x);    return *this; }
   fix& operator -=  (const float x)         { v -= ftofix(x);    return *this; }
   fix& operator -=  (const double x)        { v -= ftofix(x);    return *this; }

   fix& operator *=  (const fix x)           { v = fmul(v, x.v);           return *this; }
   fix& operator *=  (const int x)           { v *= x;                     return *this; }
   fix& operator *=  (const long x)          { v *= x;                     return *this; }
   fix& operator *=  (const float x)         { v = ftofix(fixtof(v) * x);  return *this; }
   fix& operator *=  (const double x)        { v = ftofix(fixtof(v) * x);  return *this; }

   fix& operator /=  (const fix x)           { v = fdiv(v, x.v);           return *this; }
   fix& operator /=  (const int x)           { v /= x;                     return *this; }
   fix& operator /=  (const long x)          { v /= x;                     return *this; }
   fix& operator /=  (const float x)         { v = ftofix(fixtof(v) / x);  return *this; }
   fix& operator /=  (const double x)        { v = ftofix(fixtof(v) / x);  return *this; }

   fix& operator <<= (const int x)           { v <<= x;           return *this; }
   fix& operator >>= (const int x)           { v >>= x;           return *this; }

   fix& operator ++ ()                       { v += itofix(1);    return *this; }
   fix& operator -- ()                       { v -= itofix(1);    return *this; }

   fix operator ++ (int)                     { fix t;  t.v = v;   v += itofix(1);  return t; }
   fix operator -- (int)                     { fix t;  t.v = v;   v -= itofix(1);  return t; }

   fix operator - () const                   { fix t;  t.v = -v;  return t; }

   inline friend fix operator +  (const fix x, const fix y)    { fix t;  t.v = x.v + y.v;        return t; }
   inline friend fix operator +  (const fix x, const int y)    { fix t;  t.v = x.v + itofix(y);  return t; }
   inline friend fix operator +  (const int x, const fix y)    { fix t;  t.v = itofix(x) + y.v;  return t; }
   inline friend fix operator +  (const fix x, const long y)   { fix t;  t.v = x.v + itofix(y);  return t; }
   inline friend fix operator +  (const long x, const fix y)   { fix t;  t.v = itofix(x) + y.v;  return t; }
   inline friend fix operator +  (const fix x, const float y)  { fix t;  t.v = x.v + ftofix(y);  return t; }
   inline friend fix operator +  (const float x, const fix y)  { fix t;  t.v = ftofix(x) + y.v;  return t; }
   inline friend fix operator +  (const fix x, const double y) { fix t;  t.v = x.v + ftofix(y);  return t; }
   inline friend fix operator +  (const double x, const fix y) { fix t;  t.v = ftofix(x) + y.v;  return t; }

   inline friend fix operator -  (const fix x, const fix y)    { fix t;  t.v = x.v - y.v;        return t; }
   inline friend fix operator -  (const fix x, const int y)    { fix t;  t.v = x.v - itofix(y);  return t; }
   inline friend fix operator -  (const int x, const fix y)    { fix t;  t.v = itofix(x) - y.v;  return t; }
   inline friend fix operator -  (const fix x, const long y)   { fix t;  t.v = x.v - itofix(y);  return t; }
   inline friend fix operator -  (const long x, const fix y)   { fix t;  t.v = itofix(x) - y.v;  return t; }
   inline friend fix operator -  (const fix x, const float y)  { fix t;  t.v = x.v - ftofix(y);  return t; }
   inline friend fix operator -  (const float x, const fix y)  { fix t;  t.v = ftofix(x) - y.v;  return t; }
   inline friend fix operator -  (const fix x, const double y) { fix t;  t.v = x.v - ftofix(y);  return t; }
   inline friend fix operator -  (const double x, const fix y) { fix t;  t.v = ftofix(x) - y.v;  return t; }

   inline friend fix operator *  (const fix x, const fix y)    { fix t;  t.v = fmul(x.v, y.v);           return t; }
   inline friend fix operator *  (const fix x, const int y)    { fix t;  t.v = x.v * y;                  return t; }
   inline friend fix operator *  (const int x, const fix y)    { fix t;  t.v = x * y.v;                  return t; }
   inline friend fix operator *  (const fix x, const long y)   { fix t;  t.v = x.v * y;                  return t; }
   inline friend fix operator *  (const long x, const fix y)   { fix t;  t.v = x * y.v;                  return t; }
   inline friend fix operator *  (const fix x, const float y)  { fix t;  t.v = ftofix(fixtof(x.v) * y);  return t; }
   inline friend fix operator *  (const float x, const fix y)  { fix t;  t.v = ftofix(x * fixtof(y.v));  return t; }
   inline friend fix operator *  (const fix x, const double y) { fix t;  t.v = ftofix(fixtof(x.v) * y);  return t; }
   inline friend fix operator *  (const double x, const fix y) { fix t;  t.v = ftofix(x * fixtof(y.v));  return t; }

   inline friend fix operator /  (const fix x, const fix y)    { fix t;  t.v = fdiv(x.v, y.v);           return t; }
   inline friend fix operator /  (const fix x, const int y)    { fix t;  t.v = x.v / y;                  return t; }
   inline friend fix operator /  (const int x, const fix y)    { fix t;  t.v = fdiv(itofix(x), y.v);     return t; }
   inline friend fix operator /  (const fix x, const long y)   { fix t;  t.v = x.v / y;                  return t; }
   inline friend fix operator /  (const long x, const fix y)   { fix t;  t.v = fdiv(itofix(x), y.v);     return t; }
   inline friend fix operator /  (const fix x, const float y)  { fix t;  t.v = ftofix(fixtof(x.v) / y);  return t; }
   inline friend fix operator /  (const float x, const fix y)  { fix t;  t.v = ftofix(x / fixtof(y.v));  return t; }
   inline friend fix operator /  (const fix x, const double y) { fix t;  t.v = ftofix(fixtof(x.v) / y);  return t; }
   inline friend fix operator /  (const double x, const fix y) { fix t;  t.v = ftofix(x / fixtof(y.v));  return t; }

   inline friend fix operator << (const fix x, const int y)    { fix t;  t.v = x.v << y;    return t; }
   inline friend fix operator >> (const fix x, const int y)    { fix t;  t.v = x.v >> y;    return t; }

   inline friend int operator == (const fix x, const fix y)    { return (x.v == y.v);       }
   inline friend int operator == (const fix x, const int y)    { return (x.v == itofix(y)); }
   inline friend int operator == (const int x, const fix y)    { return (itofix(x) == y.v); }
   inline friend int operator == (const fix x, const long y)   { return (x.v == itofix(y)); }
   inline friend int operator == (const long x, const fix y)   { return (itofix(x) == y.v); }
   inline friend int operator == (const fix x, const float y)  { return (x.v == ftofix(y)); }
   inline friend int operator == (const float x, const fix y)  { return (ftofix(x) == y.v); }
   inline friend int operator == (const fix x, const double y) { return (x.v == ftofix(y)); }
   inline friend int operator == (const double x, const fix y) { return (ftofix(x) == y.v); }

   inline friend int operator != (const fix x, const fix y)    { return (x.v != y.v);       }
   inline friend int operator != (const fix x, const int y)    { return (x.v != itofix(y)); }
   inline friend int operator != (const int x, const fix y)    { return (itofix(x) != y.v); }
   inline friend int operator != (const fix x, const long y)   { return (x.v != itofix(y)); }
   inline friend int operator != (const long x, const fix y)   { return (itofix(x) != y.v); }
   inline friend int operator != (const fix x, const float y)  { return (x.v != ftofix(y)); }
   inline friend int operator != (const float x, const fix y)  { return (ftofix(x) != y.v); }
   inline friend int operator != (const fix x, const double y) { return (x.v != ftofix(y)); }
   inline friend int operator != (const double x, const fix y) { return (ftofix(x) != y.v); }

   inline friend int operator <  (const fix x, const fix y)    { return (x.v < y.v);        }
   inline friend int operator <  (const fix x, const int y)    { return (x.v < itofix(y));  }
   inline friend int operator <  (const int x, const fix y)    { return (itofix(x) < y.v);  }
   inline friend int operator <  (const fix x, const long y)   { return (x.v < itofix(y));  }
   inline friend int operator <  (const long x, const fix y)   { return (itofix(x) < y.v);  }
   inline friend int operator <  (const fix x, const float y)  { return (x.v < ftofix(y));  }
   inline friend int operator <  (const float x, const fix y)  { return (ftofix(x) < y.v);  }
   inline friend int operator <  (const fix x, const double y) { return (x.v < ftofix(y));  }
   inline friend int operator <  (const double x, const fix y) { return (ftofix(x) < y.v);  }

   inline friend int operator >  (const fix x, const fix y)    { return (x.v > y.v);        }
   inline friend int operator >  (const fix x, const int y)    { return (x.v > itofix(y));  }
   inline friend int operator >  (const int x, const fix y)    { return (itofix(x) > y.v);  }
   inline friend int operator >  (const fix x, const long y)   { return (x.v > itofix(y));  }
   inline friend int operator >  (const long x, const fix y)   { return (itofix(x) > y.v);  }
   inline friend int operator >  (const fix x, const float y)  { return (x.v > ftofix(y));  }
   inline friend int operator >  (const float x, const fix y)  { return (ftofix(x) > y.v);  }
   inline friend int operator >  (const fix x, const double y) { return (x.v > ftofix(y));  }
   inline friend int operator >  (const double x, const fix y) { return (ftofix(x) > y.v);  }

   inline friend int operator <= (const fix x, const fix y)    { return (x.v <= y.v);       }
   inline friend int operator <= (const fix x, const int y)    { return (x.v <= itofix(y)); }
   inline friend int operator <= (const int x, const fix y)    { return (itofix(x) <= y.v); }
   inline friend int operator <= (const fix x, const long y)   { return (x.v <= itofix(y)); }
   inline friend int operator <= (const long x, const fix y)   { return (itofix(x) <= y.v); }
   inline friend int operator <= (const fix x, const float y)  { return (x.v <= ftofix(y)); }
   inline friend int operator <= (const float x, const fix y)  { return (ftofix(x) <= y.v); }
   inline friend int operator <= (const fix x, const double y) { return (x.v <= ftofix(y)); }
   inline friend int operator <= (const double x, const fix y) { return (ftofix(x) <= y.v); }

   inline friend int operator >= (const fix x, const fix y)    { return (x.v >= y.v);       }
   inline friend int operator >= (const fix x, const int y)    { return (x.v >= itofix(y)); }
   inline friend int operator >= (const int x, const fix y)    { return (itofix(x) >= y.v); }
   inline friend int operator >= (const fix x, const long y)   { return (x.v >= itofix(y)); }
   inline friend int operator >= (const long x, const fix y)   { return (itofix(x) >= y.v); }
   inline friend int operator >= (const fix x, const float y)  { return (x.v >= ftofix(y)); }
   inline friend int operator >= (const float x, const fix y)  { return (ftofix(x) >= y.v); }
   inline friend int operator >= (const fix x, const double y) { return (x.v >= ftofix(y)); }
   inline friend int operator >= (const double x, const fix y) { return (ftofix(x) >= y.v); }

   inline friend fix sqrt(fix x)          { fix t;  t.v = fsqrt(x.v);  return t; }
   inline friend fix cos(fix x)           { fix t;  t.v = fcos(x.v);   return t; }
   inline friend fix sin(fix x)           { fix t;  t.v = fsin(x.v);   return t; }
   inline friend fix tan(fix x)           { fix t;  t.v = ftan(x.v);   return t; }
   inline friend fix acos(fix x)          { fix t;  t.v = facos(x.v);  return t; }
   inline friend fix asin(fix x)          { fix t;  t.v = fasin(x.v);  return t; }
   inline friend fix atan(fix x)          { fix t;  t.v = fatan(x.v);  return t; }
   inline friend fix atan2(fix x, fix y)  { fix t;  t.v = fatan2(x.v, y.v);  return t; }
};


extern "C" {

#endif   /* ifdef __cplusplus, back to C code */


AL_INLINE(fixed, dot_product, (fixed x1, fixed y1, fixed z1, fixed x2, fixed y2, fixed z2),
{
   return fmul(x1, x2) + fmul(y1, y2) + fmul(z1, z2);
})


AL_INLINE(float, dot_product_f, (float x1, float y1, float z1, float x2, float y2, float z2),
{
   return (x1 * x2) + (y1 * y2) + (z1 * z2);
})


#define CALC_ROW(n)     (fmul(x, m->v[n][0]) +        \
                         fmul(y, m->v[n][1]) +        \
                         fmul(z, m->v[n][2]) +        \
                         m->t[n])

AL_INLINE(void, apply_matrix, (MATRIX *m, fixed x, fixed y, fixed z, fixed *xout, fixed *yout, fixed *zout),
{
   *xout = CALC_ROW(0);
   *yout = CALC_ROW(1);
   *zout = CALC_ROW(2);
})

#undef CALC_ROW


AL_INLINE(void, persp_project, (fixed x, fixed y, fixed z, fixed *xout, fixed *yout),
{
   *xout = fmul(fdiv(x, z), _persp_xscale) + _persp_xoffset;
   *yout = fmul(fdiv(y, z), _persp_yscale) + _persp_yoffset;
})


AL_INLINE(void, persp_project_f, (float x, float y, float z, float *xout, float *yout),
{
   float z1 = 1.0f / z;
   *xout = ((x * z1) * _persp_xscale_f) + _persp_xoffset_f;
   *yout = ((y * z1) * _persp_yscale_f) + _persp_yoffset_f;
})


#ifdef __cplusplus

}  /* end of extern "C", starting C++ wrapper functions */


inline void get_translation_matrix(MATRIX *m, fix x, fix y, fix z)
{ 
   get_translation_matrix(m, x.v, y.v, z.v);
}


inline void get_scaling_matrix(MATRIX *m, fix x, fix y, fix z)
{ 
   get_scaling_matrix(m, x.v, y.v, z.v); 
}


inline void get_x_rotate_matrix(MATRIX *m, fix r)
{ 
   get_x_rotate_matrix(m, r.v);
}


inline void get_y_rotate_matrix(MATRIX *m, fix r)
{ 
   get_y_rotate_matrix(m, r.v);
}


inline void get_z_rotate_matrix(MATRIX *m, fix r)
{ 
   get_z_rotate_matrix(m, r.v);
}


inline void get_rotation_matrix(MATRIX *m, fix x, fix y, fix z)
{ 
   get_rotation_matrix(m, x.v, y.v, z.v);
}


inline void get_align_matrix(MATRIX *m, fix xfront, fix yfront, fix zfront, fix xup, fix yup, fix zup)
{ 
   get_align_matrix(m, xfront.v, yfront.v, zfront.v, xup.v, yup.v, zup.v);
}


inline void get_vector_rotation_matrix(MATRIX *m, fix x, fix y, fix z, fix a)
{ 
   get_vector_rotation_matrix(m, x.v, y.v, z.v, a.v);
}


inline void get_transformation_matrix(MATRIX *m, fix scale, fix xrot, fix yrot, fix zrot, fix x, fix y, fix z)
{ 
   get_transformation_matrix(m, scale.v, xrot.v, yrot.v, zrot.v, x.v, y.v, z.v);
}


inline void get_camera_matrix(MATRIX *m, fix x, fix y, fix z, fix xfront, fix yfront, fix zfront, fix xup, fix yup, fix zup, fix fov, fix aspect)
{ 
   get_camera_matrix(m, x.v, y.v, z.v, xfront.v, yfront.v, zfront.v, xup.v, yup.v, zup.v, fov.v, aspect.v);
}


inline void qtranslate_matrix(MATRIX *m, fix x, fix y, fix z)
{
   qtranslate_matrix(m, x.v, y.v, z.v);
}


inline void qscale_matrix(MATRIX *m, fix scale)
{
   qscale_matrix(m, scale.v);
}


inline fix vector_length(fix x, fix y, fix z)
{ 
   fix t;
   t.v = vector_length(x.v, y.v, z.v);
   return t;
}


inline void normalize_vector(fix *x, fix *y, fix *z)
{ 
   normalize_vector(&x->v, &y->v, &z->v);
}


inline void cross_product(fix x1, fix y1, fix z1, fix x2, fix y2, fix z2, fix *xout, fix *yout, fix *zout)
{ 
   cross_product(x1.v, y1.v, z1.v, x2.v, y2.v, z2.v, &xout->v, &yout->v, &zout->v);
}


inline fix dot_product(fix x1, fix y1, fix z1, fix x2, fix y2, fix z2)
{ 
   fix t;
   t.v = dot_product(x1.v, y1.v, z1.v, x2.v, y2.v, z2.v);
   return t;
}


inline void apply_matrix(MATRIX *m, fix x, fix y, fix z, fix *xout, fix *yout, fix *zout)
{ 
   apply_matrix(m, x.v, y.v, z.v, &xout->v, &yout->v, &zout->v);
}


inline void persp_project(fix x, fix y, fix z, fix *xout, fix *yout)
{ 
   persp_project(x.v, y.v, z.v, &xout->v, &yout->v);
}


extern "C" {

#endif   /* ifdef __cplusplus, back to C code */



/**************************************/
/************ GUI routines ************/
/**************************************/


AL_INLINE(int, object_message, (DIALOG *d, int msg, int c),
{
   int ret;

   if (msg == MSG_DRAW) {
      if (d->flags & D_HIDDEN) return D_O_K;
      acquire_screen();
   }

   ret = d->proc(msg, d, c);

   if (msg == MSG_DRAW)
      release_screen();

   if (ret & D_REDRAWME) {
      d->flags |= D_DIRTY;
      ret &= ~D_REDRAWME;
   }

   return ret;
})



/****************************************************/
/************ For backward compatibility ************/
/****************************************************/


#ifndef ALLEGRO_NO_CLEAR_BITMAP_ALIAS
   #if (defined ALLEGRO_GCC)
      static __attribute__((unused)) inline void clear(BITMAP *bmp)
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

#define KB_NORMAL       1
#define KB_EXTENDED     2

#define SEND_MESSAGE        object_message
#define OLD_FILESEL_WIDTH   -1
#define OLD_FILESEL_HEIGHT  -1

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

