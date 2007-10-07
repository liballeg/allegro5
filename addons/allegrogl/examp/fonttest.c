#include <allegro.h>
#include <alleggl.h>

/* Make it compile with Allegro 4.2.0, which is missing make_trans_font. */
#if ALLEGRO_VERSION == 4 && ALLEGRO_SUB_VERSION == 2 && ALLEGRO_WIP_VERSION == 0
static void make_trans_font(FONT *f) {(void)f;}
#endif

#define OPENGL_FORMATS 6
#define FONTS_COUNT 4

int main(void)
{
    /* The fonts. */
    FONT *allegro_font[OPENGL_FORMATS];
    PALETTE pal[FONTS_COUNT];
    FONT *allegrogl_font[FONTS_COUNT * OPENGL_FORMATS];
    char const *font_file[] = {"a32.tga", "a24.tga", "a8.bmp", "a1.bmp"};
    char const *font_name[] = {"32 bit", "24 bit", "8 bit", "1 bit"};
    /* OpenGL texture formats to test. */
    int opengl_format[] = {
        GL_RGBA, GL_RGB, GL_ALPHA, GL_LUMINANCE,
        GL_INTENSITY, GL_LUMINANCE_ALPHA};
    char const *opengl_format_name[][2] = {
        {"RGBA", ""}, {"RGB", ""}, {"ALPHA", ""}, {"LUMINANCE", ""},
        {"INTENSITY", ""}, {"LUMINANCE", "_ALPHA"}
        };
    /* Make our window have just the right size. */
    int w = 80 + OPENGL_FORMATS * 96;
    int h = 32 + FONTS_COUNT * 80;
    int x, y, i;

    allegro_init();
    install_timer();
    install_allegro_gl();
    allegro_gl_set(AGL_COLOR_DEPTH, 32);
    allegro_gl_set(AGL_SUGGEST, AGL_COLOR_DEPTH);
    set_gfx_mode(GFX_OPENGL_WINDOWED, w, h, 0, 0);
    install_keyboard();

    allegro_gl_set_allegro_mode();

    /* Load the fonts. */
    for (i = 0; i < FONTS_COUNT; i++)
    {
        allegro_font[i] = load_font(font_file[i], pal[i], NULL);
        if (!allegro_font[i])
        {
            set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
            allegro_message("Unable to load \"%s\".\n", font_file[i]);
            return -1;
        }
        transpose_font(allegro_font[i], 'a' - ' ');
    }

    make_trans_font(allegro_font[0]);
    set_trans_blender(0, 0, 0, 0);

    /* Convert the fonts to all the format combination we want to test. */
    for (i = 0; i < FONTS_COUNT * OPENGL_FORMATS; i++)
    {
        select_palette(pal[i % FONTS_COUNT]);

        allegrogl_font[i] = allegro_gl_convert_allegro_font_ex(
            allegro_font[i % FONTS_COUNT],
            AGL_FONT_TYPE_TEXTURED, 1, opengl_format[i / FONTS_COUNT]);
    }

    while (!key[KEY_ESC])
    {
        int s = 16;
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        /* Draw background pattern. */
        for (y = 32; y < h; y += s)
        {
            for (x = 80; x < w; x += s)
            {
                int sx = (x / s) & 1;
                int sy = (y / s) & 1;
                float c = sx ^ sy;
                glColor3f(0.2 + c * 0.6, 0.2 + c * 0.6, c);
                glBegin(GL_QUADS);
                glVertex2d(x, y);
                glVertex2d(x + s, y);
                glVertex2d(x + s, y + s);
                glVertex2d(x, y + s);
                glEnd();
            }
        }

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);

        /* Draw left info text. */
        for (y = 0; y < FONTS_COUNT; y++)
        {
            glBlendFunc(GL_ONE, GL_ZERO);
            textprintf_ex(screen, font, 4, 32 + y * 80,
                makecol(0, 0, 0), -1, "%s", font_name[y]);
        }

        /* Columns are texture formats, rows are font formats. */
        i = 0;
        for (x = 0; x < OPENGL_FORMATS; x++)
        {
            int j;
            /* Draw top info text. */
            glBlendFunc(GL_ONE, GL_ZERO);
            for (j = 0; j < 2; j++)
                textprintf_ex(screen, font, 80 + x * 96, 4 + 16 * j,
                    makecol(0, 0, 0), -1, "%s", opengl_format_name[x][j]);

            /* Draw the example glyph for the current cell. */ 
            for (y = 0; y < FONTS_COUNT; y++)
            {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glTranslatef(80 + x * 96, 32 + y * 80, 0);
                glScalef(1, -1, 1);
                glColor4f(1, 1, 1, 1);
                allegro_gl_printf_ex(allegrogl_font[i++], 0, 0, 0, "a");
                glPopMatrix();

            }
        }

        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);

        allegro_gl_flip();
        /* Update about 60 times / second. */
        rest(1000 / 60);
    }
    return 0;
}
END_OF_MAIN()
