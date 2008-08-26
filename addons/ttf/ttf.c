#include <allegro5/a5_ttf.h>

#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_ttf_cfg.h"

#include <ft2build.h>
#include FT_FREETYPE_H

typedef struct ALLEGRO_TTF_GLYPH_DATA
{
    ALLEGRO_BITMAP *bitmap;
    int x, y;
} ALLEGRO_TTF_GLYPH_DATA;

typedef struct ALLEGRO_TTF_FONT_DATA
{
    FT_Face face;
    int glyphs_count;
    ALLEGRO_TTF_GLYPH_DATA *cache;
    int flags;
} ALLEGRO_TTF_FONT_DATA;

static bool once = true;
static FT_Library ft;
static ALLEGRO_FONT_VTABLE vt;

static int font_height(ALLEGRO_FONT const *f)
{
   ASSERT(f);
   return f->height;
}

static int render_glyph(ALLEGRO_FONT const *f, int prev, int ch,
    int xpos, int ypos, bool only_measure)
{
    ALLEGRO_TTF_FONT_DATA *data = f->data;
    FT_Face face = data->face;
    int ft_index = FT_Get_Char_Index(face, ch);
    unsigned char *row;
    int startpos = xpos;

    ALLEGRO_TTF_GLYPH_DATA *glyph = data->cache + ft_index;
    if (glyph->bitmap || only_measure) {
        FT_Load_Glyph(face, ft_index, FT_LOAD_DEFAULT);
    }
    else {
        // FIXME: make this a config setting? FT_LOAD_FORCE_AUTOHINT
        FT_Load_Glyph(face, ft_index, FT_LOAD_RENDER);
        int x, y;
        int w = face->glyph->bitmap.width;
        int h = face->glyph->bitmap.rows;

        if (w == 0) w = 1;
        if (h == 0) h = 1;
        
        _al_push_new_bitmap_parameters();
        _al_push_target_bitmap();

        al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
        glyph->bitmap = al_create_bitmap(w, h);

        al_set_target_bitmap(glyph->bitmap);
        al_clear(al_map_rgba(0, 0, 0, 0));
        ALLEGRO_LOCKED_REGION lr;
        al_lock_bitmap(glyph->bitmap, &lr, ALLEGRO_LOCK_WRITEONLY);
        row = face->glyph->bitmap.buffer;
        for (y = 0; y < face->glyph->bitmap.rows; y++) {
            unsigned char *ptr = row;
            for (x = 0; x < face->glyph->bitmap.width; x++) {
                unsigned char c = *ptr;
                al_put_pixel(x, y, al_map_rgba(255, 255, 255, c));
                ptr++;
            }
            row += face->glyph->bitmap.pitch;
        }
        al_unlock_bitmap(glyph->bitmap);
        glyph->x = face->glyph->bitmap_left;
        glyph->y = f->height - face->glyph->bitmap_top;
        
        _al_pop_target_bitmap();
        _al_pop_new_bitmap_parameters();
    }

    /* Do kerning? */
    if (!(data->flags & ALLEGRO_TTF_NO_KERNING) && prev) {
        FT_Vector delta;
        FT_Get_Kerning(face, FT_Get_Char_Index(face, prev), ft_index,
            FT_KERNING_DEFAULT, &delta );
        xpos += delta.x >> 6;
    } 

    if (!only_measure)
        al_draw_bitmap(glyph->bitmap, xpos + glyph->x, ypos + glyph->y , 0);

    xpos += face->glyph->advance.x >> 6;
    return xpos - startpos;
}

static int render_char(ALLEGRO_FONT const *f, int ch, int xpos, int ypos)
{
    return render_glyph(f, '\0', ch, xpos, ypos, false);
}

static int char_length(ALLEGRO_FONT const *f, int ch)
{
    return render_glyph(f, '\0', ch, 0, 0, true);
}

static void render(ALLEGRO_FONT const *f, char const *text,
    int x, int y, int count)
{
    char prev = '\0';
    char const *p = text;
    int i;
    for (i = 0; i < count; i++) {
        int ch = ugetxc(&p);
        x += render_glyph(f, prev, ch, x, y, false);
        prev = ch;
    }
}

static int text_length(ALLEGRO_FONT const *f, char const *text, int count)
{
    char prev = '\0';
    char const *p = text;
    int i;
    int x = 0;
    for (i = 0; i < count; i++) {
        int ch = ugetxc(&p);
        x += render_glyph(f, prev, ch, x, 0, true);
        prev = ch;
    }
    return x;
}

static void destroy(ALLEGRO_FONT *f)
{
    ALLEGRO_TTF_FONT_DATA *data = f->data;
    FT_Done_Face(data->face);
    int i;
    for (i = 0; i < data->glyphs_count; i++) {
        if (data->cache[i].bitmap) {
            al_destroy_bitmap(data->cache[i].bitmap);
        }
    }
    _AL_FREE(data->cache);
    _AL_FREE(data);
    _AL_FREE(f);
}

ALLEGRO_FONT *al_ttf_load_font(char const *filename, int size, int flags)
{
    if (once) {
        FT_Init_FreeType(&ft);
        vt.font_height = font_height;
        vt.char_length = char_length;
        vt.text_length = text_length;
        vt.render_char = render_char;
        vt.render = render;
        vt.destroy = destroy;
        once = false;
    }
    FT_Face face;
    FT_New_Face(ft, filename, 0, &face);
    FT_Set_Pixel_Sizes(face, 0, size);

    FT_UInt g, m = 0;
    FT_ULong unicode = FT_Get_First_Char(face, &g);
    while (g) {
        unicode = FT_Get_Next_Char(face, unicode, &g);
        if (g > m) m = g;
    }

    ALLEGRO_TTF_FONT_DATA *data = _AL_MALLOC(sizeof *data);
    data->face = face;
    data->flags = flags;
    data->glyphs_count = m;
    int bytes = (m + 1) * sizeof(ALLEGRO_TTF_GLYPH_DATA);
    data->cache = _AL_MALLOC(bytes);
    memset(data->cache, 0, bytes);
    TRACE("al-ttf: %s: Preparing cache for %d glyphs.\n", filename, m);

    ALLEGRO_FONT *f = _AL_MALLOC(sizeof *f);
    f->height = size;
    f->vtable = &vt;
    f->data = data;

    return f;
}
