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
 *      DirectX implementation of some of the primitive routines.
 *
 *
 *      By Pavel Sountsov.
 *      Implementation by Trent Gamblin.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_primitives.h"
#include "allegro5/internal/aintern_prim_directx.h"
#include "allegro5/internal/aintern_prim.h"

#include "allegro5/platform/alplatf.h"

#ifdef ALLEGRO_CFG_D3D
#include "allegro5/a5_direct3d.h"

#define A5V_FVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1)

static int al_blender_to_d3d(int al_mode)
{
   int num_modes = 4;

   int allegro_modes[] = {
      ALLEGRO_ZERO,
      ALLEGRO_ONE,
      ALLEGRO_ALPHA,
      ALLEGRO_INVERSE_ALPHA
   };

   int d3d_modes[] = {
      D3DBLEND_ZERO,
      D3DBLEND_ONE,
      D3DBLEND_SRCALPHA,
      D3DBLEND_INVSRCALPHA
   };

   int i;

   for (i = 0; i < num_modes; i++) {
      if (al_mode == allegro_modes[i]) {
         return d3d_modes[i];
      }
   }

   return D3DBLEND_ONE;
}

static void set_blender(ALLEGRO_DISPLAY *display)
{
   int src, dst, alpha_src, alpha_dst;
   ALLEGRO_COLOR color;
   LPDIRECT3DDEVICE9 device = al_d3d_get_device(display);

   al_get_separate_blender(&src, &dst, &alpha_src, &alpha_dst, &color);

   src = al_blender_to_d3d(src);
   dst = al_blender_to_d3d(dst);
   alpha_src = al_blender_to_d3d(alpha_src);
   alpha_dst = al_blender_to_d3d(alpha_dst);

   IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLEND, src);
   IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLEND, dst);

   IDirect3DDevice9_SetRenderState(device, D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
   IDirect3DDevice9_SetRenderState(device, D3DRS_SRCBLENDALPHA, alpha_src);
   IDirect3DDevice9_SetRenderState(device, D3DRS_DESTBLENDALPHA, alpha_dst);

   IDirect3DDevice9_SetTextureStageState(device, 0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
}
#endif

int _al_draw_prim_directx(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_D3D
   int num_primitives = 0;
   int num_vtx;
   ALLEGRO_VERTEX *vtx;
   ALLEGRO_DISPLAY *display;
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DBASETEXTURE9 d3d_texture;

   display = al_get_current_display();
   device = al_d3d_get_device(display);
   num_vtx = end - start;
   vtx = ((ALLEGRO_VERTEX*)vbuff->data) + start;

   set_blender(display);

   if (texture) {
      d3d_texture = (LPDIRECT3DBASETEXTURE9)al_d3d_get_video_texture(texture);
      IDirect3DDevice9_SetTexture(device, 0, d3d_texture);
   }
   else {
      IDirect3DDevice9_SetTexture(device, 0, NULL);
   }
   
   IDirect3DDevice9_SetFVF(device, A5V_FVF);

   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         num_primitives = num_vtx / 2;
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINELIST, num_primitives, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         num_primitives = num_vtx - 1;
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINESTRIP, num_primitives, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         ALLEGRO_VERTEX final[2];
         num_primitives = num_vtx;
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINESTRIP, num_primitives-1, vtx, sizeof(ALLEGRO_VERTEX));
         memcpy(&final[0], &vtx[num_vtx-1], sizeof(ALLEGRO_VERTEX));
         memcpy(&final[1], &vtx[0], sizeof(ALLEGRO_VERTEX));
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_LINELIST, 1, final, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         num_primitives = num_vtx / 3;
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLELIST, num_primitives, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         num_primitives = num_vtx - 2;
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLESTRIP, num_primitives, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         num_primitives = num_vtx - 2;
         IDirect3DDevice9_DrawPrimitiveUP(device, D3DPT_TRIANGLEFAN, num_primitives, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
   }

   return num_primitives;
#else
   (void)texture;
   (void)vbuff;
   (void)start;
   (void)end;
   (void)type;

   return 0;
#endif
}

int _al_draw_prim_indexed_directx(ALLEGRO_BITMAP* texture, ALLEGRO_VBUFFER* vbuff, int* indices, int num_vtx, int type)
{
#ifdef ALLEGRO_CFG_D3D
   int num_primitives = 0;
   ALLEGRO_VERTEX *vtx;
   ALLEGRO_DISPLAY *display;
   LPDIRECT3DDEVICE9 device;
   LPDIRECT3DBASETEXTURE9 d3d_texture;

   display = al_get_current_display();
   device = al_d3d_get_device(display);
   vtx = ((ALLEGRO_VERTEX*)vbuff->data);

   set_blender(display);

   if (texture) {
      d3d_texture = (LPDIRECT3DBASETEXTURE9)al_d3d_get_video_texture(texture);
      IDirect3DDevice9_SetTexture(device, 0, d3d_texture);
   }
   else {
      IDirect3DDevice9_SetTexture(device, 0, NULL);
   }
   
   IDirect3DDevice9_SetFVF(device, A5V_FVF);

   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         num_primitives = num_vtx / 2;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         num_primitives = num_vtx - 1;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINESTRIP, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         int in[2];
         num_primitives = num_vtx;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINESTRIP, 0, num_vtx, num_primitives-1, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         in[0] = indices[0];
         in[1] = indices[num_vtx-1];
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_LINELIST, 0, 2, 1, in, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         num_primitives = num_vtx / 3;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLELIST, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         num_primitives = num_vtx - 2;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLESTRIP, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         num_primitives = num_vtx - 2;
         IDirect3DDevice9_DrawIndexedPrimitiveUP(device, D3DPT_TRIANGLEFAN, 0, num_vtx, num_primitives, indices, D3DFMT_INDEX32, vtx, sizeof(ALLEGRO_VERTEX));
         break;
      };
   }

   return num_primitives;
#else
   (void)texture;
   (void)vbuff;
   (void)indices;
   (void)num_vtx;
   (void)type;

   return 0;
#endif
}

void _al_use_transform_directx(ALLEGRO_TRANSFORM* trans)
{
#ifdef ALLEGRO_CFG_D3D
   ALLEGRO_DISPLAY *display;
   LPDIRECT3DDEVICE9 device;

   display = al_get_current_display();
   device = al_d3d_get_device(display);

   IDirect3DDevice9_SetTransform(device, D3DTS_VIEW, (D3DMATRIX *)trans);
#else
   (void)trans;
#endif
}
