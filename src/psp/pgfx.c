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
 *      PSP gfx driver.
 *
 *      By diedel.
 *
 *      The blit to screen routine is based on the Basilisk II PSP refresh
 *      routines by J.F.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"
#include <stdio.h>
#include <pspdisplay.h>
#include <pspgu.h>

#ifndef ALLEGRO_PSP
   #error something is wrong with the makefile
#endif


#define BUF_WIDTH       640
#define SCR_WIDTH       480
#define SCR_HEIGHT      272
#define PIXEL_SIZE        4

static unsigned int FRAME_SIZE = BUF_WIDTH * SCR_HEIGHT * PIXEL_SIZE;
static unsigned int DISP_BUF = 0;
static unsigned int DRAW_BUF;
static unsigned int TEX_BUF;
static unsigned int DEBUG_BUF;


static enum PspDisplayPixelFormats psp_display_pixel_format;
static int gu_psm_format;

static unsigned int __attribute__((aligned(16))) list[16384];

struct TEX_VERTEX
{
   unsigned short u, v;
   short x, y, z;
};



static BITMAP *psp_display_init(int, int, int, int, int);
static void setup_gu(void);
static void psp_created_sub_bitmap(BITMAP *bmp, BITMAP *parent);
static void psp_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
static void psp_draw_to_screen(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);



GFX_DRIVER gfx_psp =
{
   GFX_PSP,
   empty_string,
   empty_string,
   "PSP gfx driver",
   psp_display_init,             /* AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth)); */
   NULL,                         /* AL_METHOD(void, exit, (struct BITMAP *b)); */
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   NULL,//sceDisplayWaitVblankStart,    /* AL_METHOD(void, vsync, (void)); */
   NULL,                         /* AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   NULL, //   create_psp_video_bitmap,                         /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   NULL, //   destroy_psp_video_bitmap,                        /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, show_video_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   NULL,                         /* AL_METHOD(int, fetch_mode_list, (void)); */
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE                         /* true if driver runs windowed */
};



/* psp_display_init:
 *  Initializes the gfx mode.
 *  If the screen dimensions (w, h) are greater than the PSP
 *  screen dimensions we downscale it.
 *  TODO: Support for virtual screens, scrolling and triple buffer.
 */
static BITMAP *psp_display_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *psp_screen;
   void *vram_start;
   int bytes_per_line;

   switch (color_depth) {
      // TODO: Support 8 and maybe 24 bpp.
      case 16:
         psp_display_pixel_format = PSP_DISPLAY_PIXEL_FORMAT_565;
         gu_psm_format = GU_PSM_5650;
         break;

      case 32:
         psp_display_pixel_format = PSP_DISPLAY_PIXEL_FORMAT_8888;
         gu_psm_format = GU_PSM_8888;
         break;

      default:
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
         return NULL;
   }

   //printf_file = fopen("printf.log", "w");

   //vram_start = (void *)sceGeEdramGetAddr();
   vram_start = (void *)( 0x40000000 | (uint32_t)sceGeEdramGetAddr() );
   bytes_per_line = BYTES_PER_PIXEL(color_depth) * BUF_WIDTH;
   psp_screen = _make_bitmap(w, h, (uintptr_t)vram_start, &gfx_psp, color_depth, bytes_per_line);
   if (!psp_screen) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }

   DRAW_BUF = FRAME_SIZE;
   TEX_BUF = DRAW_BUF + FRAME_SIZE;
   DEBUG_BUF = TEX_BUF + 64*4*576;

   setup_gu();
   //sceGuInit();
   //sceDisplaySetFrameBuf(vram_start, BUF_WIDTH, psp_display_pixel_format, PSP_DISPLAY_SETBUF_NEXTFRAME);

   /* physical (not virtual!) screen size */
   gfx_psp.w = psp_screen->cr = w;
   gfx_psp.h = psp_screen->cb = h;

   /* We install the blit accelerated versions and other specific functions. */
   __linear_vtable16.created_sub_bitmap = psp_created_sub_bitmap;
   _screen_vtable.blit_from_memory = psp_draw_to_screen;
   __linear_vtable16.blit_to_self = psp_blit_to_self;

   /* We register our load_bitmap_object routine. */
//   register_datafile_object(DAT_BITMAP, psp_load_bitmap_object, NULL);
   /* TODO: register our .pcx, .bmp, ... reading routines. */

   return psp_screen;
}



static void setup_gu(void)
{
   sceGuInit();
   sceGuStart(GU_DIRECT, list);
   sceGuDrawBuffer(gu_psm_format, 0, BUF_WIDTH);
   sceGuDispBuffer(SCR_WIDTH, SCR_HEIGHT, 0, BUF_WIDTH);
   sceGuDepthBuffer((void*)DEBUG_BUF,BUF_WIDTH);
   sceGuOffset(2048 - (SCR_WIDTH/2),2048 - (SCR_HEIGHT/2));
   sceGuViewport(2048,2048,SCR_WIDTH,SCR_HEIGHT);
   sceGuDepthRange(65535,0);
   sceGuDepthMask(GU_TRUE);
   sceGuDisable(GU_DEPTH_TEST);
   sceGuDisable(GU_BLEND);
   sceGuScissor(0,0,SCR_WIDTH,SCR_HEIGHT);
   sceGuEnable(GU_SCISSOR_TEST);
   sceGuFrontFace(GU_CW);
   sceGuEnable(GU_TEXTURE_2D);
   sceGuClear(GU_COLOR_BUFFER_BIT);
   sceGuFinish();
   sceGuSync(0,0);
   sceDisplayWaitVblankStart();
   sceGuDisplay(GU_TRUE);
}



/* psp_created_sub_bitmap:
 *  We need some parent bitmap info when blitting sub bitmaps.
 */
static void psp_created_sub_bitmap(BITMAP *bmp, BITMAP *parent)
{
   bmp->extra = malloc(sizeof(struct BMP_EXTRA_INFO));
   BMP_EXTRA(bmp)->parent = parent;
}



/* psp_blit_to_self:
 *  Accelerated ram -> ram blitting routine.
 */
static void psp_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   if (is_sub_bitmap(source)) {
      source_x = source->x_ofs + source_x;
      source_y = source->y_ofs + source_y;
      source=BMP_EXTRA(source)->parent;
   }

   if (is_sub_bitmap(dest)) {
      dest_x = dest->x_ofs + dest_x;
      dest_y = dest->y_ofs + dest_y;
      dest=BMP_EXTRA(dest)->parent;
   }

   /* The interesting part. */
   sceGuStart(GU_DIRECT,list);
   sceGuCopyImage(gu_psm_format, source_x, source_y, width, height, BMP_EXTRA(source)->pitch, source->line[0],
                   dest_x, dest_y, BMP_EXTRA(dest)->pitch, dest->line[0]);
   sceGuFinish();
   sceGuSync(0,0);

   return;
}



/* psp_draw_to_screen:
 *  Accelerated ram -> screen blitting routine using textures
 *  and maximizing the use of the texture-cache.
 */
static void psp_draw_to_screen(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   unsigned int tex_slice;
   int h_slice = 64;
   int v_slice = height;

   //int bpp = BYTES_PER_PIXEL(source->vtable->color_depth);
   int bpp = 2;

   int i, j;
   struct TEX_VERTEX *vertices;

   int temp=width/h_slice;
   //unsigned int base_addr= 0x40000000 | ((unsigned int)sceGeEdramGetAddr() + TEX_BUF);

   sceGuStart(GU_DIRECT,list);

   // copy the block to vram
   //sceGuCopyImage(gu_psm_format, 0, 0, BMP_EXTRA(source)->pitch, height, BMP_EXTRA(source)->pitch, source->dat, 0, 0, BMP_EXTRA(source)->pitch, (void*)base_addr);
   //sceGuTexSync();

   sceGuEnable(GU_TEXTURE_2D);
   sceGuTexMode(gu_psm_format,0,0,0);
   sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
   sceGuTexFilter(GU_NEAREST, GU_NEAREST);

   for (j=0; j<height/v_slice; j++)
      for (i=0; i<temp; i++) {

         /* The texture chunk to draw. */
         //tex_slice = base_addr + j * v_slice * 640 * bpp + i * h_slice * bpp;
         tex_slice = (unsigned int)(source->dat) + j * v_slice * 640 * bpp + i * h_slice * bpp;
         sceGuTexImage(0, h_slice, 512, BMP_EXTRA(source)->pitch, (void *)tex_slice);
         sceGuTexSync();

         vertices = (struct TEX_VERTEX*)sceGuGetMemory(2 * sizeof(struct TEX_VERTEX));

         /* These chunk texture coordinates map to... */
         vertices[0].u = 0;
         vertices[0].v = 0;

         vertices[1].u = h_slice;
         vertices[1].v = v_slice;

         /* ... these PSP screen coordinates. */
         vertices[0].x = i * h_slice * SCR_WIDTH / width;
         vertices[0].y = j * v_slice * SCR_HEIGHT / height;
         vertices[0].z = 0;

         vertices[1].x = (i+1) * h_slice * SCR_WIDTH / width;
         if (vertices[1].x > SCR_WIDTH)
            vertices[1].x = SCR_WIDTH;
         vertices[1].y = (j+1) * v_slice * SCR_HEIGHT / height;
         if (vertices[1].y > SCR_HEIGHT)
            vertices[1].y = SCR_HEIGHT;
         vertices[1].z = 0;

         /* Finally we draw the texture chunk. */
         sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 2,0,vertices);
      }

   sceGuFinish();
   sceGuSync(0,0);
}


/*static void scaled_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   if (!source->extra->scaled) {
      BITMAP *tmp = source;
      source = create_bitmap(source->w * scale_factor, source->h * scale_factor);
      if (!bmp) {
         *allegro_errno = ENOMEM;
         return NULL;
      }

      stretch_blit(tmp, source, 0, 0,
   }

   return;
}
*/
