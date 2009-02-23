#include "allegro5/allegro5.h"

#ifdef __cplusplus
   extern "C" {
#endif

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_DIALOG_SRC
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

typedef struct ALLEGRO_NATIVE_FILE_DIALOG ALLEGRO_NATIVE_FILE_DIALOG;

A5_DIALOG_FUNC(
ALLEGRO_NATIVE_FILE_DIALOG *, al_create_native_file_dialog, (
    ALLEGRO_PATH const *initial_path,
    char const *title,
    char const *patterns,
    int mode));
A5_DIALOG_FUNC(
void, al_show_native_file_dialog, (ALLEGRO_NATIVE_FILE_DIALOG *fd));
A5_DIALOG_FUNC(
int, al_get_native_file_dialog_count, (ALLEGRO_NATIVE_FILE_DIALOG *fc));
A5_DIALOG_FUNC(
ALLEGRO_PATH *, al_get_native_file_dialog_path, (
   ALLEGRO_NATIVE_FILE_DIALOG *fc, int i);
void al_destroy_native_file_dialog(ALLEGRO_NATIVE_FILE_DIALOG *fc));


#define ALLEGRO_FILECHOOSER_FILE_MUST_EXIST 1
#define ALLEGRO_FILECHOOSER_SAVE 2
#define ALLEGRO_FILECHOOSER_FOLDER 4
#define ALLEGRO_FILECHOOSER_PICTURES 8
#define ALLEGRO_FILECHOOSER_SHOW_HIDDEN 16
#define ALLEGRO_FILECHOOSER_MULTIPLE 32

#ifdef __cplusplus
   }
#endif
