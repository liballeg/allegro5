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
 *      The core GUI routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Peter Pavlovic modified the drawing and positioning of menus.
 *
 *      Menu auto-opening added by Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include <limits.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



/* if set, the input focus follows the mouse pointer */
int gui_mouse_focus = TRUE;


/* font alignment value */
int gui_font_baseline = 0;


/* pointer to the currently active dialog and menu objects */
static DIALOG_PLAYER *active_player = NULL;
DIALOG *active_dialog = NULL;
MENU *active_menu = NULL;


/* list of currently active (initialized) dialog players */
struct al_active_player {
   DIALOG_PLAYER *player;
   struct al_active_player *next;
};

static struct al_active_player *first_active_player = 0;



/* hook function for reading the mouse x position */
static int default_mouse_x(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_x;
}

END_OF_STATIC_FUNCTION(default_mouse_x);



/* hook function for reading the mouse y position */
static int default_mouse_y(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_y;
}

END_OF_STATIC_FUNCTION(default_mouse_y);



/* hook function for reading the mouse z position */
static int default_mouse_z(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_z;
}

END_OF_STATIC_FUNCTION(default_mouse_z);



/* hook function for reading the mouse button state */
static int default_mouse_b(void)
{
   if (mouse_needs_poll())
      poll_mouse();

   return mouse_b;
}

END_OF_STATIC_FUNCTION(default_mouse_b);



/* hook functions for reading the mouse state */
int (*gui_mouse_x)(void) = default_mouse_x;
int (*gui_mouse_y)(void) = default_mouse_y;
int (*gui_mouse_z)(void) = default_mouse_z;
int (*gui_mouse_b)(void) = default_mouse_b;


/* timer to handle menu auto-opening */
static int gui_timer;

static int gui_menu_opening_delay;


/* Checking for double clicks is complicated. The user could release the
 * mouse button at almost any point, and I might miss it if I am doing some
 * other processing at the same time (eg. sending the single-click message).
 * To get around this I install a timer routine to do the checking for me,
 * so it will notice double clicks whenever they happen.
 */

static volatile int dclick_status, dclick_time;

static int gui_install_count = 0;
static int gui_install_time = 0;

#define DCLICK_START      0
#define DCLICK_RELEASE    1
#define DCLICK_AGAIN      2
#define DCLICK_NOT        3



/* dclick_check:
 *  Double click checking user timer routine.
 */
static void dclick_check(void)
{
   gui_timer++;
   
   if (dclick_status==DCLICK_START) {              /* first click... */
      if (!gui_mouse_b()) {
	 dclick_status = DCLICK_RELEASE;           /* aah! released first */
	 dclick_time = 0;
	 return;
      }
   }
   else if (dclick_status==DCLICK_RELEASE) {       /* wait for second click */
      if (gui_mouse_b()) {
	 dclick_status = DCLICK_AGAIN;             /* yes! the second click */
	 dclick_time = 0;
	 return;
      }
   }
   else
      return;

   /* timeout? */
   if (dclick_time++ > 10)
      dclick_status = DCLICK_NOT;
}

END_OF_STATIC_FUNCTION(dclick_check);



/* gui_switch_callback:
 *  Sets the dirty flags whenever a display switch occurs.
 */
static void gui_switch_callback(void)
{
   if (active_player)
      active_player->res |= D_REDRAW;
}



/* position_dialog:
 *  Moves all the objects in a dialog to the specified position.
 */
void position_dialog(DIALOG *dialog, int x, int y)
{
   int min_x = INT_MAX;
   int min_y = INT_MAX;
   int xc, yc;
   int c;

   /* locate the upper-left corner */
   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x < min_x) 
	 min_x = dialog[c].x;

      if (dialog[c].y < min_y) 
	 min_y = dialog[c].y;
   }

   /* move the dialog */
   xc = min_x - x;
   yc = min_y - y;

   for (c=0; dialog[c].proc; c++) {
      dialog[c].x -= xc;
      dialog[c].y -= yc;
   }
}



/* centre_dialog:
 *  Moves all the objects in a dialog so that the dialog is centered in
 *  the screen.
 */
void centre_dialog(DIALOG *dialog)
{
   int min_x = INT_MAX;
   int min_y = INT_MAX;
   int max_x = INT_MIN;
   int max_y = INT_MIN;
   int xc, yc;
   int c;

   /* find the extents of the dialog (done in many loops due to a bug
    * in MSVC that prevents the more sensible version from working)
    */ 
   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x < min_x)
	 min_x = dialog[c].x;
   }

   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].y < min_y)
	 min_y = dialog[c].y;
   }

   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].x + dialog[c].w > max_x)
	 max_x = dialog[c].x + dialog[c].w;
   }

   for (c=0; dialog[c].proc; c++) {
      if (dialog[c].y + dialog[c].h > max_y)
	 max_y = dialog[c].y + dialog[c].h;
   }

   /* how much to move by? */
   xc = (SCREEN_W - (max_x - min_x)) / 2 - min_x;
   yc = (SCREEN_H - (max_y - min_y)) / 2 - min_y;

   /* move it */
   for (c=0; dialog[c].proc; c++) {
      dialog[c].x += xc;
      dialog[c].y += yc;
   }
}



/* set_dialog_color:
 *  Sets the foreground and background colors of all the objects in a dialog.
 */
void set_dialog_color(DIALOG *dialog, int fg, int bg)
{
   int c;

   for (c=0; dialog[c].proc; c++) {
      dialog[c].fg = fg;
      dialog[c].bg = bg;
   }
}



/* find_dialog_focus:
 *  Searches the dialog for the object which has the input focus, returning
 *  its index, or -1 if the focus is not set. Useful after do_dialog() exits
 *  if you need to know which object was selected.
 */
int find_dialog_focus(DIALOG *dialog)
{
   int c;

   for (c=0; dialog[c].proc; c++)
      if (dialog[c].flags & D_GOTFOCUS)
	 return c;

   return -1;
}



/* dialog_message:
 *  Sends a message to all the objects in a dialog. If any of the objects
 *  return values other than D_O_K, returns the value and sets obj to the 
 *  object which produced it.
 */
int dialog_message(DIALOG *dialog, int msg, int c, int *obj)
{
   int count, res, r, force;

   if (msg == MSG_DRAW) {
      scare_mouse();
      acquire_screen();
   }

   force = ((msg == MSG_START) || (msg == MSG_END));

   res = D_O_K;

   for (count=0; dialog[count].proc; count++) { 
      if ((force) || (!(dialog[count].flags & D_HIDDEN))) {
	 r = dialog[count].proc(msg, &dialog[count], c);

	 if (r & D_REDRAWME) {
	    dialog[count].flags |= D_DIRTY;
	    r &= ~D_REDRAWME;
	 }

	 if (r != D_O_K) {
	    res |= r;
	    *obj = count;
	 }

	 if ((msg == MSG_IDLE) && (dialog[count].flags & (D_DIRTY | D_HIDDEN)) == D_DIRTY) {
	    dialog[count].flags &= ~D_DIRTY;
	    scare_mouse();
	    object_message(dialog+count, MSG_DRAW, 0);
	    unscare_mouse();
	 }
      }
   }

   if (msg == MSG_DRAW) {
      release_screen();
      unscare_mouse();
   }

   return res;
}



/* broadcast_dialog_message:
 *  Broadcasts a message to all the objects in the active dialog. If any of
 *  the dialog procedures return values other than D_O_K, it returns that
 *  value.
 */
int broadcast_dialog_message(int msg, int c)
{
   int nowhere;

   if (active_dialog)
      return dialog_message(active_dialog, msg, c, &nowhere);
   else
      return D_O_K;
}



/* find_mouse_object:
 *  Finds which object the mouse is on top of.
 */
static int find_mouse_object(DIALOG *d)
{
   int mouse_object = -1;
   int c;

   for (c=0; d[c].proc; c++)
      if ((gui_mouse_x() >= d[c].x) && (gui_mouse_y() >= d[c].y) &&
	  (gui_mouse_x() < d[c].x + d[c].w) && (gui_mouse_y() < d[c].y + d[c].h) &&
	  (!(d[c].flags & (D_HIDDEN | D_DISABLED))))
	 mouse_object = c;

   return mouse_object;
}



/* offer_focus:
 *  Offers the input focus to a particular object.
 */
int offer_focus(DIALOG *d, int obj, int *focus_obj, int force)
{
   int res = D_O_K;

   if ((obj == *focus_obj) || 
       ((obj >= 0) && (d[obj].flags & (D_HIDDEN | D_DISABLED))))
      return D_O_K;

   /* check if object wants the focus */
   if (obj >= 0) {
      res = object_message(d+obj, MSG_WANTFOCUS, 0);
      if (res & D_WANTFOCUS)
	 res ^= D_WANTFOCUS;
      else
	 obj = -1;
   }

   if ((obj >= 0) || (force)) {
      /* take focus away from old object */
      if (*focus_obj >= 0) {
	 res |= object_message(d+*focus_obj, MSG_LOSTFOCUS, 0);
	 if (res & D_WANTFOCUS) {
	    if (obj < 0)
	       return D_O_K;
	    else
	       res &= ~D_WANTFOCUS;
	 }
	 d[*focus_obj].flags &= ~D_GOTFOCUS;
	 scare_mouse();
	 res |= object_message(d+*focus_obj, MSG_DRAW, 0);
	 unscare_mouse();
      }

      *focus_obj = obj;

      /* give focus to new object */
      if (obj >= 0) {
	 scare_mouse();
	 d[obj].flags |= D_GOTFOCUS;
	 res |= object_message(d+obj, MSG_GOTFOCUS, 0);
	 res |= object_message(d+obj, MSG_DRAW, 0);
	 unscare_mouse();
      }
   }

   return res;
}



#define MAX_OBJECTS     512

typedef struct OBJ_LIST
{
   int index;
   int diff;
} OBJ_LIST;



/* obj_list_cmp:
 *  Callback function for qsort().
 */
static int obj_list_cmp(AL_CONST void *e1, AL_CONST void *e2)
{
   return (((OBJ_LIST *)e1)->diff - ((OBJ_LIST *)e2)->diff);
}



/* cmp_tab:
 *  Comparison function for tab key movement.
 */
static int cmp_tab(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (int)d2 - (int)d1;

   if (ret < 0)
      ret += 0x10000;

   return ret;
}



/* cmp_shift_tab:
 *  Comparison function for shift+tab key movement.
 */
static int cmp_shift_tab(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (int)d1 - (int)d2;

   if (ret < 0)
      ret += 0x10000;

   return ret;
}



/* cmp_right:
 *  Comparison function for right arrow key movement.
 */
static int cmp_right(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (d2->x - d1->x) + ABS(d1->y - d2->y) * 8;

   if (d1->x >= d2->x)
      ret += 0x10000;

   return ret;
}



/* cmp_left:
 *  Comparison function for left arrow key movement.
 */
static int cmp_left(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (d1->x - d2->x) + ABS(d1->y - d2->y) * 8;

   if (d1->x <= d2->x)
      ret += 0x10000;

   return ret;
}



/* cmp_down:
 *  Comparison function for down arrow key movement.
 */
static int cmp_down(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (d2->y - d1->y) + ABS(d1->x - d2->x) * 8;

   if (d1->y >= d2->y)
      ret += 0x10000;

   return ret;
}



/* cmp_up:
 *  Comparison function for up arrow key movement.
 */
static int cmp_up(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2)
{
   int ret = (d1->y - d2->y) + ABS(d1->x - d2->x) * 8;

   if (d1->y <= d2->y)
      ret += 0x10000;

   return ret;
}



/* move_focus:
 *  Handles arrow key and tab movement through a dialog, deciding which
 *  object should be given the input focus.
 */
static int move_focus(DIALOG *d, int ascii, int scan, int *focus_obj)
{
   int (*cmp)(AL_CONST DIALOG *d1, AL_CONST DIALOG *d2);
   OBJ_LIST obj[MAX_OBJECTS];
   int obj_count = 0;
   int fobj, c;
   int res = D_O_K;

   /* choose a comparison function */ 
   switch (scan) {
      case KEY_TAB:   cmp = (ascii == '\t') ? cmp_tab : cmp_shift_tab; break;
      case KEY_RIGHT: cmp = cmp_right; break;
      case KEY_LEFT:  cmp = cmp_left;  break;
      case KEY_DOWN:  cmp = cmp_down;  break;
      case KEY_UP:    cmp = cmp_up;    break;
      default:        return D_O_K;
   }

   /* fill temporary table */
   for (c=0; d[c].proc; c++) {
      if ((*focus_obj < 0) || (c != *focus_obj)) {
	 obj[obj_count].index = c;
	 if (*focus_obj >= 0)
	    obj[obj_count].diff = cmp(d+*focus_obj, d+c);
	 else
	    obj[obj_count].diff = c;
	 obj_count++;
	 if (obj_count >= MAX_OBJECTS)
	    break;
      }
   }

   /* sort table */
   qsort(obj, obj_count, sizeof(OBJ_LIST), obj_list_cmp);

   /* find an object to give the focus to */
   fobj = *focus_obj;
   for (c=0; c<obj_count; c++) {
      res |= offer_focus(d, obj[c].index, focus_obj, FALSE);
      if (fobj != *focus_obj)
	 break;
   } 

   return res;
}



#define MESSAGE(i, msg, c) {                       \
   r = object_message(player->dialog+i, msg, c);   \
   if (r != D_O_K) {                               \
      player->res |= r;                            \
      player->obj = i;                             \
   }                                               \
}



/* do_dialog:
 *  The basic dialog manager. The list of dialog objects should be
 *  terminated by one with a null dialog procedure. Returns the index of 
 *  the object which caused it to exit.
 */
int do_dialog(DIALOG *dialog, int focus_obj)
{
   BITMAP *mouse_screen = _mouse_screen;
   int screen_count = _gfx_mode_set_count;
   void *player;

   if (!is_same_bitmap(_mouse_screen, screen))
      show_mouse(screen);

   player = init_dialog(dialog, focus_obj);

   while (update_dialog(player))
      ;

   if (_gfx_mode_set_count == screen_count)
      show_mouse(mouse_screen);

   return shutdown_dialog(player);
}



/* popup_dialog:
 *  Like do_dialog(), but it stores the data on the screen before drawing
 *  the dialog and restores it when the dialog is closed. The screen area
 *  to be stored is calculated from the dimensions of the first object in
 *  the dialog, so all the other objects should lie within this one.
 */
int popup_dialog(DIALOG *dialog, int focus_obj)
{
   BITMAP *bmp;
   int ret;

   bmp = create_bitmap(dialog->w+1, dialog->h+1); 

   if (bmp) {
      scare_mouse();
      blit(screen, bmp, dialog->x, dialog->y, 0, 0, dialog->w+1, dialog->h+1);
      unscare_mouse();
   }
   else
      *allegro_errno = ENOMEM;

   ret = do_dialog(dialog, focus_obj);

   if (bmp) {
      scare_mouse();
      blit(bmp, screen, 0, 0, dialog->x, dialog->y, dialog->w+1, dialog->h+1);
      unscare_mouse();
      destroy_bitmap(bmp);
   }

   return ret;
}



/* init_dialog:
 *  Sets up a dialog, returning a player object that can be used with
 *  the update_dialog() and shutdown_dialog() functions.
 */
DIALOG_PLAYER *init_dialog(DIALOG *dialog, int focus_obj)
{
   DIALOG_PLAYER *player = malloc(sizeof(DIALOG_PLAYER));
   struct al_active_player *n = 0;
   char tmp1[64], tmp2[64];
   int c;

   if (!player)
      return NULL;

   /* add player to the list */
   n = malloc(sizeof(struct al_active_player));
   if (!n) {
      free (player);
      return NULL;
   }
   n->next = first_active_player;
   n->player = player;
   first_active_player = n;

   player->res = D_REDRAW;
   player->joy_on = TRUE;
   player->click_wait = FALSE;
   player->dialog = dialog;
   player->obj = -1;
   player->mouse_obj = -1;
   player->mouse_oz = gui_mouse_z();
   player->mouse_b = gui_mouse_b();

   /* set up the global  dialog pointer */
   active_player = player;
   active_dialog = dialog;

   /* set up dclick checking code */
   if (gui_install_count <= 0) {
      LOCK_VARIABLE(gui_timer);
      LOCK_VARIABLE(dclick_status);
      LOCK_VARIABLE(dclick_time);
      LOCK_VARIABLE(gui_mouse_x);
      LOCK_VARIABLE(gui_mouse_y);
      LOCK_VARIABLE(gui_mouse_z);
      LOCK_VARIABLE(gui_mouse_b);
      LOCK_FUNCTION(default_mouse_x);
      LOCK_FUNCTION(default_mouse_y);
      LOCK_FUNCTION(default_mouse_z);
      LOCK_FUNCTION(default_mouse_b);
      LOCK_FUNCTION(dclick_check);

      install_int(dclick_check, 20);

      switch (get_display_switch_mode()) {
         case SWITCH_AMNESIA:
         case SWITCH_BACKAMNESIA:
            set_display_switch_callback(SWITCH_IN, gui_switch_callback);
      }

      /* gets menu auto-opening delay (in milliseconds) from config file */
      gui_menu_opening_delay = get_config_int(uconvert_ascii("system", tmp1), uconvert_ascii("menu_opening_delay", tmp2), 300);
      if (gui_menu_opening_delay >= 0) {
         /* adjust for actual timer speed */
         gui_menu_opening_delay /= 20;
      }
      else {
         /* no auto opening */
         gui_menu_opening_delay = -1;
      }
      
      gui_install_count = 1;
      gui_install_time = _allegro_count;
   }
   else
      gui_install_count++;

   /* initialise the dialog */
   set_clip(screen, 0, 0, SCREEN_W-1, SCREEN_H-1);
   player->res |= dialog_message(dialog, MSG_START, 0, &player->obj);

   player->mouse_obj = find_mouse_object(dialog);
   if (player->mouse_obj >= 0)
      dialog[player->mouse_obj].flags |= D_GOTMOUSE;

   for (c=0; dialog[c].proc; c++)
      dialog[c].flags &= ~D_GOTFOCUS;

   if (focus_obj >= 0)
      c = focus_obj;
   else
      c = player->mouse_obj;

   if ((c >= 0) && ((object_message(dialog+c, MSG_WANTFOCUS, 0)) & D_WANTFOCUS)) {
      dialog[c].flags |= D_GOTFOCUS;
      player->focus_obj = c;
   }
   else
      player->focus_obj = -1;

   return player;
}



/* check_for_redraw:
 *  Checks whether any parts of the current dialog need to be redraw.
 */
static void check_for_redraw(DIALOG_PLAYER *player)
{
   int c, r;

   /* need to draw it? */
   if (player->res & D_REDRAW) {
      player->res ^= D_REDRAW;
      player->res |= dialog_message(player->dialog, MSG_DRAW, 0, &player->obj);
   }

   /* check if any widget has to be redrawn */
   for (c=0; player->dialog[c].proc; c++) {
      if ((player->dialog[c].flags & (D_DIRTY | D_HIDDEN)) == D_DIRTY) {
	 player->dialog[c].flags &= ~D_DIRTY;
	 scare_mouse();
	 MESSAGE(c, MSG_DRAW, 0);
	 unscare_mouse();
      }
   }
}



/* update_dialog:
 *  Updates the status of a dialog object returned by init_dialog(),
 *  returning TRUE if it is still active or FALSE if it has finished.
 */
int update_dialog(DIALOG_PLAYER *player)
{
   int c, cascii, cscan, ccombo, r, ret, nowhere, z;
   int new_mouse_b;

   if (player->res & D_CLOSE)
      return FALSE;

   /* deal with mouse buttons presses and releases */
   new_mouse_b = gui_mouse_b();
   if (new_mouse_b != player->mouse_b) {
      player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, FALSE);

      if (player->mouse_obj >= 0) {
	 /* send press and release messages */
         if ((new_mouse_b & 1) && !(player->mouse_b & 1))
	    MESSAGE(player->mouse_obj, MSG_LPRESS, new_mouse_b);
         if (!(new_mouse_b & 1) && (player->mouse_b & 1))
	    MESSAGE(player->mouse_obj, MSG_LRELEASE, new_mouse_b);

         if ((new_mouse_b & 4) && !(player->mouse_b & 4))
	    MESSAGE(player->mouse_obj, MSG_MPRESS, new_mouse_b);
         if (!(new_mouse_b & 4) && (player->mouse_b & 4))
	    MESSAGE(player->mouse_obj, MSG_MRELEASE, new_mouse_b);

         if ((new_mouse_b & 2) && !(player->mouse_b & 2))
	    MESSAGE(player->mouse_obj, MSG_RPRESS, new_mouse_b);
         if (!(new_mouse_b & 2) && (player->mouse_b & 2))
	    MESSAGE(player->mouse_obj, MSG_RRELEASE, new_mouse_b);

         player->mouse_b = new_mouse_b;
      }
      else
	 dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);
   }

   /* need to reinstall the dclick and switch handlers? */
   if (gui_install_time != _allegro_count) {
      install_int(dclick_check, 20);

      switch (get_display_switch_mode()) {
         case SWITCH_AMNESIA:
         case SWITCH_BACKAMNESIA:
            set_display_switch_callback(SWITCH_IN, gui_switch_callback);
      }

      gui_install_time = _allegro_count;
   }

   /* are we dealing with a mouse click? */
   if (player->click_wait) {
      if ((ABS(player->mouse_ox-gui_mouse_x()) > 8) || 
	  (ABS(player->mouse_oy-gui_mouse_y()) > 8))
	 dclick_status = DCLICK_NOT;

      /* waiting... */
      if ((dclick_status != DCLICK_AGAIN) && (dclick_status != DCLICK_NOT)) {
	 dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);
	 check_for_redraw(player);
	 return TRUE;
      }

      player->click_wait = FALSE;

      /* double click! */
      if ((dclick_status==DCLICK_AGAIN) &&
	  (gui_mouse_x() >= player->dialog[player->mouse_obj].x) && 
	  (gui_mouse_y() >= player->dialog[player->mouse_obj].y) &&
	  (gui_mouse_x() <= player->dialog[player->mouse_obj].x + player->dialog[player->mouse_obj].w) &&
	  (gui_mouse_y() <= player->dialog[player->mouse_obj].y + player->dialog[player->mouse_obj].h)) {
	 MESSAGE(player->mouse_obj, MSG_DCLICK, 0);
      }

      goto getout;
   }

   player->res &= ~D_USED_CHAR;

   /* need to give the input focus to someone? */
   if (player->res & D_WANTFOCUS) {
      player->res ^= D_WANTFOCUS;
      player->res |= offer_focus(player->dialog, player->obj, &player->focus_obj, FALSE);
   }

   /* has mouse object changed? */
   c = find_mouse_object(player->dialog);
   if (c != player->mouse_obj) {
      if (player->mouse_obj >= 0) {
	 player->dialog[player->mouse_obj].flags &= ~D_GOTMOUSE;
	 MESSAGE(player->mouse_obj, MSG_LOSTMOUSE, 0);
      }
      if (c >= 0) {
	 player->dialog[c].flags |= D_GOTMOUSE;
	 MESSAGE(c, MSG_GOTMOUSE, 0);
      }
      player->mouse_obj = c; 

      /* move the input focus as well? */
      if ((gui_mouse_focus) && (player->mouse_obj != player->focus_obj))
	 player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, TRUE);
   }

   /* deal with mouse button clicks */
   if (new_mouse_b) {
      player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, FALSE);

      if (player->mouse_obj >= 0) {
	 dclick_time = 0;
	 dclick_status = DCLICK_START;
	 player->mouse_ox = gui_mouse_x();
	 player->mouse_oy = gui_mouse_y();

	 /* send click message */
	 MESSAGE(player->mouse_obj, MSG_CLICK, new_mouse_b);

	 if (player->res == D_O_K)
	    player->click_wait = TRUE;
      }
      else
	 player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);

      /* goto getout; */  /* to avoid an updating delay */
   }

   /* deal with mouse wheel clicks */
   z = gui_mouse_z();

   if (z != player->mouse_oz) {
      player->res |= offer_focus(player->dialog, player->mouse_obj, &player->focus_obj, FALSE);

      if (player->mouse_obj >= 0) {
	 MESSAGE(player->mouse_obj, MSG_WHEEL, z-player->mouse_oz);
      }
      else
	 player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &nowhere);

      player->mouse_oz = z;

      /* goto getout; */  /* to avoid an updating delay */
   }

   /* fake joystick input by converting it to key presses */
   if (player->joy_on)
      rest(20);

   poll_joystick();

   if (player->joy_on) {
      if ((!joy[0].stick[0].axis[0].d1) && (!joy[0].stick[0].axis[0].d2) && 
	  (!joy[0].stick[0].axis[1].d1) && (!joy[0].stick[0].axis[1].d2) &&
	  (!joy[0].button[0].b) && (!joy[0].button[1].b)) {
	 player->joy_on = FALSE;
	 rest(20);
      }
      cascii = cscan = 0;
   }
   else {
      if (joy[0].stick[0].axis[0].d1) {
	 cascii = 0;
	 cscan = KEY_LEFT;
	 player->joy_on = TRUE;
      }
      else if (joy[0].stick[0].axis[0].d2) {
	 cascii = 0;
	 cscan = KEY_RIGHT;
	 player->joy_on = TRUE;
      }
      else if (joy[0].stick[0].axis[1].d1) {
	 cascii = 0;
	 cscan = KEY_UP;
	 player->joy_on = TRUE;
      }
      else if (joy[0].stick[0].axis[1].d2) {
	 cascii = 0;
	 cscan = KEY_DOWN;
	 player->joy_on = TRUE;
      }
      else if ((joy[0].button[0].b) || (joy[0].button[1].b)) {
	 cascii = ' ';
	 cscan = KEY_SPACE;
	 player->joy_on = TRUE;
      }
      else
	 cascii = cscan = 0;
   }

   /* deal with keyboard input */
   if ((cascii) || (cscan) || (keypressed())) {
      if ((!cascii) && (!cscan))
	 cascii = ureadkey(&cscan);

      ccombo = (cscan<<8) | ((cascii <= 255) ? cascii : '^');

      /* let object deal with the key */
      if (player->focus_obj >= 0) {
	 MESSAGE(player->focus_obj, MSG_CHAR, ccombo);
	 if (player->res & D_USED_CHAR)
	    goto getout;

	 MESSAGE(player->focus_obj, MSG_UCHAR, cascii);
	 if (player->res & D_USED_CHAR)
	    goto getout;
      }

      /* keyboard shortcut? */
      for (c=0; player->dialog[c].proc; c++) {
	 if ((((cascii > 0) && (cascii <= 255) && 
	       (utolower(player->dialog[c].key) == utolower((cascii)))) ||
	      ((!cascii) && (player->dialog[c].key == (cscan<<8)))) && 
	     (!(player->dialog[c].flags & (D_HIDDEN | D_DISABLED)))) {
	    MESSAGE(c, MSG_KEY, ccombo);
	    goto getout;
	 }
      }

      /* broadcast in case any other objects want it */
      for (c=0; player->dialog[c].proc; c++) {
	 if (!(player->dialog[c].flags & (D_HIDDEN | D_DISABLED))) {
	    MESSAGE(c, MSG_XCHAR, ccombo);
	    if (player->res & D_USED_CHAR)
	       goto getout;
	 }
      }

      /* pass <CR> or <SPACE> to selected object */
      if (((cascii == '\r') || (cascii == '\n') || (cascii == ' ')) && 
	  (player->focus_obj >= 0)) {
	 MESSAGE(player->focus_obj, MSG_KEY, ccombo);
	 goto getout;
      }

      /* ESC closes dialog */
      if (cascii == 27) {
	 player->res |= D_CLOSE;
	 player->obj = -1;
	 goto getout;
      }

      /* move focus around the dialog */
      player->res |= move_focus(player->dialog, cascii, cscan, &player->focus_obj);
   }

   /* redraw? */
   check_for_redraw(player);

   /* send idle messages */
   player->res |= dialog_message(player->dialog, MSG_IDLE, 0, &player->obj);

   getout:

   ret = (!(player->res & D_CLOSE));
   player->res &= ~D_CLOSE;
   return ret;
}



/* shutdown_dialog:
 *  Destroys a dialog object returned by init_dialog(), returning the index
 *  of the object that caused it to exit.
 */
int shutdown_dialog(DIALOG_PLAYER *player)
{
   struct al_active_player *iter, *prev;
   int obj;

   /* send the finish messages */
   dialog_message(player->dialog, MSG_END, 0, &player->obj);

   /* remove the double click handler */
   gui_install_count--;

   if (gui_install_count <= 0) {
      remove_int(dclick_check);
      remove_display_switch_callback(gui_switch_callback);
   }

   if (player->mouse_obj >= 0)
      player->dialog[player->mouse_obj].flags &= ~D_GOTMOUSE;

   /* remove dialog player from the list of active players */
   for (iter = first_active_player, prev = 0; iter != 0; prev = iter, iter = iter->next) {
      if (iter->player == player) {
	 if (prev)
	    prev->next = iter->next;
	 else
	    first_active_player = iter->next;
	 free (iter);
	 break;
      }
   }

   if (first_active_player)
      active_player = first_active_player->player;
   else
      active_player = NULL;

   if (active_player)
      active_dialog = active_player->dialog;
   else
      active_dialog = NULL;

   obj = player->obj;

   free(player);

   return obj;
}



typedef struct MENU_INFO            /* information about a popup menu */
{
   MENU *menu;                      /* the menu itself */
   struct MENU_INFO *parent;        /* the parent menu, or NULL for root */
   int bar;                         /* set if it is a top level menu bar */
   int size;                        /* number of items in the menu */
   int sel;                         /* selected item */
   int x, y, w, h;                  /* screen position of the menu */
   int (*proc)();                   /* callback function */
   BITMAP *saved;                   /* saved what was underneath it */
} MENU_INFO;



void (*gui_menu_draw_menu)(int x, int y, int w, int h) = NULL;
void (*gui_menu_draw_menu_item)(MENU *m, int x, int y, int w, int h, int bar, int sel);



/* get_menu_pos:
 *  Calculates the coordinates of an object within a top level menu bar.
 */
static void get_menu_pos(MENU_INFO *m, int c, int *x, int *y, int *w)
{
   int c2;

   if (m->bar) {
      *x = m->x+1;

      for (c2=0; c2<c; c2++)
	 *x += gui_strlen(m->menu[c2].text) + 16;

      *y = m->y+1;
      *w = gui_strlen(m->menu[c].text) + 16;
   }
   else {
      *x = m->x+1;
      *y = m->y+c*(text_height(font)+4)+1;
      *w = m->w-2;
   }
}



/* draw_menu_item:
 *  Draws an item from a popup menu onto the screen.
 */
static void draw_menu_item(MENU_INFO *m, int c)
{
   int fg, bg;
   int x, y, w;
   char *buf, *tok;
   char *last;
   int my;
   int rtm;
   char tmp[16];

   get_menu_pos(m, c, &x, &y, &w);

   if (gui_menu_draw_menu_item) {
      gui_menu_draw_menu_item(&m->menu[c], x, y, w, text_height(font)+4,
			      m->bar, (c == m->sel) ? TRUE : FALSE);
      return;
   }

   if (m->menu[c].flags & D_DISABLED) {
      if (c == m->sel) {
	 fg = gui_mg_color;
	 bg = gui_fg_color;
      }
      else {
	 fg = gui_mg_color;
	 bg = gui_bg_color;
      } 
   }
   else {
      if (c == m->sel) {
	 fg = gui_bg_color;
	 bg = gui_fg_color;
      }
      else {
	 fg = gui_fg_color;
	 bg = gui_bg_color;
      } 
   }

   rectfill(screen, x, y, x+w-1, y+text_height(font)+3, bg);
   rtm = text_mode(bg);

   if (ugetc(m->menu[c].text)) {
      buf = ustrdup(m->menu[c].text);
      tok = ustrtok_r(buf, uconvert_ascii("\t", tmp), &last);

      gui_textout(screen, tok, x+8, y+1, fg, FALSE);

      tok = ustrtok_r(NULL, empty_string, &last);
      if (tok)
 	 gui_textout(screen, tok, x+w-gui_strlen(tok)-10, y+1, fg, FALSE);

      if ((m->menu[c].child) && (!m->bar)) {
         my = y + text_height(font)/2;
         hline(screen, x+w-8, my+1, x+w-4, fg);
         hline(screen, x+w-8, my+0, x+w-5, fg);
         hline(screen, x+w-8, my-1, x+w-6, fg);
         hline(screen, x+w-8, my-2, x+w-7, fg);
         putpixel(screen, x+w-8, my-3, fg);
         hline(screen, x+w-8, my+2, x+w-5, fg);
         hline(screen, x+w-8, my+3, x+w-6, fg);
         hline(screen, x+w-8, my+4, x+w-7, fg);
         putpixel(screen, x+w-8, my+5, fg);
      }

      free(buf);
   }
   else
      hline(screen, x, y+text_height(font)/2+2, x+w, fg);

   if (m->menu[c].flags & D_SELECTED) {
      line(screen, x+1, y+text_height(font)/2+1, x+3, y+text_height(font)+1, fg);
      line(screen, x+3, y+text_height(font)+1, x+6, y+2, fg);
   }

   text_mode(rtm);
}



/* draw_menu:
 *  Draws a popup menu onto the screen.
 */
static void draw_menu(MENU_INFO *m)
{
   int c;

   if (gui_menu_draw_menu)
      gui_menu_draw_menu(m->x, m->y, m->w, m->h);
   else {
      rect(screen, m->x, m->y, m->x+m->w-1, m->y+m->h-1, gui_fg_color);
      vline(screen, m->x+m->w, m->y+1, m->y+m->h, gui_fg_color);
      hline(screen, m->x+1, m->y+m->h, m->x+m->w, gui_fg_color);
   }

   for (c=0; m->menu[c].text; c++)
      draw_menu_item(m, c);
}



/* menu_mouse_object:
 *  Returns the index of the object the mouse is currently on top of.
 */
static int menu_mouse_object(MENU_INFO *m)
{
   int c;
   int x, y, w;

   for (c=0; c<m->size; c++) {
      get_menu_pos(m, c, &x, &y, &w);

      if ((gui_mouse_x() >= x) && (gui_mouse_x() < x+w) &&
	  (gui_mouse_y() >= y) && (gui_mouse_y() < y+(text_height(font)+4)))
	 return (ugetc(m->menu[c].text)) ? c : -1;
   }

   return -1;
}



/* mouse_in_single_menu:
 *  Checks if the mouse is inside a single menu.
 */
static INLINE int mouse_in_single_menu(MENU_INFO *m)
{
   if ((gui_mouse_x() >= m->x) && (gui_mouse_x() < m->x+m->w) &&
       (gui_mouse_y() >= m->y) && (gui_mouse_y() < m->y+m->h))
      return TRUE;
   else
      return FALSE;
}



/* mouse_in_parent_menu:
 *  Recursively checks if the mouse is inside a menu (or any of its parents)
 *  and simultaneously not on the selected item of the menu.
 */
static int mouse_in_parent_menu(MENU_INFO *m) 
{
   int c;

   if (!m)
      return FALSE;

   c = menu_mouse_object(m);
   if ((c >= 0) && (c != m->sel))
      return TRUE;

   return mouse_in_parent_menu(m->parent);
}



/* fill_menu_info:
 *  Fills a menu info structure when initialising a menu.
 */
static void fill_menu_info(MENU_INFO *m, MENU *menu, MENU_INFO *parent, int bar, int x, int y, int minw, int minh)
{
   char *buf, *tok, *last;
   int extra = 0;
   int c;
   int child = FALSE;
   char tmp[16];

   m->menu = menu;
   m->parent = parent;
   m->bar = bar;
   m->x = x;
   m->y = y;
   m->w = 2;
   m->h = (m->bar) ? (text_height(font)+6) : 2;
   m->proc = NULL;
   m->sel = -1;

   /* calculate size of the menu */
   for (m->size=0; m->menu[m->size].text; m->size++) {

      if ((m->menu[m->size].child) && (!m->bar))
         child = TRUE;

      if (ugetc(m->menu[m->size].text)) {
         buf = ustrdup(m->menu[m->size].text);
         tok = ustrtok_r(buf, uconvert_ascii("\t", tmp), &last);
         c = gui_strlen(tok);
      }
      else {
         buf = NULL;
         c = 0;
      }

      if (m->bar) {
	 m->w += c+16;
      }
      else {
	 m->h += text_height(font)+4;
	 m->w = MAX(m->w, c+16);
      }

      if (buf) {
	 tok = ustrtok_r(NULL, empty_string, &last);
	 if (tok) {
	    c = gui_strlen(tok);
	    extra = MAX(extra, c);
	 }
	 free(buf);
      }
   }

   if (extra)
      m->w += extra+16;

   if (child)
      m->w += 22;

   m->w = MAX(m->w, minw);
   m->h = MAX(m->h, minh);
}



/* menu_key_shortcut:
 *  Returns true if c is indicated as a keyboard shortcut by a '&' character
 *  in the specified string.
 */
static int menu_key_shortcut(int c, AL_CONST char *s)
{
   int d;

   while ((d = ugetxc(&s)) != 0) {
      if (d == '&') {
	 d = ugetc(s);
	 if ((d != '&') && (utolower(d) == utolower(c & 0xFF)))
	    return TRUE;
      }
   }

   return FALSE;
}



/* menu_alt_key:
 *  Searches a menu for keyboard shortcuts, for the alt+letter to bring
 *  up a menu.
 */
int menu_alt_key(int k, MENU *m)
{
   static unsigned char alt_table[] =
   {
      KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, 
      KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, 
      KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z
   };

   AL_CONST char *s;
   int c, d;

   if (k & 0xFF)
      return 0;

   k >>= 8;

   c = scancode_to_ascii(k);
   if (c) {
      k = c;
   } else {
      for (c=0; c<(int)sizeof(alt_table); c++) {
	 if (k == alt_table[c]) {
	    k = c + 'a';
	    break;
	 }
      }

      if (c >= (int)sizeof(alt_table))
	 return 0;
   }

   for (c=0; m[c].text; c++) {
      s = m[c].text;
      while ((d = ugetxc(&s)) != 0) {
	 if (d == '&') {
	    d = ugetc(s);
	    if ((d != '&') && (utolower(d) == utolower(k)))
	       return k;
	 }
      }
   }

   return 0;
}



/* _do_menu:
 *  The core menu control function, called by do_menu() and d_menu_proc().
 *  The navigation through the arborescence of menus can be done:
 *   - with the arrow keys,
 *   - with mouse point-and-clicks,
 *   - with mouse movements when the mouse button is being held down,
 *   - with mouse movements only if gui_menu_opening_delay is non negative.
 */
static int _do_menu(MENU *menu, MENU_INFO *parent, int bar, int x, int y, int repos, int *dret, int minw, int minh)
{
   static int auto_open = TRUE;  /* global property */
   MENU_INFO m;
   MENU_INFO *i;
   int c, c2;
   int mouse_button_was_pressed;
   int old_sel, mouse_sel;
   int child_x, child_y;
   int redraw = TRUE;
   int back_from_child = FALSE;
   int ret = -1;

   scare_mouse();

   fill_menu_info(&m, menu, parent, bar, x, y, minw, minh);

   if (repos) {
      m.x = MID(0, m.x, SCREEN_W-m.w-1);
      m.y = MID(0, m.y, SCREEN_H-m.h-1);
   }

   /* save screen under the menu */
   m.saved = create_bitmap(m.w+1, m.h+1); 

   if (m.saved)
      blit(screen, m.saved, m.x, m.y, 0, 0, m.w+1, m.h+1);
   else
      *allegro_errno = ENOMEM;

   /* setup state variables */
   m.sel = mouse_sel = menu_mouse_object(&m);
   mouse_button_was_pressed = gui_mouse_b();
   gui_timer = 0;

   unscare_mouse();

   do {                                               /* main event loop */
      yield_timeslice();

      old_sel = m.sel;

      c = menu_mouse_object(&m);

      if ((gui_mouse_b()) || (c != mouse_sel)) {
	 m.sel = mouse_sel = c;
	 auto_open = TRUE;
      }

      if (gui_mouse_b()) {                            /* button pressed? */
	 /* dismiss menu if:
	  *  - the mouse cursor is outside the menu and inside the parent menu, or
	  *  - the mouse cursor is outside the menu and the button has just been pressed
	  */
	 if (!mouse_in_single_menu(&m)) {
	    if (mouse_in_parent_menu(m.parent) || (!mouse_button_was_pressed))
	       break;
	 }

	 if ((m.sel >= 0) && (m.menu[m.sel].child))   /* bring up child menu? */
	    ret = m.sel;

	 /* don't trigger the 'select' event on button press for non menu item */
	 mouse_button_was_pressed = TRUE;

	 clear_keybuf();
      }
      else {                                          /* button not pressed */
	 /* trigger the 'select' event only on button release for non menu item */
	 if (mouse_button_was_pressed) {
	    ret = m.sel;
	    mouse_button_was_pressed = FALSE;
	 }

	 if (keypressed()) {                          /* keyboard input */
	    gui_timer = 0;
	    auto_open = FALSE;

	    c = readkey();

	    if ((c & 0xFF) == 27) {
	       ret = -1;
	       break;
	    }

	    switch (c >> 8) {

	       case KEY_LEFT:
		  if (m.parent) {
		     if (m.parent->bar) {
			simulate_keypress(KEY_LEFT<<8);
			simulate_keypress(KEY_DOWN<<8);
		     }
		     ret = -1;
		     goto getout;
		  }
		  /* fall through */

	       case KEY_UP:
		  if ((((c >> 8) == KEY_LEFT) && (m.bar)) ||
		      (((c >> 8) == KEY_UP) && (!m.bar))) {
		     c = m.sel;
		     do {
			c--;
			if (c < 0)
			   c = m.size - 1;
		     } while ((!ugetc(m.menu[c].text)) && (c != m.sel));
		     m.sel = c;
		  }
		  break;

	       case KEY_RIGHT:
		  if (((m.sel < 0) || (!m.menu[m.sel].child)) &&
		      (m.parent) && (m.parent->bar)) {
		     simulate_keypress(KEY_RIGHT<<8);
		     simulate_keypress(KEY_DOWN<<8);
		     ret = -1;
		     goto getout;
		  }
		  /* fall through */

	       case KEY_DOWN:
		  if ((m.sel >= 0) && (m.menu[m.sel].child) &&
		      ((((c >> 8) == KEY_RIGHT) && (!m.bar)) ||
		       (((c >> 8) == KEY_DOWN) && (m.bar)))) {
		     ret = m.sel;
		  }
		  else if ((((c >> 8) == KEY_RIGHT) && (m.bar)) ||
			   (((c >> 8) == KEY_DOWN) && (!m.bar))) {
		     c = m.sel;
		     do {
			c++;
			if (c >= m.size)
			   c = 0;
		     } while ((!ugetc(m.menu[c].text)) && (c != m.sel));
		     m.sel = c;
		  }
		  break;

	       case KEY_SPACE:
	       case KEY_ENTER:
		  if (m.sel >= 0)
		     ret = m.sel;
		  break;

	       default:
		  if ((!m.parent) && ((c & 0xFF) == 0))
		     c = menu_alt_key(c, m.menu);
		  for (c2=0; m.menu[c2].text; c2++) {
		     if (menu_key_shortcut(c, m.menu[c2].text)) {
			ret = m.sel = c2;
			break;
		     }
		  }
		  if (m.parent) {
		     i = m.parent;
		     for (c2=0; i->parent; c2++)
			i = i->parent;
		     c = menu_alt_key(c, i->menu);
		     if (c) {
			while (c2-- > 0)
			   simulate_keypress(27);
			simulate_keypress(c);
			ret = -1;
			goto getout;
		     }
		  }
		  break;
	    }
	 }
      }  /* end of input processing */

      if ((redraw) || (m.sel != old_sel)) {           /* selection changed? */
         gui_timer = 0;

	 scare_mouse();
	 acquire_screen();

	 if (redraw) {
	    draw_menu(&m);
	    redraw = FALSE;
	 }
	 else {
	    if (old_sel >= 0)
	       draw_menu_item(&m, old_sel);

	    if (m.sel >= 0)
	       draw_menu_item(&m, m.sel);
	 }

	 release_screen();
	 unscare_mouse();
      }

      if (auto_open &&
	  (gui_menu_opening_delay >= 0)) {            /* menu auto-opening on? */
	 if (!mouse_in_single_menu(&m)) {
	    if (mouse_in_parent_menu(m.parent)) {
	       /* automatically goes back to parent */
	       ret = -2;
	       break;
	    }
	 }

         if ((mouse_sel >= 0) && (m.menu[mouse_sel].child)) {
            if (m.bar) {
               /* top level menu auto-opening if back from child */
               if (back_from_child) {
                  gui_timer = 0;
                  ret = mouse_sel;
               }
            }
            else {
               /* sub menu auto-opening if enough time has passed */
               if (gui_timer > gui_menu_opening_delay)
                  ret = mouse_sel;
            }
         }

         back_from_child = FALSE;
      }

      if (ret >= 0) {                               /* item selected? */
	 if (m.menu[ret].flags & D_DISABLED) {
	    ret = -1;  /* continue */
         }
         else if (m.menu[ret].child) {              /* child menu? */
	    if (m.bar) {
	       get_menu_pos(&m, ret, &child_x, &child_y, &c);
	       child_x += 6;
	       child_y += text_height(font) + 7;
	    }
	    else {
	       child_x = m.x+m.w - 3;
	       child_y = m.y + (text_height(font)+4)*ret + text_height(font)/4 + 1;
	    }

            /* recursively call child menu */
	    c = _do_menu(m.menu[ret].child, &m, FALSE, child_x, child_y, TRUE, NULL, 0, 0);

	    if (c < 0) {                            /* return to parent? */
	       ret = -1;
	       mouse_button_was_pressed = FALSE;
	       mouse_sel = menu_mouse_object(&m);

	       if (c == -2) {                       /* return caused by mouse movement? */
		  m.sel = mouse_sel;
		  redraw = TRUE;
		  gui_timer = 0;
		  back_from_child = TRUE;
	       }
	    }
	 }
      }

      if ((m.bar) && (!gui_mouse_b()) && (!keypressed()) && (!mouse_in_single_menu(&m)))
	 break;

   } while (ret < 0);

 getout:
   if (dret)
      *dret = 0;

   if ((!m.proc) && (ret >= 0)) {    /* callback function? */
      active_menu = &m.menu[ret];
      m.proc = active_menu->proc;
   }

   if (ret >= 0) {
      if (parent)
	 parent->proc = m.proc;
      else  {
	 if (m.proc) {
	    c = m.proc();
	    if (dret)
	       *dret = c;
	 }
      }
   }

   /* restore screen */
   if (m.saved) {
      scare_mouse();
      blit(m.saved, screen, 0, 0, m.x, m.y, m.w+1, m.h+1);
      unscare_mouse();
      destroy_bitmap(m.saved);
   }

   return ret;
}



/* do_menu:
 *  Displays and animates a popup menu at the specified screen position,
 *  returning the index of the item that was selected, or -1 if it was
 *  dismissed. If the menu crosses the edge of the screen it will be moved.
 */
int do_menu(MENU *menu, int x, int y)
{
   int ret = _do_menu(menu, NULL, FALSE, x, y, TRUE, NULL, 0, 0);

   do {
   } while (gui_mouse_b());

   return ret;
}



/* d_menu_proc:
 *  Dialog procedure for adding drop down menus to a GUI dialog. This 
 *  displays the top level menu items as a horizontal bar (eg. across the
 *  top of the screen), and pops up child menus when they are clicked.
 *  When it executes one of the menu callback routines, it passes the
 *  return value back to the dialog manager, so these can return D_O_K,
 *  D_CLOSE, D_REDRAW, etc.
 */
int d_menu_proc(int msg, DIALOG *d, int c)
{ 
   MENU_INFO m;
   int ret = D_O_K;
   int x, i;

   switch (msg) {

      case MSG_START:
	 fill_menu_info(&m, d->dp, NULL, TRUE, d->x-1, d->y-1, d->w+2, d->h+2);
	 d->w = m.w-2;
	 d->h = m.h-2;
	 break;

      case MSG_DRAW:
	 fill_menu_info(&m, d->dp, NULL, TRUE, d->x-1, d->y-1, d->w+2, d->h+2);
	 draw_menu(&m);
	 break;

      case MSG_XCHAR:
	 x = menu_alt_key(c, d->dp);
	 if (!x)
	    break;

	 ret |= D_USED_CHAR;
	 simulate_keypress(x);
	 /* fall through */

      case MSG_GOTMOUSE:
      case MSG_CLICK:
	 /* steal the mouse */
	 for (i=0; active_dialog[i].proc; i++)
	    if (active_dialog[i].flags & D_GOTMOUSE) {
	       active_dialog[i].flags &= ~D_GOTMOUSE;
	       object_message(active_dialog+i, MSG_LOSTMOUSE, 0);
	       break;
	    }

	 /* run the menu */
	 _do_menu(d->dp, NULL, TRUE, d->x-1, d->y-1, FALSE, &x, d->w+2, d->h+2);
	 ret |= x;
	 do {
	 } while (gui_mouse_b());

	 /* put the mouse */
	 i = find_mouse_object(active_dialog);
	 if ((i >= 0) && (&active_dialog[i] != d)) {
	    active_dialog[i].flags |= D_GOTMOUSE;
	    object_message(active_dialog+i, MSG_GOTMOUSE, 0);
	 }
	 break;
   }

   return ret;
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



/* alert3:
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one, two, or three buttons. The text for these buttons 
 *  is passed in b1, b2, and b3 (NULL for buttons which are not used), and
 *  the keyboard shortcuts in c1 and c2. Returns 1, 2, or 3 depending on 
 *  which button was selected.
 */
int alert3(AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, AL_CONST char *b3, int c1, int c2, int c3)
{
   char tmp[16];
   int avg_w, avg_h;
   int len1, len2, len3;
   int maxlen = 0;
   int buttons = 0;
   int b[3];
   int c;

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

   c = popup_dialog(alert_dialog, A_B1);

   if (c == A_B1)
      return 1;
   else if (c == A_B2)
      return 2;
   else
      return 3;
}



/* alert:
 *  Displays a simple alert box, containing three lines of text (s1-s3),
 *  and with either one or two buttons. The text for these buttons is passed
 *  in b1 and b2 (b2 may be null), and the keyboard shortcuts in c1 and c2.
 *  Returns 1 or 2 depending on which button was selected.
 */
int alert(AL_CONST char *s1, AL_CONST char *s2, AL_CONST char *s3, AL_CONST char *b1, AL_CONST char *b2, int c1, int c2)
{
   int ret;

   ret = alert3(s1, s2, s3, b1, b2, NULL, c1, c2, 0);

   if (ret > 2)
      ret = 2;

   return ret;
}

