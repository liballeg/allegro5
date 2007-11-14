/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file gui.c
  * \brief AllegroGL GUI wrappers
  *
  * These are replacements for Allegro's do_dialog routine and 
  * standard dialogs (to use our version of the routine).
  */


#include "alleggl.h"
#include "allglint.h"

#include <allegro/internal/aintern.h>


static struct {
	GLuint texture;
	int hidden;
	int xfocus;
	int yfocus;
	int width;
	int height;
} allegro_gl_mouse = { 0, TRUE, 0, 0, 0, 0};


/**
 * \ingroup gui
 * \brief AllegroGL-friendly version of do_dialog
 *
 * This behaves exactly like do_dialog but forces a screen clear, 
 * rerender, and flip, after each iteration of update_dialog.
 *
 * User gui components can do OpenGL or Allegro rendering to draw 
 * themselves.  They should take care not to alter any OpenGL state
 * (or be aware that this will affect other components).  For the 
 * main render, they will be called in order, but this is not 
 * guarranteed at other times -- they may be called out of order;
 * however the results will never be visible, so just don't crash.
 *
 * Before drawing the final (in-order) pass, the color and depth buffers will
 * be cleared -- set your clear color to black or green or whatever you like.
 * You can overdraw it with an object of course.
 *
 * Further notes: This routine uses allegro_gl_set_allegro_mode(), so
 * your GUI components can use allegro_gl_unset_allegro_mode() to restore
 * the old state while they draw themselves, provided that they use 
 * allegro_gl_set_allegro_mode() again afterwards.
 *
 * \param dialog an array of dialog objects terminated by one with a NULL 
 * dialog procedure.
 * \param focus_obj index of the object on which the focus is set (-1 if you
 * don't want anything to have the focus
 *
 * \sa algl_draw_mouse
 */
int algl_do_dialog (DIALOG *dialog, int focus_obj)
{
	DIALOG_PLAYER *player;

	AGL_LOG(2, "allegro_gl_do_dialog\n");

	/* Allegro GUI routines generally use the 2D gfx functions therefore
	   we set default behaviour to allegro_gl_set_allegro_mode so that we
	   can use the GUI functions "as is" */
	allegro_gl_set_allegro_mode();

	player = init_dialog (dialog, focus_obj);
	show_mouse(screen);

	/* Nothing to do here.
	 * Redrawing is done from d_algl_viewport_proc() callback. */
	while (update_dialog (player)) {}

	show_mouse(NULL);
	/* restore previous projection matrices */
	allegro_gl_unset_allegro_mode();

	return shutdown_dialog (player);
}



/**
 * \ingroup gui
 * \brief AllegroGL-friendly version of popup_dialog
 *
 * This routine is likely to be very slow.  It has to take a copy of the
 * screen on entry, then where algl_do_dialog() would just clear the screen,
 * this routine has to blit that copy back again after clearing.  This is
 * the only way to do overlays without knowing what type of flipping is
 * going on.
 *
 * Also, note that we don't save the depth buffer or anything like that so
 * don't go around thinking that algl_popup_dialog won't affect anything
 * when it's gone.
 *
 * So, unless you need overlays, it's recommended that you just use
 * algl_do_dialog(), and if you do need overlays, it's recommended that you
 * just use algl_do_dialog() and redraw the thing you're overlaying yourself.
 * If you're lazy or that's impossible, use this routine...
 *
 * \param dialog an array of dialog objects terminated by one with a NULL 
 * dialog procedure.
 * \param focus_obj index of the object on which the focus is set (-1 if you
 * don't want anything to have the focus
 *
 * \sa algl_do_dialog, algl_draw_mouse
 */
int algl_popup_dialog (DIALOG *dialog, int focus_obj)
{
	void *backdrop;
	DIALOG_PLAYER *player;
	GLint read_buffer;

	AGL_LOG(2, "allegro_gl_popup_dialog\n");

	/* Allegro GUI routines generally use the 2D gfx functions therefore
	   we set default behaviour to allegro_gl_set_allegro_mode so that we
	   can use the GUI functions "as is" */
	allegro_gl_set_allegro_mode();

	glGetIntegerv(GL_READ_BUFFER, &read_buffer);
	glReadBuffer (GL_FRONT); /* TODO: don't clobber */
	glDisable(GL_DEPTH_TEST);
	backdrop = malloc (SCREEN_W * SCREEN_H * 3 * 4);
	glReadPixels (0, 0, SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, backdrop);
	glReadBuffer(read_buffer);

	player = init_dialog (dialog, focus_obj);
	show_mouse(screen);

	while (update_dialog (player)) {

		/* Redraw the GUI every frame */
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glRasterPos2f (0., SCREEN_H-.5); /* TODO: don't clobber */
		glDrawPixels (SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, backdrop);
		broadcast_dialog_message (MSG_DRAW, 0);

		/* Draw the mouse cursor */
		algl_draw_mouse();

		/* Flip buffers */
		allegro_gl_flip();
	}

	glRasterPos2f (0., SCREEN_H-.5); /* TODO: don't clobber */
	glDrawPixels (SCREEN_W, SCREEN_H, GL_RGB, GL_UNSIGNED_BYTE, backdrop);
	glEnable(GL_DEPTH_TEST);
	free (backdrop);

	show_mouse(NULL);
	/* restore previous projection matrices */
	allegro_gl_unset_allegro_mode();

	return shutdown_dialog (player);
}



/* User mouse drawing callback */
static void (*__algl_user_draw_mouse)(void) = NULL;


/**
 * \ingroup gui
 * \brief Draws a mouse pointer on the screen
 *
 * This function draws a mouse pointer on the screen. By default, it displays
 * Allegro's standard black arrow cursor. However the settings of the cursor
 * can be altered by Allegro's functions for mouse cursor management like
 * show_mouse, set_mouse_sprite, scare_mouse, and so on... As a consequence,
 * it should be stressed that if show_mouse(NULL) is called then
 * algl_draw_mouse() won't draw anything.
 * 
 * Unlike Allegro, AllegroGL does not manage the mouse cursor with an
 * interrupt routine, hence algl_draw_mouse() must be regularly called 
 * in order to display the mouse cursor (ideally it should be the last function
 * called before allegro_gl_flip()). However if you use algl_do_dialog()
 * then you do not need to make explicit calls to algl_draw_mouse() since
 * algl_do_dialog() takes care of that for you.
 *
 * \sa algl_set_mouse_drawer
 */
void algl_draw_mouse (void)
{
	AGL_LOG(2, "allegro_gl_draw_mouse\n");

	/* don't draw the mouse if it's not in our window */
	if (!_mouse_on || allegro_gl_mouse.hidden) return;

	if (__algl_user_draw_mouse) {

		__algl_user_draw_mouse();

	} else {

#if 0
		float x = mouse_x;
		float y = mouse_y;

		int depth_enabled = glIsEnabled (GL_DEPTH_TEST);
		int cull_enabled = glIsEnabled (GL_CULL_FACE);
		if (depth_enabled) glDisable (GL_DEPTH_TEST);
		if (cull_enabled) glDisable (GL_CULL_FACE);

		glBegin (GL_TRIANGLES);

			#define draw(dx,dy) \
				glVertex2f (x + dx, y + dy); \
				glVertex2f (x + dx, y + dy + 10); \
				glVertex2f (x + dx + 7, y + dy + 7); \
				glVertex2f (x + dx + 1.5, y + dy + 6); \
				glVertex2f (x + dx + 5.5, y + dy + 14); \
				glVertex2f (x + dx + 7.5, y + dy + 14); \
				glVertex2f (x + dx + 3.5, y + dy + 6); \
				glVertex2f (x + dx + 1.5, y + dy + 6); \
				glVertex2f (x + dx + 7.5, y + dy + 14);

			glColor3f (0, 0, 0);
			draw(-1,0)
			draw(1,0)
			draw(0,-1)
			draw(0,1)

			glColor3f (1, 1, 1);
			draw(0,0)

			#undef draw

		glEnd();

		if (depth_enabled) glEnable (GL_DEPTH_TEST);
		if (cull_enabled) glEnable (GL_CULL_FACE);
#endif

		int x = mouse_x - allegro_gl_mouse.xfocus;
		int y = mouse_y - allegro_gl_mouse.yfocus;
	
		glPushAttrib(GL_COLOR_BUFFER_BIT);
		glAlphaFunc(GL_GREATER, 0.5);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_ALPHA_TEST);

		glBindTexture(GL_TEXTURE_2D, allegro_gl_mouse.texture);
		glColor4f(1., 1., 1., 1.);
		glTranslatef(-0.375, -0.375, 0);
		glBegin(GL_QUADS);
			glTexCoord2f(0., 1.);
			glVertex2f(x, y);
			glTexCoord2f(0., 0.);
			glVertex2f(x, y + allegro_gl_mouse.height);
			glTexCoord2f(1., 0.);
			glVertex2f(x + allegro_gl_mouse.width, y + allegro_gl_mouse.height);
			glTexCoord2f(1., 1.);
			glVertex2f(x + allegro_gl_mouse.width, y);
		glEnd();
		glTranslatef(0.375, 0.375, 0);
		glPopAttrib();
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
	}
}


/**
 * \ingroup gui
 * \brief Sets (or clears) a user mouse drawing callback
 *
 * This function allows to use a user-defined routine to display the mouse
 * cursor. This allows nice effects like a spinning cube or any fancy thing
 * you can think of to be used as a mouse cursor.
 *
 * When a user mouse drawing callback is enabled, set_mouse_sprite has no effect.
 * However show_mouse and scare_mouse are still enabled.
 *
 * \param user_draw_mouse user routine that displays the mouse cursor (NULL if
 * you want to get back to the standard behaviour)
 *
 * \sa algl_draw_mouse
 */
void algl_set_mouse_drawer (void (*user_draw_mouse)(void))
{
	AGL_LOG(2, "allegro_gl_set_mouse_drawer\n");

	__algl_user_draw_mouse = user_draw_mouse;
}





static DIALOG alert_dialog[] =
{
   /* (dialog proc)        (x)   (y)   (w)   (h)   (fg)  (bg)  (key) (flags)  (d1)  (d2)  (dp)  (dp2) (dp3) */
   { _gui_shadow_box_proc, 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_ctext_proc,      0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { _gui_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL, NULL, NULL  },
   { _gui_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL, NULL, NULL  },
   { _gui_button_proc,     0,    0,    0,    0,    0,    0,    0,    D_EXIT,  0,    0,    NULL, NULL, NULL  },
   { d_yield_proc,         0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  },
   { NULL,                 0,    0,    0,    0,    0,    0,    0,    0,       0,    0,    NULL, NULL, NULL  }
};


#define A_S1  1
#define A_S2  2
#define A_S3  3
#define A_B1  4
#define A_B2  5
#define A_B3  6



/**
 * \ingroup gui
 * \brief AllegroGL-friendly version of Allegro's alert3.
 *
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one, two, or three buttons. The text for these buttons 
 *  is passed in b1, b2, and b3 (NULL for buttons which are not used), and
 *  the keyboard shortcuts in c1 and c2. Returns 1, 2, or 3 depending on 
 *  which button was selected.
 */
int algl_alert3(AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, AL_CONST char *b3, int c1, int c2, int c3)
{
   char tmp[16];
   int avg_w, avg_h;
   int len1, len2, len3;
   int maxlen = 0;
   int buttons = 0;
   int b[3];
   int c;

   AGL_LOG(2, "allegro_gl_alert3\n");

   #define SORT_OUT_BUTTON(x) {                                            \
      if (b##x) {                                                          \
	 alert_dialog[A_B##x].flags &= ~D_HIDDEN;                          \
	 alert_dialog[A_B##x].key = c##x;                                  \
	 alert_dialog[A_B##x].dp = (char *)b##x;                           \
	 len##x = gui_strlen(b##x);                                        \
	 b[buttons++] = A_B##x;                                            \
      }                                                                    \
      else {                                                               \
	 alert_dialog[A_B##x].flags |= D_HIDDEN;                           \
	 len##x = 0;                                                       \
      }                                                                    \
   }

   usetc(tmp+usetc(tmp, ' '), 0);

   avg_w = text_length(font, tmp);
   avg_h = text_height(font);

   alert_dialog[A_S1].dp = alert_dialog[A_S2].dp = alert_dialog[A_S3].dp = 
   alert_dialog[A_B1].dp = alert_dialog[A_B2].dp = empty_string;

   if (s1) {
      alert_dialog[A_S1].dp = (char *)s1;
      maxlen = text_length(font, s1);
   }

   if (s2) {
      alert_dialog[A_S2].dp = (char *)s2;
      len1 = text_length(font, s2);
      if (len1 > maxlen)
	 maxlen = len1;
   }

   if (s3) {
      alert_dialog[A_S3].dp = (char *)s3;
      len1 = text_length(font, s3);
      if (len1 > maxlen)
	 maxlen = len1;
   }

   SORT_OUT_BUTTON(1);
   SORT_OUT_BUTTON(2);
   SORT_OUT_BUTTON(3);

   len1 = MAX(len1, MAX(len2, len3)) + avg_w*3;
   if (len1*buttons > maxlen)
      maxlen = len1*buttons;

   maxlen += avg_w*4;
   alert_dialog[0].w = maxlen;
   alert_dialog[A_S1].x = alert_dialog[A_S2].x = alert_dialog[A_S3].x = 
						alert_dialog[0].x + maxlen/2;

   alert_dialog[A_B1].w = alert_dialog[A_B2].w = alert_dialog[A_B3].w = len1;

   alert_dialog[A_B1].x = alert_dialog[A_B2].x = alert_dialog[A_B3].x = 
				       alert_dialog[0].x + maxlen/2 - len1/2;

   if (buttons == 3) {
      alert_dialog[b[0]].x = alert_dialog[0].x + maxlen/2 - len1*3/2 - avg_w;
      alert_dialog[b[2]].x = alert_dialog[0].x + maxlen/2 + len1/2 + avg_w;
   }
   else if (buttons == 2) {
      alert_dialog[b[0]].x = alert_dialog[0].x + maxlen/2 - len1 - avg_w;
      alert_dialog[b[1]].x = alert_dialog[0].x + maxlen/2 + avg_w;
   }

   alert_dialog[0].h = avg_h*8;
   alert_dialog[A_S1].y = alert_dialog[0].y + avg_h;
   alert_dialog[A_S2].y = alert_dialog[0].y + avg_h*2;
   alert_dialog[A_S3].y = alert_dialog[0].y + avg_h*3;
   alert_dialog[A_S1].h = alert_dialog[A_S2].h = alert_dialog[A_S3].h = avg_h;
   alert_dialog[A_B1].y = alert_dialog[A_B2].y = alert_dialog[A_B3].y = alert_dialog[0].y + avg_h*5;
   alert_dialog[A_B1].h = alert_dialog[A_B2].h = alert_dialog[A_B3].h = avg_h*2;

   centre_dialog(alert_dialog);
   set_dialog_color(alert_dialog, gui_fg_color, gui_bg_color);
   for (c = 0; alert_dialog[c].proc; c++)
      if (alert_dialog[c].proc == _gui_ctext_proc)
	 alert_dialog[c].bg = -1;

   clear_keybuf();

   do {
   } while (gui_mouse_b());

   c = algl_popup_dialog(alert_dialog, A_B1);

   if (c == A_B1)
      return 1;
   else if (c == A_B2)
      return 2;
   else
      return 3;
}



/**
 * \ingroup gui
 * \brief AllegroGL-friendly version of Allegro's alert.
 *
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one or two buttons. The text for these buttons is passed
 *  in b1 and b2 (b2 may be null), and the keyboard shortcuts in c1 and c2.
 *  Returns 1 or 2 depending on which button was selected.
 */
int algl_alert(AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, int c1, int c2)
{
   int ret;

   AGL_LOG(2, "allegro_gl_alert\n");

   ret = algl_alert3(s1, s2, s3, b1, b2, NULL, c1, c2, 0);

   if (ret > 2)
      ret = 2;

   return ret;
}



/**
 * \ingroup gui
 * \brief Creates a viewport object where OpenGL commands can be performed.
 *
 *  The viewport and the scissor are updated so that this GUI object behaves
 *  somewhat like a window. The dp field of the DIALOG object points to a
 *  callback function : int (*callback)(BITMAP* viewport, int msg, int c) where:
 *  viewport is a sub-bitmap of the screen limited to the area of the
 *  DIALOG object. msg and c are the values that come from the GUI manager.
 *  The callback function must return a sensible value to the GUI manager
 *  like D_O_K if everything went right or D_EXIT to close the dialog.
 */
int d_algl_viewport_proc(int msg, DIALOG *d, int c)
{
	int ret = D_O_K;
	typedef int (*_callback)(BITMAP*, int, int);
	_callback callback = (_callback) d->dp;
	BITMAP *viewport = create_sub_bitmap(screen, d->x, d->y, d->w, d->h);

	AGL_LOG(3, "d_algl_viewport_proc\n");

	if (msg == MSG_DRAW) {
		/* Draws the background */
		clear_to_color(viewport, d->bg);
	}

	/* First we get back into a 3D mode */
	allegro_gl_unset_allegro_mode();

	/* Save the Viewport and Scissor states */
	glPushAttrib(GL_SCISSOR_BIT | GL_VIEWPORT_BIT);

	/* Adapt the viewport to the object size */
	glViewport(d->x, SCREEN_H - d->y - d->h, d->w, d->h);
	glScissor(d->x, SCREEN_H - d->y - d->h, d->w, d->h);
	glEnable(GL_SCISSOR_TEST);
	
	/* Clear the depth buffer for this scissor region */
	if (msg == MSG_DRAW) {
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	/* Call the callback function */
	if (callback)
		ret = callback(viewport, msg, c);

	/* Restore the previous state */
	glPopAttrib();
	allegro_gl_set_allegro_mode();
	destroy_bitmap(viewport);

	/* Redraw the GUI every frame */
	if (msg == MSG_IDLE) {
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		broadcast_dialog_message (MSG_DRAW, 0);

		/* Draw the mouse cursor */
		algl_draw_mouse();

		/* Flip buffers */
		allegro_gl_flip();
	}


	return ret;
}



/*****************/
/* Mouse manager */
/*****************/

int allegro_gl_set_mouse_sprite(BITMAP *sprite, int xfocus, int yfocus)
{
	BITMAP *bmp = NULL;
	GLint old_texture;
	
	AGL_LOG(2, "allegro_gl_set_mouse_sprite\n");

	glGetIntegerv(GL_TEXTURE_2D_BINDING, &old_texture);

	bmp = create_bitmap_ex(bitmap_color_depth(sprite),
	         __allegro_gl_make_power_of_2(sprite->w),
	         __allegro_gl_make_power_of_2(sprite->h));

	if (allegro_gl_mouse.texture) {
		glDeleteTextures(1, &allegro_gl_mouse.texture);
		allegro_gl_mouse.texture = 0;
	}
	
	clear_to_color(bmp, bitmap_mask_color(sprite));
	blit(sprite, bmp, 0, 0, 0, 0, sprite->w, sprite->h);
#ifdef DEBUGMODE
	save_bmp("mcursor.bmp",bmp,NULL);
#endif

	allegro_gl_mouse.texture = allegro_gl_make_texture_ex(AGL_TEXTURE_RESCALE
	                        | AGL_TEXTURE_MASKED | AGL_TEXTURE_FLIP, bmp, -1);
	if (!allegro_gl_mouse.texture) {
		destroy_bitmap(bmp);
		return -1;
	}

	glBindTexture(GL_TEXTURE_2D, allegro_gl_mouse.texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	
	if (allegro_gl_extensions_GL.SGIS_texture_edge_clamp) {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	glBindTexture(GL_TEXTURE_2D, old_texture);

	allegro_gl_mouse.width  = bmp->w;
	allegro_gl_mouse.height = bmp->h;
	allegro_gl_mouse.xfocus = xfocus;
	allegro_gl_mouse.yfocus = yfocus;

	destroy_bitmap(bmp);
	return 0;
}



int allegro_gl_show_mouse(BITMAP* bmp, int x, int y)
{
	AGL_LOG(3, "allegro_gl_show_mouse\n");
	allegro_gl_mouse.hidden = FALSE;
	return 0;
}



void allegro_gl_hide_mouse(void)
{
	AGL_LOG(3, "allegro_gl_hide_mouse\n");
	allegro_gl_mouse.hidden = TRUE;
}



void allegro_gl_move_mouse(int x, int y)
{
	AGL_LOG(3, "allegro_gl_move_mouse\n");
	/* This function is not called from the main thread, so
	 * we must not call any OpenGL command there !!!
	 */
}

