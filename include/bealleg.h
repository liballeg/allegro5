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
 *      The Be Allegro Header File.
 *
 *      Aren't you glad you used the BeOS header file?
 *      Don't you wish everybody did?
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#ifndef BE_ALLEGRO_H
#define BE_ALLEGRO_H

/* There is namespace cross-talk between the Be API and
   Allegro, lets use my favorite namespace detergent.
   "Kludge!" Now with 25% more hack. */

#ifdef __cplusplus
# define color_map    al_color_map
# define drawing_mode al_drawing_mode
#endif

#include "allegro.h"

#ifdef __cplusplus
# undef color_map
# undef drawing_mode
# undef MIN
# undef MAX
# undef TRACE
# undef ASSERT

#ifdef DEBUGMODE
# define AL_ASSERT(condition)  { if (!(condition)) al_assert(__FILE__, __LINE__); }
# define AL_TRACE al_trace
#else
# define AL_ASSERT(condition)
# define AL_TRACE 1 ? (void) 0 : al_trace
#endif

#ifndef SCAN_DEPEND
   #include <Be.h>
#endif



class BeAllegroView
   : public BView
{
   public:
      BeAllegroView(BRect, const char *, uint32, uint32);
     ~BeAllegroView();

      void AttachedToWindow();
      void MessageReceived(BMessage *);
};



class BeAllegroWindow
   : public BDirectWindow
{
   public:
      BeAllegroWindow(BRect, const char *, window_look, window_feel, uint32, uint32, uint32, uint32, uint32);
     ~BeAllegroWindow();

      void DirectConnected(direct_buffer_info *);
      void MessageReceived(BMessage *);
      bool QuitRequested(void);
      void WindowActivated(bool);

      void *screen_data;	  
      void **screen_line; 
      uint32 screen_height;
      uint32 screen_depth;
      uint32 scroll_x;
      uint32 scroll_y;

      void **display_line;
      uint32 display_depth;

      uint32 num_rects;
      clipping_rect *rects;
      clipping_rect window;
      void (*blitter)(void **src, void **dest, int sx, int sy, int sw, int sh);

      bool connected;
      bool dying;
      BLocker *locker;
      thread_id	drawing_thread_id;
};



class BeAllegroScreen
   : public BWindowScreen
{
   public:
      BeAllegroScreen(const char *title, uint32 space, status_t *error, bool debugging = false);

      void MessageReceived(BMessage *);
      bool QuitRequested(void);
      void ScreenConnected(bool connected);
};



class BeAllegroApp
   : public BApplication
{
   public:
      BeAllegroApp(const char *signature);

      bool QuitRequested(void);
      void ReadyToRun(void);
};



extern BeAllegroApp    *be_allegro_app;
extern BeAllegroWindow *be_allegro_window;
extern BeAllegroView   *be_allegro_view; 
extern BeAllegroScreen *be_allegro_screen;

extern sem_id   be_mouse_view_attached;
extern BWindow *be_mouse_window;
extern BView   *be_mouse_view; 
extern bool     be_mouse_window_mode;


void be_terminate(thread_id caller);



#endif /* ifdef C++ */



#endif /* ifndef BE_ALLEGRO_H */
