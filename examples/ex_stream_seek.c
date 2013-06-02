/*
 *    Example program for the Allegro library, by Todd Cope.
 *
 *    Stream seeking.
 */

#include <stdio.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

ALLEGRO_DISPLAY *display;
ALLEGRO_TIMER *timer;
ALLEGRO_EVENT_QUEUE *queue;
ALLEGRO_FONT *basic_font = NULL;
ALLEGRO_AUDIO_STREAM *music_stream = NULL;
char *stream_filename = NULL;

float slider_pos = 0.0;
float loop_start, loop_end;
int mouse_button[16] = {0};

bool exiting = false;

static void initialize(void)
{
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_init_primitives_addon();
   al_init_image_addon();
   al_init_font_addon();
   if (!al_install_keyboard()) {
      abort_example("Could not init keyboard!\n");
   }
   if (!al_install_mouse()) {
      abort_example("Could not init mouse!\n");
   }
   
   al_init_acodec_addon();

   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }
   if (!al_reserve_samples(16)) {
      abort_example("Could not set up voice and mixer.\n");
   }

   display = al_create_display(640, 228);
   if (!display) {
      abort_example("Could not create display!\n");
   }
   
   basic_font = al_load_font("data/font.tga", 0, 0);
   if (!basic_font) {
      abort_example("Could not load font!\n");
   }
   timer = al_create_timer(1.000 / 30);
   if (!timer) {
      abort_example("Could not init timer!\n");
   }
   queue = al_create_event_queue();
   if (!queue) {
      abort_example("Could not create event queue!\n");
   }
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_timer_event_source(timer));
}

static void logic(void)
{
   /* calculate the position of the slider */
   double w = al_get_display_width(display) - 20;
   double pos = al_get_audio_stream_position_secs(music_stream);
   double len = al_get_audio_stream_length_secs(music_stream);
   slider_pos = w * (pos / len);
}

static void print_time(int x, int y, float t)
{
   int hours, minutes;
   hours = (int)t / 3600;
   t -= hours * 3600;
   minutes = (int)t / 60;
   t -= minutes * 60;
   al_draw_textf(basic_font, al_map_rgb(255, 255, 255), x, y, 0, "%02d:%02d:%05.2f", hours, minutes, t);
}

static void render(void)
{
   double pos = al_get_audio_stream_position_secs(music_stream);
   double length = al_get_audio_stream_length_secs(music_stream);
   double w = al_get_display_width(display) - 20;
   double loop_start_pos = w * (loop_start / length);
   double loop_end_pos = w * (loop_end / length);
   ALLEGRO_COLOR c = al_map_rgb(255, 255, 255);

   al_clear_to_color(al_map_rgb(64, 64, 128));
   
   /* render "music player" */
   al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
   al_draw_textf(basic_font, c, 0, 0, 0, "Playing %s", stream_filename);
   print_time(8, 24, pos);
   al_draw_textf(basic_font, c, 100, 24, 0, "/");
   print_time(110, 24, length);
   al_draw_filled_rectangle(10.0, 48.0 + 7.0, 10.0 + w, 48.0 + 9.0, al_map_rgb(0, 0, 0));
   al_draw_line(10.0 + loop_start_pos, 46.0, 10.0 + loop_start_pos, 66.0, al_map_rgb(0, 168, 128), 0);
   al_draw_line(10.0 + loop_end_pos, 46.0, 10.0 + loop_end_pos, 66.0, al_map_rgb(255, 0, 0), 0);
   al_draw_filled_rectangle(10.0 + slider_pos - 2.0, 48.0, 10.0 + slider_pos + 2.0, 64.0,
      al_map_rgb(224, 224, 224));
   
   /* show help */
   al_draw_textf(basic_font, c, 0, 96, 0, "Drag the slider to seek.");
   al_draw_textf(basic_font, c, 0, 120, 0, "Middle-click to set loop start.");
   al_draw_textf(basic_font, c, 0, 144, 0, "Right-click to set loop end.");
   al_draw_textf(basic_font, c, 0, 168, 0, "Left/right arrows to seek.");
   al_draw_textf(basic_font, c, 0, 192, 0, "Space to pause.");
   
   al_flip_display();
}

static void myexit(void)
{
   bool playing;
   playing = al_get_mixer_playing(al_get_default_mixer());
   if (playing && music_stream)
      al_drain_audio_stream(music_stream);
   al_destroy_audio_stream(music_stream);
}

static void maybe_fiddle_sliders(int mx, int my)
{
   double seek_pos;
   double w = al_get_display_width(display) - 20;

   if (!(mx >= 10 && mx < 10 + w && my >= 48 && my < 64)) {
      return;
   }

   seek_pos = al_get_audio_stream_length_secs(music_stream) * ((mx - 10) / w);
   if (mouse_button[1]) {
      al_seek_audio_stream_secs(music_stream, seek_pos);
   }
   else if (mouse_button[2]) {
      loop_end = seek_pos;
      al_set_audio_stream_loop_secs(music_stream, loop_start, loop_end);
   }
   else if (mouse_button[3]) {
      loop_start = seek_pos;
      al_set_audio_stream_loop_secs(music_stream, loop_start, loop_end);
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
      case ALLEGRO_EVENT_KEY_CHAR:
         if (event->keyboard.keycode == ALLEGRO_KEY_LEFT) {
            double pos = al_get_audio_stream_position_secs(music_stream);
            pos -= 5.0;
            if (pos < 0.0)
               pos = 0.0;
            al_seek_audio_stream_secs(music_stream, pos);
         }
         else if (event->keyboard.keycode == ALLEGRO_KEY_RIGHT) {
            double pos = al_get_audio_stream_position_secs(music_stream);
            pos += 5.0;
            if (!al_seek_audio_stream_secs(music_stream, pos))
               log_printf("seek error!\n");
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
   ALLEGRO_CONFIG *config;
   ALLEGRO_EVENT event;
   unsigned buffer_count;
   unsigned samples;
   const char *s;

   initialize();

   if (argc < 2) {
      log_printf("This example needs to be run from the command line.\nUsage: %s {audio_files}\n", argv[0]);
      goto done;
   }

   buffer_count = 0;
   samples = 0;
   config = al_load_config_file("ex_stream_seek.cfg");
   if (config) {
      if ((s = al_get_config_value(config, "", "buffer_count"))) {
         buffer_count = atoi(s);
      }
      if ((s = al_get_config_value(config, "", "samples"))) {
         samples = atoi(s);
      }
      al_destroy_config(config);
   }
   if (buffer_count == 0) {
      buffer_count = 4;
   }
   if (samples == 0) {
      samples = 1024;
   }

   stream_filename = argv[1];
   music_stream = al_load_audio_stream(stream_filename, buffer_count, samples);
   if (!music_stream) {
      abort_example("Stream error!\n");
   }

   loop_start = 0.0;
   loop_end = al_get_audio_stream_length_secs(music_stream);
   al_set_audio_stream_loop_secs(music_stream, loop_start, loop_end);

   al_set_audio_stream_playmode(music_stream, ALLEGRO_PLAYMODE_LOOP);
   al_attach_audio_stream_to_mixer(music_stream, al_get_default_mixer());
   al_start_timer(timer);

   while (!exiting) {
      al_wait_for_event(queue, &event);
      event_handler(&event);
   }

done:
   myexit();
   al_destroy_display(display);
   close_log(true);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
