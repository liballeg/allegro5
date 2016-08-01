#ifndef __al_included_allegro5_allegro_font_h
#define __al_included_allegro5_allegro_font_h

#include "allegro5/allegro.h"

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
   #define ALLEGRO_FONT_FUNC(type, name, args)      _ALLEGRO_FONT_DLL type __cdecl name args
   #define ALLEGRO_FONT_METHOD(type, name, args)    type (__cdecl *name) args
   #define ALLEGRO_FONT_FUNCPTR(type, name, args)   extern _ALLEGRO_FONT_DLL type (__cdecl *name) args
   #define ALLEGRO_FONT_PRINTFUNC(type, name, args, a, b)  ALLEGRO_FONT_FUNC(type, name, args)
#elif defined ALLEGRO_MINGW32
   #define ALLEGRO_FONT_FUNC(type, name, args)            extern type name args
   #define ALLEGRO_FONT_METHOD(type, name, args)          type (*name) args
   #define ALLEGRO_FONT_FUNCPTR(type, name, args)         extern _ALLEGRO_FONT_DLL type (*name) args
   #define ALLEGRO_FONT_PRINTFUNC(type, name, args, a, b) ALLEGRO_FONT_FUNC(type, name, args) __attribute__ ((format (printf, a, b)))
#elif defined ALLEGRO_BCC32
   #define ALLEGRO_FONT_FUNC(type, name, args)      extern _ALLEGRO_FONT_DLL type name args
   #define ALLEGRO_FONT_METHOD(type, name, args)    type (*name) args
   #define ALLEGRO_FONT_FUNCPTR(type, name, args)   extern _ALLEGRO_FONT_DLL type (*name) args
   #define ALLEGRO_FONT_PRINTFUNC(type, name, args, a, b)    ALLEGRO_FONT_FUNC(type, name, args)
#else
   #define ALLEGRO_FONT_FUNC      AL_FUNC
   #define ALLEGRO_FONT_METHOD    AL_METHOD
   #define ALLEGRO_FONT_FUNCPTR   AL_FUNCPTR
   #define ALLEGRO_FONT_PRINTFUNC AL_PRINTFUNC
#endif


#ifdef __cplusplus
   extern "C" {
#endif


/* Type: ALLEGRO_FONT
*/
typedef struct ALLEGRO_FONT ALLEGRO_FONT;
#if defined(ALLEGRO_UNSTABLE) || defined(ALLEGRO_INTERNAL_UNSTABLE) || defined(ALLEGRO_FONT_SRC)

/* Type: ALLEGRO_GLYPH
*/
typedef struct ALLEGRO_GLYPH ALLEGRO_GLYPH;

struct ALLEGRO_GLYPH
{
   ALLEGRO_BITMAP *bitmap;
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
   ALLEGRO_NO_KERNING       = -1,
   ALLEGRO_ALIGN_LEFT       = 0,
   ALLEGRO_ALIGN_CENTRE     = 1,
   ALLEGRO_ALIGN_CENTER     = 1,
   ALLEGRO_ALIGN_RIGHT      = 2,
   ALLEGRO_ALIGN_INTEGER    = 4,
};

ALLEGRO_FONT_FUNC(bool, al_register_font_loader, (const char *ext, ALLEGRO_FONT *(*load)(const char *filename, int size, int flags)));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_load_bitmap_font, (const char *filename));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_load_bitmap_font_flags, (const char *filename, int flags));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_load_font, (const char *filename, int size, int flags));

ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_grab_font_from_bitmap, (ALLEGRO_BITMAP *bmp, int n, const int ranges[]));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_create_builtin_font, (void));

ALLEGRO_FONT_FUNC(void, al_draw_ustr, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, int flags, ALLEGRO_USTR const *ustr));
ALLEGRO_FONT_FUNC(void, al_draw_text, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, int flags, char const *text));
ALLEGRO_FONT_FUNC(void, al_draw_justified_text, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x1, float x2, float y, float diff, int flags, char const *text));
ALLEGRO_FONT_FUNC(void, al_draw_justified_ustr, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x1, float x2, float y, float diff, int flags, ALLEGRO_USTR const *text));
ALLEGRO_FONT_PRINTFUNC(void, al_draw_textf, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, int flags, char const *format, ...), 6, 7);
ALLEGRO_FONT_PRINTFUNC(void, al_draw_justified_textf, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x1, float x2, float y, float diff, int flags, char const *format, ...), 8, 9);
ALLEGRO_FONT_FUNC(int, al_get_text_width, (const ALLEGRO_FONT *f, const char *str));
ALLEGRO_FONT_FUNC(int, al_get_ustr_width, (const ALLEGRO_FONT *f, const ALLEGRO_USTR *ustr));
ALLEGRO_FONT_FUNC(int, al_get_font_line_height, (const ALLEGRO_FONT *f));
ALLEGRO_FONT_FUNC(int, al_get_font_ascent, (const ALLEGRO_FONT *f));
ALLEGRO_FONT_FUNC(int, al_get_font_descent, (const ALLEGRO_FONT *f));
ALLEGRO_FONT_FUNC(void, al_destroy_font, (ALLEGRO_FONT *f));
ALLEGRO_FONT_FUNC(void, al_get_ustr_dimensions, (const ALLEGRO_FONT *f,
   ALLEGRO_USTR const *text,
   int *bbx, int *bby, int *bbw, int *bbh));
ALLEGRO_FONT_FUNC(void, al_get_text_dimensions, (const ALLEGRO_FONT *f,
   char const *text,
   int *bbx, int *bby, int *bbw, int *bbh));
ALLEGRO_FONT_FUNC(bool, al_init_font_addon, (void));
ALLEGRO_FONT_FUNC(void, al_shutdown_font_addon, (void));
ALLEGRO_FONT_FUNC(uint32_t, al_get_allegro_font_version, (void));
ALLEGRO_FONT_FUNC(int, al_get_font_ranges, (ALLEGRO_FONT *font,
   int ranges_count, int *ranges));

ALLEGRO_FONT_FUNC(void, al_draw_glyph, (const ALLEGRO_FONT *font,
   ALLEGRO_COLOR color, float x, float y, int codepoint));
ALLEGRO_FONT_FUNC(int, al_get_glyph_width, (const ALLEGRO_FONT *f,
   int codepoint));
ALLEGRO_FONT_FUNC(bool, al_get_glyph_dimensions, (const ALLEGRO_FONT *f,
   int codepoint, int *bbx, int *bby, int *bbw, int *bbh));
ALLEGRO_FONT_FUNC(int, al_get_glyph_advance, (const ALLEGRO_FONT *f,
   int codepoint1, int codepoint2));
#if defined(ALLEGRO_UNSTABLE) || defined(ALLEGRO_INTERNAL_UNSTABLE) || defined(ALLEGRO_FONT_SRC)
ALLEGRO_FONT_FUNC(bool, al_get_glyph, (const ALLEGRO_FONT *f, int prev_codepoint, int codepoint, ALLEGRO_GLYPH *glyph));
#endif

ALLEGRO_FONT_FUNC(void, al_draw_multiline_text, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, float max_width, float line_height, int flags, const char *text));
ALLEGRO_FONT_FUNC(void, al_draw_multiline_textf, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, float max_width, float line_height, int flags, const char *format, ...));
ALLEGRO_FONT_FUNC(void, al_draw_multiline_ustr, (const ALLEGRO_FONT *font, ALLEGRO_COLOR color, float x, float y, float max_width, float line_height, int flags, const ALLEGRO_USTR *text));

ALLEGRO_FONT_FUNC(void, al_do_multiline_text, (const ALLEGRO_FONT *font,
   float max_width, const char *text,
   bool (*cb)(int line_num, const char *line, int size, void *extra),
   void *extra));

ALLEGRO_FONT_FUNC(void, al_do_multiline_ustr, (const ALLEGRO_FONT *font,
   float max_width, const ALLEGRO_USTR *ustr,
   bool (*cb)(int line_num, const ALLEGRO_USTR *line, void *extra),
   void *extra));

ALLEGRO_FONT_FUNC(void, al_set_fallback_font, (ALLEGRO_FONT *font,
   ALLEGRO_FONT *fallback));
ALLEGRO_FONT_FUNC(ALLEGRO_FONT *, al_get_fallback_font, (
   ALLEGRO_FONT *font));

#ifdef __cplusplus
   }
#endif

#endif
