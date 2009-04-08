#ifndef __al_included_a5_native_dialog_h
#define __al_included_a5_native_dialog_h

#include "allegro5/allegro5.h"

#ifdef __cplusplus
   extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_NATIVE_DIALOG_SRC
         #define _A5_DIALOG_DLL __declspec(dllexport)
      #else
         #define _A5_DIALOG_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_DIALOG_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_DIALOG_FUNC(type, name, args)      _A5_DIALOG_DLL type __cdecl name args
#elif defined ALLEGRO_MINGW32
   #define A5_DIALOG_FUNC(type, name, args)      extern type name args
#elif defined ALLEGRO_BCC32
   #define A5_DIALOG_FUNC(type, name, args)      extern _A5_DIALOG_DLL type name args
#else
   #define A5_DIALOG_FUNC      AL_FUNC
#endif

/* Type: ALLEGRO_NATIVE_DIALOG
 */
typedef struct ALLEGRO_NATIVE_DIALOG ALLEGRO_NATIVE_DIALOG;

A5_DIALOG_FUNC(
   ALLEGRO_NATIVE_DIALOG *, al_create_native_file_dialog, (
      ALLEGRO_PATH const *initial_path, char const *title,
      char const *patterns, int mode));
A5_DIALOG_FUNC(
   void, al_show_native_file_dialog, (ALLEGRO_NATIVE_DIALOG *fd));
A5_DIALOG_FUNC(int, al_native_message_box, (
      char const *title, char const *text, char const *buttons, int flags));
A5_DIALOG_FUNC(
   int, al_get_native_file_dialog_count, (
      const ALLEGRO_NATIVE_DIALOG *fc));
A5_DIALOG_FUNC(
   const ALLEGRO_PATH *, al_get_native_file_dialog_path, (
      const ALLEGRO_NATIVE_DIALOG *fc, size_t index));
A5_DIALOG_FUNC(void, al_destroy_native_dialog, (ALLEGRO_NATIVE_DIALOG *fc));

#define ALLEGRO_FILECHOOSER_FILE_MUST_EXIST 1
#define ALLEGRO_FILECHOOSER_SAVE 2
#define ALLEGRO_FILECHOOSER_FOLDER 4
#define ALLEGRO_FILECHOOSER_PICTURES 8
#define ALLEGRO_FILECHOOSER_SHOW_HIDDEN 16
#define ALLEGRO_FILECHOOSER_MULTIPLE 32

#define ALLEGRO_MESSAGEBOX_WARN (1<<0)
#define ALLEGRO_MESSAGEBOX_ERROR (1<<1)
#define ALLEGRO_MESSAGEBOX_OK_CANCEL (1<<2)
#define ALLEGRO_MESSAGEBOX_YES_NO (1<<3)
#define ALLEGRO_MESSAGEBOX_QUESTION (1<<4)

#ifdef __cplusplus
   }
#endif

#endif   /* __al_included_a5_native_dialog_h */
