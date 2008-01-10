/* Module for taking screenshots for Allegro games.
   Written by Miran Amon.
   version: 1.01
   date: 27. June 2003
*/

#ifndef		SCREENSHOT_H
#define		SCREENSHOT_H

#include <allegro.h>

#ifdef __cplusplus
extern "C" {
#endif


/* The structure that actually takes screenshots and counts them. The user
   should create an instance of this structure with create_screenshot()
   and destroy it with destroy_screenshot(). The user should not use the
   contents of the structure directly, the functions for manipulating with
   the screenshot structure should be used instead.
*/
   typedef struct SCREENSHOT {
      int counter;
      char *name;
      char *ext;
   } SCREENSHOT;


/* Creates an instance of the screenshot structure and initializes it. The user
   should call this function somewhere at the beginning of the program and
   use the value it returns to take screenshots. This function makes sure
   previously made screenshots will not be overwritten.

   Parameters:
      char *name - the base of the name for the screenshots; actual names
                   will have their number appended to them; typically the name
                   should have no more than 4 characters (for compatibility)
      char *ext - extension of the screenshot bitmaps; typically this should
                  be either bmp, pcx, tga or lbm, anything else will fail to
                  produce screenshots unless you have and addon library that
		  integrates itself with save_bitmap()

   Returns:
      SCREENSHOT *ss - an instance of the SCREENSHOT structure
*/
   SCREENSHOT *create_screenshot(char *name, char *ext);


/* Destroys an SCREENSHOT object. The user should call this function
   when he's done taking screenshots, i.e. at the end of the program.

   Parameters:
      SCREENSHOT *ss - a pointer to an SCREENSHOT object

   Returns:
      nothing
*/
   void destroy_screenshot(SCREENSHOT * ss);


/* Takes a screenshot and saves it to a file.

   Parameters:
      SCREENSHOT *ss - a pointer to an SCREENSHOT object
      BITMAP *buffer - the bitmap from which we take a screenshot; this can be
                       the screen bitmap but taking screenshots from video
		       bitmaps like the screen is a bit slower so if you use a
		       memory double buferring system you should pass your
		       buffer to this function instead

   Returns:
      nothing
*/
   void take_screenshot(SCREENSHOT * ss, BITMAP * buffer);


#ifdef __cplusplus
}
#endif
#endif
