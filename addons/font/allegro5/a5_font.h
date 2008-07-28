#ifndef A5FONT_H
#define A5FONT_H

#if (defined ALLEGRO_MINGW32) || (defined ALLEGRO_MSVC) || (defined ALLEGRO_BCC32)
   #ifndef ALLEGRO_STATICLINK
      #ifdef A5_FONT_SRC
         #define _A5_FONT_DLL __declspec(dllexport)
      #else
         #define _A5_FONT_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5_FONT_DLL
   #endif
#endif

#if defined ALLEGRO_MSVC
   #define A5_FONT_VAR(type, name)             extern _A5_FONT_DLL type name
   #define A5_FONT_ARRAY(type, name)           extern _A5_FONT_DLL type name[]
   #define A5_FONT_FUNC(type, name, args)      _A5_FONT_DLL type __cdecl name args
   #define A5_FONT_METHOD(type, name, args)    type (__cdecl *name) args
   #define A5_FONT_FUNCPTR(type, name, args)   extern _A5_FONT_DLL type (__cdecl *name) args
   #define A5_FONT_PRINTFUNC(type, name, args, a, b)  A5_FONT_FUNC(type, name, args)
#elif defined ALLEGRO_MINGW32
   #define A5_FONT_VAR(type, name)                   extern _A5_FONT_DLL type name
   #define A5_FONT_ARRAY(type, name)                 extern _A5_FONT_DLL type name[]
   #define A5_FONT_FUNC(type, name, args)            extern type name args
   #define A5_FONT_METHOD(type, name, args)          type (*name) args
   #define A5_FONT_FUNCPTR(type, name, args)         extern _A5_FONT_DLL type (*name) args
   #define A5_FONT_PRINTFUNC(type, name, args, a, b) A5_FONT_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))
#elif defined ALLEGRO_BCC32
   #define A5_FONT_VAR(type, name)             extern _A5_FONT_DLL type name
   #define A5_FONT_ARRAY(type, name)           extern _A5_FONT_DLL type name[]
   #define A5_FONT_FUNC(type, name, args)      extern _A5_FONT_DLL type name args
   #define A5_FONT_METHOD(type, name, args)    type (*name) args
   #define A5_FONT_FUNCPTR(type, name, args)   extern _A5_FONT_DLL type (*name) args
   #define A5_FONT_PRINTFUNC(type, name, args, a, b)    A5_FONT_FUNC(type, name, args)
#else
   #define A5_FONT_VAR       AL_VAR
   #define A5_FONT_ARRAY     AL_ARRAY
   #define A5_FONT_FUNC      AL_FUNC
   #define A5_FONT_METHOD    AL_METHOD
   #define A5_FONT_FUNCPTR   AL_FUNCPTR
   #define A5_FONT_PRINTFUNC AL_PRINTFUNC
#endif


#ifdef __cplusplus
   extern "C" {
#endif


struct A5FONT_FONT_VTABLE;

typedef struct A5FONT_FONT
{
   void *data;
   int height;
   struct A5FONT_FONT_VTABLE *vtable;
} A5FONT_FONT;

/* text- and font-related stuff */
typedef struct A5FONT_FONT_VTABLE
{
   A5_FONT_METHOD(int, font_height, (AL_CONST A5FONT_FONT *f));
   A5_FONT_METHOD(int, char_length, (AL_CONST A5FONT_FONT *f, int ch));
   A5_FONT_METHOD(int, text_length, (AL_CONST A5FONT_FONT *f, AL_CONST char *text, int count));
   A5_FONT_METHOD(int, render_char, (AL_CONST A5FONT_FONT *f, int ch, int x, int y));
   A5_FONT_METHOD(void, render, (AL_CONST A5FONT_FONT *f, AL_CONST char *text, int x, int y, int count));
   A5_FONT_METHOD(void, destroy, (A5FONT_FONT *f));

   A5_FONT_METHOD(int, get_font_ranges, (A5FONT_FONT *f));
   A5_FONT_METHOD(int, get_font_range_begin, (A5FONT_FONT *f, int range));
   A5_FONT_METHOD(int, get_font_range_end, (A5FONT_FONT *f, int range));
   A5_FONT_METHOD(A5FONT_FONT *, extract_font_range, (A5FONT_FONT *f, int begin, int end));
   A5_FONT_METHOD(A5FONT_FONT *, merge_fonts, (A5FONT_FONT *f1, A5FONT_FONT *f2));
   A5_FONT_METHOD(int, transpose_font, (A5FONT_FONT *f, int drange));
} A5FONT_FONT_VTABLE;


typedef struct A5FONT_FONT_COLOR_DATA
{
   int begin, end;                   /* first char and one-past-the-end char */
   ALLEGRO_BITMAP *glyphs;           /* our glyphs */
   ALLEGRO_BITMAP **bitmaps;         /* sub bitmaps pointing to our glyphs */
   struct A5FONT_FONT_COLOR_DATA *next;  /* linked list structure */
} A5FONT_FONT_COLOR_DATA;


struct ALLEGRO_BITMAP;

struct A5FONT_FONT_VTABLE;

struct A5FONT_FONT;

A5_FONT_FUNC(int, a5font_is_compatible_font, (A5FONT_FONT *f1, A5FONT_FONT *f2));

A5_FONT_FUNC(void, a5font_register_font_file_type, (AL_CONST char *ext, A5FONT_FONT *(*load)(AL_CONST char *filename, void *param)));
A5_FONT_FUNC(A5FONT_FONT *, a5font_load_bitmap_font, (AL_CONST char *filename, void *param));
A5_FONT_FUNC(A5FONT_FONT *, a5font_load_font, (AL_CONST char *filename, void *param));

A5_FONT_FUNC(A5FONT_FONT *, a5font_grab_font_from_bitmap, (ALLEGRO_BITMAP *bmp));

A5_FONT_FUNC(int, a5font_get_font_ranges, (A5FONT_FONT *f));
A5_FONT_FUNC(int, a5font_get_font_range_begin, (A5FONT_FONT *f, int range));
A5_FONT_FUNC(int, a5font_get_font_range_end, (A5FONT_FONT *f, int range));
A5_FONT_FUNC(A5FONT_FONT *, a5font_extract_font_range, (A5FONT_FONT *f, int begin, int end));
A5_FONT_FUNC(A5FONT_FONT *, a5font_merge_fonts, (A5FONT_FONT *f1, A5FONT_FONT *f2));
A5_FONT_FUNC(int, a5font_transpose_font, (A5FONT_FONT *f, int drange));

A5_FONT_VAR(int, a5font_404_char);
A5_FONT_FUNC(void, a5font_textout, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y));
A5_FONT_FUNC(void, a5font_textout_centre, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y));
A5_FONT_FUNC(void, a5font_textout_right, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y));
A5_FONT_FUNC(void, a5font_textout_justify, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff));
A5_FONT_PRINTFUNC(void, a5font_textprintf, (AL_CONST struct A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
A5_FONT_PRINTFUNC(void, a5font_textprintf_centre, (AL_CONST struct A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
A5_FONT_PRINTFUNC(void, a5font_textprintf_right, (AL_CONST struct A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
A5_FONT_PRINTFUNC(void, a5font_textprintf_justify, (AL_CONST struct A5FONT_FONT *f, int x1, int x2, int y, int diff, AL_CONST char *format, ...), 6, 7);
A5_FONT_FUNC(int, a5font_text_length, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str));
A5_FONT_FUNC(int, a5font_text_length_count, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int count));
A5_FONT_FUNC(int, a5font_text_height, (AL_CONST struct A5FONT_FONT *f));
A5_FONT_FUNC(void, a5font_destroy_font, (struct A5FONT_FONT *f));

extern A5FONT_FONT_VTABLE *a5font_font_vtable_color;

A5_FONT_FUNC(ALLEGRO_BITMAP *, _a5font_color_find_glyph, (AL_CONST A5FONT_FONT *f, int ch));

A5_FONT_FUNC(void, _a5font_register_font_file_type_init, (void));
A5_FONT_FUNC(void, a5font_init, (void));

/* New count based routines */
A5_FONT_FUNC(void, a5font_textout_count, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y, int count));
A5_FONT_FUNC(void, a5font_textout_centre_count, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y, int count));
A5_FONT_FUNC(void, a5font_textout_right_count, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y, int count));
A5_FONT_PRINTFUNC(void, a5font_textprintf_count, (AL_CONST struct A5FONT_FONT *f, int x, int y, int count, AL_CONST char *format, ...), 5, 6);
A5_FONT_PRINTFUNC(void, a5font_textprintf_centre_count, (AL_CONST struct A5FONT_FONT *f, int x, int y, int count, AL_CONST char *format, ...), 5, 6);
A5_FONT_PRINTFUNC(void, a5font_textprintf_right_count, (AL_CONST struct A5FONT_FONT *f, int x, int y, int count, AL_CONST char *format, ...), 5, 6);


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef A5FONT_H */




