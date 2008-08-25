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
    ALLEGRO_COLOR blue = al_map_rgba_f(0, 0, 1, 1);
    ALLEGRO_COLOR green = al_map_rgba_f(0, 0.5, 0, 1);

    al_clear(white);

    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, black);

    al_font_textout(ex.f1, "Tulip (kerning)", 50, 50);
    al_font_textout(ex.f2, "Tulip (no kerning)", 50, 100);
    al_font_textout(ex.f3, "This font has a size of 12 pixels, "
        "the one above has 48 pixels.", 50, 200);

    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, red);
    al_font_textout(ex.f3, "The color can be changed simply "
        "by using a different blender.", 50, 220);
        
    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, green);
    al_font_textout(ex.f3, "Some unicode symbols:", 50, 240);
    al_font_textout(ex.f3, "■□▢▣▤▥▦▧▨▩▪▫▬▭▮▯▰▱", 50, 260);
    al_font_textout(ex.f3, "▲△▴▵▶▷▸▹►▻▼▽▾▿◀◁◂◃◄◅◆◇◈◉◊", 50, 280);
    al_font_textout(ex.f3, "○◌◍◎●◐◑◒◓◔◕◖◗◘◙", 50, 300);

    al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, black);
    al_font_textprintf_right(ex.f3, al_get_display_width(), 0, "%.1f FPS", ex.fps);
}

int main(void)
{
    ALLEGRO_DISPLAY *display;
    bool redraw = true;
    double t = 0;

    al_init();
    al_install_mouse();
    al_font_init();

    display = al_create_display(640, 480);
    al_install_keyboard();

    al_show_mouse_cursor();

    ex.f1 = al_ttf_load_font("DejaVuSans.ttf", 48, 0);
    ex.f2 = al_ttf_load_font("DejaVuSans.ttf", 48, ALLEGRO_TTF_NO_KERNING);
    ex.f3 = al_ttf_load_font("DejaVuSans.ttf", 12, 0);

    ALLEGRO_TIMER *timer = al_install_timer(1.0 / 60);

    ALLEGRO_EVENT_QUEUE *queue = al_create_event_queue();
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);
    al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)timer);

    al_start_timer(timer);
    while (true) {
        ALLEGRO_EVENT event;
        al_wait_for_event(queue, &event);
        if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) break;
        if (event.type == ALLEGRO_EVENT_KEY_DOWN) break;
        if (event.type == ALLEGRO_EVENT_TIMER) redraw = true;

        if (redraw) {
            redraw = false;
            
            double dt = al_current_time();
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
