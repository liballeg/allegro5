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

A5_DIALOG_FUNC(ALLEGRO_EVENT_SOURCE *, al_spawn_native_file_dialog,
    (ALLEGRO_PATH const *initial_path,
    char const *patterns, int mode));
A5_DIALOG_FUNC(int, al_finalize_native_file_dialog,
    (ALLEGRO_EVENT *event, ALLEGRO_PATH *pathes[], int n));

#define ALLEGRO_FILECHOOSER_FILE_MUST_EXIST 1
#define ALLEGRO_FILECHOOSER_SAVE 2
#define ALLEGRO_FILECHOOSER_FOLDER 4
#define ALLEGRO_FILECHOOSER_PICTURES 8
#define ALLEGRO_FILECHOOSER_SHOW_HIDDEN 16
#define ALLEGRO_FILECHOOSER_MULTIPLE 32

// FIXME: 1024 is the first user event. There needs to be an area
// reserved for addons. In fact, we might want to have a function
// al_get_new_user_event_number() which returns the next unused number
// at runtime, and each addon could allocate their own dynamic numbers.
#define ALLEGRO_EVENT_NATIVE_FILE_CHOOSER 65536

#ifdef __cplusplus
   }
#endif
