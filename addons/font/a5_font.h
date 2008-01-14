#ifndef A5FONT_H
#define A5FONT_H


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
   AL_METHOD(int, font_height, (AL_CONST A5FONT_FONT *f));
   AL_METHOD(int, char_length, (AL_CONST A5FONT_FONT *f, int ch));
   AL_METHOD(int, text_length, (AL_CONST A5FONT_FONT *f, AL_CONST char *text));
   AL_METHOD(int, render_char, (AL_CONST A5FONT_FONT *f, int ch, int x, int y));
   AL_METHOD(void, render, (AL_CONST A5FONT_FONT *f, AL_CONST char *text, int x, int y));
   AL_METHOD(void, destroy, (A5FONT_FONT *f));

   AL_METHOD(int, get_font_ranges, (A5FONT_FONT *f));
   AL_METHOD(int, get_font_range_begin, (A5FONT_FONT *f, int range));
   AL_METHOD(int, get_font_range_end, (A5FONT_FONT *f, int range));
   AL_METHOD(A5FONT_FONT *, extract_font_range, (A5FONT_FONT *f, int begin, int end));
   AL_METHOD(A5FONT_FONT *, merge_fonts, (A5FONT_FONT *f1, A5FONT_FONT *f2));
   AL_METHOD(int, transpose_font, (A5FONT_FONT *f, int drange));
} A5FONT_FONT_VTABLE;


typedef struct A5FONT_FONT_COLOR_DATA
{
   int begin, end;                   /* first char and one-past-the-end char */
   ALLEGRO_BITMAP **bitmaps;         /* our glyphs */
   struct A5FONT_FONT_COLOR_DATA *next;  /* linked list structure */
} A5FONT_FONT_COLOR_DATA;


struct ALLEGRO_BITMAP;

struct A5FONT_FONT_VTABLE;

struct A5FONT_FONT;

AL_FUNC(int, a5font_is_compatible_font, (A5FONT_FONT *f1, A5FONT_FONT *f2));

AL_FUNC(void, a5font_register_font_file_type, (AL_CONST char *ext, A5FONT_FONT *(*load)(AL_CONST char *filename, void *param)));
AL_FUNC(A5FONT_FONT *, a5font_load_bitmap_font, (AL_CONST char *filename, void *param));
AL_FUNC(A5FONT_FONT *, a5font_load_font, (AL_CONST char *filename, void *param));

AL_FUNC(A5FONT_FONT *, a5font_grab_font_from_bitmap, (ALLEGRO_BITMAP *bmp));

AL_FUNC(int, a5font_get_font_ranges, (A5FONT_FONT *f));
AL_FUNC(int, a5font_get_font_range_begin, (A5FONT_FONT *f, int range));
AL_FUNC(int, a5font_get_font_range_end, (A5FONT_FONT *f, int range));
AL_FUNC(A5FONT_FONT *, a5font_extract_font_range, (A5FONT_FONT *f, int begin, int end));
AL_FUNC(A5FONT_FONT *, a5font_merge_fonts, (A5FONT_FONT *f1, A5FONT_FONT *f2));
AL_FUNC(int, a5font_transpose_font, (A5FONT_FONT *f, int drange));

AL_VAR(int, a5font_404_char);
AL_FUNC(void, a5font_textout, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y));
AL_FUNC(void, a5font_textout_centre, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y));
AL_FUNC(void, a5font_textout_right, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x, int y));
AL_FUNC(void, a5font_textout_justify, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str, int x1, int x2, int y, int diff));
AL_PRINTFUNC(void, a5font_textprintf, (AL_CONST struct A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
AL_PRINTFUNC(void, a5font_textprintf_centre, (AL_CONST struct A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
AL_PRINTFUNC(void, a5font_textprintf_right, (AL_CONST struct A5FONT_FONT *f, int x, int y, AL_CONST char *format, ...), 4, 5);
AL_PRINTFUNC(void, a5font_textprintf_justify, (AL_CONST struct A5FONT_FONT *f, int x1, int x2, int y, int diff, AL_CONST char *format, ...), 6, 7);
AL_FUNC(int, a5font_text_length, (AL_CONST struct A5FONT_FONT *f, AL_CONST char *str));
AL_FUNC(int, a5font_text_height, (AL_CONST struct A5FONT_FONT *f));
AL_FUNC(void, a5font_destroy_font, (struct A5FONT_FONT *f));

extern A5FONT_FONT_VTABLE *a5font_font_vtable_color;

AL_FUNC(ALLEGRO_BITMAP *, _a5font_color_find_glyph, (AL_CONST A5FONT_FONT *f, int ch));

AL_FUNC(void, _a5font_register_font_file_type_init, (void));
AL_FUNC(void, a5font_init, (void));


#ifdef __cplusplus
   }
#endif

#endif          /* ifndef A5FONT_H */




