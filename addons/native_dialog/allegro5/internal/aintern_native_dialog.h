#ifndef __al_included_allegro_aintern_native_dialog_h
#define __al_included_allegro_aintern_native_dialog_h

#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_native_dialog_cfg.h"

typedef struct ALLEGRO_NATIVE_DIALOG ALLEGRO_NATIVE_DIALOG;

/* We could use different structs for the different dialogs. But why
 * bother.
 */
struct ALLEGRO_NATIVE_DIALOG
{
   ALLEGRO_USTR *title;
   int flags;

   /* Only used by file chooser. */
   ALLEGRO_PATH *fc_initial_path;
   size_t fc_path_count;
   ALLEGRO_PATH **fc_paths;
   ALLEGRO_USTR *fc_patterns;

   /* Only used by message box. */
   ALLEGRO_USTR *mb_heading;
   ALLEGRO_USTR *mb_text;
   ALLEGRO_USTR *mb_buttons;
   int mb_pressed_button;

   /* Only used by text log. */
   ALLEGRO_THREAD *tl_thread;
   ALLEGRO_COND *tl_text_cond;
   ALLEGRO_MUTEX *tl_text_mutex;
   ALLEGRO_USTR *tl_pending_text;
   bool tl_init_error;
   bool tl_done;
   bool tl_have_pending;
   ALLEGRO_EVENT_SOURCE tl_events;
   void *tl_textview;

   /* Only used by platform implementations. */
   bool is_active;
   void *window;
   void *async_queue;
};

extern bool _al_init_native_dialog_addon(void);
extern void _al_shutdown_native_dialog_addon(void);
extern bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd);
extern int _al_show_native_message_box(ALLEGRO_DISPLAY *display,
   ALLEGRO_NATIVE_DIALOG *fd);
extern bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);
extern void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);
extern void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog);

typedef struct ALLEGRO_MENU_ITEM ALLEGRO_MENU_ITEM;

struct ALLEGRO_MENU_ITEM
{
   ALLEGRO_MENU *parent;
   ALLEGRO_MENU *popup;
   ALLEGRO_USTR *caption;
   ALLEGRO_BITMAP *icon;
   
   uint16_t id;
   int flags;
   void *extra1, *extra2;
};

struct ALLEGRO_MENU
{
   ALLEGRO_EVENT_SOURCE es;
   ALLEGRO_DISPLAY *display;
   ALLEGRO_MENU_ITEM *parent;
   _AL_VECTOR items;

   bool is_event_source;
   bool is_popup_menu;

   void *extra1;
};

/* Platform implementation must call this when a menu item is clicked.
 *   display: the display associated with the menu
 *   id:      the user supplied menu id
 *
 *  The function will find the appropriate ALLEGRO_MENU and emit the event.
 */
extern bool _al_emit_menu_event(ALLEGRO_DISPLAY *display, uint16_t id);

extern bool _al_walk_over_menu(ALLEGRO_MENU *menu, bool (*proc)
   (ALLEGRO_MENU *menu, ALLEGRO_MENU_ITEM *item, int index, void *extra),
   void *extra);
ALLEGRO_MENU *_al_find_parent_menu_by_id(ALLEGRO_DISPLAY *display, uint16_t id);
   
/* Platform Specific Functions
 * ---------------------------
 * Each of these should return true if successful. If at all possible, they
 * should all be implemented and always return true (if menus are implemented
 * at all). All of the parameters will be valid.
 */

extern bool _al_init_menu(ALLEGRO_MENU *menu);
extern bool _al_init_popup_menu(ALLEGRO_MENU *menu);
extern bool _al_destroy_menu(ALLEGRO_MENU *menu);

/* The "int i" parameter represents the indexed location of the item in the
 * item->parent->items vector. This should map up identically to what is displayed
 * within the platform menu... However, if there are silent entries (e.g., on OS X),
 * the index may not represent what is seen on screen. If such discrepencies exist,
 * then the platform imlpementation must compensate accordingly. */

extern bool _al_insert_menu_item_at(ALLEGRO_MENU_ITEM *item, int i);
extern bool _al_destroy_menu_item_at(ALLEGRO_MENU_ITEM *item, int i);
extern bool _al_update_menu_item_at(ALLEGRO_MENU_ITEM *item, int i);

extern bool _al_show_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu);
extern bool _al_hide_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu);
extern bool _al_show_popup_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu);

#endif
