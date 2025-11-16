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
 *      OpenGL drawing routines
 *
 *      By Elias Pschernig.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_opengl.h"
#include "allegro5/internal/aintern_display.h"
#include "allegro5/internal/aintern_memdraw.h"
#include "allegro5/internal/aintern_opengl.h"
#include "allegro5/internal/aintern_primitives.h"
#include "allegro5/internal/aintern_prim_soft.h"

#ifdef ALLEGRO_ANDROID
#include "allegro5/internal/aintern_android.h"
#endif

#include "ogl_helpers.h"

ALLEGRO_DEBUG_CHANNEL("opengl")

/* FIXME: For some reason x86_64 Android crashes for me when calling
 * glBlendColor - so adding this hack to disable it.
 */
#ifdef ALLEGRO_ANDROID
#if defined(__x86_64__) || defined(__i686__)
#define ALLEGRO_ANDROID_HACK_X86_64
#endif
#endif

#define MAX_BATCH_SIZE ((1 << 16) - 1)

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

static void setup_state(ALLEGRO_DISPLAY *display, const char* vtxs, const ALLEGRO_VERTEX_DECL* decl, ALLEGRO_BITMAP* texture)
{
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
         if(!e->attribute)
            e = &decl->elements[_ALLEGRO_PRIM_TEX_COORD_INTERNAL];
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

         for (i = 0; i < ALLEGRO_PRIM_MAX_USER_ATTR; i++) {
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
         if(!e->attribute)
            e = &decl->elements[_ALLEGRO_PRIM_TEX_COORD_INTERNAL];
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
         {1, 0, 0, 0},
         {0, 1, 0, 0},
         {0, 0, 1, 0},
         {0, 0, 0, 1}
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

      if (decl) {
         if (decl->elements[ALLEGRO_PRIM_TEX_COORD_PIXEL].attribute) {
            mat[0][0] = 1.0f / true_w;
            mat[1][1] = -1.0f / true_h;
         }
         else if (decl->elements[_ALLEGRO_PRIM_TEX_COORD_INTERNAL].attribute) {
            mat[3][0] = 0.;
            mat[3][1] = 0.;
         }
         else {
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

static void revert_state(ALLEGRO_DISPLAY *display, ALLEGRO_BITMAP* texture)
{
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

static int draw_prim_common(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture,
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
   setup_state(disp, vtx, decl, texture);

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

   revert_state(disp, texture);

   if (vertex_buffer) {
      glBindBuffer(GL_ARRAY_BUFFER, 0);
   }

   return num_primitives;
}

static int draw_prim_indexed_common(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture,
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

   setup_state(disp, vtx, decl, texture);

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

   revert_state(disp, texture);

   if (use_buffers) {
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
   }

#if defined ALLEGRO_IPHONE
   al_free(iphone_idx);
#endif

   return num_primitives;
}

static int ogl_draw_prim(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, int start, int end, int type)
{
   return draw_prim_common(target, texture, 0, vtxs, decl, start, end, type);
}

static int ogl_draw_vertex_buffer(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, int start, int end, int type)
{
   return draw_prim_common(target, texture, vertex_buffer, 0, vertex_buffer->decl, start, end, type);
}

static int ogl_draw_prim_indexed(ALLEGRO_BITMAP *target, ALLEGRO_BITMAP* texture, const void* vtxs, const ALLEGRO_VERTEX_DECL* decl, const int* indices, int num_vtx, int type)
{
   return draw_prim_indexed_common(target, texture, NULL, vtxs, decl, NULL, indices, 0, num_vtx, type);
}

static int ogl_draw_indexed_buffer(ALLEGRO_BITMAP* target, ALLEGRO_BITMAP* texture, ALLEGRO_VERTEX_BUFFER* vertex_buffer, ALLEGRO_INDEX_BUFFER* index_buffer, int start, int end, int type)
{
   return draw_prim_indexed_common(target, texture, vertex_buffer, NULL, vertex_buffer->decl, index_buffer, NULL, start, end, type);
}

static bool create_buffer_common(_AL_BUFFER_COMMON* common, GLenum type, const void* initial_data, GLsizeiptr size, int flags)
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

static bool ogl_create_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf, const void* initial_data, size_t num_vertices, int flags)
{
   int stride = buf->decl ? buf->decl->stride : (int)sizeof(ALLEGRO_VERTEX);
   return create_buffer_common(&buf->common, GL_ARRAY_BUFFER, initial_data, num_vertices * stride, flags);
}

static bool ogl_create_index_buffer(ALLEGRO_INDEX_BUFFER* buf, const void* initial_data, size_t num_indices, int flags)
{
   return create_buffer_common(&buf->common, GL_ELEMENT_ARRAY_BUFFER, initial_data, num_indices * buf->index_size, flags);;
}

static void ogl_destroy_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf)
{
   glDeleteBuffers(1, (GLuint*)&buf->common.handle);
   al_free(buf->common.locked_memory);
}

static void ogl_destroy_index_buffer(ALLEGRO_INDEX_BUFFER* buf)
{
   glDeleteBuffers(1, (GLuint*)&buf->common.handle);
   al_free(buf->common.locked_memory);
}

static void* lock_buffer_common(_AL_BUFFER_COMMON* common, GLenum type)
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

static void* ogl_lock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf)
{
   return lock_buffer_common(&buf->common, GL_ARRAY_BUFFER);
}

static void* ogl_lock_index_buffer(ALLEGRO_INDEX_BUFFER* buf)
{
   return lock_buffer_common(&buf->common, GL_ELEMENT_ARRAY_BUFFER);
}

static void unlock_buffer_common(_AL_BUFFER_COMMON* common, GLenum type)
{
   if (common->lock_flags != ALLEGRO_LOCK_READONLY) {
      glBindBuffer(type, (GLuint)common->handle);
      glBufferSubData(type, common->lock_offset, common->lock_length, common->locked_memory);
      glBindBuffer(type, 0);
   }
}

static void ogl_unlock_vertex_buffer(ALLEGRO_VERTEX_BUFFER* buf)
{
   unlock_buffer_common(&buf->common, GL_ARRAY_BUFFER);
}

static void ogl_unlock_index_buffer(ALLEGRO_INDEX_BUFFER* buf)
{
   unlock_buffer_common(&buf->common, GL_ELEMENT_ARRAY_BUFFER);
}

static void try_const_color(ALLEGRO_DISPLAY *ogl_disp, ALLEGRO_COLOR *c)
{
   #ifdef ALLEGRO_CFG_OPENGLES
      #ifndef ALLEGRO_CFG_OPENGLES2
         return;
      #endif
      // Only OpenGL ES 2.0 has glBlendColor
      if (ogl_disp->ogl_extras->ogl_info.version < _ALLEGRO_OPENGL_VERSION_2_0) {
         return;
      }
   #else
   (void)ogl_disp;
   #endif
   glBlendColor(c->r, c->g, c->b, c->a);
}

bool _al_opengl_set_blender(ALLEGRO_DISPLAY *ogl_disp)
{
   int op, src_color, dst_color, op_alpha, src_alpha, dst_alpha;
   ALLEGRO_COLOR const_color;
   const int blend_modes[10] = {
      GL_ZERO, GL_ONE, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
      GL_SRC_COLOR, GL_DST_COLOR, GL_ONE_MINUS_SRC_COLOR,
      GL_ONE_MINUS_DST_COLOR,
#if defined(ALLEGRO_CFG_OPENGLES2) || !defined(ALLEGRO_CFG_OPENGLES)
      GL_CONSTANT_COLOR, GL_ONE_MINUS_CONSTANT_COLOR
#else
      GL_ONE, GL_ONE
#endif
   };
   const int blend_equations[3] = {
      GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT
   };

   (void)ogl_disp;

   al_get_separate_bitmap_blender(&op, &src_color, &dst_color,
      &op_alpha, &src_alpha, &dst_alpha);
   const_color = al_get_bitmap_blend_color();
   /* glBlendFuncSeparate was only included with OpenGL 1.4 */
#if !defined ALLEGRO_CFG_OPENGLES
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_1_4) {
#else
   /* FIXME: At this time (09/2014) there are a lot of Android phones that
    * don't support glBlendFuncSeparate even though they claim OpenGL ES 2.0
    * support. Rather than not work on 20-25% of phones, we just don't support
    * separate blending on Android for now.
    */
#if defined ALLEGRO_ANDROID && !defined ALLEGRO_CFG_OPENGLES3
   if (false) {
#else
   if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_2_0) {
#endif
#endif
      glEnable(GL_BLEND);
      try_const_color(ogl_disp, &const_color);
      glBlendFuncSeparate(blend_modes[src_color], blend_modes[dst_color],
         blend_modes[src_alpha], blend_modes[dst_alpha]);
      if (ogl_disp->ogl_extras->ogl_info.version >= _ALLEGRO_OPENGL_VERSION_2_0) {
         glBlendEquationSeparate(
            blend_equations[op],
            blend_equations[op_alpha]);
      }
      else {
         glBlendEquation(blend_equations[op]);
      }
   }
   else {
      if (src_color == src_alpha && dst_color == dst_alpha) {
         glEnable(GL_BLEND);
         try_const_color(ogl_disp, &const_color);
         glBlendFunc(blend_modes[src_color], blend_modes[dst_color]);
      }
      else {
         ALLEGRO_ERROR("Blender unsupported with this OpenGL version (%d %d %d %d %d %d)\n",
            op, src_color, dst_color, op_alpha, src_alpha, dst_alpha);
         return false;
      }
   }
   return true;
}

/* These functions make drawing calls use shaders or the fixed pipeline
 * based on what the user has set up. FIXME: OpenGL only right now.
 */

static void vert_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
/* Only use this shader stuff with GLES2+ or equivalent */
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.pos_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->varlocs.pos_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glEnableClientState(GL_VERTEX_ARRAY);
      glVertexPointer(n, t, stride, v);
#endif
   }
}

static void vert_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.pos_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->varlocs.pos_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glDisableClientState(GL_VERTEX_ARRAY);
#endif
   }
}

static void color_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.color_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->varlocs.color_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(n, t, stride, v);
#endif
   }
}

static void color_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.color_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->varlocs.color_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glDisableClientState(GL_COLOR_ARRAY);
#endif
   }
}

static void tex_ptr_on(ALLEGRO_DISPLAY *display, int n, GLint t, int stride, void *v)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.texcoord_loc >= 0) {
         glVertexAttribPointer(display->ogl_extras->varlocs.texcoord_loc, n, t, false, stride, v);
         glEnableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(n, t, stride, v);
#endif
   }
}

static void tex_ptr_off(ALLEGRO_DISPLAY *display)
{
   if (display->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (display->ogl_extras->varlocs.texcoord_loc >= 0) {
         glDisableVertexAttribArray(display->ogl_extras->varlocs.texcoord_loc);
      }
#endif
   }
   else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#endif
   }
}

static void ogl_clear(ALLEGRO_DISPLAY *d, ALLEGRO_COLOR *color)
{
   ALLEGRO_DISPLAY *ogl_disp = (void *)d;
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_target;
   float r, g, b, a;

   if (target->parent)
      target = target->parent;

   ogl_target = target->extra;

   if ((!ogl_target->is_backbuffer &&
         ogl_disp->ogl_extras->opengl_target != target)
      || target->locked)
   {
      _al_clear_bitmap_by_locking(target, color);
      return;
   }

   al_unmap_rgba_f(*color, &r, &g, &b, &a);

   glClearColor(r, g, b, a);
   glClear(GL_COLOR_BUFFER_BIT);
}


static void ogl_draw_pixel(ALLEGRO_DISPLAY *d, float x, float y,
   ALLEGRO_COLOR *color)
{
   ALLEGRO_BITMAP *target = al_get_target_bitmap();
   ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_target;
   GLfloat vert[2];
   GLfloat color_array[4];

   /* For sub-bitmaps */
   if (target->parent) {
      target = target->parent;
   }

   ogl_target = target->extra;

   if ((!ogl_target->is_backbuffer &&
      d->ogl_extras->opengl_target != target) ||
      target->locked || !_al_opengl_set_blender(d))  {
      _al_draw_pixel_memory(target, x, y, color);
      return;
   }

   vert[0] = x;
   vert[1] = y;

   color_array[0] = color->r;
   color_array[1] = color->g;
   color_array[2] = color->b;
   color_array[3] = color->a;

   vert_ptr_on(d, 2, GL_FLOAT, 2*sizeof(float), vert);
   color_ptr_on(d, 4, GL_FLOAT, 4*sizeof(float), color_array);

   // Should this be here if it's in the if above?
   if (!_al_opengl_set_blender(d)) {
      return;
   }

   glDrawArrays(GL_POINTS, 0, 1);

   vert_ptr_off(d);
   color_ptr_off(d);
}

static void* ogl_prepare_vertex_cache(ALLEGRO_DISPLAY* disp,
                                      int num_new_vertices)
{
   disp->num_cache_vertices += num_new_vertices;
   if (!disp->vertex_cache) {
      disp->vertex_cache = al_malloc(num_new_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));

      disp->vertex_cache_size = num_new_vertices;
   } else if (disp->num_cache_vertices > disp->vertex_cache_size) {
      disp->vertex_cache = al_realloc(disp->vertex_cache,
                              2 * disp->num_cache_vertices * sizeof(ALLEGRO_OGL_BITMAP_VERTEX));

      disp->vertex_cache_size = 2 * disp->num_cache_vertices;
   }
   return (ALLEGRO_OGL_BITMAP_VERTEX*)disp->vertex_cache +
         (disp->num_cache_vertices - num_new_vertices);
}

static void ogl_flush_vertex_cache(ALLEGRO_DISPLAY *disp)
{
   GLuint current_texture;
   ALLEGRO_OGL_EXTRAS *o = disp->ogl_extras;
   (void)o; /* not used in all ports */

   if (!disp->vertex_cache)
      return;
   if (disp->num_cache_vertices == 0)
      return;

   if (!_al_opengl_set_blender(disp)) {
      disp->num_cache_vertices = 0;
      return;
   }

   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (disp->ogl_extras->varlocs.use_tex_loc >= 0) {
         glUniform1i(disp->ogl_extras->varlocs.use_tex_loc, 1);
      }
      if (disp->ogl_extras->varlocs.use_tex_matrix_loc >= 0) {
         glUniform1i(disp->ogl_extras->varlocs.use_tex_matrix_loc, 0);
      }
#endif
   }
   else {
      glEnable(GL_TEXTURE_2D);
   }

   glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&current_texture);
   if (current_texture != disp->cache_texture) {
      if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
         /* Use texture unit 0 */
         glActiveTexture(GL_TEXTURE0);
         if (disp->ogl_extras->varlocs.tex_loc >= 0)
            glUniform1i(disp->ogl_extras->varlocs.tex_loc, 0);
#endif
      }
      glBindTexture(GL_TEXTURE_2D, disp->cache_texture);
   }

#if !defined(ALLEGRO_CFG_OPENGLES)
#if defined(ALLEGRO_MACOSX)
   if (disp->flags & (ALLEGRO_PROGRAMMABLE_PIPELINE | ALLEGRO_OPENGL_3_0)) {
#else
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#endif
      int stride = sizeof(ALLEGRO_OGL_BITMAP_VERTEX);
      int bytes = disp->num_cache_vertices * stride;

      /* We create the VAO and VBO on first use. */
      if (o->vao == 0) {
         glGenVertexArrays(1, &o->vao);
         ALLEGRO_DEBUG("new VAO: %u\n", o->vao);
      }
      glBindVertexArray(o->vao);

      if (o->vbo == 0) {
         glGenBuffers(1, &o->vbo);
         ALLEGRO_DEBUG("new VBO: %u\n", o->vbo);
      }
      glBindBuffer(GL_ARRAY_BUFFER, o->vbo);

      /* Then we upload data into it. */
      glBufferData(GL_ARRAY_BUFFER, bytes, disp->vertex_cache, GL_STREAM_DRAW);

      /* Finally set the "pos", "texccord" and "color" attributes used by our
       * shader and enable them.
       */
      if (o->varlocs.pos_loc >= 0)  {
         glVertexAttribPointer(o->varlocs.pos_loc, 3, GL_FLOAT, false, stride,
            (void *)offsetof(ALLEGRO_OGL_BITMAP_VERTEX, x));
         glEnableVertexAttribArray(o->varlocs.pos_loc);
      }

      if (o->varlocs.texcoord_loc >= 0) {
         glVertexAttribPointer(o->varlocs.texcoord_loc, 2, GL_FLOAT, false, stride,
            (void *)offsetof(ALLEGRO_OGL_BITMAP_VERTEX, tx));
         glEnableVertexAttribArray(o->varlocs.texcoord_loc);
      }

      if (o->varlocs.color_loc >= 0) {
         glVertexAttribPointer(o->varlocs.color_loc, 4, GL_FLOAT, false, stride,
            (void *)offsetof(ALLEGRO_OGL_BITMAP_VERTEX, r));
         glEnableVertexAttribArray(o->varlocs.color_loc);
      }
   }
   else
#endif
   {
      vert_ptr_on(disp, 3, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX),
         (char *)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, x));
      tex_ptr_on(disp, 2, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX),
         (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, tx));
      color_ptr_on(disp, 4, GL_FLOAT, sizeof(ALLEGRO_OGL_BITMAP_VERTEX),
         (char*)(disp->vertex_cache) + offsetof(ALLEGRO_OGL_BITMAP_VERTEX, r));

#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      if (!(disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE))
         glDisableClientState(GL_NORMAL_ARRAY);
#endif
   }

   glGetError(); /* clear error */
   glDrawArrays(GL_TRIANGLES, 0, disp->num_cache_vertices);

#ifdef DEBUGMODE
   {
      int e = glGetError();
      if (e) {
         ALLEGRO_WARN("glDrawArrays failed: %s\n", _al_gl_error_string(e));
      }
   }
#endif

#if !defined ALLEGRO_CFG_OPENGLES && !defined ALLEGRO_MACOSX
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
      if (o->varlocs.pos_loc >= 0)
         glDisableVertexAttribArray(o->varlocs.pos_loc);
      if (o->varlocs.texcoord_loc >= 0)
         glDisableVertexAttribArray(o->varlocs.texcoord_loc);
      if (o->varlocs.color_loc >= 0)
         glDisableVertexAttribArray(o->varlocs.color_loc);
      glBindBuffer(GL_ARRAY_BUFFER, 0);
      glBindVertexArray(0);
   }
   else
#endif
   {
      vert_ptr_off(disp);
      tex_ptr_off(disp);
      color_ptr_off(disp);
   }

   disp->num_cache_vertices = 0;

   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
      if (disp->ogl_extras->varlocs.use_tex_loc >= 0)
         glUniform1i(disp->ogl_extras->varlocs.use_tex_loc, 0);
#endif
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }
}

static int ogl_prepare_batch(ALLEGRO_DISPLAY* disp, int num_new_vertices, int num_new_indices, void **vertices, void **indices)
{
   int first_index = disp->batch_vertices_length;
   disp->batch_vertices_length += num_new_vertices;
   // TODO: We have to bail out here based on size.
   if (!disp->batch_vertices) {
      disp->batch_vertices = al_malloc(num_new_vertices * sizeof(ALLEGRO_VERTEX));
      disp->batch_vertices_capacity = num_new_vertices;
   }
   else {
      bool do_realloc = false;
      while (disp->batch_vertices_length > disp->batch_vertices_capacity) {
         disp->batch_vertices_capacity *= 2;
         do_realloc = true;
      }
      if (do_realloc)
         disp->batch_vertices = al_realloc(disp->batch_vertices, disp->batch_vertices_capacity * sizeof(ALLEGRO_VERTEX));
   }
   *vertices = (ALLEGRO_VERTEX*)disp->batch_vertices + (disp->batch_vertices_length - num_new_vertices);

   disp->batch_indices_length += num_new_indices;
   if (!disp->batch_indices) {
      disp->batch_indices = al_malloc(num_new_indices * disp->index_size);
      disp->batch_indices_capacity = num_new_indices;
   }
   else {
      bool do_realloc = false;
      while (disp->batch_indices_length > disp->batch_indices_capacity) {
         disp->batch_indices_capacity *= 2;
         do_realloc = true;
      }
      if (do_realloc)
         disp->batch_indices = al_realloc(disp->batch_indices, disp->batch_indices_capacity * disp->index_size);
   }
   *indices = (char*)disp->batch_indices + disp->index_size * (disp->batch_indices_length - num_new_indices);
   return first_index;
}

static void ogl_draw_batch(ALLEGRO_DISPLAY *disp)
{
   ALLEGRO_OGL_EXTRAS *o = disp->ogl_extras;

   if (!disp->bitmap_vertex_decl) {
      const ALLEGRO_VERTEX_ELEMENT elems[] = {
         {ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_FLOAT_3, offsetof(ALLEGRO_VERTEX, x)},
         {_ALLEGRO_PRIM_TEX_COORD_INTERNAL, ALLEGRO_PRIM_FLOAT_2, offsetof(ALLEGRO_VERTEX, u)},
         {ALLEGRO_PRIM_COLOR_ATTR, 0, offsetof(ALLEGRO_VERTEX, color)},
         {0, 0, 0}
      };
      disp->bitmap_vertex_decl = _al_create_vertex_decl(elems, sizeof(ALLEGRO_VERTEX));
   }

   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
     if (o->vao == 0) {
        glGenVertexArrays(1, &o->vao);
        ALLEGRO_DEBUG("new VAO: %u\n", o->vao);
     }
     glBindVertexArray(o->vao);
   }

   if (disp->batch_vertices_length == 0)
      goto exit;
   if (disp->batch_indices_length == 0)
      goto exit;
   if (!_al_opengl_set_blender(disp))
      goto exit;

   if (!disp->batch_vertex_buffer) {
      disp->batch_vertex_buffer = _al_create_vertex_buffer(disp->bitmap_vertex_decl, NULL, MAX_BATCH_SIZE, ALLEGRO_PRIM_BUFFER_DYNAMIC);
   }
   if (!disp->batch_vertex_buffer)
      goto exit;

   if (!disp->batch_index_buffer)
      disp->batch_index_buffer = _al_create_index_buffer(sizeof(uint16_t), NULL, MAX_BATCH_SIZE, ALLEGRO_PRIM_BUFFER_DYNAMIC);
   if (!disp->batch_index_buffer)
      goto exit;

   void *batch_vertices = _al_lock_vertex_buffer(disp->batch_vertex_buffer, 0, disp->batch_vertices_length, ALLEGRO_LOCK_WRITEONLY);
   if (!batch_vertices)
      goto exit;
   void *batch_indices = _al_lock_index_buffer(disp->batch_index_buffer, 0, disp->batch_indices_length, ALLEGRO_LOCK_WRITEONLY);
   if (!batch_indices)
      goto exit;

   memcpy(batch_vertices, disp->batch_vertices, disp->batch_vertices_length * sizeof(ALLEGRO_VERTEX));
   memcpy(batch_indices, disp->batch_indices, disp->batch_indices_length * sizeof(uint16_t));

   _al_unlock_vertex_buffer(disp->batch_vertex_buffer);
   _al_unlock_index_buffer(disp->batch_index_buffer);

   _al_draw_indexed_buffer(disp->batch_vertex_buffer, disp->batch_texture, disp->batch_index_buffer, 0, disp->batch_indices_length, ALLEGRO_PRIM_TRIANGLE_LIST);

exit:
   disp->batch_vertices_length = 0;
   disp->batch_indices_length = 0;
}

static void ogl_update_transformation(ALLEGRO_DISPLAY* disp,
   ALLEGRO_BITMAP *target)
{
   if (disp->flags & ALLEGRO_PROGRAMMABLE_PIPELINE) {
#ifdef ALLEGRO_CFG_SHADER_GLSL
      GLint loc = disp->ogl_extras->varlocs.projview_matrix_loc;
      ALLEGRO_TRANSFORM projview;
      al_copy_transform(&projview, &target->transform);
      al_compose_transform(&projview, &target->proj_transform);
      al_copy_transform(&disp->projview_transform, &projview);

      if (disp->ogl_extras->program_object > 0 && loc >= 0) {
         _al_glsl_set_projview_matrix(loc, &disp->projview_transform);
      }
#endif
   } else {
#ifdef ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
      glMatrixMode(GL_PROJECTION);
      glLoadMatrixf((float *)target->proj_transform.m);
      glMatrixMode(GL_MODELVIEW);
      glLoadMatrixf((float *)target->transform.m);
#endif
   }

   if (target->parent) {
      ALLEGRO_BITMAP_EXTRA_OPENGL *ogl_extra = target->parent->extra;
      /* glViewport requires the bottom-left coordinate of the corner. */
      glViewport(target->xofs, ogl_extra->true_h - (target->yofs + target->h), target->w, target->h);
   } else {
      glViewport(0, 0, target->w, target->h);
   }
}

static void ogl_clear_depth_buffer(ALLEGRO_DISPLAY *display, float x)
{
   (void)display;

#if defined(ALLEGRO_CFG_OPENGLES)
   glClearDepthf(x);
#else
   glClearDepth(x);
#endif

   /* We may want to defer this to the next glClear call as a combined
    * color/depth clear may be faster.
    */
   glClear(GL_DEPTH_BUFFER_BIT);
}

/* Add drawing commands to the vtable. */
void _al_ogl_add_drawing_functions(ALLEGRO_DISPLAY_INTERFACE *vt)
{
   vt->clear = ogl_clear;
   vt->draw_pixel = ogl_draw_pixel;
   vt->clear_depth_buffer = ogl_clear_depth_buffer;

   vt->prepare_batch = ogl_prepare_batch;
   vt->draw_batch = ogl_draw_batch;

   vt->flush_vertex_cache = ogl_flush_vertex_cache;
   vt->prepare_vertex_cache = ogl_prepare_vertex_cache;
   vt->update_transformation = ogl_update_transformation;

   vt->draw_prim = ogl_draw_prim;
   vt->draw_prim_indexed = ogl_draw_prim_indexed;

   vt->create_vertex_buffer = ogl_create_vertex_buffer;
   vt->destroy_vertex_buffer = ogl_destroy_vertex_buffer;
   vt->lock_vertex_buffer = ogl_lock_vertex_buffer;
   vt->unlock_vertex_buffer = ogl_unlock_vertex_buffer;

   vt->create_index_buffer = ogl_create_index_buffer;
   vt->destroy_index_buffer = ogl_destroy_index_buffer;
   vt->lock_index_buffer = ogl_lock_index_buffer;
   vt->unlock_index_buffer = ogl_unlock_index_buffer;

   vt->draw_vertex_buffer = ogl_draw_vertex_buffer;
   vt->draw_indexed_buffer = ogl_draw_indexed_buffer;
}

/* vim: set sts=3 sw=3 et: */
