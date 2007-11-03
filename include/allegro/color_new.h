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
 * An ALLEGRO_COLOR structure describes a color in a particular
 * pixel format. A color that appears the same in one format may
 * be internally very different in another format. Notably,
 * color->raw[0] does not always represent the same color component
 * (red, green, blue, alpha) and will not always be in the same scale
 * or even data type. Users will not normally have to access the
 * internals of this structure directly. The al_map_* and al_unmap_*
 * functions should do the work for them.
 *
 * > typedef struct ALLEGRO_COLOR {
 * >        uint64_t raw[4];
 * > }
 */
struct ALLEGRO_COLOR
{
   uint64_t raw[4];
};

#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
