/*
 *    Example program for the Allegro library, by Todd Cope.
 *
 *    Stream seeking.
 */

#include <allegro5/allegro5.h>
#include <allegro5/a5_font.h>
#include <allegro5/kcm_audio.h>
#include <allegro5/acodec.h>
#include <allegro5/a5_primitives.h>
#include <stdio.h>

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

int initialize(void)
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
   if (al_install_audio(ALLEGRO_AUDIO_DRIVER_AUTODETECT)) {
      printf("Could not init sound!\n");
      return 0;
   }
   if (!al_reserve_samples(16)) {
      printf("Could not set up voice and mixer.\n");
      return 0;
   }

   display = al_create_display(320, 180);
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
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_keyboard());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)al_get_mouse());
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)display);
   al_register_event_source(queue, (ALLEGRO_EVENT_SOURCE *)timer);

   return 1;
}

void logic(void)
{
   /* calculate the position of the slider */
   slider_pos = 300.0 * (al_get_stream_position(music_stream) / al_get_stream_length(music_stream));
}

void render(void)
{
   double pos = al_get_stream_position(music_stream);
   double length = al_get_stream_length(music_stream);
   double loop_start_pos = 300.0 * (loop_start / al_get_stream_length(music_stream));
   double loop_end_pos = 300.0 * (loop_end / al_get_stream_length(music_stream));

   al_clear(al_map_rgb(64, 64, 128));
   
   /* render "music player" */
   al_set_blender(ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA, al_map_rgb(255, 255, 255));
   al_font_textprintf(basic_font, 0, 0, "Playing %s", stream_filename);
   al_font_textprintf(basic_font, 0, 24, "Pos = %f / %f", pos, length);
   al_draw_filled_rectangle(10.0, 48.0 + 7.0, 310.0, 48.0 + 9.0, al_map_rgb(0, 0, 0));
   al_draw_line(10.0 + loop_start_pos, 46.0, 10.0 + loop_start_pos, 66.0, al_map_rgb(255, 0, 0), 0);
   al_draw_line(10.0 + loop_end_pos, 46.0, 10.0 + loop_end_pos, 66.0, al_map_rgb(255, 0, 0), 0);
   al_draw_filled_rectangle(10.0 + slider_pos - 2.0, 48.0, 10.0 + slider_pos + 2.0, 64.0, al_map_rgb(224, 224, 224));
   
   /* show help */
   al_font_textprintf(basic_font, 0, 96, "Drag the slider to seek.");
   al_font_textprintf(basic_font, 0, 120, "Middle-click to set loop start.");
   al_font_textprintf(basic_font, 0, 144, "Right-click to set loop end.");
   
   al_flip_display();
}

void myexit(void)
{
   bool playing;
   al_get_mixer_bool(al_get_default_mixer(), ALLEGRO_AUDIOPROP_PLAYING,
      &playing);
   if (playing)
      al_drain_stream(music_stream);
   al_destroy_stream(music_stream);
}

void maybe_fiddle_sliders(int mx, int my)
{
   float seek_pos;

   if (!(mx >= 10 && mx < 310 && my >= 48 && my < 64)) {
      return;
   }

   if (mouse_button[1]) {
      seek_pos = al_get_stream_length(music_stream) * ((mx - 10) / 300.0);
      al_seek_stream(music_stream, seek_pos);
   }
   else if (mouse_button[2]) {
      loop_end = al_get_stream_length(music_stream) * ((mx - 10) / 300.0);
      al_set_stream_loop(music_stream, loop_start, loop_end);
   }
   else if (mouse_button[3]) {
      loop_start = al_get_stream_length(music_stream) * ((mx - 10) / 300.0);
      al_set_stream_loop(music_stream, loop_start, loop_end);
   }
}

void event_handler(const ALLEGRO_EVENT * event)
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
            double pos = al_get_stream_position(music_stream);
            pos -= 5.0;
            if (pos < 0.0)
               pos = 0.0;
            al_seek_stream(music_stream, pos);
         }
         else if (event->keyboard.keycode == ALLEGRO_KEY_RIGHT) {
            double pos = al_get_stream_position(music_stream);
            pos += 5.0;
            if (!al_seek_stream(music_stream, pos))
               printf("seek error!\n");
         }
         else if (event->keyboard.keycode == ALLEGRO_KEY_SPACE) {
            bool playing;
            al_get_mixer_bool(al_get_default_mixer(),
               ALLEGRO_AUDIOPROP_PLAYING, &playing);
            playing = !playing;
            al_set_mixer_bool(al_get_default_mixer(),
               ALLEGRO_AUDIOPROP_PLAYING, playing);
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
   music_stream = al_stream_from_file(4, 4096, stream_filename);
   if (!music_stream) {
      printf("Stream error!\n");
      return 1;
   }

   loop_start = 0.0;
   loop_end = al_get_stream_length(music_stream);
   al_set_stream_enum(music_stream, ALLEGRO_AUDIOPROP_LOOPMODE, ALLEGRO_PLAYMODE_LOOP);
   al_attach_stream_to_mixer(al_get_default_mixer(), music_stream);
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
