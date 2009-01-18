#include "allegro5/allegro5.h"
#include "allegro5/internal/aintern_memory.h"
#include "allegro5/internal/aintern_vector.h"

#include "allegro5/a5_ttf.h"
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
    _AL_VECTOR cache_bitmaps;
    int cache_pos_x;
    int cache_pos_y;
    int cache_line_height;
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

// FIXME: Add a special case for when a single glyph rendering won't fit
// into 256x256 pixels.
static void push_new_cache_bitmap(ALLEGRO_TTF_FONT_DATA *data)
{
    ALLEGRO_BITMAP **back = _al_vector_alloc_back(&data->cache_bitmaps);
    *back = al_create_bitmap(256, 256);
}

static ALLEGRO_BITMAP* create_glyph_cache(ALLEGRO_FONT const *f, int w,
   int h, bool new)
{
    ALLEGRO_TTF_FONT_DATA *data = f->data;
    ALLEGRO_BITMAP **p_cache;
    ALLEGRO_BITMAP *cache;
    ALLEGRO_BITMAP *ret;

    if (_al_vector_is_empty(&data->cache_bitmaps) || new) {
        push_new_cache_bitmap(data);
        data->cache_pos_x = 0;
        data->cache_pos_y = 0;
    }

    p_cache = _al_vector_ref_back(&data->cache_bitmaps);
    cache = *p_cache;

    if (data->cache_pos_x + w > al_get_bitmap_width(cache)) {
        data->cache_pos_y += data->cache_line_height + 2;
        data->cache_pos_x = 0;
        data->cache_line_height = 0;
    }

    if (data->cache_pos_y + h > al_get_bitmap_height(cache)) {
        return create_glyph_cache(f, w, h, true);
    }
    
    ret = al_create_sub_bitmap(cache, data->cache_pos_x,
        data->cache_pos_y, w, h);
    data->cache_pos_x += w + 2;
    if (h > data->cache_line_height)
        data->cache_line_height = h;
    return ret;
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
        int x, y, w, h;
        ALLEGRO_LOCKED_REGION lr;
        ALLEGRO_STATE backup;

        FT_Load_Glyph(face, ft_index, FT_LOAD_RENDER);
        w = face->glyph->bitmap.width;
        h = face->glyph->bitmap.rows;

        if (w == 0) w = 1;
        if (h == 0) h = 1;

        al_store_state(&backup, ALLEGRO_STATE_BITMAP);

        al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA);
        glyph->bitmap = create_glyph_cache(f, w, h, false);

        al_set_target_bitmap(glyph->bitmap);
        al_clear(al_map_rgba(0, 0, 0, 0));
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
        glyph->y = (face->size->metrics.ascender >> 6) - face->glyph->bitmap_top;

        al_restore_state(&backup);
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

/* Function: al_ttf_get_text_dimensions
 * 
 * Sometimes, the <al_font_text_width> and <al_font_text_height>
 * functions are not enough for exact text placement, so this function
 * returns some additional information.
 * 
 * Returned variables (all in pixel):
 * x, y - Offset to upper left corner of bounding box.
 * w, h - Dimensions of bounding box.
 * ascent - Ascent of the font.
 * descent - Descent of the font.
 * 
 * If the X is the position you specify to draw text, the meaning of
 * ascent and descent and the font height is like in the figure below.
 * Note that glyphs may go to the left and upwards of the X, in which
 * case x and y will have negative values.
 * 
 * > X------------------------
 * >     /\         |        |
 * >    /  \        |        |
 * >   /____\       ascent   |
 * >  /      \      |        |
 * > /        \     |        height
 * > ----------------        |
 * >                |        |
 * >                descent  |
 * >                |        |
 * > -------------------------
 */
void al_ttf_get_text_dimensions(ALLEGRO_FONT const *f, char const *text,
    int count, int *bbx, int *bby, int *bbw, int *bbh, int *ascent,
    int *descent)
{
    ALLEGRO_TTF_FONT_DATA *data = f->data;
    FT_Face face = data->face;
    char prev = '\0';
    char const *p = text;
    int i;
    int x = 0;
    if (count == -1) {
       count = ustrlen(text);
    }
    for (i = 0; i < count; i++) {
        int ch = ugetxc(&p);
        x += render_glyph(f, prev, ch, x, 0, true);
        prev = ch;
    }
    *bbx = 0; // FIXME
    *bby = 0; // FIXME
    *bbw = x;
    *bbh = f->height; // FIXME, we want the bounding box!
    *ascent = face->size->metrics.ascender >> 6;
    *descent = (-face->size->metrics.descender) >> 6;
}

static void destroy(ALLEGRO_FONT *f)
{
    int i;
    ALLEGRO_TTF_FONT_DATA *data = f->data;
    _AL_VECTOR *vec = &data->cache_bitmaps;
    FT_Done_Face(data->face);
    for (i = 0; i < data->glyphs_count; i++) {
        if (data->cache[i].bitmap) {
            al_destroy_bitmap(data->cache[i].bitmap);
        }
    }
    while (_al_vector_is_nonempty(vec)) {
        ALLEGRO_BITMAP **bmp = _al_vector_ref_back(vec);
        al_destroy_bitmap(*bmp);
        _al_vector_delete_at(vec, _al_vector_size(vec)-1);
    }
    _al_vector_free(&data->cache_bitmaps);
    _AL_FREE(data->cache);
    _AL_FREE(data);
    _AL_FREE(f);
}

/* Function: al_ttf_load_font
 */
ALLEGRO_FONT *al_ttf_load_font(char const *filename, int size, int flags)
{
    FT_Face face;
    FT_ULong unicode;
    FT_UInt g, m = 0;
    ALLEGRO_TTF_FONT_DATA *data;
    ALLEGRO_FONT *f;
    int bytes;
    ALLEGRO_PATH *path;
    char extension[32];

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

    if (FT_New_Face(ft, filename, 0, &face) != 0) {
        return NULL;
    }
    /* Small hack for Type1 fonts which store kerning information in
     * a separate file - and we try to guess the name of that file.
     */
    path = al_path_create(filename);
    al_path_get_extension(path, extension, sizeof extension);
    if (!ustrcmp(extension, "pfa")) {
        char helper[PATH_MAX];
        TRACE("a5-ttf: Type1 font assumed for %s.\n", filename);

        al_path_set_extension(path, "afm");
        al_path_to_string(path, helper, sizeof(helper), '/');
        FT_Attach_File(face, helper); 
        TRACE("a5-ttf: Guessed afm file %s.\n", helper);

        al_path_set_extension(path, "tfm");
        al_path_to_string(path, helper, sizeof(helper), '/');
        FT_Attach_File(face, helper); 
        TRACE("a5-ttf: Guessed tfm file %s.\n", helper);
    }
    al_path_free(path);

    if (size > 0) {
       FT_Set_Pixel_Sizes(face, 0, size);
    }
    else {
       /* Set the "real dimension" of the font to be the passed size,
        * in pixels.
        */
       FT_Size_RequestRec req;
       req.type = FT_SIZE_REQUEST_TYPE_REAL_DIM;
       req.width = 0;
       req.height = (-size) << 6;
       req.horiResolution = 0;
       req.vertResolution = 0;
       FT_Request_Size(face, &req);
    }
    
    TRACE("al-ttf: Font %s loaded with pixel size %d.\n", filename,
        size);
    TRACE("al-ttf:     ascent=%.1f, descent=%.1f, height=%.1f\n",
        face->size->metrics.ascender / 64.0,
        face->size->metrics.descender / 64.0,
        face->size->metrics.height / 64.0);

    unicode = FT_Get_First_Char(face, &g);
    while (g) {
        unicode = FT_Get_Next_Char(face, unicode, &g);
        if (g > m) m = g;
    }

    data = _AL_MALLOC(sizeof *data);
    data->face = face;
    data->flags = flags;
    data->glyphs_count = m;
    bytes = (m + 1) * sizeof(ALLEGRO_TTF_GLYPH_DATA);
    data->cache = _AL_MALLOC(bytes);
    memset(data->cache, 0, bytes);
    _al_vector_init(&data->cache_bitmaps, sizeof(ALLEGRO_BITMAP*));
    TRACE("al-ttf: %s: Preparing cache for %d glyphs.\n", filename, m);

    f = _AL_MALLOC(sizeof *f);
    f->height = face->size->metrics.height >> 6;
    
    f->vtable = &vt;
    f->data = data;

    return f;
}
