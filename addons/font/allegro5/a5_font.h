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


struct ALLEGRO_FONT_VTABLE;

typedef struct ALLEGRO_FONT
{
   void *data;
   int height;
   struct ALLEGRO_FONT_VTABLE *vtable;
} ALLEGRO_FONT;

/* text- and font-related stuff */
typedef struct ALLEGRO_FONT_VTABLE
{
   ALLEGRO_FONT_METHOD(int, font_height, (AL_CONST ALLEGRO_FONT *f));
   ALLEGRO_FONT_METHOD(int, char_length, (AL_CONST ALLEGRO_FONT *f, int ch));
   ALLEGRO_FONT_METHOD(int, text_length, (AL_CONST ALLEGRO_FONT *f, AL_CONST char *text, int count));
   ALLEGRO_FONT_METHOD(int, render_char, (AL_CONST ALLEGRO_FONT *f, int ch, int x, int y));
   ALLEGRO_FONT_METHOD(void, render, (AL_CONST ALLEGRO_FONT *f, AL_CONST char *text, int x, int y, int count));
   ALLEGRO_FONT_METHOD(void, destroy, (ALLEGRO_FONT *f));
/*
   ALLEGRO_FONT_METHOD(int, get_font_ranges, (ALLEGRO_FONT *f));
   ALLEGRO_FONT_METHOD(int, get_font_range_begin, (ALLEGRO_FONT *f, int range));
   ALLEGRO_FONT_METHOD(int, get_font_range_end, (ALLEGRO_FONT *f, int range));
   ALLEGRO_FONT_METHOD(ALLEGRO_FONT *, extract_font_range, (ALLEGRO_FONT *f, int begin, int end));
   ALLEGRO_FONT_METHOD(ALLEGRO_FONT *, merge_fonts, (ALLEGRO_FONT *f1, ALLEGRO_FONT *f2));
   ALLEGRO_FONT_METHOD(int, transpose_font, (ALLEGRO_FONT *f, int drange));
   */
} ALLEGRO_FONT_VTABLE;

struct ALLEGRO_BITMAP;

struct ALLEGRO_FONT_VTABLE;

struct ALLEGRO_FONT;

ALLEGRO_FONT_FUNC(int, al_font_is_compatible_font, (ALLEGRO_FONT *f1, ALLEGRO_FONT *f2));

ALLEGRO_FONT_FUNC(void, al_font_register_font_file_type, (AL_CONST char *ext, ALLEGRO_FONT *(*load)(AL_CONST char *filename, void *param)));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_load_bitmap_font, (AL_CONST char *filename, void *param));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_load_font, (AL_CONST char *filename, void *param));

ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_grab_font_from_bitmap, (ALLEGRO_BITMAP *bmp));

/*
ALLEGRO_FONT_FUNC(int, al_font_get_font_ranges, (ALLEGRO_FONT *f));
ALLEGRO_FONT_FUNC(int, al_font_get_font_range_begin, (ALLEGRO_FONT *f, int range));
ALLEGRO_FONT_FUNC(int, al_font_get_font_range_end, (ALLEGRO_FONT *f, int range));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_extract_font_range, (ALLEGRO_FONT *f, int begin, int end));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_font_merge_fonts, (ALLEGRO_FONT *f1, ALLEGRO_FONT *f2));
ALLEGRO_FONT_FUNC(int, al_font_transpose_font, (ALLEGRO_FONT *f, int drange));
*/

//ALLEGRO_FONT_VAR(int, al_font_404_char);
ALLEGRO_FONT_FUNC(void, al_font_textout, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int x, int y));
ALLEGRO_FONT_FUNC(void, al_font_textout_centre, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int x, int y));
ALLEGRO_FONT_FUNC(void, al_font_textout_right, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int x, int y));
ALLEGRO_FONT_FUNC(void, al_font_textout_justify, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff));
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf, (AL_CONST struct ALLEGRO_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_centre, (AL_CONST struct ALLEGRO_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_right, (AL_CONST struct ALLEGRO_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_justify, (AL_CONST struct ALLEGRO_FONT *f, int x1, int x2, int y, int diff, AL_CONST char *format, ...), 6, 7);
ALLEGRO_FONT_FUNC(int, al_font_text_length, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str));
ALLEGRO_FONT_FUNC(int, al_font_text_length_count, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int count));
ALLEGRO_FONT_FUNC(int, al_font_text_height, (AL_CONST struct ALLEGRO_FONT *f));
ALLEGRO_FONT_FUNC(void, al_font_destroy_font, (struct ALLEGRO_FONT *f));

//extern ALLEGRO_FONT_VTABLE *al_font_vtable_color;

//ALLEGRO_FONT_FUNC(ALLEGRO_BITMAP *, _al_font_color_find_glyph, (AL_CONST ALLEGRO_FONT *f, int ch));

ALLEGRO_FONT_FUNC(void, al_font_init, (void));

/* New count based routines */
ALLEGRO_FONT_FUNC(void, al_font_textout_count, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int x, int y, int count));
ALLEGRO_FONT_FUNC(void, al_font_textout_centre_count, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int x, int y, int count));
ALLEGRO_FONT_FUNC(void, al_font_textout_right_count, (AL_CONST struct ALLEGRO_FONT *f, AL_CONST char *str, int x, int y, int count));
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_count, (AL_CONST struct ALLEGRO_FONT *f, int x, int y, int count, AL_CONST char *format, ...), 5, 6);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_centre_count, (AL_CONST struct ALLEGRO_FONT *f, int x, int y, int count, AL_CONST char *format, ...), 5, 6);
ALLEGRO_FONT_PRINTFUNC(void, al_font_textprintf_right_count, (AL_CONST struct ALLEGRO_FONT *f, int x, int y, int count, AL_CONST char *format, ...), 5, 6);


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_FONT_H */




