/*
 * Example of using D3D calls
 * by Jacob Dawid
 */
#include <allegro5/allegro5.h>
#include <allegro5/winalleg.h>
#include <d3d9.h>              
#include <d3dx9.h>       

using namespace std;

#define D3DFVF_CUSTOMVERTEX   (D3DFVF_XYZ | D3DFVF_DIFFUSE)   
struct D3DVERTEX
{
   float fX, fY, fZ;
   DWORD dwColor;
};

int main(void)
{
   ALLEGRO_DISPLAY *display;
      
   al_init();
   al_install_keyboard();
   al_set_new_display_flags(ALLEGRO_DIRECT3D);
   display = al_create_display(640, 480);
   
   ALLEGRO_KBDSTATE state;
   
   IDirect3DDevice9 *lpD3DD = al_d3d_get_device();
   
   lpD3DD->SetRenderState(D3DRS_AMBIENT, 0x11111111);   
   lpD3DD->SetRenderState(D3DRS_LIGHTING,false);         
   lpD3DD->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);      
   lpD3DD->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE);
   lpD3DD->SetFVF(D3DFVF_CUSTOMVERTEX);       

   D3DXMATRIX m_matProjection;         
   float m_fAspectRatio = 1.0f,            
         m_fFieldOfView = D3DX_PI / 4.0f,            
         m_fNearPlane   = 1.0f,            
         m_fFarPlane    = 1000.0f;     
                      
   D3DXMatrixPerspectiveFovLH( &m_matProjection,
                         m_fFieldOfView,
                         m_fAspectRatio,
                         m_fNearPlane,
                         m_fFarPlane);
                         
   lpD3DD->SetTransform(D3DTS_PROJECTION, &m_matProjection);
   
   LPDIRECT3DVERTEXBUFFER9 pTriangleVB = NULL;
   VOID* pData;   
   
   D3DVERTEX aTriangle[ ] = {
      { -0.5,  -0.5f, 2.0f, 0xffff0000 },
      {  0.5f, -0.5f, 2.0f, 0xff00ff00 },
      {  0.0f,  0.5f, 2.0f, 0xff0000ff }
   };  
                       
   lpD3DD->CreateVertexBuffer(  sizeof(aTriangle),
                       D3DUSAGE_WRITEONLY,
                       D3DFVF_CUSTOMVERTEX,
                       D3DPOOL_MANAGED,
                       &pTriangleVB,
                       NULL);
                       
   pTriangleVB->Lock(0,sizeof(pData),(void**)&pData,0);
   memcpy(pData,aTriangle,sizeof(aTriangle));       
   pTriangleVB->Unlock();                  
  
   do
   {
      lpD3DD->Clear(0,NULL,D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),1.0f,0);
    
      lpD3DD->SetStreamSource(0,pTriangleVB,0,sizeof(D3DVERTEX));     
      lpD3DD->DrawPrimitive(D3DPT_TRIANGLELIST,0,1);         
    
      al_flip_display();

      al_get_keyboard_state( &state );   
   }
   while( !al_key_down( &state, KEY_ESC ) );
   
   return 0;
}
END_OF_MAIN()

