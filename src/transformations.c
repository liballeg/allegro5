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

#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern.h"
#include ALLEGRO_INTERNAL_HEADER
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_display.h"
#include <math.h>

/* ALLEGRO_DEBUG_CHANNEL("transformations") */

/* Function: al_copy_transform
 */
void al_copy_transform(const ALLEGRO_TRANSFORM *src, ALLEGRO_TRANSFORM *dest)
{
   ASSERT(src);
   ASSERT(dest);
   
   memcpy(dest, src, sizeof(ALLEGRO_TRANSFORM));
}

/* Function: al_use_transform
 */
void al_use_transform(const ALLEGRO_TRANSFORM *trans)
{
   ALLEGRO_DISPLAY *current_display = _al_get_current_display();
   ASSERT(current_display);
   ASSERT(trans);

   al_copy_transform(trans, &current_display->cur_transform);
   if (current_display->vt) {
      current_display->vt->update_transformation(current_display);
   }
}

/* Function: al_get_current_transform
 */
const ALLEGRO_TRANSFORM *al_get_current_transform()
{
   ALLEGRO_DISPLAY *current_display = _al_get_current_display();
   ASSERT(current_display);

   return &current_display->cur_transform;
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

/* Function: al_transform_transform
 */
void al_transform_transform(const ALLEGRO_TRANSFORM *trans, ALLEGRO_TRANSFORM *trans2)
{
   float t;
   ASSERT(trans);
   ASSERT(trans2);
   
   /*
   First column
   */
   t = trans2->m[0][0];
   trans2->m[0][0] =  trans->m[0][0] * t + trans->m[1][0] * trans2->m[0][1];
   trans2->m[0][1] =  trans->m[0][1] * t + trans->m[1][1] * trans2->m[0][1];
   
   /*
   Second column
   */
   t = trans2->m[1][0];
   trans2->m[1][0] =  trans->m[0][0] * t + trans->m[1][0] * trans2->m[1][1];
   trans2->m[1][1] =  trans->m[0][1] * t + trans->m[1][1] * trans2->m[1][1];
   
   /*
   Fourth column
   */
   t = trans2->m[3][0];
   trans2->m[3][0] =  trans->m[0][0] * t + trans->m[1][0] * trans2->m[3][1] +  trans->m[3][0];
   trans2->m[3][1] =  trans->m[0][1] * t + trans->m[1][1] * trans2->m[3][1] +  trans->m[3][1];
}

/* vim: set sts=3 sw=3 et: */
