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
 *      Mouse input routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Mark Wodrich added double-buffered drawing of the mouse pointer and
 *      the set_mouse_sprite_focus() function.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* dummy driver for systems without a mouse */
static int nomouse_init(void) { return 0; }
static void nomouse_exit(void) { }

MOUSE_DRIVER mousedrv_none =
{
   MOUSEDRV_NONE,
   empty_string,
   empty_string,
   "No mouse",
   nomouse_init,
   nomouse_exit,
   NULL, NULL, NULL, NULL, NULL, NULL, NULL
};


MOUSE_DRIVER *mouse_driver = NULL;     /* the active driver */
int _mouse_type = MOUSEDRV_AUTODETECT;  /* driver ID */
int _mouse_installed = FALSE;

volatile int mouse_x = 0;              /* user-visible position */
volatile int mouse_y = 0;
volatile int mouse_z = 0;
volatile int mouse_b = 0; 
volatile int mouse_pos = 0; 

int _mouse_x = 0;                      /* internal position */
int _mouse_y = 0;
int _mouse_z = 0;
int _mouse_b = 0;
int _mouse_on = TRUE;

static int mon = TRUE;

static int emulate_three = FALSE;

volatile int freeze_mouse_flag = FALSE;

void (*mouse_callback)(int flags) = NULL;

int mouse_x_focus = 1;                 /* focus point in mouse sprite */
int mouse_y_focus = 1;


#define MOUSE_OFFSCREEN    -4096       /* somewhere to put unwanted cursors */


#define DEFAULT_SPRITE_W   10          /* default arrow cursor */
#define DEFAULT_SPRITE_H   16

static char mouse_pointer_data[DEFAULT_SPRITE_H][DEFAULT_SPRITE_W] =
{
   { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 1, 2, 1, 0, 0, 0, 0, 0, 0, 0 },
   { 1, 2, 2, 1, 0, 0, 0, 0, 0, 0 },
   { 1, 2, 2, 2, 1, 0, 0, 0, 0, 0 },
   { 1, 2, 2, 2, 2, 1, 0, 0, 0, 0 },
   { 1, 2, 2, 2, 2, 2, 1, 0, 0, 0 },
   { 1, 2, 2, 2, 2, 2, 2, 1, 0, 0 },
   { 1, 2, 2, 2, 2, 2, 2, 2, 1, 0 },
   { 1, 2, 2, 2, 2, 2, 2, 2, 2, 1 },
   { 1, 2, 2, 2, 2, 2, 1, 1, 1, 0 },
   { 1, 2, 2, 1, 2, 2, 1, 0, 0, 0 },
   { 1, 2, 1, 0, 1, 2, 2, 1, 0, 0 },
   { 0, 1, 0, 0, 1, 2, 2, 1, 0, 0 },
   { 0, 0, 0, 0, 0, 1, 2, 2, 1, 0 },
   { 0, 0, 0, 0, 0, 1, 2, 2, 1, 0 },
   { 0, 0, 0, 0, 0, 0, 1, 1, 0, 0 }
};


BITMAP *_mouse_pointer = NULL;         /* default mouse pointer */

BITMAP *mouse_sprite = NULL;	       /* current mouse pointer */

BITMAP *_mouse_screen = NULL;          /* where to draw the pointer */

static int got_hw_cursor = FALSE;      /* hardware pointer available? */
static int hw_cursor_dirty = FALSE;    /* need to set a new pointer? */

static int mx, my;                     /* previous mouse position */
static BITMAP *ms = NULL;              /* previous screen data */
static BITMAP *mtemp = NULL;           /* double-buffer drawing area */

#define SCARED_SIZE   16               /* for unscare_mouse() */

static BITMAP *scared_screen[SCARED_SIZE]; 
static int scared_freeze[SCARED_SIZE]; 
static int scared_size = 0;

static int mouse_polled = FALSE;       /* are we in polling mode? */

static int mouse_semaphore = FALSE;    /* reentrant interrupt? */



/* draw_mouse_doublebuffer:
 *  Eliminates mouse-cursor flicker by using an off-screen buffer for
 *  updating the cursor, and blitting only the final screen image.
 *  newx and newy contain the new cursor position, and mx and my are 
 *  assumed to contain previous cursor pos. This routine is called if 
 *  mouse cursor is to be erased and redrawn, and the two position overlap.
 */
static void draw_mouse_doublebuffer(int newx, int newy) 
{
   int x1, y1, w, h;

   /* grab bit of screen containing where we are and where we'll be */
   x1 = MIN(mx, newx) - mouse_x_focus;
   y1 = MIN(my, newy) - mouse_y_focus;

   /* get width of area */
   w = MAX(mx, newx) - x1 + mouse_sprite->w+1;
   h = MAX(my, newy) - y1 + mouse_sprite->h+1;

   /* make new co-ords relative to 'mtemp' bitmap co-ords */
   newx -= mouse_x_focus+x1;
   newy -= mouse_y_focus+y1;

   /* save screen image in 'mtemp' */
   blit(_mouse_screen, mtemp, x1, y1, 0, 0, w, h);

   /* blit saved image in 'ms' to corect place in this buffer */
   blit(ms, mtemp, 0, 0, mx-mouse_x_focus-x1, my-mouse_y_focus-y1,
			 mouse_sprite->w, mouse_sprite->h);

   /* draw mouse at correct place in 'mtemp' */
   blit(mtemp, ms, newx, newy, 0, 0, mouse_sprite->w, mouse_sprite->h);
   draw_sprite(mtemp, mouse_sprite, newx, newy);

   /* blit 'mtemp' to screen */
   blit(mtemp, _mouse_screen, 0, 0, x1, y1, w, h);
}

END_OF_STATIC_FUNCTION(draw_mouse_doublebuffer);



/* draw_mouse:
 *  Mouse pointer drawing routine. If remove is set, deletes the old mouse
 *  pointer. If add is set, draws a new one.
 */
static void draw_mouse(int remove, int add)
{
   int normal_draw = (remove ^ add); 

   int newmx = _mouse_x;
   int newmy = _mouse_y;

   int cf = _mouse_screen->clip;
   int cl = _mouse_screen->cl;
   int cr = _mouse_screen->cr;
   int ct = _mouse_screen->ct;
   int cb = _mouse_screen->cb;

   _mouse_screen->clip = TRUE;
   _mouse_screen->cl = _mouse_screen->ct = 0;
   _mouse_screen->cr = _mouse_screen->w;
   _mouse_screen->cb = _mouse_screen->h;

   if (!_mouse_on) {
      newmx = MOUSE_OFFSCREEN;
      newmy = MOUSE_OFFSCREEN;
      mon = FALSE;
   }
   else
      mon = TRUE;

   if (!normal_draw) {
      if ((newmx <= mx-mouse_sprite->w) || (newmx >= mx+mouse_sprite->w) ||
	  (newmy <= my-mouse_sprite->h) || (newmy >= my+mouse_sprite->h))
	 normal_draw = 1;
   }

   if (normal_draw) {
      if (remove)
	 blit(ms, _mouse_screen, 0, 0, mx-mouse_x_focus, my-mouse_y_focus, mouse_sprite->w, mouse_sprite->h);

      if (add) {
	 blit(_mouse_screen, ms, newmx-mouse_x_focus, newmy-mouse_y_focus, 0, 0, mouse_sprite->w, mouse_sprite->h);
	 draw_sprite(_mouse_screen, mouse_sprite, newmx-mouse_x_focus, newmy-mouse_y_focus);
      }
   }
   else
      draw_mouse_doublebuffer(newmx, newmy);

   mx = newmx;
   my = newmy;

   _mouse_screen->clip = cf;
   _mouse_screen->cl = cl;
   _mouse_screen->cr = cr;
   _mouse_screen->ct = ct;
   _mouse_screen->cb = cb;
}

END_OF_STATIC_FUNCTION(draw_mouse);



/* update_mouse:
 *  Worker function to update the mouse position variables with new values.
 */
static void update_mouse(void)
{
   int x, y, z, b, flags = 0;

   if (freeze_mouse_flag) {
      x = mx;
      y = my;
   }
   else {
      x = _mouse_x;
      y = _mouse_y;
   }

   z = _mouse_z;
   b = _mouse_b;

   if (emulate_three) {
      if ((b & 3) == 3)
	 b = 4;
   }

   if ((mouse_x != x) || 
       (mouse_y != y) ||
       (mouse_z != z) ||
       (mouse_b != b)) {

      if (mouse_callback) {
	 if ((mouse_x != x) || (mouse_y != y))
	    flags |= MOUSE_FLAG_MOVE;

	 if (mouse_z != z)
	    flags |= MOUSE_FLAG_MOVE_Z;

	 if ((b & 1) && !(mouse_b & 1))
	    flags |= MOUSE_FLAG_LEFT_DOWN;
	 else if (!(b & 1) && (mouse_b & 1))
	    flags |= MOUSE_FLAG_LEFT_UP;

	 if ((b & 2) && !(mouse_b & 2))
	    flags |= MOUSE_FLAG_RIGHT_DOWN;
	 else if (!(b & 2) && (mouse_b & 2))
	    flags |= MOUSE_FLAG_RIGHT_UP;

	 if ((b & 4) && !(mouse_b & 4))
	    flags |= MOUSE_FLAG_MIDDLE_DOWN;
	 else if (!(b & 4) && (mouse_b & 4))
	    flags |= MOUSE_FLAG_MIDDLE_UP;

	 mouse_x = x;
	 mouse_y = y;
	 mouse_z = z;
	 mouse_b = b;
	 mouse_pos = ((x & 0xFFFF) << 16) | (y & 0xFFFF);

	 mouse_callback(flags);
      }
      else {
	 mouse_x = x;
	 mouse_y = y;
	 mouse_z = z;
	 mouse_b = b;
	 mouse_pos = ((x & 0xFFFF) << 16) | (y & 0xFFFF);
      }
   }
}

END_OF_STATIC_FUNCTION(update_mouse);



/* mouse_move:
 *  Timer interrupt handler for redrawing the mouse pointer.
 */
static void mouse_move(void)
{
   if (mouse_semaphore)
      return;

   mouse_semaphore = TRUE;

   /* periodic poll */
   if (mouse_driver->timer_poll) {
      mouse_driver->timer_poll();
      if (!mouse_polled)
	 update_mouse();
   }

   /* redraw pointer */
   if ((!freeze_mouse_flag) && (_mouse_screen) && ((mx != _mouse_x) || (my != _mouse_y) || (mon != _mouse_on))) {
      acquire_bitmap(_mouse_screen);

      if (gfx_capabilities & GFX_HW_CURSOR) {
	 if (_mouse_on) {
	    gfx_driver->move_mouse(mx=_mouse_x, my=_mouse_y);
	    mon = TRUE;
	 }
	 else {
	    gfx_driver->move_mouse(mx=MOUSE_OFFSCREEN, my=MOUSE_OFFSCREEN);
	    mon = FALSE;
	 }
      }
      else {
	 /* bodge to avoid using non legacy 386 asm code inside a timer handler */
	 int old_capabilities = cpu_capabilities;
	 cpu_capabilities = 0;

	 draw_mouse(TRUE, TRUE);

	 cpu_capabilities = old_capabilities;
      }

      release_bitmap(_mouse_screen);
   }

   mouse_semaphore = FALSE;
}

END_OF_STATIC_FUNCTION(mouse_move);



/* _handle_mouse_input:
 *  Callback for an asynchronous driver to tell us when it has changed the
 *  position.
 */
void _handle_mouse_input(void)
{
   if (!mouse_polled)
      update_mouse();
}

END_OF_FUNCTION(_handle_mouse_input);



/* create_mouse_pointer:
 *  Creates the default arrow mouse sprite using the current color depth
 *  and palette.
 */
static BITMAP *create_mouse_pointer(void)
{
   BITMAP *bmp;
   int x, y;
   int col;
   
   bmp = create_bitmap(DEFAULT_SPRITE_W, DEFAULT_SPRITE_H);

   for (y=0; y<DEFAULT_SPRITE_H; y++) {
      for (x=0; x<DEFAULT_SPRITE_W; x++) {
	 switch (mouse_pointer_data[y][x]) {
	    case 1:  col = makecol(255, 255, 255);  break;
	    case 2:  col = makecol(0, 0, 0);        break;
	    default: col = bmp->vtable->mask_color; break;
	 }
	 putpixel(bmp, x, y, col);
      }
   }

   return bmp;
}



/* set_mouse_sprite:
 *  Sets the sprite to be used for the mouse pointer. If the sprite is
 *  NULL, restores the default arrow.
 */
void set_mouse_sprite(struct BITMAP *sprite)
{
   BITMAP *old_mouse_screen = _mouse_screen;

   if (!mouse_driver)
      return;

   if (_mouse_screen)
      show_mouse(NULL);

   if (sprite)
      mouse_sprite = sprite;
   else {
      if (_mouse_pointer)
	 destroy_bitmap(_mouse_pointer);
      _mouse_pointer = create_mouse_pointer();
      mouse_sprite = _mouse_pointer;
   }

   lock_bitmap((struct BITMAP*)mouse_sprite);

   /* make sure the ms bitmap is big enough */
   if ((!ms) || (ms->w < mouse_sprite->w) || (ms->h < mouse_sprite->h) ||
       (bitmap_color_depth(mouse_sprite) != bitmap_color_depth(ms))) {
      if (ms) {
	 destroy_bitmap(ms);
	 destroy_bitmap(mtemp);
      }

      ms = create_bitmap(mouse_sprite->w, mouse_sprite->h);
      lock_bitmap(ms);

      mtemp = create_bitmap(mouse_sprite->w*2, mouse_sprite->h*2);
      lock_bitmap(mtemp);
   }

   mouse_x_focus = 1;
   mouse_y_focus = 1;

   hw_cursor_dirty = TRUE;

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_sprite_focus:
 *  Sets co-ordinate (x, y) in the sprite to be the mouse location.
 *  Call after set_mouse_sprite(). Doesn't redraw the sprite.
 */
void set_mouse_sprite_focus(int x, int y) 
{
   if (!mouse_driver)
      return;

   mouse_x_focus = x;
   mouse_y_focus = y;

   hw_cursor_dirty = TRUE;
}



/* show_mouse:
 *  Tells Allegro to display a mouse pointer. This only works when the timer 
 *  module is active. The mouse pointer will be drawn onto the bitmap bmp, 
 *  which should normally be the hardware screen. To turn off the mouse 
 *  pointer, which you must do before you draw anything onto the screen, call 
 *  show_mouse(NULL). If you forget to turn off the mouse pointer when 
 *  drawing something, the SVGA bank switching code will become confused and 
 *  will produce garbage all over the screen.
 */
void show_mouse(BITMAP *bmp)
{
   if (!mouse_driver)
      return;

   remove_int(mouse_move);

   if (_mouse_screen) {
      acquire_bitmap(_mouse_screen);

      if (gfx_capabilities & GFX_HW_CURSOR) {
	 gfx_driver->hide_mouse();
	 gfx_capabilities &= ~GFX_HW_CURSOR;
      }
      else
	 draw_mouse(TRUE, FALSE);

      release_bitmap(_mouse_screen);
   }

   _mouse_screen = bmp;

   if (bmp) {
      acquire_bitmap(_mouse_screen);

      if (hw_cursor_dirty) {
	 got_hw_cursor = FALSE;

	 if ((gfx_driver) && (gfx_driver->set_mouse_sprite) && (!_dispsw_status))
	    if (gfx_driver->set_mouse_sprite(mouse_sprite, mouse_x_focus, mouse_y_focus) == 0)
	       got_hw_cursor = TRUE;

	 hw_cursor_dirty = FALSE;
      }

      if ((got_hw_cursor) && (bmp->vtable == &_screen_vtable))
	 if (gfx_driver->show_mouse(bmp, mx=mouse_x, my=mouse_y) == 0)
	    gfx_capabilities |= GFX_HW_CURSOR;

      if (!(gfx_capabilities & GFX_HW_CURSOR))
	 draw_mouse(FALSE, TRUE);

      release_bitmap(_mouse_screen);

      install_int(mouse_move, 20);
   }
   else {
      if (mouse_driver->timer_poll) 
	 install_int(mouse_move, 20);
   }
}



/* scare_mouse:
 *  Removes the mouse pointer prior to a drawing operation, if that is
 *  required (ie. noop if the mouse is on a memory bitmap, or a hardware
 *  cursor is in use). This operation can later be reversed by calling
 *  unscare_mouse().
 */
void scare_mouse(void)
{
   if (!mouse_driver)
      return;

   if ((is_same_bitmap(screen, _mouse_screen)) && (!(gfx_capabilities & GFX_HW_CURSOR))) {
      if (scared_size < SCARED_SIZE) {
	 scared_screen[scared_size] = _mouse_screen;
	 scared_freeze[scared_size] = FALSE;
      }
      show_mouse(NULL);
   }
   else {
      if (scared_size < SCARED_SIZE) {
	 scared_screen[scared_size] = NULL;
	 scared_freeze[scared_size] = FALSE;
      }
   }

   scared_size++;
}



/* scare_mouse_area:
 *  Removes the mouse pointer prior to a drawing operation, if that is
 *  required (ie. noop if the mouse is on a memory bitmap, or a hardware
 *  cursor is in use, or the mouse lies outside of the specified bounds
 *  (in this last case, the mouse is frozen)). This operation can later
 *  be reversed by calling unscare_mouse().
 */
void scare_mouse_area(int x, int y, int w, int h)
{
   int was_frozen;

   if (!mouse_driver)
      return;

   if ((is_same_bitmap(screen, _mouse_screen)) && (!(gfx_capabilities & GFX_HW_CURSOR))) {
      was_frozen = freeze_mouse_flag;
      freeze_mouse_flag = TRUE;

      if ((mouse_x - mouse_x_focus < x + w) &&
	  (mouse_y - mouse_y_focus < y + h) &&
	  (mouse_x - mouse_x_focus + mouse_sprite->w >= x) &&
	  (mouse_y - mouse_y_focus + mouse_sprite->h >= y)) {

	 if (scared_size < SCARED_SIZE) {
	    scared_screen[scared_size] = _mouse_screen;
	    scared_freeze[scared_size] = FALSE;
	 }

	 freeze_mouse_flag = was_frozen;
	 show_mouse(NULL);
      }
      else {
	 if (scared_size < SCARED_SIZE) {
	    scared_screen[scared_size] = NULL;

	    if (was_frozen) {
	       scared_freeze[scared_size] = FALSE;
	       freeze_mouse_flag = was_frozen;
	    }
	    else
	       scared_freeze[scared_size] = TRUE;
	 }
      }
   }
   else {
      if (scared_size < SCARED_SIZE) {
	 scared_screen[scared_size] = NULL;
	 scared_freeze[scared_size] = FALSE;
      }
   }

   scared_size++;
}



/* unscare_mouse:
 *  Restores the original mouse state, after a call to scare_mouse() or
 *  scare_mouse_area.
 */
void unscare_mouse(void)
{
   if (!mouse_driver)
      return;

   if (scared_size > 0)
      scared_size--;

   if (scared_size < SCARED_SIZE) {
      if (scared_screen[scared_size])
	 show_mouse(scared_screen[scared_size]);

      if (scared_freeze[scared_size])
	 freeze_mouse_flag = FALSE;

      scared_screen[scared_size] = NULL;
      scared_freeze[scared_size] = FALSE;
   }
}



/* position_mouse:
 *  Moves the mouse to screen position x, y. This is safe to call even
 *  when a mouse pointer is being displayed.
 */
void position_mouse(int x, int y)
{
   BITMAP *old_mouse_screen = _mouse_screen;

   if (!mouse_driver)
      return;

   if (_mouse_screen)
      show_mouse(NULL);

   if (mouse_driver->position) {
      mouse_driver->position(x, y);
   }
   else {
      _mouse_x = x;
      _mouse_y = y;
   }

   update_mouse();

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* position_mouse_z:
 *  Sets the mouse third axis to position z.
 */
void position_mouse_z(int z)
{
   if (!mouse_driver)
      return;

   _mouse_z = z;
   update_mouse();
}



/* set_mouse_range:
 *  Sets the screen area within which the mouse can move. Pass the top left 
 *  corner and the bottom right corner (inclusive). If you don't call this 
 *  function the range defaults to (0, 0, SCREEN_W-1, SCREEN_H-1).
 */
void set_mouse_range(int x1, int y1, int x2, int y2)
{
   BITMAP *old_mouse_screen = _mouse_screen;

   if (!mouse_driver)
      return;

   if (_mouse_screen)
      show_mouse(NULL);

   if (mouse_driver->set_range)
      mouse_driver->set_range(x1, y1, x2, y2);

   update_mouse();

   if (old_mouse_screen)
      show_mouse(old_mouse_screen);
}



/* set_mouse_speed:
 *  Sets the mouse speed. Larger values of xspeed and yspeed represent 
 *  slower mouse movement: the default for both is 2.
 */
void set_mouse_speed(int xspeed, int yspeed)
{
   if ((mouse_driver) && (mouse_driver->set_speed))
      mouse_driver->set_speed(xspeed, yspeed);
}



/* get_mouse_mickeys:
 *  Measures the mickey count (how far the mouse has moved since the last
 *  call to this function).
 */
void get_mouse_mickeys(int *mickeyx, int *mickeyy)
{
   if ((mouse_driver) && (mouse_driver->get_mickeys)) {
      mouse_driver->get_mickeys(mickeyx, mickeyy);
   }
   else {
      *mickeyx = 0;
      *mickeyy = 0;
   }
}



/* poll_mouse:
 *  Polls the current mouse state, and updates the user-visible information
 *  accordingly. On some drivers this is actually required to get the
 *  input, while on others it is only present to keep compatibility with
 *  systems that do need it. So that people can test their polling code
 *  even on platforms that don't strictly require it, after this function
 *  has been called once, the entire system will switch into polling mode 
 *  and will no longer operate asynchronously even if the driver actually
 *  does support that.
 */
int poll_mouse(void)
{
   if (!mouse_driver)
      return -1;

   if (mouse_driver->poll)
      mouse_driver->poll();

   update_mouse();

   mouse_polled = TRUE;

   return 0;
}

END_OF_FUNCTION(poll_mouse);



/* mouse_needs_poll:
 *  Checks whether the current driver uses polling.
 */
int mouse_needs_poll(void)
{
   return mouse_polled;
}

END_OF_FUNCTION(mouse_needs_poll);



/* set_mouse_etc:
 *  Hook for setting up the motion range, cursor graphic, etc, called by
 *  the mouse init and whenever we change the graphics mode.
 */
static void set_mouse_etc(void)
{
   if ((!mouse_driver) || (!gfx_driver))
      return;

   if ((!_mouse_pointer) || 
       ((screen) && (_mouse_pointer) &&
	(bitmap_color_depth(_mouse_pointer) != bitmap_color_depth(screen))))
      set_mouse_sprite(NULL);
   else
      hw_cursor_dirty = TRUE;

   set_mouse_range(0, 0, SCREEN_W-1, SCREEN_H-1);
   set_mouse_speed(2, 2);
   position_mouse(SCREEN_W/2, SCREEN_H/2);
}



/* install_mouse:
 *  Installs the Allegro mouse handler. You must do this before using any
 *  other mouse functions. Return -1 if it can't find a mouse driver,
 *  otherwise the number of buttons on the mouse.
 */
int install_mouse(void)
{
   _DRIVER_INFO *driver_list;
   int num_buttons = -1;
   AL_CONST char *emulate;
   char tmp1[64], tmp2[64];
   int i;

   if (mouse_driver)
      return 0;

   LOCK_VARIABLE(mouse_driver);
   LOCK_VARIABLE(mousedrv_none);
   LOCK_VARIABLE(mouse_x);
   LOCK_VARIABLE(mouse_y);
   LOCK_VARIABLE(mouse_z);
   LOCK_VARIABLE(mouse_b);
   LOCK_VARIABLE(mouse_pos);
   LOCK_VARIABLE(_mouse_x);
   LOCK_VARIABLE(_mouse_y);
   LOCK_VARIABLE(_mouse_z);
   LOCK_VARIABLE(_mouse_b);
   LOCK_VARIABLE(_mouse_on);
   LOCK_VARIABLE(mon);
   LOCK_VARIABLE(emulate_three);
   LOCK_VARIABLE(freeze_mouse_flag);
   LOCK_VARIABLE(mouse_callback);
   LOCK_VARIABLE(mouse_x_focus);
   LOCK_VARIABLE(mouse_y_focus);
   LOCK_VARIABLE(mouse_sprite);
   LOCK_VARIABLE(mouse_pointer_data);
   LOCK_VARIABLE(_mouse_pointer);
   LOCK_VARIABLE(_mouse_screen);
   LOCK_VARIABLE(mx);
   LOCK_VARIABLE(my);
   LOCK_VARIABLE(ms);
   LOCK_VARIABLE(mtemp);
   LOCK_VARIABLE(mouse_polled);
   LOCK_VARIABLE(mouse_semaphore);
   LOCK_FUNCTION(draw_mouse_doublebuffer);
   LOCK_FUNCTION(draw_mouse);
   LOCK_FUNCTION(update_mouse);
   LOCK_FUNCTION(mouse_move);
   LOCK_FUNCTION(poll_mouse);
   LOCK_FUNCTION(mouse_needs_poll);
   LOCK_FUNCTION(_handle_mouse_input);

   if (system_driver->mouse_drivers)
      driver_list = system_driver->mouse_drivers();
   else
      driver_list = _mouse_driver_list;

   if (_mouse_type == MOUSEDRV_AUTODETECT)
      _mouse_type = get_config_id(uconvert_ascii("mouse", tmp1), uconvert_ascii("mouse", tmp2), MOUSEDRV_AUTODETECT);

   if (_mouse_type != MOUSEDRV_AUTODETECT) {
      for (i=0; driver_list[i].driver; i++) {
	 if (driver_list[i].id == _mouse_type) {
	    mouse_driver = driver_list[i].driver;
	    break;
	 }
      }
   }

   if (mouse_driver) {
      mouse_driver->name = mouse_driver->desc = get_config_text(mouse_driver->ascii_name);
      num_buttons = mouse_driver->init();
   } 
   else {
      for (i=0; num_buttons<0; i++) {
	 if (!driver_list[i].driver)
	    break;

	 mouse_driver = driver_list[i].driver;
	 mouse_driver->name = mouse_driver->desc = get_config_text(mouse_driver->ascii_name);
	 num_buttons = mouse_driver->init();
      }
   }

   if (num_buttons < 0) {
      mouse_driver = NULL;
      return -1;
   }

   num_buttons = get_config_int(uconvert_ascii("mouse", tmp1), uconvert_ascii("num_buttons", tmp2), num_buttons);
   emulate = get_config_string(uconvert_ascii("mouse", tmp1), uconvert_ascii("emulate_three", tmp2), NULL);

   if ((emulate) && ((i = ugetc(emulate)) != 0)) {
      if ((i == 'y') || (i == 'Y') || (i == '1'))
	 emulate_three = TRUE;
      else
	 emulate_three = FALSE;
   }
   else {
      if (num_buttons < 3)
	 emulate_three = TRUE;
      else
	 emulate_three = FALSE;
   }

   mouse_polled = (mouse_driver->poll) ? TRUE : FALSE;

   _mouse_installed = TRUE;

   set_mouse_etc();
   _add_exit_func(remove_mouse);

   if (mouse_driver->timer_poll)
      install_int(mouse_move, 20);

   return num_buttons;
}



/* remove_mouse:
 *  Removes the mouse handler. You don't normally need to call this, because
 *  allegro_exit() will do it for you.
 */
void remove_mouse(void)
{
   if (!mouse_driver)
      return;

   show_mouse(NULL);
   remove_int(mouse_move);

   mouse_driver->exit();
   mouse_driver = NULL;

   _mouse_installed = FALSE;

   mouse_x = mouse_y = _mouse_x = _mouse_y = 0;
   mouse_z = _mouse_z = 0;
   mouse_b = _mouse_b = 0;
   mouse_pos = 0;

   mouse_polled = FALSE;

   if (_mouse_pointer) {
      destroy_bitmap(_mouse_pointer);
      _mouse_pointer = NULL;
   }

   if (ms) {
      destroy_bitmap(ms);
      ms = NULL;

      destroy_bitmap(mtemp);
      mtemp = NULL;
   }

   _remove_exit_func(remove_mouse);
}



/* _mouse_constructor:
 *  Register mouse functions if this object file is linked in.
 */
#ifdef CONSTRUCTOR_FUNCTION
   CONSTRUCTOR_FUNCTION(void _mouse_constructor());
#endif

static struct _AL_LINKER_MOUSE mouse_linker = {
   set_mouse_etc,
   show_mouse,
   &_mouse_screen
};

void _mouse_constructor(void)
{
   _al_linker_mouse = &mouse_linker;
}
