#ifndef __al_included_allegro5_allegro_native_dialog_h
#define __al_included_allegro5_allegro_native_dialog_h

#include "allegro5/allegro.h"

#ifdef __cplusplus
   extern "C" {
#endif

#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
   #ifndef A5O_STATICLINK
      #ifdef A5O_NATIVE_DIALOG_SRC
         #define _A5O_DIALOG_DLL __declspec(dllexport)
      #else
         #define _A5O_DIALOG_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_DIALOG_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_DIALOG_FUNC(type, name, args)      _A5O_DIALOG_DLL type __cdecl name args
#elif defined A5O_MINGW32
   #define A5O_DIALOG_FUNC(type, name, args)      extern type name args
#elif defined A5O_BCC32
   #define A5O_DIALOG_FUNC(type, name, args)      extern _A5O_DIALOG_DLL type name args
#else
   #define A5O_DIALOG_FUNC      AL_FUNC
#endif

#ifdef A5O_WITH_XWINDOWS
   #define A5O_GTK_TOPLEVEL  A5O_GTK_TOPLEVEL_INTERNAL
#endif

/* Type: A5O_FILECHOOSER
 */
typedef struct A5O_FILECHOOSER A5O_FILECHOOSER;

/* Type: A5O_TEXTLOG
 */
typedef struct A5O_TEXTLOG A5O_TEXTLOG;

/* Type: A5O_MENU
 */
typedef struct A5O_MENU A5O_MENU;

/* Type: A5O_MENU_INFO
 */
typedef struct A5O_MENU_INFO {
   const char *caption;
   uint16_t id;
   int flags;
   A5O_BITMAP *icon;
} A5O_MENU_INFO;

#define A5O_MENU_SEPARATOR             { NULL,         -1, 0, NULL }
#define A5O_START_OF_MENU(caption, id) { caption "->", id, 0, NULL }
#define A5O_END_OF_MENU                { NULL,          0, 0, NULL }

A5O_DIALOG_FUNC(bool, al_init_native_dialog_addon, (void));
A5O_DIALOG_FUNC(bool, al_is_native_dialog_addon_initialized, (void));
A5O_DIALOG_FUNC(void, al_shutdown_native_dialog_addon, (void));

A5O_DIALOG_FUNC(A5O_FILECHOOSER *, al_create_native_file_dialog, (char const *initial_path,
   char const *title, char const *patterns, int mode));
A5O_DIALOG_FUNC(bool, al_show_native_file_dialog, (A5O_DISPLAY *display, A5O_FILECHOOSER *dialog));
A5O_DIALOG_FUNC(int, al_get_native_file_dialog_count, (const A5O_FILECHOOSER *dialog));
A5O_DIALOG_FUNC(const char *, al_get_native_file_dialog_path, (const A5O_FILECHOOSER *dialog,
   size_t index));
A5O_DIALOG_FUNC(void, al_destroy_native_file_dialog, (A5O_FILECHOOSER *dialog));

A5O_DIALOG_FUNC(int, al_show_native_message_box, (A5O_DISPLAY *display, char const *title,
   char const *heading, char const *text, char const *buttons, int flags));

A5O_DIALOG_FUNC(A5O_TEXTLOG *, al_open_native_text_log, (char const *title, int flags));
A5O_DIALOG_FUNC(void, al_close_native_text_log, (A5O_TEXTLOG *textlog));
A5O_DIALOG_FUNC(void, al_append_native_text_log, (A5O_TEXTLOG *textlog, char const *format, ...));
A5O_DIALOG_FUNC(A5O_EVENT_SOURCE *, al_get_native_text_log_event_source, (A5O_TEXTLOG *textlog));

/* creating/modifying menus */
A5O_DIALOG_FUNC(A5O_MENU *, al_create_menu, (void));
A5O_DIALOG_FUNC(A5O_MENU *, al_create_popup_menu, (void));
A5O_DIALOG_FUNC(A5O_MENU *, al_build_menu, (A5O_MENU_INFO *info));
A5O_DIALOG_FUNC(int, al_append_menu_item, (A5O_MENU *parent, char const *title, uint16_t id, int flags,
   A5O_BITMAP *icon, A5O_MENU *submenu));
A5O_DIALOG_FUNC(int, al_insert_menu_item, (A5O_MENU *parent, int pos, char const *title, uint16_t id,
   int flags, A5O_BITMAP *icon, A5O_MENU *submenu));
A5O_DIALOG_FUNC(bool, al_remove_menu_item, (A5O_MENU *menu, int pos));
A5O_DIALOG_FUNC(A5O_MENU *, al_clone_menu, (A5O_MENU *menu));
A5O_DIALOG_FUNC(A5O_MENU *, al_clone_menu_for_popup, (A5O_MENU *menu));
A5O_DIALOG_FUNC(void, al_destroy_menu, (A5O_MENU *menu));

/* properties */
A5O_DIALOG_FUNC(const char *, al_get_menu_item_caption, (A5O_MENU *menu, int pos));
A5O_DIALOG_FUNC(void, al_set_menu_item_caption, (A5O_MENU *menu, int pos, const char *caption));
A5O_DIALOG_FUNC(int, al_get_menu_item_flags, (A5O_MENU *menu, int pos));
A5O_DIALOG_FUNC(void, al_set_menu_item_flags, (A5O_MENU *menu, int pos, int flags));
A5O_DIALOG_FUNC(A5O_BITMAP *, al_get_menu_item_icon, (A5O_MENU *menu, int pos));
A5O_DIALOG_FUNC(void, al_set_menu_item_icon, (A5O_MENU *menu, int pos, A5O_BITMAP *icon));

#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_NATIVE_DIALOG_SRC)
A5O_DIALOG_FUNC(int, al_toggle_menu_item_flags, (A5O_MENU *menu, int pos, int flags));
#endif
 
/* querying menus */
A5O_DIALOG_FUNC(A5O_MENU *, al_find_menu, (A5O_MENU *haystack, uint16_t id));
A5O_DIALOG_FUNC(bool, al_find_menu_item, (A5O_MENU *haystack, uint16_t id, A5O_MENU **menu, int *index));
 
/* menu events */
A5O_DIALOG_FUNC(A5O_EVENT_SOURCE *, al_get_default_menu_event_source, (void));
A5O_DIALOG_FUNC(A5O_EVENT_SOURCE *, al_enable_menu_event_source, (A5O_MENU *menu));
A5O_DIALOG_FUNC(void, al_disable_menu_event_source, (A5O_MENU *menu));
 
/* displaying menus */
A5O_DIALOG_FUNC(A5O_MENU *, al_get_display_menu, (A5O_DISPLAY *display));
A5O_DIALOG_FUNC(bool, al_set_display_menu, (A5O_DISPLAY *display, A5O_MENU *menu));
A5O_DIALOG_FUNC(bool, al_popup_menu, (A5O_MENU *popup, A5O_DISPLAY *display));
A5O_DIALOG_FUNC(A5O_MENU *, al_remove_display_menu, (A5O_DISPLAY *display));

A5O_DIALOG_FUNC(uint32_t, al_get_allegro_native_dialog_version, (void));

enum {
   A5O_FILECHOOSER_FILE_MUST_EXIST = 1,
   A5O_FILECHOOSER_SAVE            = 2,
   A5O_FILECHOOSER_FOLDER          = 4,
   A5O_FILECHOOSER_PICTURES        = 8,
   A5O_FILECHOOSER_SHOW_HIDDEN     = 16,
   A5O_FILECHOOSER_MULTIPLE        = 32
};

enum {
   A5O_MESSAGEBOX_WARN             = 1<<0,
   A5O_MESSAGEBOX_ERROR            = 1<<1,
   A5O_MESSAGEBOX_OK_CANCEL        = 1<<2,
   A5O_MESSAGEBOX_YES_NO           = 1<<3,
   A5O_MESSAGEBOX_QUESTION         = 1<<4
};

enum {
   A5O_TEXTLOG_NO_CLOSE            = 1<<0,
   A5O_TEXTLOG_MONOSPACE           = 1<<1
};

enum {
   A5O_EVENT_NATIVE_DIALOG_CLOSE   = 600,
   A5O_EVENT_MENU_CLICK            = 601
};

enum {
   A5O_MENU_ITEM_ENABLED            = 0,
   A5O_MENU_ITEM_CHECKBOX           = 1,
   A5O_MENU_ITEM_CHECKED            = 2,
   A5O_MENU_ITEM_DISABLED           = 4
};


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set sts=3 sw=3 et: */
