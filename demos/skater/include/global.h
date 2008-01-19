#ifndef		__DEMO_GLOBAL_H__
#define		__DEMO_GLOBAL_H__

#include "defines.h"
#include "keyboard.h"
#include "updtedvr.h"
#include <allegro.h>

/* Some configuration settings. All of these variables are recorded
   in the configuration file. */
extern int fullscreen;           /* selects fullscreen or windowed mode */
extern int bit_depth;            /* screen colour depth (15/16/24/32)
                                    Note that this is ignored when in windowed
                                    mode and the desktop colour depth can be
                                    retreived. */
extern int screen_width;         /* horizontal screen resolution */
extern int screen_height;        /* vertical screen resolution */
extern int update_driver_id;     /* selected screen update driver */
extern int use_vsync;            /* enables/disables vsync-ing */
extern int logic_framerate;      /* target logic framerate */
extern int max_frame_skip;       /* max number of skipped logic frames if
                                    the CPU isn't fast enough */
extern int limit_framerate;      /* enables/disables unlimited framerate */
extern int display_framerate;    /* enables/disables FPS counter */
extern int reduce_cpu_usage;     /* enables/disables power saving by giving
                                    up the CPU when not needed */
extern int sound_volume;         /* sound volume in range [0,10] */
extern int music_volume;         /* music volume in range [0,10] */
extern int controller_id;        /* ID of the selected input controller */

/* Offset of the text shadow in game menus as number of pixels. */
extern int shadow_offset;

/* Array of available input controllers. New controllers may be added
   here in the future. */
extern VCONTROLLER *controller[2];

/* Absolute path of the config file. */
extern char config_path[DEMO_PATH_LENGTH];

/* Absolute path of the datafile. */
extern char data_path[DEMO_PATH_LENGTH];

/* The datafile that contains all game data. */
extern DATAFILE *demo_data;

/* The main menu font (monochrome). */
extern FONT *demo_font;

/* The big title font (coloured). */
extern FONT *demo_font_logo;

/* The big title font (monocrome in AllegroGL mode, same as demo_font_logo
   in normal mode). Needed because of limitation in AllegroGL text output.*/
extern FONT *demo_font_logo_m;

/* Font made of default allegro font (monochrome). */
extern FONT *plain_font;

/* The update driver object that is used by the framework. */
extern DEMO_SCREEN_UPDATE_DRIVER update_driver;


/*
   Converts an error code to an error description.

   Parameters:
      int id - error code (see defines.h)

   Returns:
      String containing the description of the error code.
*/
extern const char *demo_error(int id);


/*
   Switches the gfx mode to settings defined by the global variables
   declared in this file and reloads all data if necessary.

   Parameters:
      none

   Returns:
      Error code: DEMO_OK on succes, otherwise the code of the error
      that caused the function to fail. See defines.h for a list of
      possible error codes.
*/
extern int change_gfx_mode(void);


/*
   Reads global configuration settings from a config file.

   Parameters:
      char *config - Path to the config file.

   Returns:
      nothing
*/
extern void read_config(const char *config);


/*
   Writes global configuration settings to a config file.

   Parameters:
      char *config - Path to the config file.

   Returns:
      nothing
*/
extern void write_config(const char *config);


/*
   Unloads all game data. Required before changing gfx mode and before
   shutting down the framework.

   Parameters:
      none

   Returns:
      nothing
*/
extern void unload_data(void);


/*
   Facade for text output functions. Implements a common interface for text
   output using Allegro's or AllegroGL's text output functions.
   Text alignment is selected with a parameter.

   Parameters:
      BITMAP *canvas, int x, int y, int col and char *format have
         exactly the same meaning as in the equivalent Allegro built-in
         text output functions.
      FONT *font can be either plain Allegro font or a font converted with
      AllegroGL's allegro_gl_convert_allegro_font_ex().
      int align - defines alignemnt: 0 = left, 1 = right, 2 = centre

   Returns:
      nothing
*/
extern void demo_textprintf_ex(BITMAP * canvas, const FONT * font, int x, int y,
      int col, int bg, int align, const char *format, ...);
extern void demo_textprintf(BITMAP * canvas, const FONT * font, int x, int y,
      int col, int bg, const char *format, ...);
extern void demo_textprintf_right(BITMAP * canvas, const FONT * font, int x, int y,
      int col, int bg, const char *format, ...);
extern void demo_textprintf_centre(BITMAP * canvas, const FONT * font, int x, int y,
      int col, int bg, const char *format, ...);
extern void demo_textout(BITMAP *bmp, const FONT *f, const char *s, int x,
      int y, int color, int bg);
extern void demo_textout_right(BITMAP *bmp, const FONT *f, const char *s,
      int x, int y, int color, int bg);
extern void demo_textout_centre(BITMAP *bmp, const FONT *f, const char *s,
      int x, int y, int color, int bg);

/*
   Custom text output function. Similar to the Allegro's built-in functions
   except that text alignment is selected with a parameter and the text is
   printed with a black shadow. Offset of the shadow is defined with the
   shadow_offset variable.

   Parameters:
      BITMAP *canvas, FONT *font, int x, int y, int col and char *text have
         exactly the same meaning as in the equivalent Allegro built-in
         text output functions.
      int align - defines alignemnt: 0 = left, 1 = right, 2 = centre

   Returns:
      nothing
*/
extern void shadow_textprintf(BITMAP * canvas, FONT * font, int x, int y,
      int col, int align, const char *text, ...);

/*
   Selects the specified update driver by filling in the update_driver
   object with pointers to appropriate functions.

   Parameters:
      int id - ID of the driver as defined in defines.h

   Returns:
      Error code: DEMO_OK if initialization was successfull, otherwise
      the code of the error that caused the function to fail. See
      defines.h for a list of possible error codes.
*/
extern int select_update_driver(int id);

#endif				/* __DEMO_GLOBAL_H__ */
