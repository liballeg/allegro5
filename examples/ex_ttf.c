#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#include "common.c"

#define MAX_RANGES  256

const char *font_file = "data/DejaVuSans.ttf";

struct Example
{
    double fps;
    ALLEGRO_FONT *f1, *f2, *f3, *f4, *f5;
    ALLEGRO_CONFIG *config;
    int ranges_count;
} ex;

static void print_ranges(ALLEGRO_FONT *f)
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

static void render(void)
{
    ALLEGRO_COLOR white = al_map_rgba_f(1, 1, 1, 1);
    ALLEGRO_COLOR black = al_map_rgba_f(0, 0, 0, 1);
    ALLEGRO_COLOR red = al_map_rgba_f(1, 0, 0, 1);
    ALLEGRO_COLOR green = al_map_rgba_f(0, 0.5, 0, 1);
    ALLEGRO_COLOR blue = al_map_rgba_f(0.1, 0.2, 1, 1);
    int x, y, w, h, as, de, xpos, ypos;
    unsigned int index;
    int target_w, target_h;
    ALLEGRO_USTR_INFO info, sub_info;
    const ALLEGRO_USTR *u;
    const char *tulip = "Tulip";

    al_clear_to_color(white);

    al_hold_bitmap_drawing(true);
   
    al_draw_textf(ex.f1, black, 50,  50, 0, "Tulip (kerning)");
    al_draw_textf(ex.f2, black, 50, 100, 0, "Tulip (no kerning)");
    al_draw_textf(ex.f3, black, 50, 200, 0, "This font has a size of 12 pixels, "
        "the one above has 48 pixels.");

    al_hold_bitmap_drawing(false);
    al_hold_bitmap_drawing(true);

    al_draw_textf(ex.f3, red, 50, 220, 0, "The color can simply be changed.");
        
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

    /* Glyph rendering tests. */
    al_draw_textf(ex.f3, red, 50, 410, 0, "Glyph kern Tu: %d, draw: ",
                        al_get_glyph_kerning(ex.f3, 'T', 'u'));
    x = 50 ; y = 425; 
    for (index = 0; index < strlen(tulip); index ++) {
       int cp  = tulip[index];
       /* Use al_get_glyph_width for the stride, with no kerning. */
       int width = al_get_glyph_width(ex.f3, cp);
       al_draw_glyph(ex.f3, red, x, y, cp);
       x += width;
    }
    
    x = 50 ; y = 440; 
    for (index = 0; index < strlen(tulip); index ++) {
       int cp  = tulip[index];
       int pcp = (index < 1) ? -1 : tulip[index - 1];
       /* Use al_get_glyph_width for the stride and apply kerning. */
       x += al_get_glyph_kerning(ex.f3, pcp, cp);
       int width = al_get_glyph_width(ex.f3, cp);
       al_draw_glyph(ex.f3, red, x, y, cp);
       x += width;
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

    al_draw_rectangle(x, y, x + w, y + h, black, 0);
    al_draw_line(x, y + as, x + w, y + as, black, 0);
    al_draw_line(x, y + as + de, x + w, y + as + de, black, 0);

    al_hold_bitmap_drawing(true);
    al_draw_textf(ex.f4, blue, xpos, ypos, 0, "Allegro");
    al_hold_bitmap_drawing(false);

    al_hold_bitmap_drawing(true);

    al_draw_textf(ex.f3, black, target_w, 0, ALLEGRO_ALIGN_RIGHT,
       "%.1f FPS", ex.fps);

    al_draw_textf(ex.f3, black, 0, 0, 0, "%s: %d unicode ranges", font_file,
       ex.ranges_count);
       
    al_hold_bitmap_drawing(false);
}

int main(int argc, char **argv)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT_QUEUE *queue;
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
    init_platform_specific();

#ifdef ALLEGRO_IPHONE
    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
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
    ex.f2 = al_load_font(font_file, 48, ALLEGRO_TTF_NO_KERNING);
    ex.f3 = al_load_font(font_file, 12, 0);
    /* Specifying negative values means we specify the glyph height
     * in pixels, not the usual font size.
     */
    ex.f4 = al_load_font(font_file, -140, 0);
    ex.f5 = al_load_font(font_file, 12, ALLEGRO_TTF_MONOCHROME);

    if (!ex.f1 || !ex.f2 || !ex.f3 || !ex.f4) {
        abort_example("Could not load font: %s\n", font_file);
    }

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
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE)
            break;
        if (event.type == ALLEGRO_EVENT_KEY_DOWN &&
                event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            break;
        }
        if (event.type == ALLEGRO_EVENT_TIMER)
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
