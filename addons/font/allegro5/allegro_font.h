#ifndef __al_included_allegro5_allegro_font_h
#define __al_included_allegro5_allegro_font_h

#include "allegro5/allegro.h"

#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
   #ifndef A5O_STATICLINK
      #ifdef A5O_FONT_SRC
         #define _A5O_FONT_DLL __declspec(dllexport)
      #else
         #define _A5O_FONT_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_FONT_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_FONT_FUNC(type, name, args)      _A5O_FONT_DLL type __cdecl name args
   #define A5O_FONT_METHOD(type, name, args)    type (__cdecl *name) args
   #define A5O_FONT_FUNCPTR(type, name, args)   extern _A5O_FONT_DLL type (__cdecl *name) args
   #define A5O_FONT_PRINTFUNC(type, name, args, a, b)  A5O_FONT_FUNC(type, name, args)
#elif defined A5O_MINGW32
   #define A5O_FONT_FUNC(type, name, args)            extern type name args
   #define A5O_FONT_METHOD(type, name, args)          type (*name) args
   #define A5O_FONT_FUNCPTR(type, name, args)         extern _A5O_FONT_DLL type (*name) args
   #define A5O_FONT_PRINTFUNC(type, name, args, a, b) A5O_FONT_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))
#elif defined A5O_BCC32
   #define A5O_FONT_FUNC(type, name, args)      extern _A5O_FONT_DLL type name args
   #define A5O_FONT_METHOD(type, name, args)    type (*name) args
   #define A5O_FONT_FUNCPTR(type, name, args)   extern _A5O_FONT_DLL type (*name) args
   #define A5O_FONT_PRINTFUNC(type, name, args, a, b)    A5O_FONT_FUNC(type, name, args)
#else
   #define A5O_FONT_FUNC      AL_FUNC
   #define A5O_FONT_METHOD    AL_METHOD
   #define A5O_FONT_FUNCPTR   AL_FUNCPTR
   #define A5O_FONT_PRINTFUNC AL_PRINTFUNC
#endif


#ifdef __cplusplus
   extern "C" {
#endif


/* Type: A5O_FONT
*/
typedef struct A5O_FONT A5O_FONT;
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_FONT_SRC)

/* Type: A5O_GLYPH
*/
typedef struct A5O_GLYPH A5O_GLYPH;

struct A5O_GLYPH
{
   A5O_BITMAP *bitmap;
   int x;
   int y;
   int w;
   int h;
   int kerning;
   int offset_x;
   int offset_y;
   int advance;
};
#endif

enum {
   A5O_NO_KERNING       = -1,
   A5O_ALIGN_LEFT       = 0,
   A5O_ALIGN_CENTRE     = 1,
   A5O_ALIGN_CENTER     = 1,
   A5O_ALIGN_RIGHT      = 2,
   A5O_ALIGN_INTEGER    = 4,
};

A5O_FONT_FUNC(bool, al_register_font_loader, (const char *ext, A5O_FONT *(*load)(const char *filename, int size, int flags)));
A5O_FONT_FUNC(A5O_FONT *, al_load_bitmap_font, (const char *filename));
A5O_FONT_FUNC(A5O_FONT *, al_load_bitmap_font_flags, (const char *filename, int flags));
A5O_FONT_FUNC(A5O_FONT *, al_load_font, (const char *filename, int size, int flags));

A5O_FONT_FUNC(A5O_FONT *, al_grab_font_from_bitmap, (A5O_BITMAP *bmp, int n, const int ranges[]));
A5O_FONT_FUNC(A5O_FONT *, al_create_builtin_font, (void));

A5O_FONT_FUNC(void, al_draw_ustr, (const A5O_FONT *font, A5O_COLOR color, float x, float y, int flags, A5O_USTR const *ustr));
A5O_FONT_FUNC(void, al_draw_text, (const A5O_FONT *font, A5O_COLOR color, float x, float y, int flags, char const *text));
A5O_FONT_FUNC(void, al_draw_justified_text, (const A5O_FONT *font, A5O_COLOR color, float x1, float x2, float y, float diff, int flags, char const *text));
A5O_FONT_FUNC(void, al_draw_justified_ustr, (const A5O_FONT *font, A5O_COLOR color, float x1, float x2, float y, float diff, int flags, A5O_USTR const *text));
A5O_FONT_PRINTFUNC(void, al_draw_textf, (const A5O_FONT *font, A5O_COLOR color, float x, float y, int flags, char const *format, ...), 6, 7);
A5O_FONT_PRINTFUNC(void, al_draw_justified_textf, (const A5O_FONT *font, A5O_COLOR color, float x1, float x2, float y, float diff, int flags, char const *format, ...), 8, 9);
A5O_FONT_FUNC(int, al_get_text_width, (const A5O_FONT *f, const char *str));
A5O_FONT_FUNC(int, al_get_ustr_width, (const A5O_FONT *f, const A5O_USTR *ustr));
A5O_FONT_FUNC(int, al_get_font_line_height, (const A5O_FONT *f));
A5O_FONT_FUNC(int, al_get_font_ascent, (const A5O_FONT *f));
A5O_FONT_FUNC(int, al_get_font_descent, (const A5O_FONT *f));
A5O_FONT_FUNC(void, al_destroy_font, (A5O_FONT *f));
A5O_FONT_FUNC(void, al_get_ustr_dimensions, (const A5O_FONT *f,
   A5O_USTR const *text,
   int *bbx, int *bby, int *bbw, int *bbh));
A5O_FONT_FUNC(void, al_get_text_dimensions, (const A5O_FONT *f,
   char const *text,
   int *bbx, int *bby, int *bbw, int *bbh));
A5O_FONT_FUNC(bool, al_init_font_addon, (void));
A5O_FONT_FUNC(bool, al_is_font_addon_initialized, (void));
A5O_FONT_FUNC(void, al_shutdown_font_addon, (void));
A5O_FONT_FUNC(uint32_t, al_get_allegro_font_version, (void));
A5O_FONT_FUNC(int, al_get_font_ranges, (A5O_FONT *font,
   int ranges_count, int *ranges));

A5O_FONT_FUNC(void, al_draw_glyph, (const A5O_FONT *font,
   A5O_COLOR color, float x, float y, int codepoint));
A5O_FONT_FUNC(int, al_get_glyph_width, (const A5O_FONT *f,
   int codepoint));
A5O_FONT_FUNC(bool, al_get_glyph_dimensions, (const A5O_FONT *f,
   int codepoint, int *bbx, int *bby, int *bbw, int *bbh));
A5O_FONT_FUNC(int, al_get_glyph_advance, (const A5O_FONT *f,
   int codepoint1, int codepoint2));
#if defined(A5O_UNSTABLE) || defined(A5O_INTERNAL_UNSTABLE) || defined(A5O_FONT_SRC)
A5O_FONT_FUNC(bool, al_get_glyph, (const A5O_FONT *f, int prev_codepoint, int codepoint, A5O_GLYPH *glyph));
#endif

A5O_FONT_FUNC(void, al_draw_multiline_text, (const A5O_FONT *font, A5O_COLOR color, float x, float y, float max_width, float line_height, int flags, const char *text));
A5O_FONT_PRINTFUNC(void, al_draw_multiline_textf, (const A5O_FONT *font, A5O_COLOR color, float x, float y, float max_width, float line_height, int flags, const char *format, ...), 8, 9);
A5O_FONT_FUNC(void, al_draw_multiline_ustr, (const A5O_FONT *font, A5O_COLOR color, float x, float y, float max_width, float line_height, int flags, const A5O_USTR *text));

A5O_FONT_FUNC(void, al_do_multiline_text, (const A5O_FONT *font,
   float max_width, const char *text,
   bool (*cb)(int line_num, const char *line, int size, void *extra),
   void *extra));

A5O_FONT_FUNC(void, al_do_multiline_ustr, (const A5O_FONT *font,
   float max_width, const A5O_USTR *ustr,
   bool (*cb)(int line_num, const A5O_USTR *line, void *extra),
   void *extra));

A5O_FONT_FUNC(void, al_set_fallback_font, (A5O_FONT *font,
   A5O_FONT *fallback));
A5O_FONT_FUNC(A5O_FONT *, al_get_fallback_font, (
   A5O_FONT *font));

#ifdef __cplusplus
   }
#endif

#endif
