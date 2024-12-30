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
 *      OpenGL implementation of some of the primitive routines.
 *
 *
 *      By Pavel Sountsov.
 *
 *      See readme.txt for copyright information.
 */

#define ALLEGRO_INTERNAL_UNSTABLE

#include "allegro5/allegro.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_prim_opengl.h"
#include "allegro5/internal/aintern_prim_soft.h"
#include "allegro5/platform/alplatf.h"
#include "allegro5/internal/aintern_prim.h"

#ifdef ALLEGRO_CFG_OPENGL

#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_opengl.h"

static void convert_storage(ALLEGRO_PRIM_STORAGE storage, GLenum* type, int* ncoord, bool* normalized)
{
   switch(storage) {
      case ALLEGRO_PRIM_FLOAT_2:
         *type = GL_FLOAT;
         *ncoord = 2;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_FLOAT_3:
         *type = GL_FLOAT;
         *ncoord = 3;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_SHORT_2:
         *type = GL_SHORT;
         *ncoord = 2;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_FLOAT_1:
         *type = GL_FLOAT;
         *ncoord = 1;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_FLOAT_4:
         *type = GL_FLOAT;
         *ncoord = 4;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_UBYTE_4:
         *type = GL_UNSIGNED_BYTE;
         *ncoord = 4;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_SHORT_4:
         *type = GL_SHORT;
         *ncoord = 4;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_NORMALIZED_UBYTE_4:
         *type = GL_UNSIGNED_BYTE;
         *ncoord = 4;
         *normalized = true;
         break;
      case ALLEGRO_PRIM_NORMALIZED_SHORT_2:
         *type = GL_SHORT;
         *ncoord = 2;
         *normalized = true;
         break;
      case ALLEGRO_PRIM_NORMALIZED_SHORT_4:
         *type = GL_SHORT;
         *ncoord = 4;
         *normalized = true;
         break;
      case ALLEGRO_PRIM_NORMALIZED_USHORT_2:
         *type = GL_UNSIGNED_SHORT;
         *ncoord = 2;
         *normalized = true;
         break;
      case ALLEGRO_PRIM_NORMALIZED_USHORT_4:
         *type = GL_UNSIGNED_SHORT;
         *ncoord = 4;
         *normalized = true;
         break;
#ifndef ALLEGRO_CFG_OPENGLES
      case ALLEGRO_PRIM_HALF_FLOAT_2:
         *type = GL_HALF_FLOAT;
         *ncoord = 2;
         *normalized = false;
         break;
      case ALLEGRO_PRIM_HALF_FLOAT_4:
         *type = GL_HALF_FLOAT;
         *ncoord = 4;
         *normalized = false;
         break;
#endif
      default:
         ASSERT(0);
   }
}

static void setup_state(const char* vtxs, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();
   GLenum type;
   int ncoord;
   bool normalized;

   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if(decl) {
         ALLEGRO_VERTEX_ELEMENT* e;
         int i;

         e = &decl->elements[ALLEGRO_PRIM_POSITION];
         if(e->attribute) {
            convert_storage(e->storage, &type, &ncoord, &normalized);

            if (display->ogl_extras->varlocs.pos_loc >= 0) {
               glVertexAttribPointer(display->ogl_extras->varlocs.pos_loc, ncoord, type, normalized, decl->stride, vtxs + e->offset);
               glEnableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
            }
         } else {
            if (display->ogl_extras->varlocs.pos_loc >= 0) {
               glDisableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
            }
         }

         e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
         if(!e->attribute)
            e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
         if(e->attribute) {
            convert_storage(e->storage, &type, &ncoord, &normalized);

            if (display->ogl_extras->varlocs.texcoord_loc >= 0) {
               glVertexAttribPointer(display->ogl_extras->varlocs.texcoord_loc, ncoord, type, normalized, decl->stride, vtxs + e->offset);
               glEnableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
            }
         } else {
            if (display->ogl_extras->varlocs.texcoord_loc >= 0) {
               glDisableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
            }
         }

         e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
         if(e->attribute) {
            if (display->ogl_extras->varlocs.color_loc >= 0) {
               glVertexAttribPointer(display->ogl_extras->varlocs.color_loc, 4, GL_FLOAT, true, decl->stride, vtxs + e->offset);
               glEnableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
            }
         } else {
            if (display->ogl_extras->varlocs.color_loc >= 0) {
               glDisableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
            }
         }

         for (i = 0; i < _ALLEGRO_PRIM_MAX_USER_ATTR; i++) {
            e = &decl->elements[ALLEGRO_PRIM_USER_ATTR + i];
            if (e->attribute) {
               convert_storage(e->storage, &type, &ncoord, &normalized);

               if (display->ogl_extras->varlocs.user_attr_loc[i] >= 0) {
                  glVertexAttribPointer(display->ogl_extras->varlocs.user_attr_loc[i], ncoord, type, normalized, decl->stride, vtxs + e->offset);
                  glEnableVertexAttribArray(display->ogl_extras->varlocs.user_attr_loc[i]);
               }
            } else {
               if (display->ogl_extras->varlocs.user_attr_loc[i] >= 0) {
                  glDisableVertexAttribArray(display->ogl_extras->varlocs.user_attr_loc[i]);
               }
            }
         }
      } else {
         if (display->ogl_extras->varlocs.pos_loc >= 0) {
            glVertexAttribPointer(display->ogl_extras->varlocs.pos_loc, 3, GL_FLOAT, false, sizeof(ALLEGRO_VERTEX), vtxs + offsetof(ALLEGRO_VERTEX, x));
            glEnableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
         }

         if (display->ogl_extras->varlocs.texcoord_loc >= 0) {
            glVertexAttribPointer(display->ogl_extras->varlocs.texcoord_loc, 2, GL_FLOAT, false, sizeof(ALLEGRO_VERTEX), vtxs + offsetof(ALLEGRO_VERTEX, u));
            glEnableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
         }

         if (display->ogl_extras->varlocs.color_loc >= 0) {
            glVertexAttribPointer(display->ogl_extras->varlocs.color_loc, 4, GL_FLOAT, true, sizeof(ALLEGRO_VERTEX), vtxs + offsetof(ALLEGRO_VERTEX, color));
            glEnableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
         }
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      if(decl) {
         ALLEGRO_VERTEX_ELEMENT* e;
         e = &decl->elements[ALLEGRO_PRIM_POSITION];
         if(e->attribute) {
            glEnableClientState(GL_VERTEX_ARRAY);

            convert_storage(e->storage, &type, &ncoord, &normalized);

            glVertexPointer(ncoord, type, decl->stride, vtxs + e->offset);
         } else {
            glDisableClientState(GL_VERTEX_ARRAY);
         }

         e = &decl->elements[ALLEGRO_PRIM_TEX_COORD];
         if(!e->attribute)
            e = &decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL];
         if(texture && e->attribute) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);

            convert_storage(e->storage, &type, &ncoord, &normalized);

            glTexCoordPointer(ncoord, type, decl->stride, vtxs + e->offset);
         } else {
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
         }

         e = &decl->elements[ALLEGRO_PRIM_COLOR_ATTR];
         if(e->attribute) {
            glEnableClientState(GL_COLOR_ARRAY);

            glColorPointer(4, GL_FLOAT, decl->stride, vtxs + e->offset);
         } else {
            glDisableClientState(GL_COLOR_ARRAY);
            glColor4f(1, 1, 1, 1);
         }
      } else {
         glEnableClientState(GL_COLOR_ARRAY);
         glEnableClientState(GL_VERTEX_ARRAY);
         glEnableClientState(GL_TEXTURE_COORD_ARRAY);
         if (!(display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE))
            glDisableClientState(GL_NORMAL_ARRAY);

         glVertexPointer(3, GL_FLOAT, sizeof(ALLEGRO_VERTEX), vtxs + offsetof(ALLEGRO_VERTEX, x));
         glColorPointer(4, GL_FLOAT, sizeof(ALLEGRO_VERTEX), vtxs + offsetof(ALLEGRO_VERTEX, color));
         glTexCoordPointer(2, GL_FLOAT, sizeof(ALLEGRO_VERTEX), vtxs + offsetof(ALLEGRO_VERTEX, u));
      }
#endif
   }

   if (texture) {
      GLuint gl_texture = al_get_opengl_texture(texture);
      int true_w, true_h;
      int tex_x, tex_y;
      float mat[4][4] = {
         {1,  0,  0, 0},
         {0, -1,  0, 0},
         {0,  0,  1, 0},
         {0,  0,  0, 1}
      };
      int height;

      if (texture->parent)
         height = texture->parent->h;
      else
         height = texture->h;

      al_get_opengl_texture_size(texture, &true_w, &true_h);
      al_get_opengl_texture_position(texture, &tex_x, &tex_y);

      mat[3][0] = (float)tex_x / true_w;
      mat[3][1] = (float)(height - tex_y) / true_h;

      if(decl) {
         if(decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL].attribute) {
            mat[0][0] = 1.0f / true_w;
            mat[1][1] = -1.0f / true_h;
         } else {
            mat[0][0] = (float)al_get_bitmap_width(texture) / true_w;
            mat[1][1] = -(float)al_get_bitmap_height(texture) / true_h;
         }
      } else {
         mat[0][0] = 1.0f / true_w;
         mat[1][1] = -1.0f / true_h;
      }

      if (!(display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
         glBindTexture(GL_TEXTURE_2D, gl_texture);
      }

      ALLEGRO_BITMAP_WRAP wrap_u, wrap_v;
      _al_get_bitmap_wrap(texture, &wrap_u, &wrap_v);

      if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
         GLint handle;

         handle = display->ogl_extras->varlocs.tex_matrix_loc;
         if (handle >= 0)
            glUniformMatrix4fv(handle, 1, false, (float *)mat);

         handle = display->ogl_extras->varlocs.use_tex_matrix_loc;
         if (handle >= 0)
            glUniform1i(handle, 1);

         if (display->ogl_extras->varlocs.use_tex_loc >= 0) {
            glUniform1i(display->ogl_extras->varlocs.use_tex_loc, 1);
         }
         if (display->ogl_extras->varlocs.tex_loc >= 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, al_get_opengl_texture(texture));
            glUniform1i(display->ogl_extras->varlocs.tex_loc, 0); // 0th sampler

            if (wrap_u == ALLEGRO_BITMAP_WRAP_DEFAULT)
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            if (wrap_v == ALLEGRO_BITMAP_WRAP_DEFAULT)
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
         }
#endif
      }
      else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
         glMatrixMode(GL_TEXTURE);
         glLoadMatrixf(mat[0]);
         glMatrixMode(GL_MODELVIEW);
         glEnable(GL_TEXTURE_2D);
         if (wrap_u == ALLEGRO_BITMAP_WRAP_DEFAULT)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
         if (wrap_v == ALLEGRO_BITMAP_WRAP_DEFAULT)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
#endif
      }
   } else {
      /* Don't unbind the texture here if shaders are used, since the user may
       * have set the 0'th texture unit manually via the shader API. */
      if (!(display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE)) {
         glBindTexture(GL_TEXTURE_2D, 0);
      }
   }
}

static void revert_state(ALLEGRO_BITMAP* texture)
{
   ALLEGRO_DISPLAY *display = al_get_current_display();

   if(texture) {
      ALLEGRO_BITMAP_WRAP wrap_u, wrap_v;
      _al_get_bitmap_wrap(texture, &wrap_u, &wrap_v);

      if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
         float identity[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
         };
         GLint handle;
         handle = display->ogl_extras->varlocs.tex_matrix_loc;
         if (handle >= 0)
            glUniformMatrix4fv(handle, 1, false, identity);
         handle = display->ogl_extras->varlocs.use_tex_matrix_loc;
         if (handle >= 0)
            glUniform1i(handle, 0);
         if (display->ogl_extras->varlocs.use_tex_loc >= 0)
            glUniform1i(display->ogl_extras->varlocs.use_tex_loc, 0);

         if (display->ogl_extras->varlocs.tex_loc >= 0) {
            if (wrap_u == ALLEGRO_BITMAP_WRAP_DEFAULT)
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            if (wrap_v == ALLEGRO_BITMAP_WRAP_DEFAULT)
               glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
         }
#endif
      }
      else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
         if (wrap_u == ALLEGRO_BITMAP_WRAP_DEFAULT)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
         if (wrap_v == ALLEGRO_BITMAP_WRAP_DEFAULT)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

         glDisable(GL_TEXTURE_2D);
         glMatrixMode(GL_TEXTURE);
         glLoadIdentity();
         glMatrixMode(GL_MODELVIEW);
#endif
      }
   }

   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.pos_loc >= 0)
         glDisableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
      if (display->ogl_extras->varlocs.color_loc >= 0)
         glDisableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
      if (display->ogl_extras->varlocs.texcoord_loc >= 0)
         glDisableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glDisableClientState(GL_COLOR_ARRAY);
      glDisableClientState(GL_VERTEX_ARRAY);
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
   }
}

static int draw_prim_raw(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture,
   ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   const void* vtx, const ALLEGRO_VERTEX_DECL* decl,
   int start, int end, int type)
{
   int num_primitives = 0;
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_BITMAP *opengl_target = target;
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra;
   int num_vtx = end - start;

   if (target->parent) {
       opengl_target = target->parent;
   }
   extra = opengl_target->extra;

   if ((!extra->is_backbuffer && disp->ogl_extras->opengl_target !=
      opengl_target) || al_is_bitmap_locked(target)) {
      if (vertex_buffer) {
         return _al_draw_buffer_common_soft(vertex_buffer, texture, NULL, start, end, type);
      }
      else {
         return _al_draw_prim_soft(texture, vtx, decl, start, end, type);
      }
   }

   if (vertex_buffer) {
      glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vertex_buffer->common.handle);
   }

   _al_opengl_set_blender(disp);
   setup_state(vtx, decl, texture);

   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         glDrawArrays(GL_LINES, start, num_vtx);
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         glDrawArrays(GL_LINE_STRIP, start, num_vtx);
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         glDrawArrays(GL_LINE_LOOP, start, num_vtx);
         num_primitives = num_vtx;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         glDrawArrays(GL_TRIANGLES, start, num_vtx);
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         glDrawArrays(GL_TRIANGLE_STRIP, start, num_vtx);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         glDrawArrays(GL_TRIANGLE_FAN, start, num_vtx);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_POINT_LIST: {
         glDrawArrays(GL_POINTS, start, num_vtx);
         num_primitives = num_vtx;
         break;
      };
   }

   revert_state(texture);

   if (vertex_buffer) {
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   return num_primitives;
}

static int draw_prim_indexed_raw(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture,
   ALLEGRO_VERTEX_BUFFER* vertex_buffer,
   const void* vtx, const ALLEGRO_VERTEX_DECL* decl,
   ALLEGRO_INDEX_BUFFER* index_buffer,
   const int* indices,
   int start, int end, int type)
{
   int num_primitives = 0;
   ALLEGRO_DISPLAY *disp = _al_get_bitmap_display(target);
   ALLEGRO_BITMAP *opengl_target = target;
   ALLEGRO_BITMAP_EXTRA_OPENGL *extra;
   const char* idx = (const char*)indices;
   int start_offset = 0;
   GLenum idx_size = GL_UNSIGNED_INT;
   bool use_buffers = index_buffer != NULL;
   int num_vtx = end - start;
#if defined ALLEGRO_IPHONE
   GLushort* iphone_idx = NULL;
#endif

   if (use_buffers) {
      idx_size = index_buffer->index_size == 4 ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
      start_offset = start * index_buffer->index_size;
   }

   if (target->parent) {
      opengl_target = target->parent;
   }
   extra = opengl_target->extra;

   if ((!extra->is_backbuffer && disp->ogl_extras->opengl_target !=
      opengl_target) || al_is_bitmap_locked(target)) {
      if (use_buffers) {
         return _al_draw_buffer_common_soft(vertex_buffer, texture, index_buffer, start, end, type);
      }
      else {
         return _al_draw_prim_indexed_soft(texture, vtx, decl, indices, num_vtx, type);
      }
   }

#if defined ALLEGRO_IPHONE
   if (!use_buffers) {
      int ii;
      iphone_idx = al_malloc(num_vtx * sizeof(GLushort));
      for (ii = start; ii < end; ii++) {
         iphone_idx[ii] = (GLushort)indices[ii];
      }
      idx = iphone_idx;
      start = 0;
      idx_size = GL_UNSIGNED_SHORT;
   }
#endif

   _al_opengl_set_blender(disp);

   if (use_buffers) {
      glBindBuffer(GL_ARRAY_BUFFER, (GLuint)vertex_buffer->common.handle);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)index_buffer->common.handle);
   }

   setup_state(vtx, decl, texture);

   switch (type) {
      case ALLEGRO_PRIM_LINE_LIST: {
         glDrawElements(GL_LINES, num_vtx, idx_size, idx + start_offset);
         num_primitives = num_vtx / 2;
         break;
      };
      case ALLEGRO_PRIM_LINE_STRIP: {
         glDrawElements(GL_LINE_STRIP, num_vtx, idx_size, idx + start_offset);
         num_primitives = num_vtx - 1;
         break;
      };
      case ALLEGRO_PRIM_LINE_LOOP: {
         /* Unimplemented, as it's too hard to do for Direct3D */
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_LIST: {
         glDrawElements(GL_TRIANGLES, num_vtx, idx_size, idx + start_offset);
         num_primitives = num_vtx / 3;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_STRIP: {
         glDrawElements(GL_TRIANGLE_STRIP, num_vtx, idx_size, idx + start_offset);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_TRIANGLE_FAN: {
         glDrawElements(GL_TRIANGLE_FAN, num_vtx, idx_size, idx + start_offset);
         num_primitives = num_vtx - 2;
         break;
      };
      case ALLEGRO_PRIM_POINT_LIST: {
         /* Unimplemented, as it's too hard to do for Direct3D */
         break;
      };
   }

   revert_state(texture);

   if (use_buffers) {
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   }

#if defined ALLEGRO_IPHONE
   al_free(iphone_idx);
#endif

   return num_primitives;
}

#endif /* ALLEGRO_CFG_OPENGL */

int _al_draw_prim_opengl(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_OPENGL
   return draw_prim_raw(target, texture, 0, vtxs, decl, start, end, type);
#else
   (void)target;
   (void)texture;
   (void)vtxs;
   (void)start;
   (void)end;
   (void)type;
   (void)decl;

   return 0;
#endif
}

int _al_draw_vertex_buffer_opengl(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_OPENGL
   return draw_prim_raw(target, texture, vertex_buffer, 0, vertex_buffer->decl, start, end, type);
#else
   (void)target;
   (void)texture;
   (void)vertex_buffer;
   (void)start;
   (void)end;
   (void)type;

   return 0;
#endif
}

int _al_draw_prim_indexed_opengl(ALLEGRO_BITMAP *target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type)
{
#ifdef ALLEGRO_CFG_OPENGL
   return draw_prim_indexed_raw(target, texture, NULL, vtxs, decl, NULL, indices, 0, num_vtx, type);
#else
   (void)target;
   (void)texture;
   (void)vtxs;
   (void)decl;
   (void)indices;
   (void)num_vtx;
   (void)type;

   return 0;
#endif
}

int _al_draw_indexed_buffer_opengl(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
#ifdef ALLEGRO_CFG_OPENGL
   return draw_prim_indexed_raw(target, texture, vertex_buffer, NULL, vertex_buffer->decl, index_buffer, NULL, start, end, type);
#else
   (void)target;
   (void)texture;
   (void)vertex_buffer;
   (void)index_buffer;
   (void)start;
   (void)end;
   (void)type;

   return 0;
#endif
}

#ifdef ALLEGRO_CFG_OPENGL
static bool create_buffer_common(ALLEGRO_BUFFER_COMMON* common, GLenum type, const void* initial_data, GLsizeiptr size, int flags)
{
   GLuint vbo;
   GLenum usage;

   switch (flags)
   {
#if !defined ALLEGRO_CFG_OPENGLES
      case ALLEGRO_PRIM_BUFFER_STREAM:
         usage = GL_STREAM_DRAW;
         break;
#endif
      case ALLEGRO_PRIM_BUFFER_STATIC:
         usage = GL_STATIC_DRAW;
         break;
      case ALLEGRO_PRIM_BUFFER_DYNAMIC:
         usage = GL_DYNAMIC_DRAW;
         break;
      default:
         usage = GL_STATIC_DRAW;
   }

   glGenBuffers(1, &vbo);
   glBindBuffer(type, vbo);
   glBufferData(type, size, initial_data, usage);
   glBindBuffer(type, 0);

   if (glGetError())
      return false;

   common->handle = vbo;
   common->local_buffer_length = 0;
   return true;
}
#endif

bool _al_create_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int flags)
{
#ifdef ALLEGRO_CFG_OPENGL
   int stride = buf->decl ? buf->decl->stride : (int)sizeof(ALLEGRO_VERTEX);

   return create_buffer_common(&buf->common, GL_ARRAY_BUFFER, initial_data, num_vertices * stride, flags);
#else
   (void)buf;
   (void)initial_data;
   (void)num_vertices;
   (void)flags;

   return false;
#endif
}

bool _al_create_index_buffer_opengl(ALLEGRO_INDEX_BUFFER* buf, const void* initial_data, size_t num_indices, int flags)
{
#ifdef ALLEGRO_CFG_OPENGL
   return create_buffer_common(&buf->common, GL_ELEMENT_ARRAY_BUFFER, initial_data, num_indices * buf->index_size, flags);;
#else
   (void)buf;
   (void)initial_data;
   (void)num_indices;
   (void)flags;

   return false;
#endif
}

void _al_destroy_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_OPENGL
   glDeleteBuffers(1, (GLuint*)&buf->common.handle);
   al_free(buf->common.locked_memory);
#else
   (void)buf;
#endif
}

void _al_destroy_index_buffer_opengl(ALLEGRO_INDEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_OPENGL
   glDeleteBuffers(1, (GLuint*)&buf->common.handle);
   al_free(buf->common.locked_memory);
#else
   (void)buf;
#endif
}

#ifdef ALLEGRO_CFG_OPENGL
static void* lock_buffer_common(ALLEGRO_BUFFER_COMMON* common, GLenum type)
{
   if (common->local_buffer_length < common->lock_length) {
      common->locked_memory = al_realloc(common->locked_memory, common->lock_length);
      common->local_buffer_length = common->lock_length;
   }

   if (common->lock_flags != ALLEGRO_LOCK_WRITEONLY) {
#if !defined ALLEGRO_CFG_OPENGLES
      glBindBuffer(type, (GLuint)common->handle);
      glGetBufferSubData(type, common->lock_offset, common->lock_length, common->locked_memory);
      glBindBuffer(type, 0);
      if (glGetError())
         return 0;
#else
      (void)type;
      return 0;
#endif
   }
   return common->locked_memory;
}
#endif

void* _al_lock_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_OPENGL
   return lock_buffer_common(&buf->common, GL_ARRAY_BUFFER);
#else
   (void)buf;

   return 0;
#endif
}

void* _al_lock_index_buffer_opengl(ALLEGRO_INDEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_OPENGL
   return lock_buffer_common(&buf->common, GL_ELEMENT_ARRAY_BUFFER);
#else
   (void)buf;

   return 0;
#endif
}

#ifdef ALLEGRO_CFG_OPENGL
static void unlock_buffer_common(ALLEGRO_BUFFER_COMMON* common, GLenum type)
{
   if (common->lock_flags != ALLEGRO_LOCK_READONLY) {
      glBindBuffer(type, (GLuint)common->handle);
      glBufferSubData(type, common->lock_offset, common->lock_length, common->locked_memory);
      glBindBuffer(type, 0);
   }
}
#endif

void _al_unlock_vertex_buffer_opengl(ALLEGRO_VERTEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_OPENGL
   unlock_buffer_common(&buf->common, GL_ARRAY_BUFFER);
#else
   (void)buf;
#endif
}

void _al_unlock_index_buffer_opengl(ALLEGRO_INDEX_BUFFER* buf)
{
#ifdef ALLEGRO_CFG_OPENGL
   unlock_buffer_common(&buf->common, GL_ELEMENT_ARRAY_BUFFER);
#else
   (void)buf;
#endif
}
