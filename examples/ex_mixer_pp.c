/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    This program demonstrates a simple use of mixer postprocessing callbacks.
 */

#include <math.h>
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

#define FPS    60

static volatile float rms_l = 0.0;
static volatile float rms_r = 0.0;

static A5O_DISPLAY *display;
static A5O_BITMAP *dbuf;
static A5O_BITMAP *bmp;
static float theta;

static void update_meter(void *buf, unsigned int samples, void *data)
{
   float *fbuf = (float *)buf;
   float sum_l = 0.0;
   float sum_r = 0.0;
   unsigned int i;

   (void)data;

   for (i = samples; i > 0; i--) {
      sum_l += fbuf[0] * fbuf[0];
      sum_r += fbuf[1] * fbuf[1];
      fbuf += 2;
   }

   rms_l = sqrt(sum_l / samples);
   rms_r = sqrt(sum_r / samples);
}

static void draw(void)
{
   const float sw = al_get_bitmap_width(bmp);
   const float sh = al_get_bitmap_height(bmp);
   const float dw = al_get_bitmap_width(dbuf);
   const float dh = al_get_bitmap_height(dbuf);
   const float dx = dw / 2.0;
   const float dy = dh / 2.0;
   float db_l;
   float db_r;
   float db;
   float scale;
   float disp;

   /* Whatever looks okay. */
   if (rms_l > 0.0 && rms_r > 0.0) {
      db_l = 20 * log10(rms_l / 20e-6);
      db_r = 20 * log10(rms_r / 20e-6);
      db = (db_l + db_r) / 2.0;
      scale = db / 20.0;
      disp = (rms_l + rms_r) * 200.0;
   }
   else {
      db_l = db_r = db = scale = disp = 0.0;
   }

   al_set_target_bitmap(dbuf);
   al_set_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA);
   al_draw_filled_rectangle(0, 0, al_get_bitmap_width(dbuf), al_get_bitmap_height(dbuf),
      al_map_rgba_f(0.8, 0.3, 0.1, 0.06));
   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
   al_draw_tinted_scaled_rotated_bitmap(bmp,
      al_map_rgba_f(0.8, 0.3, 0.1, 0.2),
      sw/2.0, sh/2.0, dx, dy - disp, scale, scale, theta, 0);

   al_set_blender(A5O_ADD, A5O_ONE, A5O_ZERO);
   al_set_target_backbuffer(display);
   al_draw_bitmap(dbuf, 0, 0, 0);

   al_set_blender(A5O_ADD, A5O_ALPHA, A5O_INVERSE_ALPHA);
   al_draw_line(10, dh - db_l, 10, dh, al_map_rgb_f(1, 0.6, 0.2), 6);
   al_draw_line(20, dh - db_r, 20, dh, al_map_rgb_f(1, 0.6, 0.2), 6);

   al_flip_display();

   theta -= (rms_l + rms_r) * 0.1;
}

static void main_loop(void)
{
   A5O_EVENT_QUEUE *queue;
   A5O_EVENT event;
   bool redraw = true;

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(display));

   theta = 0.0;

   for (;;) {
      if (redraw && al_is_event_queue_empty(queue)) {
         draw();
         redraw = false;
      }

      if (!al_wait_for_event_timed(queue, &event, 1.0/FPS)) {
         redraw = true;
         continue;
      }

      if (event.type == A5O_EVENT_DISPLAY_CLOSE) {
         break;
      }
      if (event.type == A5O_EVENT_KEY_DOWN &&
            event.keyboard.keycode == A5O_KEY_ESCAPE) {
         break;
      }
   }

   al_destroy_event_queue(queue);
}

int main(int argc, char **argv)
{
   const char *filename = "../demos/cosmic_protector/data/sfx/title_music.ogg";
   A5O_VOICE *voice;
   A5O_MIXER *mixer;
   A5O_AUDIO_STREAM *stream;

   if (argc > 1) {
      filename = argv[1];
   }

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   al_init_primitives_addon();
   al_init_image_addon();
   al_init_acodec_addon();
   init_platform_specific();

   al_install_keyboard();

   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Could not create display.\n");
   }

   dbuf = al_create_bitmap(640, 480);

   bmp = al_load_bitmap("data/mysha.pcx");
   if (!bmp) {
      abort_example("Could not load data/mysha.pcx\n");
   }

   if (!al_install_audio()) {
      abort_example("Could not init sound.\n");
   }

   voice = al_create_voice(44100, A5O_AUDIO_DEPTH_INT16,
      A5O_CHANNEL_CONF_2);
   if (!voice) {
      abort_example("Could not create voice.\n");
   }

   mixer = al_create_mixer(44100, A5O_AUDIO_DEPTH_FLOAT32,
      A5O_CHANNEL_CONF_2);
   if (!mixer) {
      abort_example("Could not create mixer.\n");
   }

   if (!al_attach_mixer_to_voice(mixer, voice)) {
      abort_example("al_attach_mixer_to_voice failed.\n");
   }

   stream = al_load_audio_stream(filename, 4, 2048);
   if (!stream) {
      /* On Android we only pack this into the APK. */
      stream = al_load_audio_stream("data/welcome.wav", 4, 2048);
   }
   if (!stream) {
      abort_example("Could not load '%s'\n", filename);
   }

   al_set_audio_stream_playmode(stream, A5O_PLAYMODE_LOOP);
   al_attach_audio_stream_to_mixer(stream, mixer);

   al_set_mixer_postprocess_callback(mixer, update_meter, NULL);

   main_loop();

   al_destroy_audio_stream(stream);
   al_destroy_mixer(mixer);
   al_destroy_voice(voice);
   al_uninstall_audio();

   al_destroy_bitmap(dbuf);
   al_destroy_bitmap(bmp);

   return 0;
}
