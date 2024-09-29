#ifndef		__DEMO_GLOBAL_H__
#define		__DEMO_GLOBAL_H__

#define mouse_event WIN32_mouse_event
#include <stdio.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#undef mouse_event

#include "defines.h"
#include "keyboard.h"

#ifdef A5O_MSVC
   #define snprintf  _snprintf
   #define vsnprintf _vsnprintf
#endif

/* Some configuration settings. All of these variables are recorded
   in the configuration file. */
extern int fullscreen;           /* selects fullscreen or windowed mode */
extern int bit_depth;            /* screen colour depth (15/16/24/32)
                                    Note that this is ignored when in windowed
                                    mode and the desktop colour depth can be
                                    retrieved. */
extern int screen_width;         /* horizontal screen resolution */
extern int screen_height;        /* vertical screen resolution */
extern int screen_orientation;
extern int window_width;         /* remember last window width */
extern int window_height;        /* remember last window height */
extern int screen_samples;       /* super-sampling */
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
extern char config_path[DEMO_PATH_LENGTH + 1];

/* Absolute path of the datafile. */
extern char data_path[DEMO_PATH_LENGTH + 1];

/* The main menu font (monochrome). */
#define demo_font ((A5O_FONT *)demo_data[DEMO_FONT].dat)

/* The big title font (coloured). */
#define demo_font_logo ((A5O_FONT *)demo_data[DEMO_FONT_LOGO].dat)

/* Font made of default allegro font (monochrome). */
#define plain_font demo_font

extern A5O_DISPLAY *screen;


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
extern void read_global_config(const char *config);


/*
   Writes global configuration settings to a config file.

   Parameters:
      char *config - Path to the config file.

   Returns:
      nothing
*/
extern void write_global_config(const char *config);


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
      A5O_BITMAP *canvas, int x, int y, int col and char *format have
         exactly the same meaning as in the equivalent Allegro built-in
         text output functions.
      FONT *font can be either plain Allegro font or a font converted with
      AllegroGL's allegro_gl_convert_allegro_font_ex().
      int align - defines alignemnt: 0 = left, 1 = right, 2 = centre

   Returns:
      nothing
*/
extern void demo_textprintf_ex(const A5O_FONT * font, int x, int y,
      A5O_COLOR col, int align, const char *format, ...);
extern void demo_textprintf(const A5O_FONT * font, int x, int y,
      A5O_COLOR col, const char *format, ...);
extern void demo_textprintf_right(const A5O_FONT * font, int x, int y,
      A5O_COLOR col, const char *format, ...);
extern void demo_textprintf_centre(const A5O_FONT * font, int x, int y,
      A5O_COLOR col, const char *format, ...);
extern void demo_textout(const A5O_FONT *f, const char *s, int x,
      int y, A5O_COLOR color);
extern void demo_textout_right(const A5O_FONT *f, const char *s,
      int x, int y, A5O_COLOR color);
extern void demo_textout_centre(const A5O_FONT *f, const char *s,
      int x, int y, A5O_COLOR color);

/*
   Custom text output function. Similar to the Allegro's built-in functions
   except that text alignment is selected with a parameter and the text is
   printed with a black shadow. Offset of the shadow is defined with the
   shadow_offset variable.

   Parameters:
      A5O_BITMAP *canvas, FONT *font, int x, int y, int col and char *text have
         exactly the same meaning as in the equivalent Allegro built-in
         text output functions.
      int align - defines alignemnt: 0 = left, 1 = right, 2 = centre

   Returns:
      nothing
*/
extern void shadow_textprintf(A5O_FONT * font, int x, int y,
      A5O_COLOR col, int align, const char *text, ...);

typedef struct DATA_ENTRY {
   int id;
   char const *type;
   char *path;
   char *subfolder;
   char *name;
   char *ext;
   int size;
   void *dat;
} DATA_ENTRY;

extern DATA_ENTRY *demo_data;

void unload_data_entries(DATA_ENTRY *data);

int get_config_int(const A5O_CONFIG *cfg, const char *section,
		   const char *name, int def);

void set_config_int(A5O_CONFIG *cfg, const char *section, const char *name,
		    int val);

int my_stricmp(const char *s1, const char *s2);


#endif				/* __DEMO_GLOBAL_H__ */
