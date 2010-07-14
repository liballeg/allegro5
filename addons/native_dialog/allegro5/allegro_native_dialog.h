#ifndef __al_included_allegro_native_dialog_h
#define __al_included_allegro_native_dialog_h

#include "allegro5/allegro5.h"

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

/* Type: ALLEGRO_NATIVE_DIALOG
 */
typedef struct ALLEGRO_NATIVE_DIALOG ALLEGRO_NATIVE_DIALOG;

ALLEGRO_DIALOG_FUNC(
   ALLEGRO_NATIVE_DIALOG *, al_create_native_file_dialog, (
      ALLEGRO_PATH const *initial_path, char const *title,
      char const *patterns, int mode));
ALLEGRO_DIALOG_FUNC(
   void, al_show_native_file_dialog, (ALLEGRO_DISPLAY *display,
      ALLEGRO_NATIVE_DIALOG *fd));
ALLEGRO_DIALOG_FUNC(
   int, al_show_native_message_box, (ALLEGRO_DISPLAY *display,
      char const *title, char const *heading, char const *text,
      char const *buttons, int flags));
ALLEGRO_DIALOG_FUNC(
   int, al_get_native_file_dialog_count, (
      const ALLEGRO_NATIVE_DIALOG *fc));
ALLEGRO_DIALOG_FUNC(
   const ALLEGRO_PATH *, al_get_native_file_dialog_path, (
      const ALLEGRO_NATIVE_DIALOG *fc, size_t index));
ALLEGRO_DIALOG_FUNC(void, al_destroy_native_dialog, (ALLEGRO_NATIVE_DIALOG *fc));
ALLEGRO_DIALOG_FUNC(uint32_t, al_get_allegro_native_dialog_version, (void));

ALLEGRO_DIALOG_FUNC(ALLEGRO_NATIVE_DIALOG *, al_open_native_text_log,
   (char const *title, int flags));
ALLEGRO_DIALOG_FUNC(void, al_close_native_text_log,
   (ALLEGRO_NATIVE_DIALOG *textlog));
ALLEGRO_DIALOG_FUNC(void, al_append_native_text_log,
   (ALLEGRO_NATIVE_DIALOG *textlog, char const *format, ...));
ALLEGRO_DIALOG_FUNC(ALLEGRO_EVENT_SOURCE *,
   al_get_native_dialog_event_source, (ALLEGRO_NATIVE_DIALOG *dialog));

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
   ALLEGRO_EVENT_NATIVE_DIALOG_CLOSE   = 600
};

#ifdef __cplusplus
   }
#endif

#endif   /* __al_included_allegro_native_dialog_h */

/* vim: set sts=3 sw=3 et: */
