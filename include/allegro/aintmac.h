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
 *      Some definitions for internal use by the MacOs library code.
 *
 *      By Ronaldo H. Yamada.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTMAC_H
#define AINTMAC_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifndef ALLEGRO_MACOS
   #error bad include
#endif

#include "macalleg.h"

#ifdef __cplusplus
   extern "C" {
#endif

extern int _mac_init();
extern void _mac_exit();
extern void _mac_get_executable_name(char *output, int size);
extern void _mac_set_window_title(const char *name);
extern void _mac_message(const char *msg);
extern void _mac_assert(const char *msg);
extern void _mac_save_console_state(void);
extern void _mac_restore_console_state(void);
extern int _mac_desktop_color_depth(void);
extern void _mac_yield_timeslice(void);
extern int _mac_trace_handler(const char *msg);
extern void _mac_init_system_bitmap(void);
extern BITMAP *_mac_create_system_bitmap(int w, int h);
extern void _mac_destroy_system_bitmap(BITMAP *bmp);
extern void _mac_sys_set_clip(struct BITMAP *dst);
extern void _mac_sys_clear_to_color8(BITMAP *bmp, int color);
extern void _mac_sys_blit8(BITMAP *src, BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h);
extern void _mac_sys_selfblit8(BITMAP *src, BITMAP *dst, int src_x, int src_y, int dst_x, int dst_y, int w, int h);
extern int _mac_sys_triangle(struct BITMAP *bmp, int x1, int y1, int x2, int y2, int x3, int y3, int color);
extern void _mac_sys_rectfill8(struct BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
extern void _mac_sys_hline8(struct BITMAP *bmp, int x1, int y, int x2, int color);
extern void _mac_sys_vline8(struct BITMAP *bmp, int x, int y1, int y2, int color);
extern BITMAP *_CGrafPtr_to_system_bitmap(CGrafPtr cg);
extern BITMAP *init_drawsprocket(int w, int h, int v_w, int v_h, int color_depth);
extern void exit_drawsprocket(struct BITMAP *b);
extern void vsync_drawsprocket(void);
extern void init_mypalette();
extern void set_palette_drawsprocket(const struct RGB *p, int from, int to, int retracesync);
extern short active_drawsprocket();
extern short pause_drawsprocket();
extern short inactive_drawsprocket();
extern CGrafPtr GetBackBuffer();
extern CGrafPtr GetFrontBuffer();
extern void swap_drawsprocket();


extern void _al_findclose(void *dta);
extern int _al_findnext(void *pdta, char *nameret, int *aret);
extern void *_al_findfirst(const char *pattern, int attrib, char *nameret, int *aret);
extern int _al_file_isok(const char *filename);
extern int _al_file_exists(const char *filename, int attrib, int *aret);
extern long _al_file_size(const char *filename);
extern long _al_file_time(const char *filename);
extern int _al_getdrive(void);
extern void _al_getdcwd(int drive, char *buf, int size);
extern int _al_open(const char *filename, int mode);
extern char *strdup(const char *p);
extern long GetLogicalDrives();
extern Boolean DoWeHaveColor(void);
extern Boolean DoWeHaveSystem605(void);
extern void ptoc(StringPtr pstr, char *cstr);
extern short FindMainDevice(void);
extern Boolean RTrapAvailable(short tNumber, TrapType tType);
extern void maccleanup();
extern void MacEntry();


extern const RGBColor ForeDef;
extern const RGBColor BackDef;


/*The our QuickDraw globals */
extern QDGlobals qd;

/*app name*/
extern Str255 mname;

/*main volume number*/
extern short MainVol;

/*main directory ID*/
extern long MainDir;

/*Our main display device the display which contains the menubar on macs with more one display*/
extern GDHandle MainGDevice;

/*Our main Color Table for indexed devices*/
extern CTabHandle MainCTable;

/*Our current deph*/
extern short DrawDepth;

/*Vsync has ocurred*/
extern volatile short _sync;

/*Vsync handler installed?*/
extern short _syncOk;

/*???Used for DrawSprocket e Vsync callback*/
const char refconst[16];

/**/
extern short _mackeyboardinstalled;

/*Used for our keyboard driver*/
extern volatile KeyMap KeyNow;
extern volatile KeyMap KeyOld;

/**/
extern short _mactimerinstalled;

/**/
extern short _macmouseinstalled;


/*Our TimerManager task entry queue*/
extern TMTask   theTMTask;      

/*Our TM task is running*/
extern short _timer_running;

/*the control State of DrawSprocket*/
extern short State;

enum{kRDDNull   =0,
   kRDDStarted   =1,
   kRDDReserved=2,
   kRDDFadeOut   =4,
   kRDDActive   =8,
   kRDDPaused   =16,
   kRDDUnder   =32,
   kRDDOver   =64,
   kRDDouble   =128,
};
   
/*Our DrawSprocket context*/
extern DSpContextReference   theContext;
/*Our Buffer Underlayer Not Used Yet*/
extern DSpAltBufferReference   theUnderlay;


/*Our window title ???*/
extern char macwindowtitle[256];

/*char buffer for Trace*/
extern char macsecuremsg[256];


#ifdef __cplusplus
   }
#endif


#endif          /* ifndef AINTMAC_H */

