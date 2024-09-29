/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Test audio properties (gain and panning, for now).
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"
#include "nihgui.hpp"
#include <cstdio>

#include "common.c"

A5O_FONT *font_gui;
A5O_SAMPLE *sample;
A5O_SAMPLE_INSTANCE *sample_inst;

class Prog {
private:
   Dialog d;
   Label length_label;
   HSlider length_slider;
   ToggleButton pan_button;
   HSlider pan_slider;
   Label speed_label;
   HSlider speed_slider;
   ToggleButton bidir_button;
   ToggleButton play_button;
   Label gain_label;
   VSlider gain_slider;
   Label mixer_gain_label;
   VSlider mixer_gain_slider;
   Label two_label;
   Label one_label;
   Label zero_label;

public:
   Prog(const Theme & theme, A5O_DISPLAY *display, unsigned int instance_len);
   void run();
   void update_properties();
};

Prog::Prog(const Theme & theme, A5O_DISPLAY *display, unsigned int instance_len) :
   d(Dialog(theme, display, 40, 20)),
   length_label(Label("Length")),
   length_slider(HSlider(instance_len, instance_len)),
   pan_button(ToggleButton("Pan")),
   pan_slider(HSlider(1000, 2000)),
   speed_label(Label("Speed")),
   speed_slider(HSlider(1000, 5000)),
   bidir_button(ToggleButton("Bidir")),
   play_button(ToggleButton("Play")),
   gain_label(Label("Gain")),
   gain_slider(VSlider(1000, 2000)),
   mixer_gain_label(Label("Mixer gain")),
   mixer_gain_slider(VSlider(1000, 2000)),
   two_label(Label("2.0")),
   one_label(Label("1.0")),
   zero_label(Label("0.0"))
{
   pan_button.set_pushed(true);
   play_button.set_pushed(true);

   d.add(length_label,  2, 8,  4, 1);
   d.add(length_slider, 6, 8, 22, 1);

   d.add(pan_button, 2, 10,  4, 1);
   d.add(pan_slider, 6, 10, 22, 1);

   d.add(speed_label,  2, 12,  4, 1);
   d.add(speed_slider, 6, 12, 22, 1);

   d.add(bidir_button, 2, 14, 4, 1);
   d.add(play_button, 6, 14, 4, 1);

   d.add(gain_label,  29, 1, 2,  1);
   d.add(gain_slider, 29, 2, 2, 17);

   d.add(mixer_gain_label,  33, 1, 6,  1);
   d.add(mixer_gain_slider, 35, 2, 2, 17);

   d.add(two_label,  32,  2, 2, 1);
   d.add(one_label,  32, 10, 2, 1);
   d.add(zero_label, 32, 18, 2, 1);
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
   int length;
   float pan;
   float speed;
   float gain;
   float mixer_gain;

   if (pan_button.get_pushed())
      pan = pan_slider.get_cur_value() / 1000.0f - 1.0f;
   else
      pan = A5O_AUDIO_PAN_NONE;
   al_set_sample_instance_pan(sample_inst, pan);

   speed = speed_slider.get_cur_value() / 1000.0f;
   al_set_sample_instance_speed(sample_inst, speed);

   length = length_slider.get_cur_value();
   al_set_sample_instance_length(sample_inst, length);

   if (bidir_button.get_pushed())
      al_set_sample_instance_playmode(sample_inst, A5O_PLAYMODE_BIDIR);
   else
      al_set_sample_instance_playmode(sample_inst, A5O_PLAYMODE_LOOP);

   al_set_sample_instance_playing(sample_inst, play_button.get_pushed());

   gain = gain_slider.get_cur_value() / 1000.0f;
   al_set_sample_instance_gain(sample_inst, gain);

   mixer_gain = mixer_gain_slider.get_cur_value() / 1000.0f;
   al_set_mixer_gain(al_get_default_mixer(), mixer_gain);
}

int main(int argc, char **argv)
{
   A5O_DISPLAY *display;
   const char *filename;

   if (argc >= 2) {
      filename = argv[1];
   }
   else {
      filename = "data/welcome.wav";
   }

   if (!al_init()) {
      abort_example("Could not init Allegro\n");
   }
   al_install_keyboard();
   al_install_mouse();

   al_init_font_addon();
   al_init_primitives_addon();
   al_init_acodec_addon();
   init_platform_specific();

   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }

   if (!al_reserve_samples(1)) {
      abort_example("Could not set up voice and mixer.\n");
   }

   sample = al_load_sample(filename);
   if (!sample) {
      abort_example("Could not load sample from '%s'!\n", filename);
   }

   al_set_new_display_flags(A5O_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to create display\n");
   }

   font_gui = al_create_builtin_font();
   if (!font_gui) {
      abort_example("Failed to create builtin font\n");
   }

   /* Loop the sample. */
   sample_inst = al_create_sample_instance(sample);
   al_set_sample_instance_playmode(sample_inst, A5O_PLAYMODE_LOOP);
   al_attach_sample_instance_to_mixer(sample_inst, al_get_default_mixer());
   al_play_sample_instance(sample_inst);

   /* Don't remove these braces. */
   {
      Theme theme(font_gui);
      Prog prog(theme, display, al_get_sample_instance_length(sample_inst));
      prog.run();
   }

   al_destroy_sample_instance(sample_inst);
   al_destroy_sample(sample);
   al_uninstall_audio();

   al_destroy_font(font_gui);

   return 0;

   (void)argc;
   (void)argv;
}

/* vim: set sts=3 sw=3 et: */
