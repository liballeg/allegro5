#ifndef __al_included_allegro5_iio_h
#define __al_included_allegro5_iio_h



#include "allegro5/internal/aintern_image_cfg.h"



typedef struct PalEntry {
   int r, g, b, a;
} PalEntry;


/* FIXME: Not sure if these should be made accessible. Hide them for now. */

/* _al_png_screen_gamma is slightly overloaded (sorry):
 *
 * A value of 0.0 means: Don't do any gamma correction in load_png()
 * and load_memory_png().  This meaning was introduced in v1.4.
 *
 * A value of -1.0 means: Use the value from the environment variable
 * SCREEN_GAMMA (if available), otherwise fallback to a value of 2.2
 * (a good guess for PC monitors, and the value for sRGB colourspace).
 * This is the default.
 *
 * Otherwise, the value of _al_png_screen_gamma is taken as-is.
 */
extern double _al_png_screen_gamma;

/* Choose zlib compression level for saving file.
 * Default is Z_BEST_COMPRESSION.
 */
extern int _al_png_compression_level;



#endif

