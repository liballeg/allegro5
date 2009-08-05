/*
 *    Example program for the Allegro library, by Todd Cope.
 *
 *    Stream seeking.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_flac.h"
#include "allegro5/a5_vorbis.h"
#include "allegro5/kcm_audio.h"
#include "allegro5/a5_primitives.h"

ALLEGRO_DISPLAY *display;
ALLEGRO_TIMER *timer;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_FONT *basic_font = NULL;
ALLEGRO_STREAM *music_stream = NULL;
char *stream_filename = NULL;

float slider_pos = 0.0;
float loop_start, loop_end;
int mouse_button[16] = {0};

bool exiting = false;

static int initialize(void)
{
   al_init();

   al_init_font_addon();
   if (!al_install_keyboard()) {
      printf("Could not init keyboard!\n");
      return 0;
   }
   if (!al_install_mouse()) {
      printf("Could not init mouse!\n");
      return 0;
   }
   al_init_ogg_vorbis_addon();
   al_init_flac_addon();
   if (!al_install_audio(ALLEGRO_AUDIO_DRIVER_AUTODETECT)) {
      printf("Could not init sound!\n");
      return 0;
   }
   if (!al_reserve_samples(16)) {
      printf("Could not set up voice and mixer.\n");
      return 0;
   }

   display = al_create_display(640, 180);
   if (!display) {
      printf("Could not create display!\n");
      return 0;
   }
   
   basic_font = al_load_font("data/font.tga", 0, 0);
   if (!basic_font) {
      printf("Could not load font!\n");
      return 0;
   }
   timer = al_install_timer(1.000 / 30);
   if (!timer) {
      printf("Could not init timer!\n");
      return 0;
   }
   queue = al_create_event_queue();
   if (!queue) {
      printf("Could not create event queue!\n");
      return 0;
   }
   al_register_event_source(queue, al_get_keyboard_event_source(al_get_keyboard()));
   al_register_event_source(queue, al_get_mouse_event_source(al_get_mouse()));
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));

   return 1;
}

static void logic(void)
{
   /* calculate the position of the slider */
   double w = al_get_display_width() - 20;
   double pos = al_get_stream_position_secs(music_stream);
   double len = al_get_stream_length_secs(music_stream);
   slider_pos = w * (pos / len);
}

static void print_time(int x, int y, float t)
{
   int hours, minutes;
   hours = (int)t / 3600;
   t -= hours * 3600;
   minutes = (int)t / 60;
   t -= minutes * 60;
   al_draw_textf(basic_font, x, y, 0, "%02d:%02d:%05.2f", hours, minutes, t);
}

static void render(void)
{
   double pos = al_get_stream_position_secs(music_stream);
   double length = al_get_stream_length_secs(music_stream);
   double w = al_get_display_width() - 20;
   double loop_start_pos = w * (loop_start / length);
   double loop_end_pos = w * (loop_end / length);

   al_clear_to_color(al_map_rgb(64, 64, 128));
   
   /* render "music player" */
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(255, 255, 255));
   al_draw_textf(basic_font, 0, 0, 0, "Playing %s", stream_filename);
   print_time(8, 24, pos);
   al_draw_textf(basic_font, 100, 24, 0, "/");
   print_time(110, 24, length);
   al_draw_filled_rectangle(10.0, 48.0 + 7.0, 10.0 + w, 48.0 + 9.0, al_map_rgb(0, 0, 0));
   al_draw_line(10.0 + loop_start_pos, 46.0, 10.0 + loop_start_pos, 66.0, al_map_rgb(255, 0, 0), 0);
   al_draw_line(10.0 + loop_end_pos, 46.0, 10.0 + loop_end_pos, 66.0, al_map_rgb(255, 0, 0), 0);
   al_draw_filled_rectangle(10.0 + slider_pos - 2.0, 48.0, 10.0 + slider_pos + 2.0, 64.0,
      al_map_rgb(224, 224, 224));
   
   /* show help */
   al_draw_textf(basic_font, 0, 96, 0, "Drag the slider to seek.");
   al_draw_textf(basic_font, 0, 120, 0, "Middle-click to set loop start.");
   al_draw_textf(basic_font, 0, 144, 0, "Right-click to set loop end.");
   
   al_flip_display();
}

static void myexit(void)
{
   bool playing;
   playing = al_get_mixer_playing(al_get_default_mixer());
   if (playing)
      al_drain_stream(music_stream);
   al_destroy_stream(music_stream);
}

static void maybe_fiddle_sliders(int mx, int my)
{
   double seek_pos;
   double w = al_get_display_width() - 20;

   if (!(mx >= 10 && mx < 10 + w && my >= 48 && my < 64)) {
      return;
   }

   seek_pos = al_get_stream_length_secs(music_stream) * ((mx - 10) / w);
   if (mouse_button[1]) {
      al_seek_stream_secs(music_stream, seek_pos);
   }
   else if (mouse_button[2]) {
      loop_end = seek_pos;
      al_set_stream_loop_secs(music_stream, loop_start, loop_end);
   }
   else if (mouse_button[3]) {
      loop_start = seek_pos;
      al_set_stream_loop_secs(music_stream, loop_start, loop_end);
   }
}

static void event_handler(const ALLEGRO_EVENT * event)
{
   int i;

   switch (event->type) {
      /* Was the X button on the window pressed? */
      case ALLEGRO_EVENT_DISPLAY_CLOSE:
         exiting = true;
         break;

      /* Was a key pressed? */
      case ALLEGRO_EVENT_KEY_DOWN:
      case ALLEGRO_EVENT_KEY_REPEAT:
         if (event->keyboard.keycode == ALLEGRO_KEY_LEFT) {
            double pos = al_get_stream_position_secs(music_stream);
            pos -= 5.0;
            if (pos < 0.0)
               pos = 0.0;
            al_seek_stream_secs(music_stream, pos);
         }
         else if (event->keyboard.keycode == ALLEGRO_KEY_RIGHT) {
            double pos = al_get_stream_position_secs(music_stream);
            pos += 5.0;
            if (!al_seek_stream_secs(music_stream, pos))
               printf("seek error!\n");
         }
         else if (event->keyboard.keycode == ALLEGRO_KEY_SPACE) {
            bool playing;
            playing = al_get_mixer_playing(al_get_default_mixer());
            playing = !playing;
            al_set_mixer_playing(al_get_default_mixer(), playing);
         }
         else if (event->keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
            exiting = true;
         }
         break;

      case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
         mouse_button[event->mouse.button] = 1;
         maybe_fiddle_sliders(event->mouse.x, event->mouse.y);
         break;
      case ALLEGRO_EVENT_MOUSE_AXES:
         maybe_fiddle_sliders(event->mouse.x, event->mouse.y);
         break;
      case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
         mouse_button[event->mouse.button] = 0;
         break;
      case ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY:
         for (i = 0; i < 16; i++)
            mouse_button[i] = 0;
         break;

      /* Is it time for the next timer tick? */
      case ALLEGRO_EVENT_TIMER:
         logic();
         render();
         break;
   }
}

int main(int argc, char * argv[])
{
   ALLEGRO_EVENT event;

   if (argc < 2) {
      printf("Usage: ex_stream_seek <filename>\n");
      return -1;
   }

   if (!initialize())
      return 1;

   stream_filename = argv[1];
   music_stream = al_stream_from_file(stream_filename, 4, 4096);
   if (!music_stream) {
      printf("Stream error!\n");
      return 1;
   }

   loop_start = 0.0;
   loop_end = al_get_stream_length_secs(music_stream);
   al_set_stream_playmode(music_stream, ALLEGRO_PLAYMODE_LOOP);
   al_attach_stream_to_mixer(music_stream, al_get_default_mixer());
   al_start_timer(timer);

   while (!exiting) {
      al_wait_for_event(queue, &event);
      event_handler(&event);
   }

   myexit();
   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
