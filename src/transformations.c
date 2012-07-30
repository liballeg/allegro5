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
 *      Transformations.
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_system.h"
#include <math.h>

/* ALLEGRO_DEBUG_CHANNEL("transformations") */

/* Function: al_copy_transform
 */
void al_copy_transform(ALLEGRO_TRANSFORM *dest, const ALLEGRO_TRANSFORM *src)
{
   ASSERT(src);
   ASSERT(dest);
   
   memcpy(dest, src, sizeof(ALLEGRO_TRANSFORM));
}

/* Function: al_use_transform
 */
void al_use_transform(const ALLEGRO_TRANSFORM *trans)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_DISPLAY *display;

   if (!target)
      return;

   /* Changes to a back buffer should affect the front buffer, and vice versa.
    * Currently we rely on the fact that in the OpenGL drivers the back buffer
    * and front buffer bitmaps are exactly the same, and the DirectX driver
    * doesn't support front buffer bitmaps.
    */

   if (trans != &target->transform) {
   
      al_copy_transform(&target->transform, trans);
      
      target->inverse_transform_dirty = true;
   }

   /*
    * When the drawing is held, we apply the transformations in software,
    * so the hardware transformation has to be kept at identity.
    */
   if (!al_is_bitmap_drawing_held()) {
      display = target->display;
      if (display) {
         display->vt->update_transformation(display, target);
      }
   }
}

/* Function: al_get_current_transform
 */
const ALLEGRO_TRANSFORM *al_get_current_transform(void)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   if (!target)
      return NULL;

   return &target->transform;
}

/* Function: al_get_current_inverse_transform
 */
const ALLEGRO_TRANSFORM *al_get_current_inverse_transform(void)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();

   if (!target)
      return NULL;
      
   if (target->inverse_transform_dirty) {
      al_copy_transform(&target->inverse_transform, &target->transform);
      al_invert_transform(&target->inverse_transform);
   }

   return &target->inverse_transform;
}

/* Function: al_identity_transform
 */
void al_identity_transform(ALLEGRO_TRANSFORM *trans)
{
   ASSERT(trans);
   
   trans->m[0][0] = 1;
   trans->m[0][1] = 0;
   trans->m[0][2] = 0;
   trans->m[0][3] = 0;
   
   trans->m[1][0] = 0;
   trans->m[1][1] = 1;
   trans->m[1][2] = 0;
   trans->m[1][3] = 0;
   
   trans->m[2][0] = 0;
   trans->m[2][1] = 0;
   trans->m[2][2] = 1;
   trans->m[2][3] = 0;
   
   trans->m[3][0] = 0;
   trans->m[3][1] = 0;
   trans->m[3][2] = 0;
   trans->m[3][3] = 1;
}

/* Function: al_build_transform
 */
void al_build_transform(ALLEGRO_TRANSFORM *trans, float x, float y,
   float sx, float sy, float theta)
{
   float c, s;
   ASSERT(trans);
   
   c = cosf(theta);
   s = sinf(theta);
   
   trans->m[0][0] = sx * c;
   trans->m[0][1] = sy * s;
   trans->m[0][2] = 0;
   trans->m[0][3] = 0;
   
   trans->m[1][0] = -sx * s;
   trans->m[1][1] = sy * c;
   trans->m[1][2] = 0;
   trans->m[1][3] = 0;
   
   trans->m[2][0] = 0;
   trans->m[2][1] = 0;
   trans->m[2][2] = 1;
   trans->m[2][3] = 0;
   
   trans->m[3][0] = x;
   trans->m[3][1] = y;
   trans->m[3][2] = 0;
   trans->m[3][3] = 1;
}

/* Function: al_invert_transform
 */
void al_invert_transform(ALLEGRO_TRANSFORM *trans)
{
   float det, t;
   ASSERT(trans);
   
   det =  trans->m[0][0] *  trans->m[1][1] -  trans->m[1][0] *  trans->m[0][1];

   t =  trans->m[3][0];
   trans->m[3][0] = ( trans->m[1][0] *  trans->m[3][1] - t *  trans->m[1][1]) / det;
   trans->m[3][1] = (t *  trans->m[0][1] -  trans->m[0][0] *  trans->m[3][1]) / det;

   t =  trans->m[0][0];
   trans->m[0][0] =  trans->m[1][1] / det;
   trans->m[1][1] = t / det;
   
   trans->m[0][1] = - trans->m[0][1] / det;
   trans->m[1][0] = - trans->m[1][0] / det;
}

/* Function: al_check_inverse
 */
int al_check_inverse(const ALLEGRO_TRANSFORM *trans, float tol)
{
   float det, norm, c0, c1, c3;
   ASSERT(trans);
   
   det = fabs( trans->m[0][0] *  trans->m[1][1] -  trans->m[1][0] *  trans->m[0][1]);
   /*
   We'll use the 1-norm, as it is the easiest to compute
   */
   c0 = fabs(trans->m[0][0]) + fabs(trans->m[0][1]);
   c1 = fabs(trans->m[1][0]) + fabs(trans->m[1][1]);
   c3 = fabs(trans->m[3][0]) + fabs(trans->m[3][1]) + 1;
   norm = _ALLEGRO_MAX(_ALLEGRO_MAX(1, c0), _ALLEGRO_MAX(c1, c3));

   return det > tol * norm;
}

/* Function: al_translate_transform
 */
void al_translate_transform(ALLEGRO_TRANSFORM *trans, float x, float y)
{
   ASSERT(trans);
   
   trans->m[3][0] += x;
   trans->m[3][1] += y;
}


/* Function: al_translate_transform_3d
 */
void al_translate_transform_3d(ALLEGRO_TRANSFORM *trans, float x, float y,
    float z)
{
   ASSERT(trans);
   
   trans->m[3][0] += x;
   trans->m[3][1] += y;
   trans->m[3][2] += z;
}


/* Function: al_rotate_transform
 */
void al_rotate_transform(ALLEGRO_TRANSFORM *trans, float theta)
{
   float c, s;
   float t;
   ASSERT(trans);
   
   c = cosf(theta);
   s = sinf(theta);
   
   t = trans->m[0][0];
   trans->m[0][0] = t * c - trans->m[0][1] * s;
   trans->m[0][1] = t * s + trans->m[0][1] * c;
   
   t = trans->m[1][0];
   trans->m[1][0] = t * c - trans->m[1][1] * s;
   trans->m[1][1] = t * s + trans->m[1][1] * c;
   
   t = trans->m[3][0];
   trans->m[3][0] = t * c - trans->m[3][1] * s;
   trans->m[3][1] = t * s + trans->m[3][1] * c;
}

/* Function: al_scale_transform
 */
void al_scale_transform(ALLEGRO_TRANSFORM *trans, float sx, float sy)
{
   ASSERT(trans);
   
   trans->m[0][0] *= sx;
   trans->m[0][1] *= sy;
   
   trans->m[1][0] *= sx;
   trans->m[1][1] *= sy;
   
   trans->m[3][0] *= sx;
   trans->m[3][1] *= sy;
}


/* Function: al_scale_transform_3d
 */
void al_scale_transform_3d(ALLEGRO_TRANSFORM *trans, float sx, float sy,
    float sz)
{
   ASSERT(trans);
   
   trans->m[0][0] *= sx;
   trans->m[0][1] *= sy;
   trans->m[0][2] *= sz;
   
   trans->m[1][0] *= sx;
   trans->m[1][1] *= sy;
   trans->m[1][2] *= sz;

   trans->m[2][0] *= sx;
   trans->m[2][1] *= sy;
   trans->m[2][2] *= sz;
   
   trans->m[3][0] *= sx;
   trans->m[3][1] *= sy;
   trans->m[3][2] *= sz;
}

/* Function: al_transform_coordinates
 */
void al_transform_coordinates(const ALLEGRO_TRANSFORM *trans, float *x, float *y)
{
   float t;
   ASSERT(trans);
   ASSERT(x);
   ASSERT(y);

   t = *x;
   
   *x = t * trans->m[0][0] + *y * trans->m[1][0] + trans->m[3][0];
   *y = t * trans->m[0][1] + *y * trans->m[1][1] + trans->m[3][1];
}

/* Function: al_compose_transform
 */
void al_compose_transform(ALLEGRO_TRANSFORM *trans, const ALLEGRO_TRANSFORM *other)
{
   #define E(x, y)                        \
      (other->m[0][y] * trans->m[x][0] +  \
       other->m[1][y] * trans->m[x][1] +  \
       other->m[2][y] * trans->m[x][2] +  \
       other->m[3][y] * trans->m[x][3])   \

   const ALLEGRO_TRANSFORM tmp = {{
      { E(0, 0), E(0, 1), E(0, 2), E(0, 3) },
      { E(1, 0), E(1, 1), E(1, 2), E(1, 3) },
      { E(2, 0), E(2, 1), E(2, 2), E(2, 3) },
      { E(3, 0), E(3, 1), E(3, 2), E(3, 3) }
   }};

   *trans = tmp;

   #undef E
}

bool _al_transform_is_translation(const ALLEGRO_TRANSFORM* trans,
   float *dx, float *dy)
{
   if (trans->m[0][0] == 1 &&
          trans->m[1][0] == 0 &&
          trans->m[2][0] == 0 &&
          trans->m[0][1] == 0 &&
          trans->m[1][1] == 1 &&
          trans->m[2][1] == 0 &&
          trans->m[0][2] == 0 &&
          trans->m[1][2] == 0 &&
          trans->m[2][2] == 1 &&
          trans->m[3][2] == 0 &&
          trans->m[0][3] == 0 &&
          trans->m[1][3] == 0 &&
          trans->m[2][3] == 0 &&
          trans->m[3][3] == 1) {
      *dx = trans->m[3][0];
      *dy = trans->m[3][1];
      return true;
   }
   return false;
}

/* Function: al_orthographic_transform
 */
void al_orthographic_transform(ALLEGRO_TRANSFORM *trans,
   float left, float top, float n,
   float right, float bottom, float f)
{
   float delta_x = right - left;
   float delta_y = top - bottom;
   float delta_z = f - n;
   ALLEGRO_TRANSFORM tmp;

   al_identity_transform(&tmp);

   tmp.m[0][0] = 2.0f / delta_x;
   tmp.m[1][1] = 2.0f / delta_y;
   tmp.m[2][2] = 2.0f / delta_z;
   tmp.m[3][0] = -(right + left) / delta_x;
   tmp.m[3][1] = -(top + bottom) / delta_y;
   tmp.m[3][2] = -(f + n) / delta_z;
   tmp.m[3][3] = 1.0f;

   al_compose_transform(trans, &tmp);
}


/* Function: al_rotate_transform_3d
 */
void al_rotate_transform_3d(ALLEGRO_TRANSFORM *trans,
   float x, float y, float z, float angle)
{
   double s = sin(angle);
   double c = cos(angle);
   double cc = 1 - c;
   ALLEGRO_TRANSFORM tmp;

   al_identity_transform(&tmp);

   tmp.m[0][0] = (cc * x * x) + c;
   tmp.m[0][1] = (cc * x * y) + (z * s);
   tmp.m[0][2] = (cc * x * z) - (y * s);
   tmp.m[0][3] = 0;

   tmp.m[1][0] = (cc * x * y) - (z * s);
   tmp.m[1][1] = (cc * y * y) + c;
   tmp.m[1][2] = (cc * z * y) + (x * s);
   tmp.m[1][3] = 0;

   tmp.m[2][0] = (cc * x * z) + (y * s);
   tmp.m[2][1] = (cc * y * z) - (x * s);
   tmp.m[2][2] = (cc * z * z) + c;
   tmp.m[2][3] = 0;

   tmp.m[3][0] = 0;
   tmp.m[3][1] = 0;
   tmp.m[3][2] = 0;
   tmp.m[3][3] = 1;

   al_compose_transform(trans, &tmp);
}


/* Function: al_perspective_transform
 */
void al_perspective_transform(ALLEGRO_TRANSFORM *trans,
   float left, float top, float n,
   float right, float bottom, float f)
{
   float delta_x = right - left;
   float delta_y = top - bottom;
   float delta_z = f - n;
   ALLEGRO_TRANSFORM tmp;

   al_identity_transform(&tmp);

   tmp.m[0][0] = 2.0f * n / delta_x;
   tmp.m[1][1] = 2.0f * n / delta_y;
   tmp.m[2][0] = (right + left) / delta_x;
   tmp.m[2][1] = (top + bottom) / delta_y;
   tmp.m[2][2] = -(f + n) / delta_z;
   tmp.m[2][3] = -1.0f;
   tmp.m[3][2] = -2.0f * f * n / delta_z;
   tmp.m[3][3] = 0;

   al_compose_transform(trans, &tmp);
}


/* Function: al_get_projection_transform
 */
ALLEGRO_TRANSFORM *al_get_projection_transform(ALLEGRO_DISPLAY *display)
{
   return &display->proj_transform;
}

/* Function: al_set_projection_transform
 */
void al_set_projection_transform(ALLEGRO_DISPLAY *display, ALLEGRO_TRANSFORM *t)
{
   al_copy_transform(&display->proj_transform, t);
   display->vt->set_projection(display);
}


/* vim: set sts=3 sw=3 et: */
