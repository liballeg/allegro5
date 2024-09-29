#define A5O_UNSTABLE

#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>

#include "common.c"

#define MAX_RANGES  256

const char *font_file = "data/DejaVuSans.ttf";

struct Example
{
    double fps;
    A5O_FONT *f1, *f2, *f3, *f4, *f5, *f6;
    A5O_FONT *f_alex;
    A5O_CONFIG *config;
    int ranges_count;
} ex;

static void print_ranges(A5O_FONT *f)
{
    int ranges[MAX_RANGES * 2];
    int count;
    int i;

    count = al_get_font_ranges(f, MAX_RANGES, ranges);
    for (i = 0; i < count; i++) {
        int begin = ranges[i * 2];
        int end = ranges[i * 2 + 1];
        log_printf("range %3d: %08x-%08x (%d glyph%s)\n", i, begin, end,
            1 + end - begin, begin == end ? "" : "s");
    }
}

static const char *get_string(const char *key)
{
    const char *v = al_get_config_value(ex.config, "", key);
    return (v) ? v : key;
}

static int ustr_at(A5O_USTR *string, int index)
{
   return al_ustr_get(string, al_ustr_offset(string, index));
}

static void render(void)
{
    A5O_COLOR white = al_map_rgba_f(1, 1, 1, 1);
    A5O_COLOR black = al_map_rgba_f(0, 0, 0, 1);
    A5O_COLOR red = al_map_rgba_f(1, 0, 0, 1);
    A5O_COLOR green = al_map_rgba_f(0, 0.5, 0, 1);
    A5O_COLOR blue = al_map_rgba_f(0.1, 0.2, 1, 1);
    A5O_COLOR purple = al_map_rgba_f(0.3, 0.1, 0.2, 1);
    int x, y, w, h, as, de, xpos, ypos;
    unsigned int index;
    int target_w, target_h;
    A5O_USTR_INFO info, sub_info;
    const A5O_USTR *u;
    A5O_USTR *tulip = al_ustr_new("Tulip");
    A5O_USTR *dimension_text = al_ustr_new("Tulip");
    A5O_USTR *vertical_text  = al_ustr_new("Rose.");
    A5O_USTR *dimension_label = al_ustr_new("(dimensions)");
    int prev_cp = -1;

    al_clear_to_color(white);

    al_hold_bitmap_drawing(true);

    al_draw_textf(ex.f1, black, 50,  20, 0, "Tulip (kerning)");
    al_draw_textf(ex.f2, black, 50,  80, 0, "Tulip (no kerning)");

    x = 50;
    y = 140;
    for (index = 0; index < al_ustr_length(dimension_text); index ++) {
       int cp  = ustr_at(dimension_text, index);
       int bbx, bby, bbw, bbh;
       al_get_glyph_dimensions(ex.f2, cp, &bbx, &bby, &bbw, &bbh);
       al_draw_rectangle(x + bbx + 0.5, y + bby + 0.5, x + bbx + bbw - 0.5, y + bby + bbh - 0.5, blue, 1);
       al_draw_rectangle(x + 0.5, y + 0.5, x + bbx + bbw - 0.5, y + bby + bbh - 0.5, green, 1);
       al_draw_glyph(ex.f2, purple, x, y, cp);
       x += al_get_glyph_advance(ex.f2, cp, A5O_NO_KERNING);
    }
    al_draw_line(50.5, y+0.5, x+0.5, y+0.5, red, 1);

    for (index = 0; index < al_ustr_length(dimension_label); index++) {
       int cp = ustr_at(dimension_label, index);
       A5O_GLYPH g;
       if (al_get_glyph(ex.f2, prev_cp, cp, &g)) {
          al_draw_tinted_bitmap_region(g.bitmap, black, g.x, g.y, g.w, g.h, x + 10 + g.kerning + g.offset_x, y + g.offset_y, 0);
          x += g.advance;
       }
       prev_cp = cp;
    }

    al_draw_textf(ex.f3, black, 50, 200, 0, "This font has a size of 12 pixels, "
        "the one above has 48 pixels.");

    al_hold_bitmap_drawing(false);
    al_hold_bitmap_drawing(true);

    al_draw_textf(ex.f3, red, 50, 220, 0, "The color can simply be changed.🐊← fallback glyph");

    al_hold_bitmap_drawing(false);
    al_hold_bitmap_drawing(true);

    al_draw_textf(ex.f3, green, 50, 240, 0, "Some unicode symbols:");
    al_draw_textf(ex.f3, green, 50, 260, 0, "%s", get_string("symbols1"));
    al_draw_textf(ex.f3, green, 50, 280, 0, "%s", get_string("symbols2"));
    al_draw_textf(ex.f3, green, 50, 300, 0, "%s", get_string("symbols3"));

   #define OFF(x) al_ustr_offset(u, x)
   #define SUB(x, y) al_ref_ustr(&sub_info, u, OFF(x), OFF(y))
    u = al_ref_cstr(&info, get_string("substr1"));
    al_draw_ustr(ex.f3, green, 50, 320, 0, SUB(0, 6));
    u = al_ref_cstr(&info, get_string("substr2"));
    al_draw_ustr(ex.f3, green, 50, 340, 0, SUB(7, 11));
    u = al_ref_cstr(&info, get_string("substr3"));
    al_draw_ustr(ex.f3, green, 50, 360, 0, SUB(4, 11));
    u = al_ref_cstr(&info, get_string("substr4"));
    al_draw_ustr(ex.f3, green, 50, 380, 0, SUB(0, 11));

    al_draw_textf(ex.f5, black, 50, 395, 0, "forced monochrome");

    A5O_TRANSFORM t;
    al_identity_transform(&t);
    al_rotate_transform(&t, al_get_time());
    al_translate_transform(&t, 550, 300);
    al_use_transform(&t);
    al_draw_textf(ex.f6, black, 0, -al_get_font_line_height(ex.f6) / 2,
            A5O_ALIGN_CENTRE, "T");
    al_identity_transform(&t);
    al_use_transform(&t);

    /* Glyph rendering tests. */
    al_draw_textf(ex.f3, red, 50, 410, 0, "Glyph adv Tu: %d, draw: ",
                        al_get_glyph_advance(ex.f3, 'T', 'u'));
    x = 50;
    y = 425;
    for (index = 0; index < al_ustr_length(tulip); index ++) {
       int cp  = ustr_at(tulip, index);
       /* Use al_get_glyph_advance for the stride, with no kerning. */
       al_draw_glyph(ex.f3, red, x, y, cp);
       x += al_get_glyph_advance(ex.f3, cp, A5O_NO_KERNING);
    }

    x = 50;
    y = 440;
    /* First draw a red string using al_draw_text, that should be hidden
     * completely by the same text drawing in green per glyph
     * using al_draw_glyph and al_get_glyph_advance below. */
    al_draw_ustr(ex.f3, red, x, y, 0, tulip);
    for (index = 0; index < al_ustr_length(tulip); index ++) {
      int cp  = ustr_at(tulip, index);
      int ncp = (index < (al_ustr_length(tulip) - 1)) ?
         ustr_at(tulip, index + 1) : A5O_NO_KERNING;
      /* Use al_get_glyph_advance for the stride and apply kerning. */
      al_draw_glyph(ex.f3, green, x, y, cp);
      x += al_get_glyph_advance(ex.f3, cp, ncp);
    }

    x = 50;
    y = 466;
    al_draw_ustr(ex.f3, red, x, y, 0, tulip);
    for (index = 0; index < al_ustr_length(tulip); index ++) {
      int cp  = ustr_at(tulip, index);
      int bbx, bby, bbw, bbh;
      al_get_glyph_dimensions(ex.f3, cp, &bbx, &bby, &bbw, &bbh);
      al_draw_glyph(ex.f3, blue, x, y, cp);
      x += bbx + bbw;
    }


    x = 10;
    y = 30;
    for (index = 0; index < al_ustr_length(vertical_text); index ++) {
      int bbx, bby, bbw, bbh;
      int cp  = ustr_at(vertical_text, index);
      /* Use al_get_glyph_dimensions for the height to apply. */
      al_get_glyph_dimensions(ex.f3, cp, &bbx, &bby, &bbw, &bbh);
      al_draw_glyph(ex.f3, green, x, y, cp);
      y += bby;
      y += bbh;
    }


    x = 30;
    y = 30;
    for (index = 0; index < al_ustr_length(vertical_text); index ++) {
      int bbx, bby, bbw, bbh;
      int cp  = ustr_at(vertical_text, index);
      /* Use al_get_glyph_dimensions for the height to apply, here bby is
       * omited for the wrong result. */
      al_get_glyph_dimensions(ex.f3, cp, &bbx, &bby, &bbw, &bbh);
      al_draw_glyph(ex.f3, red, x, y, cp);
      y += bbh;
    }


    al_hold_bitmap_drawing(false);

    target_w = al_get_bitmap_width(al_get_target_bitmap());
    target_h = al_get_bitmap_height(al_get_target_bitmap());

    xpos = target_w - 10;
    ypos = target_h - 10;
    al_get_text_dimensions(ex.f4, "Allegro", &x, &y, &w, &h);
    as = al_get_font_ascent(ex.f4);
    de = al_get_font_descent(ex.f4);
    xpos -= w;
    ypos -= h;
    x += xpos;
    y += ypos;

    al_draw_rectangle(x, y, x + w - 0.5, y + h - 0.5, black, 0);
    al_draw_line(x+0.5, y + as + 0.5, x + w - 0.5, y + as + 0.5, black, 0);
    al_draw_line(x + 0.5, y + as + de + 0.5, x + w - 0.5, y + as + de + 0.5, black, 0);

    al_hold_bitmap_drawing(true);
    al_draw_textf(ex.f4, blue, xpos, ypos, 0, "Allegro");
    al_hold_bitmap_drawing(false);

    al_hold_bitmap_drawing(true);

    al_draw_textf(ex.f3, black, target_w, 0, A5O_ALIGN_RIGHT,
       "%.1f FPS", ex.fps);

    al_draw_textf(ex.f3, black, 0, 0, 0, "%s: %d unicode ranges", font_file,
       ex.ranges_count);

    al_hold_bitmap_drawing(false);
}

int main(int argc, char **argv)
{
    A5O_DISPLAY *display;
    A5O_TIMER *timer;
    A5O_EVENT_QUEUE *queue;
    int redraw = 0;
    double t = 0;

    if (!al_init()) {
        abort_example("Could not init Allegro.\n");
    }

    open_log_monospace();

    al_init_primitives_addon();
    al_install_mouse();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_image_addon();
    init_platform_specific();

#ifdef A5O_IPHONE
    al_set_new_display_flags(A5O_FULLSCREEN_WINDOW);
#endif
    display = al_create_display(640, 480);
    if (!display) {
        abort_example("Could not create display.\n");
    }
    al_install_keyboard();

    if (argc >= 2) {
        font_file = argv[1];
    }

    ex.f1 = al_load_font(font_file, 48, 0);
    ex.f2 = al_load_font(font_file, 48, A5O_TTF_NO_KERNING);
    ex.f3 = al_load_font(font_file, 12, 0);
    /* Specifying negative values means we specify the glyph height
     * in pixels, not the usual font size.
     */
    ex.f4 = al_load_font(font_file, -140, 0);
    ex.f5 = al_load_font(font_file, 12, A5O_TTF_MONOCHROME);

    al_set_new_bitmap_flags(A5O_MIN_LINEAR | A5O_MAG_LINEAR);
    ex.f6 = al_load_font(font_file, -140, 0);
    al_set_new_bitmap_flags(0);

    {
        int ranges[] = {0x1F40A, 0x1F40A};
        A5O_BITMAP *icon = al_load_bitmap("data/icon.png");
        if (!icon) {
            abort_example("Couldn't load data/icon.png.\n");
        }
        A5O_BITMAP *glyph = al_create_bitmap(50, 50);
        al_set_target_bitmap(glyph);
        al_clear_to_color(al_map_rgba_f(0, 0, 0, 0));
        al_draw_rectangle(0.5, 0.5, 49.5, 49.5, al_map_rgb_f(1, 1, 0),
         1);
        al_draw_bitmap(icon, 1, 1, 0);
        al_set_target_backbuffer(display);
        ex.f_alex = al_grab_font_from_bitmap(glyph, 1, ranges);
    }

    if (!ex.f1 || !ex.f2 || !ex.f3 || !ex.f4 || !ex.f_alex) {
        abort_example("Could not load font: %s\n", font_file);
    }

    al_set_fallback_font(ex.f3, ex.f_alex);

    ex.ranges_count = al_get_font_ranges(ex.f1, 0, NULL);
    print_ranges(ex.f1);

    ex.config = al_load_config_file("data/ex_ttf.ini");
    if (!ex.config) {
        abort_example("Could not data/ex_ttf.ini\n");
    }

    timer = al_create_timer(1.0 / 60);

    queue = al_create_event_queue();
    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(timer));

    al_start_timer(timer);
    while (true) {
        A5O_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == A5O_EVENT_DISPLAY_CLOSE)
            break;
        if (event.type == A5O_EVENT_KEY_DOWN &&
                event.keyboard.keycode == A5O_KEY_ESCAPE) {
            break;
        }
        if (event.type == A5O_EVENT_TIMER)
            redraw++;

        while (redraw > 0 && al_is_event_queue_empty(queue)) {
            double dt;
            redraw--;

            dt = al_get_time();
            render();
            dt = al_get_time() - dt;

            t = 0.99 * t + 0.01 * dt;

            ex.fps = 1.0 / t;
            al_flip_display();
        }
    }

    al_destroy_font(ex.f1);
    al_destroy_font(ex.f2);
    al_destroy_font(ex.f3);
    al_destroy_font(ex.f4);
    al_destroy_font(ex.f5);
    al_destroy_config(ex.config);

    close_log(false);

    return 0;
}

/* vim: set sts=4 sw=4 et: */
