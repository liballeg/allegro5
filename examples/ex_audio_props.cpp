/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test audio properties (gain and panning, for now).
 */

#include "allegro5/allegro5.h"
#include "allegro5/a5_font.h"
#include "allegro5/acodec.h"
#include "allegro5/kcm_audio.h"
#include "nihgui.hpp"
#include <cstdio>

ALLEGRO_FONT *font_gui;
ALLEGRO_SAMPLE *sample;
ALLEGRO_SAMPLE_INSTANCE *sample_inst;

class Prog {
private:
   Dialog d;
   ToggleButton pan_button;
   HSlider pan_slider;
   Label speed_label;
   HSlider speed_slider;
   Label gain_label;
   VSlider gain_slider;

public:
   Prog(const Theme & theme);
   void run();
   void update_properties();
};

Prog::Prog(const Theme & theme) :
   d(Dialog(theme, al_get_current_display(), 20, 20)),
   pan_button(ToggleButton("Pan")),
   pan_slider(HSlider(1000, 2000)),
   speed_label(Label("Speed")),
   speed_slider(HSlider(1000, 5000)),
   gain_label(Label("Gain")),
   gain_slider(VSlider(1000, 2000))
{
   pan_button.set_pushed(true);
   d.add(pan_button, 1, 10,  2, 1);
   d.add(pan_slider, 3, 10, 15, 1);

   d.add(speed_label,  1, 12,  2, 1);
   d.add(speed_slider, 3, 12, 15, 1);

   d.add(gain_label,  18, 1, 1,  1);
   d.add(gain_slider, 18, 2, 1, 17);
}

void Prog::run()
{
   d.prepare();

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         update_properties();
         al_clear_to_color(al_map_rgb(128, 128, 128));
         d.draw();
         al_flip_display();
      }

      d.run_step(true);
   }
}

void Prog::update_properties()
{
   float pan;
   float speed;
   float gain;

   if (pan_button.get_pushed())
      pan = pan_slider.get_cur_value() / 1000.0f - 1.0f;
   else
      pan = ALLEGRO_AUDIO_PAN_NONE;
   al_set_sample_instance_pan(sample_inst, pan);

   speed = speed_slider.get_cur_value() / 1000.0f;
   al_set_sample_instance_speed(sample_inst, speed);

   gain = gain_slider.get_cur_value() / 1000.0f;
   al_set_sample_instance_gain(sample_inst, gain);
}

int main(int argc, const char *argv[])
{
   ALLEGRO_DISPLAY *display;

   if (argc < 2) {
      fprintf(stderr, "Usage: %s {audio_file}\n", argv[0]);
      return 1;
   }

   if (!al_init()) {
      TRACE("Could not init Allegro\n");
      return 1;
   }
   al_install_keyboard();
   al_install_mouse();

   al_init_font_addon();

   if (!al_install_audio(ALLEGRO_AUDIO_DRIVER_AUTODETECT)) {
      TRACE("Could not init sound!\n");
      return 1;
   }

   if (!al_reserve_samples(1)) {
      TRACE("Could not set up voice and mixer.\n");
      return 1;
   }

   sample = al_load_sample(argv[1]);
   if (!sample) {
      fprintf(stderr, "Could not load sample from '%s'!\n", argv[1]);
      return 1;
   }

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      TRACE("Unable to create display\n");
      return 1;
   }

   font_gui = al_load_font("data/fixed_font.tga", 0, 0);
   if (!font_gui) {
      TRACE("Failed to load data/fixed_font.tga\n");
      return 1;
   }

   /* Loop the sample. */
   sample_inst = al_create_sample_instance(sample);
   al_set_sample_instance_playmode(sample_inst, ALLEGRO_PLAYMODE_LOOP);
   al_attach_sample_to_mixer(sample_inst, al_get_default_mixer());
   al_play_sample_instance(sample_inst);

   al_show_mouse_cursor();

   /* Don't remove these braces. */
   {
      Theme theme(font_gui);
      Prog prog(theme);
      prog.run();
   }

   al_destroy_sample_instance(sample_inst);
   al_destroy_sample(sample);
   al_uninstall_audio();

   al_destroy_font(font_gui);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
