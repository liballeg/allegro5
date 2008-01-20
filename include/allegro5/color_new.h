/* Title: Colors
 */

#ifndef _ALLEGRO_COLORNEW_H
#define _ALLEGRO_COLORNEW_H

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct ALLEGRO_COLOR ALLEGRO_COLOR;

typedef struct ALLEGRO_DISPLAY ALLEGRO_DISPLAY;

/* Type: ALLEGRO_COLOR
 *
 * An ALLEGRO_COLOR structure describes a color in a device
 * independant way. Use <al_map_rgb> et. al. and <al_unmap_rgb> et. al. to
 * translate from and to various color representations.
 */
struct ALLEGRO_COLOR
{
   float r, g, b, a;
};

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
