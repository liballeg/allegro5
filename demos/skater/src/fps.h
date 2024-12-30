/* Module for measuring FPS for Allegro games.
   Written by Miran Amon.
   version: 1.02
   date: 27. June 2003
*/

#ifndef                __DEMO_FPS_H__
#define                __DEMO_FPS_H__

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Underlaying structure for measuring FPS. The user should create an instance
   of this structure with create_fps() and destroy it with destroy_fps().
   The user should not use the contents of the structure directly,
   functions for manipulating with the FPS structure should be used instead.
*/
   typedef struct FPS {
      int *samples;
      int nSamples;
      int curSample;
      int frameCounter;
      int framesPerSecond;
   } FPS;


/* Creates an instance of the FPS structure and initializes it. The user
   should call this function somewhere at the beginning of the program and
   use the value it returns to access the current FPS.

   Parameters:
      int fps - the frequency of the timer used to control the speed of the program

   Returns:
      FPS *fps - an instance of the FPS structure
*/
   FPS *create_fps(int fps);


/* Destroys an FPS object. The user should call this function when he's
   done measuring the FPS, i.e. at the end of the program.

   Parameters:
      FPS *fps - a pointer to an FPS object

   Returns:
      nothing
*/
   void destroy_fps(FPS * fps);


/* Updates the FPS measuring logic. The user should call this function exactly
   fps times per second where fps is the speed of their timer. Usually this is
   implemented in the logic code, i.e. in the while(timer_counter) loop or
   an equivalent.

   Parameters:
      FPS *fps - a pointer to an FPS object

   Returns:
      nothing
*/
   void fps_tick(FPS * fps);


/* Counts the number of drawn frames. The user should call this function
   every time they draw their frame.

   Parameters:
      FPS *fps - a pointer to an FPS object

   Returns:
      nothing
*/
   void fps_frame(FPS * fps);


/* Retreives the current frame rate in frames per second. This will actually
   be the average frame rate over the last second.

   Parameters:
      FPS *fps - a pointer to an FPS object

   Returns:
      int fps - the average frame rate over the last second
*/
   int get_fps(FPS * fps);


/* Draws the current frame rate on the specified bitmap with the specified
   parameters.

   Parameters:
      FPS *fps     - a pointer to an FPS object
      ALLEGRO_BITMAP *bmp  - destination bitmap
      FONT *font   - the font used for printing the fps
      int x, y     - position of the fps text on the destination bitmap
      int color    - the color of the fps text
      char *format - the format of the text as you would pass to printf(); the
                     format should contain whatever text you wish to print and
                     a format sequence for outputting an integer;

   Returns:
      nothing

   Example:
      draw_fps(fps, screen, font, 100, 50, makecol(123,234,213), "FPS = %d");
*/
   void draw_fps(FPS * fps, ALLEGRO_FONT * font, int x, int y,
                 ALLEGRO_COLOR color, char *format);


#ifdef __cplusplus
}
#endif
#endif
