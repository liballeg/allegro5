/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Something like the start of a synthesizer.
 */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_primitives.h"
#include "allegro5/allegro_ttf.h"
#include "nihgui.hpp"

#include "common.c"

#define PI                    (ALLEGRO_PI)
#define TWOPI                 (2.0 * PI)
#define SAMPLES_PER_BUFFER    (1024)
#define STREAM_FREQUENCY      (44100)

const double dt = 1.0 / STREAM_FREQUENCY;


enum Waveform {
   WAVEFORM_NONE,
   WAVEFORM_SINE,
   WAVEFORM_SQUARE,
   WAVEFORM_TRIANGLE,
   WAVEFORM_SAWTOOTH
};


/* forward declarations */
static void generate_wave(Waveform type, float *buf, size_t samples, double t,
   float frequency, float phase);
static void sine(float *buf, size_t samples, double t,
   float frequency, float phase);
static void square(float *buf, size_t samples, double t,
   float frequency, float phase);
static void triangle(float *buf, size_t samples, double t,
   float frequency, float phase);
static void sawtooth(float *buf, size_t samples, double t,
   float frequency, float phase);


/* globals */
ALLEGRO_FONT *font_gui;
ALLEGRO_AUDIO_STREAM *stream1;
ALLEGRO_AUDIO_STREAM *stream2;
ALLEGRO_AUDIO_STREAM *stream3;
ALLEGRO_AUDIO_STREAM *stream4;
ALLEGRO_AUDIO_STREAM *stream5;

bool saving = false;
ALLEGRO_FILE *save_fp = NULL;


static void generate_wave(Waveform type, float *buf, size_t samples, double t,
   float frequency, float phase)
{
   switch (type) {
      case WAVEFORM_NONE:
         for (unsigned i = 0; i < samples; i++) {
            buf[i] = 0.0;
         }
         break;
      case WAVEFORM_SINE:
         sine(buf, samples, t, frequency, phase);
         break;
      case WAVEFORM_SQUARE:
         square(buf, samples, t, frequency, phase);
         break;
      case WAVEFORM_TRIANGLE:
         triangle(buf, samples, t, frequency, phase);
         break;
      case WAVEFORM_SAWTOOTH:
         sawtooth(buf, samples, t, frequency, phase);
         break;
   }
}


static void sine(float *buf, size_t samples, double t,
   float frequency, float phase)
{
   const double w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      double ti = t + i * dt;
      buf[i] = sin(w * ti + phase);
   }
}


static void square(float *buf, size_t samples, double t,
   float frequency, float phase)
{
   const double w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      double ti = t + i * dt;
      double x = sin(w * ti + phase);

      buf[i] = (x >= 0.0) ? 1.0 : -1.0;
   }
}


static void triangle(float *buf, size_t samples, double t,
   float frequency, float phase)
{
   const double w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      double tx = w * (t + i * dt) + PI/2.0 + phase;
      double tu = fmod(tx/PI, 2.0);

      if (tu <= 1.0)
         buf[i] = (1.0 - 2.0 * tu);
      else
         buf[i] = (-1.0 + 2.0 * (tu - 1.0));
   }
}


static void sawtooth(float *buf, size_t samples, double t,
   float frequency, float phase)
{
   const double w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      double tx = w * (t + i * dt) + PI + phase;
      double tu = fmod(tx/PI, 2.0);

      buf[i] = (-1.0 + tu);
   }
}


static void mixer_pp_callback(void *buf, unsigned int samples, void *userdata)
{
   ALLEGRO_MIXER *mixer = (ALLEGRO_MIXER *)userdata;
   int nch;
   int sample_size;

   if (!saving)
      return;

   switch (al_get_mixer_channels(mixer)) {
      case ALLEGRO_CHANNEL_CONF_1:
         nch = 1;
         break;
      case ALLEGRO_CHANNEL_CONF_2:
         nch = 2;
         break;
      default:
         /* Not supported. */
         return;
   }

   sample_size = al_get_audio_depth_size(al_get_mixer_depth(mixer));
   al_fwrite(save_fp, buf, nch * samples * sample_size);
}


class Group {
private:
   List        list;
   Label       freq_label;
   HSlider     freq_slider;
   Label       freq_val_label;
   Label       phase_label;
   HSlider     phase_slider;
   Label       phase_val_label;
   Label       gain_label;
   HSlider     gain_slider;
   Label       pan_label;
   HSlider     pan_slider;
   double      t;
   float       last_gain;
   float       last_pan;

public:
   Group();
   void add_to_dialog(Dialog & d, int x, int y);
   void update_labels();
   void generate(float *buf, size_t samples);
   bool get_gain_if_changed(float *gain);
   bool get_pan_if_changed(float *pan);

private:
   float get_frequency() const;
   float get_phase() const;
};


Group::Group() :
   freq_label(Label("f")),
   freq_slider(220, 1000),
   phase_label(Label("φ")),
   phase_slider((int)(100 * PI), (int)(2 * 100 * PI)),   /* -π .. π */
   gain_label(Label("Gain")),
   gain_slider(33, 100),                                 /* 0.0 .. 1.0 */
   pan_label(Label("Pan")),
   pan_slider(100, 200),                                 /* -1.0 .. 1.0 */
   t(0.0),
   last_gain(-10000),
   last_pan(-10000)
{
   /* Order must correspond with Waveform. */
   list.append_item("Off");
   list.append_item("Sine");
   list.append_item("Square");
   list.append_item("Triangle");
   list.append_item("Sawtooth");
}


void Group::add_to_dialog(Dialog & d, int x, int y)
{
   d.add(list,             x,    y,    4,  4);

   d.add(freq_label,       x+4,  y,    2,  1);
   d.add(freq_slider,      x+6,  y,    20, 1);
   d.add(freq_val_label,   x+26, y,    4,  1);

   d.add(phase_label,      x+4,  y+1,  2,  1);
   d.add(phase_slider,     x+6,  y+1,  20, 1);
   d.add(phase_val_label,  x+26, y+1,  4,  1);

   d.add(gain_label,       x+4,  y+2,  2,  1);
   d.add(gain_slider,      x+6,  y+2,  20, 1);

   d.add(pan_label,        x+4,  y+3,  2,  1);
   d.add(pan_slider,       x+6,  y+3,  20, 1);
}


void Group::update_labels()
{
   char buf[32];
   float frequency = get_frequency();
   float phase = get_phase();

   sprintf(buf, "%4.0f Hz", frequency);
   freq_val_label.set_text(buf);

   sprintf(buf, "%.2f π", phase/PI);
   phase_val_label.set_text(buf);
}


void Group::generate(float *buf, size_t samples)
{
   Waveform type = (Waveform) list.get_cur_value();
   float frequency = get_frequency();
   float phase = get_phase();

   generate_wave(type, buf, samples, t, frequency, phase);

   t += dt * samples;
}


float Group::get_frequency() const
{
   return freq_slider.get_cur_value();
}


float Group::get_phase() const
{
   return phase_slider.get_cur_value() / 100.0 - PI;
}


bool Group::get_gain_if_changed(float *gain)
{
   *gain = gain_slider.get_cur_value() / 100.0;
   bool changed = (last_gain != *gain);
   last_gain = *gain;
   return changed;
}


bool Group::get_pan_if_changed(float *pan)
{
   *pan = pan_slider.get_cur_value() / 100.0 - 1.0;
   bool changed = (last_pan != *pan);
   last_pan = *pan;
   return changed;
}


class SaveButton : public ToggleButton {
public:
   SaveButton() : ToggleButton("Save raw") {}
   void on_click(int mx, int my);
};


void SaveButton::on_click(int, int)
{
   if (saving) {
      log_printf("Stopped saving waveform.\n");
      saving = false;
      return;
   }
   if (!save_fp) {
      save_fp = al_fopen("ex_synth.raw", "wb");
   }
   if (save_fp) {
      log_printf("Started saving waveform.\n");
      saving = true;
   }
}


class Prog : public EventHandler {
private:
   Dialog   d;
   Group    group1;
   Group    group2;
   Group    group3;
   Group    group4;
   Group    group5;
   SaveButton save_button;
   double   t;

public:
   Prog(const Theme & theme, ALLEGRO_DISPLAY *display);
   virtual ~Prog() {}
   void run();
   void handle_event(const ALLEGRO_EVENT & event);
};


Prog::Prog(const Theme & theme, ALLEGRO_DISPLAY *display) :
   d(Dialog(theme, display, 30, 26)),
   save_button(SaveButton()),
   t(0.0)
{
   group1.add_to_dialog(d, 1, 1);
   group2.add_to_dialog(d, 1, 6);
   group3.add_to_dialog(d, 1, 11);
   group4.add_to_dialog(d, 1, 16);
   group5.add_to_dialog(d, 1, 21);
   d.add(save_button, 27, 25, 3, 1);
}


void Prog::run()
{
   d.prepare();

   d.register_event_source(al_get_audio_stream_event_source(stream1));
   d.register_event_source(al_get_audio_stream_event_source(stream2));
   d.register_event_source(al_get_audio_stream_event_source(stream3));
   d.register_event_source(al_get_audio_stream_event_source(stream4));
   d.register_event_source(al_get_audio_stream_event_source(stream5));
   d.set_event_handler(this);

   while (!d.is_quit_requested()) {
      if (d.is_draw_requested()) {
         group1.update_labels();
         group2.update_labels();
         group3.update_labels();
         group4.update_labels();
         group5.update_labels();

         al_clear_to_color(al_map_rgb(128, 128, 128));
         d.draw();
         al_flip_display();
      }

      d.run_step(true);
   }
}


void Prog::handle_event(const ALLEGRO_EVENT & event)
{
   if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
      ALLEGRO_AUDIO_STREAM *stream;
      Group *group;
      void *buf;
      float gain;
      float pan;

      stream = (ALLEGRO_AUDIO_STREAM *) event.any.source;
      buf = al_get_audio_stream_fragment(stream);
      if (!buf) {
         /* This is a normal condition that you must deal with. */
         return;
      }

      if (stream == stream1)
         group = &group1;
      else if (stream == stream2)
         group = &group2;
      else if (stream == stream3)
         group = &group3;
      else if (stream == stream4)
         group = &group4;
      else if (stream == stream5)
         group = &group5;
      else
         group = NULL;

      ALLEGRO_ASSERT(group);

      if (group) {
         group->generate((float *) buf, SAMPLES_PER_BUFFER);
         if (group->get_gain_if_changed(&gain)) {
            al_set_audio_stream_gain(stream, gain);
         }
         if (group->get_pan_if_changed(&pan)) {
            al_set_audio_stream_pan(stream, pan);
         }
      }

      if (!al_set_audio_stream_fragment(stream, buf)) {
         log_printf("Error setting stream fragment.\n");
      }
   }
}


int main(int argc, char *argv[])
{
   ALLEGRO_DISPLAY *display;
   (void)argc;
   (void)argv;

   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }

   open_log();

   al_install_keyboard();
   al_install_mouse();

   al_init_primitives_addon();
   al_init_font_addon();
   al_init_ttf_addon();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   display = al_create_display(800, 600);
   if (!display) {
      abort_example("Unable to create display\n");
   }
   al_set_window_title(display, "Synthesiser of sorts");

   font_gui = al_load_ttf_font("data/DejaVuSans.ttf", 12, 0);
   if (!font_gui) {
      abort_example("Failed to load data/fixed_font.tga\n");
   }

   if (!al_install_audio()) {
      abort_example("Could not init sound!\n");
   }

   if (!al_reserve_samples(0)) {
      abort_example("Could not set up voice and mixer.\n");
   }

   size_t buffers = 8;
   unsigned samples = SAMPLES_PER_BUFFER;
   unsigned freq = STREAM_FREQUENCY;
   ALLEGRO_AUDIO_DEPTH depth = ALLEGRO_AUDIO_DEPTH_FLOAT32;
   ALLEGRO_CHANNEL_CONF ch = ALLEGRO_CHANNEL_CONF_1;

   stream1 = al_create_audio_stream(buffers, samples, freq, depth, ch);
   stream2 = al_create_audio_stream(buffers, samples, freq, depth, ch);
   stream3 = al_create_audio_stream(buffers, samples, freq, depth, ch);
   stream4 = al_create_audio_stream(buffers, samples, freq, depth, ch);
   stream5 = al_create_audio_stream(buffers, samples, freq, depth, ch);
   if (!stream1 || !stream2 || !stream3 || !stream4 || !stream5) {
      abort_example("Could not create stream.\n");
   }

   ALLEGRO_MIXER *mixer = al_get_default_mixer();
   if (
      !al_attach_audio_stream_to_mixer(stream1, mixer) ||
      !al_attach_audio_stream_to_mixer(stream2, mixer) ||
      !al_attach_audio_stream_to_mixer(stream3, mixer) ||
      !al_attach_audio_stream_to_mixer(stream4, mixer) ||
      !al_attach_audio_stream_to_mixer(stream5, mixer)
   ) {
      abort_example("Could not attach stream to mixer.\n");
   }

   al_set_mixer_postprocess_callback(mixer, mixer_pp_callback, mixer);

   /* Prog is destroyed at the end of this scope. */
   {
      Theme theme(font_gui);
      Prog prog(theme, display);
      prog.run();
   }

   al_destroy_audio_stream(stream1);
   al_destroy_audio_stream(stream2);
   al_destroy_audio_stream(stream3);
   al_destroy_audio_stream(stream4);
   al_destroy_audio_stream(stream5);
   al_uninstall_audio();

   al_destroy_font(font_gui);

   al_fclose(save_fp);

   close_log(false);

   return 0;
}

/* vim: set sts=3 sw=3 et: */
