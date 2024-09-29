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

A5O_DISPLAY *display;
A5O_TIMER *timer;
A5O_EVENT_QUEUE *queue;
A5O_FONT *basic_font = NULL;
A5O_AUDIO_STREAM *music_stream = NULL;
const char *stream_filename = "data/welcome.wav";

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

   init_platform_specific();

   display = al_create_display(640, 252);
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
   A5O_COLOR c = al_map_rgb(255, 255, 255);

   al_clear_to_color(al_map_rgb(64, 64, 128));
   
   /* render "music player" */
   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
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
   al_draw_textf(basic_font, c, 0, 216, 0, "R to rewind.");
   
   al_flip_display();
}

static void myexit(void)
{
   bool playing;
   playing = al_get_audio_stream_playing(music_stream);
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
      if (al_set_audio_stream_loop_secs(music_stream, loop_start, seek_pos)) {
         loop_end = seek_pos;
      }
   }
   else if (mouse_button[3]) {
      if (al_set_audio_stream_loop_secs(music_stream, seek_pos, loop_end)) {
         loop_start = seek_pos;
      }
   }
}

static void event_handler(const A5O_EVENT * event)
{
   int i;

   switch (event->type) {
      /* Was the X button on the window pressed? */
      case A5O_EVENT_DISPLAY_CLOSE:
         exiting = true;
         break;

      /* Was a key pressed? */
      case A5O_EVENT_KEY_CHAR:
         if (event->keyboard.keycode == A5O_KEY_LEFT) {
            double pos = al_get_audio_stream_position_secs(music_stream);
            pos -= 5.0;
            if (pos < 0.0)
               pos = 0.0;
            al_seek_audio_stream_secs(music_stream, pos);
         }
         else if (event->keyboard.keycode == A5O_KEY_RIGHT) {
            double pos = al_get_audio_stream_position_secs(music_stream);
            pos += 5.0;
            if (!al_seek_audio_stream_secs(music_stream, pos))
               log_printf("seek error!\n");
         }
         else if (event->keyboard.keycode == A5O_KEY_R) {
            if (!al_rewind_audio_stream(music_stream)) {
               log_printf("rewind error!\n");
            }
         }
         else if (event->keyboard.keycode == A5O_KEY_SPACE) {
            bool playing;
            playing = al_get_audio_stream_playing(music_stream);
            playing = !playing;
            al_set_audio_stream_playing(music_stream, playing);
         }
         else if (event->keyboard.keycode == A5O_KEY_ESCAPE) {
            exiting = true;
         }
         break;

      case A5O_EVENT_MOUSE_BUTTON_DOWN:
         mouse_button[event->mouse.button] = 1;
         maybe_fiddle_sliders(event->mouse.x, event->mouse.y);
         break;
      case A5O_EVENT_MOUSE_AXES:
         maybe_fiddle_sliders(event->mouse.x, event->mouse.y);
         break;
      case A5O_EVENT_MOUSE_BUTTON_UP:
         mouse_button[event->mouse.button] = 0;
         break;
      case A5O_EVENT_MOUSE_LEAVE_DISPLAY:
         for (i = 0; i < 16; i++)
            mouse_button[i] = 0;
         break;

      /* Is it time for the next timer tick? */
      case A5O_EVENT_TIMER:
         logic();
         render();
         break;

      case A5O_EVENT_AUDIO_STREAM_FINISHED:
         log_printf("Stream finished.\n");
         break;
   }
}

int main(int argc, char *argv[])
{
   A5O_CONFIG *config;
   A5O_EVENT event;
   unsigned buffer_count;
   unsigned samples;
   const char *s;
   A5O_PLAYMODE playmode = A5O_PLAYMODE_LOOP;

   initialize();

   if (argc > 1) {
      stream_filename = argv[1];
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
      if ((s = al_get_config_value(config, "", "playmode"))) {
         if (!strcmp(s, "loop")) {
            playmode = A5O_PLAYMODE_LOOP;
         }
         else if (!strcmp(s, "once")) {
            playmode = A5O_PLAYMODE_ONCE;
         }
         else if (!strcmp(s, "loop_once")) {
            playmode = A5O_PLAYMODE_LOOP_ONCE;
         }
      }
      al_destroy_config(config);
   }
   if (buffer_count == 0) {
      buffer_count = 4;
   }
   if (samples == 0) {
      samples = 1024;
   }

   music_stream = al_load_audio_stream(stream_filename, buffer_count, samples);
   if (!music_stream) {
      abort_example("Stream error!\n");
   }
   al_register_event_source(queue, al_get_audio_stream_event_source(music_stream));

   loop_start = 0.0;
   loop_end = al_get_audio_stream_length_secs(music_stream);
   al_set_audio_stream_loop_secs(music_stream, loop_start, loop_end);

   al_set_audio_stream_playmode(music_stream, playmode);
   al_attach_audio_stream_to_mixer(music_stream, al_get_default_mixer());
   al_start_timer(timer);

   while (!exiting) {
      al_wait_for_event(queue, &event);
      event_handler(&event);
   }

   myexit();
   al_destroy_display(display);
   close_log(true);
   return 0;
}

/* vim: set sts=3 sw=3 et: */
