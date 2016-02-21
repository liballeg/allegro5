#ifndef __al_included_allegro5_allegro_native_dialog_h
#define __al_included_allegro5_allegro_native_dialog_h

#include "allegro5/allegro.h"

#ifdef __cplusplus
   extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_NATIVE_DIALOG_SRC
         #define _ALLEGRO_DIALOG_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_DIALOG_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_DIALOG_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_DIALOG_FUNC(type, name, args)      _ALLEGRO_DIALOG_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_DIALOG_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_DIALOG_FUNC(type, name, args)      extern _ALLEGRO_DIALOG_DLL type name args
#else
   #define ALLEGRO_DIALOG_FUNC      AL_FUNC
#endif

#ifdef ALLEGRO_WITH_XWINDOWS
   #define ALLEGRO_GTK_TOPLEVEL  ALLEGRO_GTK_TOPLEVEL_INTERNAL
#endif

/* Type: ALLEGRO_FILECHOOSER
 */
typedef struct ALLEGRO_FILECHOOSER ALLEGRO_FILECHOOSER;

/* Type: ALLEGRO_TEXTLOG
 */
typedef struct ALLEGRO_TEXTLOG ALLEGRO_TEXTLOG;

/* Type: ALLEGRO_MENU
 */
typedef struct ALLEGRO_MENU ALLEGRO_MENU;

/* Type: ALLEGRO_MENU_INFO
 */
typedef struct ALLEGRO_MENU_INFO {
   const char *caption;
   uint16_t id;
   int flags;
   ALLEGRO_BITMAP *icon;
} ALLEGRO_MENU_INFO;

#define ALLEGRO_MENU_SEPARATOR             { NULL,         -1, 0, NULL }
#define ALLEGRO_START_OF_MENU(caption, id) { caption "->", id, 0, NULL }
#define ALLEGRO_END_OF_MENU                { NULL,          0, 0, NULL }

ALLEGRO_DIALOG_FUNC(bool, al_init_native_dialog_addon, (void));
ALLEGRO_DIALOG_FUNC(void, al_shutdown_native_dialog_addon, (void));

ALLEGRO_DIALOG_FUNC(ALLEGRO_FILECHOOSER *, al_create_native_file_dialog, (char const *initial_path,
   char const *title, char const *patterns, int mode));
ALLEGRO_DIALOG_FUNC(bool, al_show_native_file_dialog, (ALLEGRO_DISPLAY *display, ALLEGRO_FILECHOOSER *dialog));
ALLEGRO_DIALOG_FUNC(int, al_get_native_file_dialog_count, (const ALLEGRO_FILECHOOSER *dialog));
ALLEGRO_DIALOG_FUNC(const char *, al_get_native_file_dialog_path, (const ALLEGRO_FILECHOOSER *dialog,
   size_t index));
ALLEGRO_DIALOG_FUNC(void, al_destroy_native_file_dialog, (ALLEGRO_FILECHOOSER *dialog));

ALLEGRO_DIALOG_FUNC(int, al_show_native_message_box, (ALLEGRO_DISPLAY *display, char const *title,
   char const *heading, char const *text, char const *buttons, int flags));

ALLEGRO_DIALOG_FUNC(ALLEGRO_TEXTLOG *, al_open_native_text_log, (char const *title, int flags));
ALLEGRO_DIALOG_FUNC(void, al_close_native_text_log, (ALLEGRO_TEXTLOG *textlog));
ALLEGRO_DIALOG_FUNC(void, al_append_native_text_log, (ALLEGRO_TEXTLOG *textlog, char const *format, ...));
ALLEGRO_DIALOG_FUNC(ALLEGRO_EVENT_SOURCE *, al_get_native_text_log_event_source, (ALLEGRO_TEXTLOG *textlog));

/* creating/modifying menus */
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_create_menu, (void));
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_create_popup_menu, (void));
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_build_menu, (ALLEGRO_MENU_INFO *info));
ALLEGRO_DIALOG_FUNC(int, al_append_menu_item, (ALLEGRO_MENU *parent, char const *title, uint16_t id, int flags,
   ALLEGRO_BITMAP *icon, ALLEGRO_MENU *submenu));
ALLEGRO_DIALOG_FUNC(int, al_insert_menu_item, (ALLEGRO_MENU *parent, int pos, char const *title, uint16_t id,
   int flags, ALLEGRO_BITMAP *icon, ALLEGRO_MENU *submenu));
ALLEGRO_DIALOG_FUNC(bool, al_remove_menu_item, (ALLEGRO_MENU *menu, int pos));
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_clone_menu, (ALLEGRO_MENU *menu));
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_clone_menu_for_popup, (ALLEGRO_MENU *menu));
ALLEGRO_DIALOG_FUNC(void, al_destroy_menu, (ALLEGRO_MENU *menu));

/* properties */
ALLEGRO_DIALOG_FUNC(const char *, al_get_menu_item_caption, (ALLEGRO_MENU *menu, int pos));
ALLEGRO_DIALOG_FUNC(void, al_set_menu_item_caption, (ALLEGRO_MENU *menu, int pos, const char *caption));
ALLEGRO_DIALOG_FUNC(int, al_get_menu_item_flags, (ALLEGRO_MENU *menu, int pos));
ALLEGRO_DIALOG_FUNC(void, al_set_menu_item_flags, (ALLEGRO_MENU *menu, int pos, int flags));
ALLEGRO_DIALOG_FUNC(ALLEGRO_BITMAP *, al_get_menu_item_icon, (ALLEGRO_MENU *menu, int pos));
ALLEGRO_DIALOG_FUNC(void, al_set_menu_item_icon, (ALLEGRO_MENU *menu, int pos, ALLEGRO_BITMAP *icon));

#if defined(ALLEGRO_UNSTABLE) || defined(ALLEGRO_INTERNAL_UNSTABLE) || defined(ALLEGRO_NATIVE_DIALOG_SRC)
ALLEGRO_DIALOG_FUNC(int, al_toggle_menu_item_flags, (ALLEGRO_MENU *menu, int pos, int flags));
#endif
 
/* querying menus */
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_find_menu, (ALLEGRO_MENU *haystack, uint16_t id));
ALLEGRO_DIALOG_FUNC(bool, al_find_menu_item, (ALLEGRO_MENU *haystack, uint16_t id, ALLEGRO_MENU **menu, int *index));
 
/* menu events */
ALLEGRO_DIALOG_FUNC(ALLEGRO_EVENT_SOURCE *, al_get_default_menu_event_source, (void));
ALLEGRO_DIALOG_FUNC(ALLEGRO_EVENT_SOURCE *, al_enable_menu_event_source, (ALLEGRO_MENU *menu));
ALLEGRO_DIALOG_FUNC(void, al_disable_menu_event_source, (ALLEGRO_MENU *menu));
 
/* displaying menus */
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_get_display_menu, (ALLEGRO_DISPLAY *display));
ALLEGRO_DIALOG_FUNC(bool, al_set_display_menu, (ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu));
ALLEGRO_DIALOG_FUNC(bool, al_popup_menu, (ALLEGRO_MENU *popup, ALLEGRO_DISPLAY *display));
ALLEGRO_DIALOG_FUNC(ALLEGRO_MENU *, al_remove_display_menu, (ALLEGRO_DISPLAY *display));

ALLEGRO_DIALOG_FUNC(uint32_t, al_get_allegro_native_dialog_version, (void));

enum {
   ALLEGRO_FILECHOOSER_FILE_MUST_EXIST = 1,
   ALLEGRO_FILECHOOSER_SAVE            = 2,
   ALLEGRO_FILECHOOSER_FOLDER          = 4,
   ALLEGRO_FILECHOOSER_PICTURES        = 8,
   ALLEGRO_FILECHOOSER_SHOW_HIDDEN     = 16,
   ALLEGRO_FILECHOOSER_MULTIPLE        = 32
};

enum {
   ALLEGRO_MESSAGEBOX_WARN             = 1<<0,
   ALLEGRO_MESSAGEBOX_ERROR            = 1<<1,
   ALLEGRO_MESSAGEBOX_OK_CANCEL        = 1<<2,
   ALLEGRO_MESSAGEBOX_YES_NO           = 1<<3,
   ALLEGRO_MESSAGEBOX_QUESTION         = 1<<4
};

enum {
   ALLEGRO_TEXTLOG_NO_CLOSE            = 1<<0,
   ALLEGRO_TEXTLOG_MONOSPACE           = 1<<1
};

enum {
   ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE   = 600,
   ALLEGRO_EVENT_MENU_CLICK            = 601
};

enum {
   ALLEGRO_MENU_ITEM_ENABLED            = 0,
   ALLEGRO_MENU_ITEM_CHECKBOX           = 1,
   ALLEGRO_MENU_ITEM_CHECKED            = 2,
   ALLEGRO_MENU_ITEM_DISABLED           = 4
};


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
