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
 *      Functions for drawing to GDI without using DirectX.
 *
 *      By Marian Dvorsky.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif


static HPALETTE current_hpalette = NULL;



/* set_gdi_color_format:
 *  Sets right values for pixel color format to work with GDI as well.
 */
void set_gdi_color_format()
{
   _rgb_r_shift_15 = 10;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;

   _rgb_r_shift_16 = 11;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;

   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;

   _rgb_r_shift_32 = 16;
   _rgb_g_shift_32 = 8;
   _rgb_b_shift_32 = 0;
}



/* destroy_hpalette:
 *  Destroys main hpalette at the end of the program.
 */
static void destroy_hpalette(void)
{
   if (current_hpalette) {
      DeleteObject(current_hpalette);
      current_hpalette = NULL;
   }

   _remove_exit_func(destroy_hpalette);
}



/* set_palette_to_hdc:
 *  Selects and realizes an Allegro PALETTE to a specified DC.
 */
void set_palette_to_hdc(HDC dc, PALETTE pal)
{
   PALETTEENTRY palPalEntry[256];
   int x;

   if (current_hpalette) {
      for (x = 0; x < 256; x++) {
	 palPalEntry[x].peRed = _rgb_scale_6[pal[x].r];
	 palPalEntry[x].peGreen = _rgb_scale_6[pal[x].g];
	 palPalEntry[x].peBlue = _rgb_scale_6[pal[x].b];
	 palPalEntry[x].peFlags = 0;
      }

      SetPaletteEntries(current_hpalette, 0, 256, (LPPALETTEENTRY) & palPalEntry);
   }
   else {
      current_hpalette = convert_palette_to_hpalette(pal);
      _add_exit_func(destroy_hpalette);
   }

   SelectPalette(dc, current_hpalette, FALSE);
   RealizePalette(dc);
   select_palette(pal);
}



/* convert_palette_to_hpalette:
 *  Converts Allegro PALETTE to Windows HPALETTE
 */
HPALETTE convert_palette_to_hpalette(PALETTE pal)
{
   HPALETTE hpal;
   LOGPALETTE *lp;
   int x;

   lp = (LOGPALETTE *) LocalAlloc(LPTR, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * 256);
   if (!lp)
      return NULL;

   lp->palNumEntries = 256;
   lp->palVersion = 0x300;

   for (x = 0; x < 256; x++) {
      lp->palPalEntry[x].peRed = _rgb_scale_6[pal[x].r];
      lp->palPalEntry[x].peGreen = _rgb_scale_6[pal[x].g];
      lp->palPalEntry[x].peBlue = _rgb_scale_6[pal[x].b];
      lp->palPalEntry[x].peFlags = 0;
   }

   hpal = CreatePalette(lp);

   LocalFree((HANDLE) lp);

   return hpal;
}



/* convert_hpalette_to_palette:
 *  Converts Windows HPALETTE to Allegro PALETTE
 */
void convert_hpalette_to_palette(HPALETTE hpal, PALETTE pal)
{
   PALETTEENTRY lp[256];
   int x;

   GetPaletteEntries(hpal, 0, 256, (LPPALETTEENTRY) & lp);

   for (x = 0; x < 256; x++) {
      pal[x].r = lp[x].peRed >> 2;
      pal[x].g = lp[x].peGreen >> 2;
      pal[x].b = lp[x].peBlue >> 2;
   }
}



/* get_dib:
 *  Creates device independent bitmap (DIB) from Allegro BITMAP.
 *  You have to free memory returned by this function by calling free()
 */
static BYTE *get_dib(BITMAP * bitmap)
{
   int bpp;
   int x, y;
   int pitch;
   int col;
   BYTE *pixels;
   BYTE *src, *dst;

   bpp = bitmap_color_depth(bitmap);
   pitch = bitmap->w * BYTES_PER_PIXEL(bpp);
   pitch = (pitch + 3) & ~3;	/* align on dword */

   pixels = (BYTE *) malloc(bitmap->h * pitch);
   if (!pixels)
      return NULL;

   /* this code will probably need optimalization. if anybody can
    * optimalize it, do so!
    */
   switch (bpp) {

      case 8:
	 for (y = 0; y < bitmap->h; y++) {
	    memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w);
	 }
	 break;

      case 15:
	 if ((_rgb_r_shift_15 == 10) && (_rgb_g_shift_15 == 5) && (_rgb_b_shift_15 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w * 2);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       src = bitmap->line[y];
	       dst = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(WORD *) (src);
		  *((WORD *) (dst)) = (WORD) ((getb15(col) >> 3) | ((getg15(col) >> 3) << 5) | ((getr15(col) >> 3) << 10));
		  src += 2;
		  dst += 2;
	       }
	    }
	 }
	 break;

      case 16:
	 for (y = 0; y < bitmap->h; y++) {
	    src = bitmap->line[y];
	    dst = pixels + y * pitch;

	    for (x = 0; x < bitmap->w; x++) {
	       col = *(WORD *) (src);
	       *((WORD *) (dst)) = (WORD) ((getb16(col) >> 3) | ((getg16(col) >> 3) << 5) | ((getr16(col) >> 3) << 10));
	       src += 2;
	       dst += 2;
	    }
	 }
	 break;

      case 24:
	 if ((_rgb_r_shift_24 == 16) && (_rgb_g_shift_24 == 8) && (_rgb_b_shift_24 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w * 3);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       src = bitmap->line[y];
	       dst = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(DWORD *) (src);
		  src += 3;
		  *(dst++) = getb24(col);
		  *(dst++) = getg24(col);
		  *(dst++) = getr24(col);
	       }
	    }
	 }
	 break;

      case 32:
	 if ((_rgb_r_shift_32 == 16) && (_rgb_g_shift_32 == 8) && (_rgb_b_shift_32 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(pixels + y * pitch, bitmap->line[y], bitmap->w * 4);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       src = bitmap->line[y];
	       dst = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(DWORD *) (src);
		  src += 4;
		  *(dst++) = getb32(col);
		  *(dst++) = getg32(col);
		  *(dst++) = getr32(col);
		  dst++;
	       }
	    }
	 }
	 break;
   }

   return pixels;
}



/* get_dib_from_hbitmap:
 *  Returns pointer to DIB created from hbitmap. You have to free memory returned by this
 *  function.
 */
static BYTE *get_dib_from_hbitmap(int bpp, HBITMAP hbitmap)
{
   BITMAPINFOHEADER bi;
   BITMAPINFO *binfo;
   HDC hdc;
   HPALETTE hpal, holdpal;
   int col;
   WINDOWS_BITMAP bm;
   int pitch;
   BYTE *pixels;
   BYTE *ptr;
   int x, y;

   if (!hbitmap)
      return NULL;

   if (bpp == 15)
      bpp = 16;

   if (!GetObject(hbitmap, sizeof(bm), (LPSTR) & bm))
      return NULL;

   pitch = bm.bmWidth * BYTES_PER_PIXEL(bpp);
   pitch = (pitch + 3) & ~3;	/* align on dword */

   pixels = (BYTE *) malloc(bm.bmHeight * pitch);
   if (!pixels)
      return NULL;

   ZeroMemory(&bi, sizeof(BITMAPINFOHEADER));

   bi.biSize = sizeof(BITMAPINFOHEADER);
   bi.biBitCount = bpp;
   bi.biPlanes = 1;
   bi.biWidth = bm.bmWidth;
   bi.biHeight = -abs(bm.bmHeight);
   bi.biClrUsed = 256;
   bi.biCompression = BI_RGB;

   binfo = malloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);
   binfo->bmiHeader = bi;

   hdc = GetDC(NULL);

   hpal = convert_palette_to_hpalette(_current_palette);
   holdpal = SelectPalette(hdc, hpal, TRUE);
   RealizePalette(hdc);

   GetDIBits(hdc, hbitmap, 0, bm.bmHeight, pixels, binfo, DIB_RGB_COLORS);

   ptr = pixels;

   /* This never occurs, because if screen or memory bitmap is 8 bit, 
    * we ask windows to convert it to true color. It "safer", but little 
    * bit slower.
    */

   if (bpp == 8) {
      for (y = 0; y < bm.bmWidth; y++) {
	 for (x = 0; x < bm.bmHeight; x++) {
	    col = *ptr;

	    if ((col < 10) || (col >= 246)) {
	       /* we have to remap colors from system palette */
	       *(ptr++) = makecol8(binfo->bmiColors[col].rgbRed, binfo->bmiColors[col].rgbGreen, binfo->bmiColors[col].rgbBlue);
	    }
	    else {
	       /* our palette is shifted by 10 */
	       *(ptr++) = col - 10;
	    }
	 }
      }
   }

   free(binfo);

   SelectPalette(hdc, holdpal, TRUE);
   DeleteObject(hpal);
   ReleaseDC(NULL, hdc);

   return pixels;
}



/* copy_dib_to_bitmap:
 *  Copies 1:1 DIB to Allegro BITMAP. They have to be same w, h, and bpp!
 */
static void copy_dib_to_bitmap(BYTE * pixels, BITMAP * bitmap)
{
   int bpp;
   int x, y;
   int pitch;
   int col;
   int b, g, r;
   BYTE *src, *dst;

   bpp = bitmap_color_depth(bitmap);
   pitch = bitmap->w * BYTES_PER_PIXEL(bpp);
   pitch = (pitch + 3) & ~3;	/* align on dword */

   /* this code will probably need optimalization. if anybody can
    * optimalize it, do so!
    */
   switch (bpp) {

      case 8:
	 for (y = 0; y < bitmap->h; y++) {
	    memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w);
	 }
	 break;

      case 15:
	 if ((_rgb_r_shift_15 == 10) && (_rgb_g_shift_15 == 5) && (_rgb_b_shift_15 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w * 2);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       dst = bitmap->line[y];
	       src = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  col = *(WORD *) (src);
		  *((WORD *) (dst)) = makecol15(_rgb_scale_5[(col >> 10) & 0x1F], _rgb_scale_5[(col >> 5) & 0x1F], _rgb_scale_5[col & 0x1F]);
		  src += 2;
		  dst += 2;
	       }
	    }
	 }
	 break;

      case 16:
	 for (y = 0; y < bitmap->h; y++) {
	    dst = bitmap->line[y];
	    src = pixels + y * pitch;

	    for (x = 0; x < bitmap->w; x++) {
	       col = *(WORD *) (src);
	       *((WORD *) (dst)) = makecol16(_rgb_scale_5[(col >> 10) & 0x1F], _rgb_scale_5[(col >> 5) & 0x1F], _rgb_scale_5[col & 0x1F]);
	       src += 2;
	       dst += 2;
	    }
	 }
	 break;

      case 24:
	 if ((_rgb_r_shift_24 == 16) && (_rgb_g_shift_24 == 8) && (_rgb_b_shift_24 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w * 3);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       dst = bitmap->line[y];
	       src = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  r = *(src++);
		  g = *(src++);
		  b = *(src++);
		  col = makecol24(r, g, b);
		  *((WORD *) dst) = (col & 0xFFFF);
		  dst += 2;
		  *(dst++) = (col >> 16);
	       }
	    }
	 }
	 break;

      case 32:
	 if ((_rgb_r_shift_32 == 16) && (_rgb_g_shift_32 == 8) && (_rgb_b_shift_32 == 0)) {
	    for (y = 0; y < bitmap->h; y++)
	       memcpy(bitmap->line[y], pixels + y * pitch, bitmap->w * 4);
	 }
	 else {
	    for (y = 0; y < bitmap->h; y++) {
	       dst = bitmap->line[y];
	       src = pixels + y * pitch;

	       for (x = 0; x < bitmap->w; x++) {
		  b = *(src++);
		  g = *(src++);
		  r = *(src++);
		  col = makecol32(r, g, b);
		  src++;
		  *((DWORD *) dst) = col;
		  dst += 4;
	       }
	    }
	 }
	 break;
   }
}



/* get_bitmap_info:
 *  Returns pointer to BITMAPINFO structure with bitmap info.
 *  You have to free this memory, when you no longer need it.
 *  Palette is color table for DIB
 */
static BITMAPINFO *get_bitmap_info(BITMAP * bitmap, PALETTE pal)
{
   BITMAPINFO *bi;
   int x;
   int bpp;

   bi = malloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256);

   bpp = bitmap_color_depth(bitmap);
   if (bpp == 15)
      bpp = 16;

   ZeroMemory(&bi->bmiHeader, sizeof(BITMAPINFOHEADER));

   bi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bi->bmiHeader.biBitCount = bpp;
   bi->bmiHeader.biPlanes = 1;
   bi->bmiHeader.biWidth = bitmap->w;
   bi->bmiHeader.biHeight = -bitmap->h;
   bi->bmiHeader.biClrUsed = 256;
   bi->bmiHeader.biCompression = BI_RGB;

   if (pal) {
      for (x = 0; x < 256; x++) {
	 bi->bmiColors[x].rgbRed = _rgb_scale_6[pal[x].r];
	 bi->bmiColors[x].rgbGreen = _rgb_scale_6[pal[x].g];
	 bi->bmiColors[x].rgbBlue = _rgb_scale_6[pal[x].b];
	 bi->bmiColors[x].rgbReserved = 0;
      }
   }

   return bi;
}



/* convert_bitmap_to_hbitmap:
 *  Converts Allegro BITMAP to device dependent bitmap HBITMAP.
 */
HBITMAP convert_bitmap_to_hbitmap(BITMAP * bitmap)
{
   HDC hdc;
   HBITMAP hbmp;
   BITMAPINFO *bi;
   HPALETTE hpal, holdpal;
   BYTE *pixels;

   pixels = get_dib(bitmap);

   /* when we have DIB, we can convert it to DDB */
   hdc = GetDC(NULL);

   bi = get_bitmap_info(bitmap, NULL);

   hpal = convert_palette_to_hpalette(_current_palette);
   holdpal = SelectPalette(hdc, hpal, TRUE);
   RealizePalette(hdc);
   hbmp = CreateDIBitmap(hdc, &bi->bmiHeader, CBM_INIT, pixels, bi, DIB_RGB_COLORS);
   ReleaseDC(NULL, hdc);

   SelectPalette(hdc, holdpal, TRUE);
   DeleteObject(hpal);

   free(pixels);
   free(bi);

   return hbmp;
}



/* convert_hbitmap_to_bitmap:
 *  Converts HBITMAP to Allegro BITMAP.
 */
BITMAP *convert_hbitmap_to_bitmap(HBITMAP bitmap)
{
   BYTE *pixels;
   BITMAP *bmp;
   WINDOWS_BITMAP bm;
   int bpp;

   if (!GetObject(bitmap, sizeof(bm), (LPSTR) & bm))
      return NULL;

   if (bm.bmBitsPixel == 8) {
      /* ask windows to save truecolor image, then convert to our format */
      bpp = 24;
   }
   else
      bpp = bm.bmBitsPixel;

   pixels = get_dib_from_hbitmap(bpp, bitmap);

   bmp = create_bitmap_ex(bpp, bm.bmWidth, bm.bmHeight);

   copy_dib_to_bitmap(pixels, bmp);

   free(pixels);

   return bmp;
}



/* draw_to_hdc:
 *  Draws entire Allegro BITMAP to HDC. See blit_to_hdc() for details.
 */
void draw_to_hdc(HDC dc, BITMAP * bitmap, int x, int y)
{
   stretch_blit_to_hdc(bitmap, dc, 0, 0, bitmap->w, bitmap->h, x, y, bitmap->w, bitmap->h);
}



/* blit_to_hdc:
 *  Blits Allegro BITMAP to HDC. Has similar syntax as Allegro blit().
 */
void blit_to_hdc(BITMAP * bitmap, HDC dc, int src_x, int src_y, int dest_x, int dest_y, int w, int h)
{
   stretch_blit_to_hdc(bitmap, dc, src_x, src_y, w, h, dest_x, dest_y, w, h);
}



/* stretch_blit_to_hdc:
 *  Blits Allegro BITMAP to HDC. Has similar syntax as Allegro stretch_blit().
 */
void stretch_blit_to_hdc(BITMAP * bitmap, HDC dc, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h)
{
   BYTE *pixels;
   BITMAPINFO *bi;

   pixels = get_dib(bitmap);
   bi = get_bitmap_info(bitmap, _current_palette);

   StretchDIBits(dc, dest_x, dest_y, dest_w, dest_h, src_x, bitmap->h - src_y - src_h, src_w, src_h, pixels, bi, DIB_RGB_COLORS, SRCCOPY);

   free(pixels);
   free(bi);
}



/* blit_from_hdc:
 *  Blits from HDC to Allegro BITMAP. Has similar syntax as Allegro blit().
 */
void blit_from_hdc(HDC dc, BITMAP * bitmap, int src_x, int src_y, int dest_x, int dest_y, int w, int h)
{
   stretch_blit_from_hdc(dc, bitmap, src_x, src_y, w, h, dest_x, dest_y, w, h);
}



/* stretch_blit_from_hdc:
 *  Blits from HDC to Allegro BITMAP. Has similar syntax as Allegro stretch_blit().
 */
void stretch_blit_from_hdc(HDC dc, BITMAP * bitmap, int src_x, int src_y, int src_w, int src_h, int dest_x, int dest_y, int dest_w, int dest_h)
{
   HBITMAP hbmp, holdbmp;
   HDC hmemdc;
   BITMAP *newbmp;

   hmemdc = CreateCompatibleDC(dc);
   hbmp = CreateCompatibleBitmap(dc, dest_w, dest_h);

   holdbmp = SelectObject(hmemdc, hbmp);

   StretchBlt(hmemdc, 0, 0, dest_w, dest_h, dc, src_x, src_y, src_w, src_h, SRCCOPY);
   SelectObject(hmemdc, holdbmp);

   newbmp = convert_hbitmap_to_bitmap(hbmp);
   blit(newbmp, bitmap, 0, 0, dest_x, dest_y, dest_w, dest_h);

   destroy_bitmap(newbmp);
   DeleteObject(hbmp);
   DeleteDC(hmemdc);
}
