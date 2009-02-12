#ifndef ALLEGRO_FONT_H
#define ALLEGRO_FONT_H

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef ALLEGRO_FONT_SRC
         #define _ALLEGRO_FONT_DLL __declspec(dllexport)
      #else
         #define _ALLEGRO_FONT_DLL __declspec(dllimport)
      #endif
   #else
      #define _ALLEGRO_FONT_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define ALLEGRO_FONT_VAR(type, name)             extern _ALLEGRO_FONT_DLL type name
   #define ALLEGRO_FONT_ARRAY(type, name)           extern _ALLEGRO_FONT_DLL type name[]
   #define ALLEGRO_FONT_FUNC(type, name, args)      _ALLEGRO_FONT_DLL type __cdecl name args
   #define ALLEGRO_FONT_METHOD(type, name, args)    type (__cdecl *name) args
   #define ALLEGRO_FONT_FUNCPTR(type, name, args)   extern _ALLEGRO_FONT_DLL type (__cdecl *name) args
   #define ALLEGRO_FONT_PRINTFUNC(type, name, args, a, b)  ALLEGRO_FONT_FUNC(type, name, args)
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_FONT_VAR(type, name)                   extern _ALLEGRO_FONT_DLL type name
   #define ALLEGRO_FONT_ARRAY(type, name)                 extern _ALLEGRO_FONT_DLL type name[]
   #define ALLEGRO_FONT_FUNC(type, name, args)            extern type name args
   #define ALLEGRO_FONT_METHOD(type, name, args)          type (*name) args
   #define ALLEGRO_FONT_FUNCPTR(type, name, args)         extern _ALLEGRO_FONT_DLL type (*name) args
   #define ALLEGRO_FONT_PRINTFUNC(type, name, args, a, b) ALLEGRO_FONT_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_FONT_VAR(type, name)             extern _ALLEGRO_FONT_DLL type name
   #define ALLEGRO_FONT_ARRAY(type, name)           extern _ALLEGRO_FONT_DLL type name[]
   #define ALLEGRO_FONT_FUNC(type, name, args)      extern _ALLEGRO_FONT_DLL type name args
   #define ALLEGRO_FONT_METHOD(type, name, args)    type (*name) args
   #define ALLEGRO_FONT_FUNCPTR(type, name, args)   extern _ALLEGRO_FONT_DLL type (*name) args
   #define ALLEGRO_FONT_PRINTFUNC(type, name, args, a, b)    ALLEGRO_FONT_FUNC(type, name, args)
#else
   #define ALLEGRO_FONT_VAR       AL_VAR
   #define ALLEGRO_FONT_ARRAY     AL_ARRAY
   #define ALLEGRO_FONT_FUNC      AL_FUNC
   #define ALLEGRO_FONT_METHOD    AL_METHOD
   #define ALLEGRO_FONT_FUNCPTR   AL_FUNCPTR
   #define ALLEGRO_FONT_PRINTFUNC AL_PRINTFUNC
#endif


#ifdef __cplusplus
   extern "C" {
#endif


typedef struct ALLEGRO_FONT ALLEGRO_FONT;
typedef struct ALLEGRO_FONT_VTABLE ALLEGRO_FONT_VTABLE;

struct ALLEGRO_FONT
{
   void *data;
   int height;
   ALLEGRO_FONT_VTABLE *vtable;
};

/* text- and font-related stuff */
struct ALLEGRO_FONT_VTABLE
{
   ALLEGRO_FONT_METHOD(int, font_height, (const ALLEGRO_FONT *f));
   ALLEGRO_FONT_METHOD(int, char_length, (const ALLEGRO_FONT *f, int ch));
   ALLEGRO_FONT_METHOD(int, text_length, (const ALLEGRO_FONT *f, const ALLEGRO_USTR text));
   ALLEGRO_FONT_METHOD(int, render_char, (const ALLEGRO_FONT *f, int ch, int x, int y));
   ALLEGRO_FONT_METHOD(int, render, (const ALLEGRO_FONT *f, const ALLEGRO_USTR text, int x, int y));
   ALLEGRO_FONT_METHOD(void, destroy, (ALLEGRO_FONT *f));
};


ALLEGRO_FONT_FUNC(int, al_font_is_compatible_font, (ALLEGRO_FONT *f1, ALLEGRO_FONT *f2));

ALLEGRO_FONT_FUNC(void, al_font_register_font_file_type, (const char *ext, ALLEGRO_FONT *(*load)(const char *filename, void *param)));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_load_bitmap_font, (const char *filename, void *param));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_load_font, (const char *filename, void *param));

ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_grab_font_from_bitmap, (ALLEGRO_BITMAP *bmp));

ALLEGRO_FONT_FUNC(void, al_font_textout, (const ALLEGRO_FONT *f, int x, int y, const char *str, int count));
ALLEGRO_FONT_FUNC(void, al_font_textout_centre, (const ALLEGRO_FONT *f, int x, int y, const char *str, int count));
ALLEGRO_FONT_FUNC(void, al_font_textout_right, (const ALLEGRO_FONT *f, int x, int y, const char *str, int count));
ALLEGRO_FONT_FUNC(void, al_font_textout_justify, (const ALLEGRO_FONT *f, int x1, int x2, int y, int diff, const char *str));
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf, (const ALLEGRO_FONT *f, int x, int y, const char *format, ...), 4, 5);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_centre, (const ALLEGRO_FONT *f, int x, int y, const char *format, ...), 4, 5);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_right, (const ALLEGRO_FONT *f, int x, int y, const char *format, ...), 4, 5);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_justify, (const ALLEGRO_FONT *f, int x1, int x2, int y, int diff, const char *format, ...), 6, 7);
ALLEGRO_FONT_FUNC(int, al_font_text_width, (const ALLEGRO_FONT *f, const char *str, int count));
ALLEGRO_FONT_FUNC(int, al_font_text_height, (const ALLEGRO_FONT *f));
ALLEGRO_FONT_FUNC(void, al_font_destroy_font, (ALLEGRO_FONT *f));

ALLEGRO_FONT_FUNC(void, al_font_init, (void));


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FONT_H */
