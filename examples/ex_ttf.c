#include "allegro5/a5_ttf.h"

struct Example
{
    double fps;
    ALLEGRO_FONT *f1, *f2, *f3;
} ex;

static void render(void)
{
    ALLEGRO_COLOR white = al_map_rgba_f(1, 1, 1, 1);
    ALLEGRO_COLOR black = al_map_rgba_f(0, 0, 0, 1);
    ALLEGRO_COLOR red = al_map_rgba_f(1, 0, 0, 1);
    ALLEGRO_COLOR green = al_map_rgba_f(0, 0.5, 0, 1);

    al_clear(white);

    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, black);

    al_font_textout(ex.f1, 50,  50, "Tulip (kerning)", -1);
    al_font_textout(ex.f2, 50, 100, "Tulip (no kerning)", -1);
    al_font_textout(ex.f3, 50, 200, "This font has a size of 12 pixels, "
        "the one above has 48 pixels.", -1);

    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, red);
    al_font_textout(ex.f3, 50, 220, "The color can be changed simply "
        "by using a different blender.", -1);
        
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, green);
    al_font_textout(ex.f3, 50, 240, "Some unicode symbols:", -1);
    al_font_textout(ex.f3, 50, 260, "■□▢▣▤▥▦▧▨▩▪▫▬▭▮▯▰▱", -1);
    al_font_textout(ex.f3, 50, 280, "▲△▴▵▶▷▸▹►▻▼▽▾▿◀◁◂◃◄◅◆◇◈◉◊", -1);
    al_font_textout(ex.f3, 50, 300, "○◌◍◎●◐◑◒◓◔◕◖◗◘◙", -1);

    al_font_textout(ex.f3, 50, 320, "«Thís»|you", 6);
    al_font_textout(ex.f3, 50, 340, "‘ìş’|shouldn't", 4);
    al_font_textout(ex.f3, 50, 360, "“cøünt”|see", 7);
    al_font_textout(ex.f3, 50, 380, "réstrïçteđ…|this.", 11);

    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, black);
    al_font_textprintf_right(ex.f3, al_get_display_width(), 0, "%.1f FPS", ex.fps);
}

int main(void)
{
    ALLEGRO_DISPLAY *display;
    ALLEGRO_TIMER *timer;
    ALLEGRO_EVENT_QUEUE *queue;
    int redraw = 0;
    double t = 0;

    if (!al_init()) {
        TRACE("Could not init Allegro.\n");
        return 1;
    }

    al_install_mouse();
    al_font_init();

    display = al_create_display(640, 480);
    al_install_keyboard();

    ex.f1 = al_ttf_load_font("data/DejaVuSans.ttf", 48, 0);
    ex.f2 = al_ttf_load_font("data/DejaVuSans.ttf", 48, ALLEGRO_TTF_NO_KERNING);
    ex.f3 = al_ttf_load_font("data/DejaVuSans.ttf", 12, 0);

    timer = al_install_timer(1.0 / 60);

    queue = al_create_event_queue();
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)timer);

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

        while (redraw > 0 && al_event_queue_is_empty(queue)) {
            double dt;
            redraw--;

            dt = al_current_time();
            render();
            dt = al_current_time() - dt;

            t = 0.99 * t + 0.01 * dt;

            ex.fps = 1.0 / t;
            al_flip_display();
        }
    }
    
    return 0;
}
END_OF_MAIN()

/* vim: set sts=4 sw=4 et: */
