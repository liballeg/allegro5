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
 *      DrawSprocket GFX drivers.
 *
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/aintern.h"
#include "macalleg.h"
#include "allegro/aintmac.h"
#include <string.h>

#define TRACE_MAC_GFX 0
/*Our main display device the display which contains the menubar on macs with more one display*/
GDHandle MainGDevice;

/*Our main Color Table for indexed devices*/
CTabHandle MainCTable=NULL;

/*Our current deph*/
short DrawDepth;

/*Vsync has ocurred*/
volatile short _sync=0;

/*Vsync handler installed?*/
short _syncOk=0;

/*the control State of DrawSprocket*/
short State=0;
/*Our DrawSprocket context*/
DSpContextReference   theContext;
/*Our Buffer Underlayer Not Used Yet*/
DSpAltBufferReference   theUnderlay;

Boolean  MyVBLTask(DSpContextReference inContext, void *inRefCon);

#pragma mark GFX_DRIVER
GFX_DRIVER gfx_drawsprocket ={
   GFX_DRAWSPROCKET,
   "",                     //AL_CONST char *name;
   "",                     //AL_CONST char *desc;
   "DrawSprocket",         //AL_CONST char *ascii_name;
   init_drawsprocket,      //AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
   exit_drawsprocket,      //AL_METHOD(void, exit, (struct BITMAP *b));
   NULL,                   //AL_METHOD(int, scroll, (int x, int y));
   vsync_drawsprocket,     //AL_METHOD(void, vsync, (void));
   set_palette_drawsprocket,//AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync));
   NULL,                   //AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                   //AL_METHOD(int, poll_scroll, (void));
   NULL,                   //AL_METHOD(void, enable_triple_buffer, (void));
   NULL,                   //AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
   NULL,                   //AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
   NULL,                   //AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
   NULL,                   //AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
   _mac_create_system_bitmap,   //AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
   _mac_destroy_system_bitmap,   //AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
   NULL,                  //AL_METHOD(int, set_mouse_sprite, (AL_CONST struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                  //AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                  //AL_METHOD(void, hide_mouse, (void));
   NULL,                  //AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                  //AL_METHOD(void, drawing_mode, (void));
   NULL,                  //AL_METHOD(void, save_video_state, (void));
   NULL,                  //AL_METHOD(void, restore_video_state, (void));
   640, 480,               // int w, h         /* physical (not virtual!) screen size */
   TRUE,                  // int linear      /* true if video memory is linear */
   0,                     // long bank_size;   /* bank size, in bytes */
   0,                     // long bank_gran;   /* bank granularity, in bytes */
   0,                     // long vid_mem;   /* video memory size, in bytes */
   0,                     // long vid_phys_base;/* physical address of video memory */
};

#pragma mark gfx driver routines
BITMAP *init_drawsprocket(int w, int h, int v_w, int v_h, int color_depth){
   OSStatus theError;
   CGrafPtr cg;
   BITMAP* b;
   DSpContextAttributes Attr;
   Fixed myfreq;

   
#if(TRACE_MAC_GFX)
   fprintf(stdout,"init_drawsprocket(%d, %d, %d, %d, %d)\n",w,h,v_w,v_h,color_depth);
   fflush(stdout);
#endif

   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0)) return (NULL);
   State|=kRDDStarted;
   Attr.frequency = _refresh_rate_request;
   Attr.reserved1 = 0;
   Attr.reserved2 = 0;
   Attr.colorNeeds = kDSpColorNeeds_Require;
   Attr.colorTable = MainCTable;
   Attr.contextOptions = 0;
   Attr.gameMustConfirmSwitch = false;
   Attr.reserved3[0] = 0;
   Attr.reserved3[1] = 0;
   Attr.reserved3[2] = 0;
   Attr.reserved3[3] = 0;
   Attr.pageCount = 1;
   Attr.displayWidth = w;
   Attr.displayHeight = h;
   Attr.displayBestDepth = color_depth;
   
   _rgb_r_shift_15 = 10;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;
   _rgb_r_shift_16 = 10;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;
   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;
   _rgb_r_shift_32 = 16;
   _rgb_g_shift_32 = 8;
   _rgb_b_shift_32 = 0;
   
   switch(color_depth){
      case 8:
         DrawDepth=8;
         Attr.displayDepthMask = kDSpDepthMask_8;
         break;
      case 15:
         DrawDepth=15;
         Attr.displayDepthMask = kDSpDepthMask_16;
         break;
      case 24:
         DrawDepth=24;
         Attr.displayDepthMask = kDSpDepthMask_32;
         break;
      default:
         goto Error;
   };
   Attr.backBufferBestDepth = color_depth;
   Attr.backBufferDepthMask = Attr.displayDepthMask;
   
   if(DSpFindBestContext(&Attr,&theContext)!=noErr)goto Error;
   
   Attr.displayWidth = w;
   Attr.displayHeight = h;   
   
   Attr.contextOptions = 0;
   
   if(DSpContext_Reserve(theContext, &Attr)!=noErr)goto Error;
   State|=kRDDReserved;

   active_drawsprocket();   

   theError=DSpContext_SetVBLProc (theContext,MyVBLTask,(void *)refconst);
   if(theError==noErr){_syncOk=1;}
   else{_syncOk=0;};

   cg=GetFrontBuffer();

   b=_CGrafPtr_to_system_bitmap(cg);
   if(b){

/*DSpContext_GetMonitorFrequency  (DSpContextReference    inContext,
                                 Fixed *                outFrequency);
EXTERN_API_C( OSStatus )
DSpContext_SetMaxFrameRate      (DSpContextReference    inContext,
                                 UInt32                 inMaxFPS);
EXTERN_API_C( OSStatus )
DSpContext_GetMaxFrameRate      (DSpContextReference    inContext,
                                 UInt32 *               outMaxFPS);*/

      DSpContext_GetMonitorFrequency  (theContext,&myfreq);
      _current_refresh_rate = Fix2Long(myfreq);

      gfx_drawsprocket.w=w;
      gfx_drawsprocket.h=h;
      return b;
   };
Error: 
#if(TRACE_MAC_GFX)
   fprintf(stdout,"init_drawsprocket()failed\n");
   fflush(stdout);
#endif
   exit_drawsprocket(b);
   return NULL;
}
void exit_drawsprocket(struct BITMAP *b){
#pragma unused b
   OSStatus theError;
#if(TRACE_MAC_GFX)
   fprintf(stdout,"exit_drawsprocket()\n");
   fflush(stdout);
#endif
   if((State&kRDDStarted)!=0){
      if((State&kRDDReserved)!=0){
         theError=DSpContext_SetState(theContext, kDSpContextState_Inactive);
         theError=DSpContext_Release(theContext);
      };
   };
   State=0;
   gfx_drawsprocket.w=0;
   gfx_drawsprocket.h=0;
   DrawDepth=0;
};

void vsync_drawsprocket(void){
   if(_syncOk){
      _sync=0;
      while(!_sync){};
   };
};

void init_mypalette(){
   MainCTable=GetCTable(8);
   DetachResource((Handle) MainCTable);
};
   
void set_palette_drawsprocket(const struct RGB *p, int from, int to, int retracesync){
   int i;OSErr theError;
#if(TRACE_MAC_GFX)
   fprintf(stdout,"set_palette");
   fflush(stdout);
#endif
   if(MainCTable==NULL){
      init_mypalette();
   };
   for(i=from;i<=to;i++){
      (**MainCTable).ctTable[i].rgb.red = p[i].r*1040;
      (**MainCTable).ctTable[i].rgb.green = p[i].g*1040;
      (**MainCTable).ctTable[i].rgb.blue = p[i].b*1040;
   };
   if(retracesync)vsync_drawsprocket();
   if(DrawDepth==8){
      theError = DSpContext_SetCLUTEntries(theContext, (**MainCTable).ctTable, from, to - from);
      if( theError ) DebugStr("\p error from CLUT ");   
   };
};
short active_drawsprocket(){
   if(!(State&kRDDActive)){
      if(!(State&kRDDPaused))
	      if(DSpContext_SetState(theContext , kDSpContextState_Active )!=noErr)
    	     return 1;
      State&=(~kRDDPaused);
      State|=kRDDActive;
   };
   return 0;
};
short pause_drawsprocket(){
   
   if(!(State&kRDDPaused)){
      if(DSpContext_SetState(theContext, kDSpContextState_Paused)!=noErr)return 1;//Fatal("\pPause");
      State&=(~kRDDActive);
      State|=kRDDPaused;
      DrawMenuBar();
   };
   return 0;
};
short inactive_drawsprocket(){
   
   if(!(State&(kRDDPaused|kRDDActive))){
      if(DSpContext_SetState(theContext, kDSpContextState_Inactive)!=noErr)return 1;//Fatal("\pInactive");
      State&=(~kRDDPaused);
      State&=(~kRDDActive);
      DrawMenuBar();
   };
   return 0;
};

CGrafPtr GetBackBuffer(){
   CGrafPtr theBuffer;
   DSpContext_GetBackBuffer( theContext, kDSpBufferKind_Normal, &theBuffer );
   return theBuffer;
};

CGrafPtr GetFrontBuffer(){
   CGrafPtr theBuffer;
   DSpContext_GetFrontBuffer(theContext, &theBuffer);   
   return theBuffer;
};
void swap_drawsprocket(){
   DSpContext_SwapBuffers(theContext, nil, nil);
};
Boolean MyVBLTask (DSpContextReference inContext,void *inRefCon){
#pragma unused inContext,inRefCon
   _sync=1;
   return false;
};

