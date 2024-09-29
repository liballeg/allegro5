/*
 * Example of using D3D calls
 * by Jacob Dawid & Trent Gamblin
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_direct3d.h>
#include <d3dx9.h>
#include <windows.h>
#include <cstdio>

#include "common.c"

#define D3DFVF_CUSTOMVERTEX   (D3DFVF_XYZ | D3DFVF_DIFFUSE)

struct D3DVERTEX
{
   float fX, fY, fZ;
   DWORD dwColor;
};

int main(int argc, char *argv[])
{
   A5O_DISPLAY *display;
   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Error initialising Allegro.\n");
   }

   open_log();

   al_install_keyboard();
   al_set_new_display_flags(A5O_DIRECT3D);
   al_set_new_display_option(A5O_DEPTH_SIZE, 16, A5O_SUGGEST);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to create display.\n");
   }

   A5O_KEYBOARD_STATE state;

   IDirect3DDevice9 *d3dd = al_get_d3d_device(display);

   d3dd->SetRenderState(D3DRS_AMBIENT, 0x11111111);
   d3dd->SetRenderState(D3DRS_LIGHTING,false);
   d3dd->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
   d3dd->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
   d3dd->SetFVF(D3DFVF_CUSTOMVERTEX);

   D3DXMATRIX m_matProjection;
   float m_fAspectRatio = 1.0f;
   float m_fFieldOfView = D3DX_PI / 4.0f;
   float m_fNearPlane   = 1.0f;
   float m_fFarPlane    = 1000.0f;

   D3DXMatrixPerspectiveFovLH( &m_matProjection, m_fFieldOfView,
      m_fAspectRatio, m_fNearPlane, m_fFarPlane);

   d3dd->SetTransform(D3DTS_PROJECTION, &m_matProjection);

   LPDIRECT3DVERTEXBUFFER9 pTriangleVB = NULL;
   VOID *pData;

   D3DVERTEX aTriangle[] = {
      { -0.5,  -0.5f, 0.0f, 0xffff0000 },
      {  0.5f, -0.5f, 0.0f, 0xff00ff00 },
      {  0.0f,  0.5f, 0.0f, 0xff0000ff }
   };

   d3dd->CreateVertexBuffer(sizeof(aTriangle), D3DUSAGE_WRITEONLY,
      D3DFVF_CUSTOMVERTEX, D3DPOOL_DEFAULT, &pTriangleVB, NULL);

   pTriangleVB->Lock(0, sizeof(pData), (void **)&pData, 0);
   memcpy(pData, aTriangle, sizeof(aTriangle));
   pTriangleVB->Unlock();

   float angle = 0.0f;
   D3DXMATRIX mat;
   double start = al_get_time();
   double start_secs = al_get_time();
   long frames = 0;

   do {
      d3dd->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

      /*
       * The X and Y translation here is to offset
       * a translation Allegro does internally so
       * that its coordinate space matches OpenGL's.
       * Normally you don't have to worry about this
       * but if you're using D3D directly you do.
       */
      D3DXMatrixTranslation(&mat, 0.5, 0.5, 2.0f);
      d3dd->SetTransform(D3DTS_WORLDMATRIX(0), &mat);
      angle += 50 * (al_get_time() - start);
      D3DXMatrixRotationY(&mat, angle);
      d3dd->MultiplyTransform(D3DTS_WORLDMATRIX(0), &mat);

      d3dd->SetFVF(D3DFVF_CUSTOMVERTEX);
      d3dd->SetStreamSource(0, pTriangleVB, 0, sizeof(D3DVERTEX));
      d3dd->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 1);

      al_flip_display();

      al_get_keyboard_state(&state);

      start = al_get_time();
      frames++;
   } while (!al_key_down(&state, A5O_KEY_ESCAPE));

   double elapsed = al_get_time() - start_secs;
   log_printf("%g fps\n", frames/elapsed);
   al_destroy_display(display);
   close_log(true);

   return 0;
}
