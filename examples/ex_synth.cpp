/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Something like the start of a synthesizer.
 */

#include <stdio.h>
#include <math.h>

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"
#include "allegro5/a5_font.h"
#include "allegro5/a5_ttf.h"
#include "nihgui.hpp"


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
static void generate_wave(Waveform type, float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude);
static void sine(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude);
static void square(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude);
static void triangle(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude);
static void sawtooth(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude);


/* globals */
ALLEGRO_FONT *font_gui;
ALLEGRO_STREAM *stream;


static void generate_wave(Waveform type, float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude)
{
   switch (type) {
      case WAVEFORM_NONE:
         break;
      case WAVEFORM_SINE:
         sine(buf, samples, t, frequency, phase, amplitude);
         break;
      case WAVEFORM_SQUARE:
         square(buf, samples, t, frequency, phase, amplitude);
         break;
      case WAVEFORM_TRIANGLE:
         triangle(buf, samples, t, frequency, phase, amplitude);
         break;
      case WAVEFORM_SAWTOOTH:
         sawtooth(buf, samples, t, frequency, phase, amplitude);
         break;
   }
}


static void sine(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude)
{
   const float w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      float ti = t + i * dt;
      buf[i] += amplitude * sin(w * ti + phase);
   }
}


static void square(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude)
{
   const float w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      float ti = t + i * dt;
      float x = sin(w * ti + phase);

      buf[i] += (x >= 0.0) ? amplitude : -amplitude;
   }
}


static void triangle(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude)
{
   const float w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      float tx = w * (t + i * dt) + PI/2.0 + phase;
      float tu = fmodf(tx/PI, 2.0);

      if (tu <= 1.0)
         buf[i] += amplitude * (1.0 - 2.0 * tu);
      else
         buf[i] += amplitude * (-1.0 + 2.0 * (tu - 1.0));
   }
}


void sawtooth(float *buf, size_t samples, float t,
   float frequency, float phase, float amplitude)
{
   const float w = TWOPI * frequency;
   unsigned i;

   for (i = 0; i < samples; i++) {
      float tx = w * (t + i * dt) + PI + phase;
      float tu = fmodf(tx/PI, 2.0);

      buf[i] += amplitude * (-1.0 + tu);
   }
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

public:
   Group();
   void add_to_dialog(Dialog & d, int x, int y);
   void update_labels();
   void generate(float *buf, size_t samples, float t);

private:
   float get_frequency() const;
   float get_phase() const;
};


Group::Group() :
   freq_label(Label("f")),
   freq_slider(220, 1000),
   phase_label(Label("φ")),
   phase_slider((int)(100 * PI), (int)(2 * 100 * PI)) /* -π .. π */
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
   d.add(freq_label,       x+5,  y,    1,  1);
   d.add(freq_slider,      x+6,  y,    20, 1);
   d.add(freq_val_label,   x+26, y,    4,  1);
   d.add(phase_label,      x+5,  y+2,  1,  1);
   d.add(phase_slider,     x+6,  y+2,  20, 1);
   d.add(phase_val_label,  x+26, y+2,  4,  1);
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


void Group::generate(float *buf, size_t samples, float t)
{
   Waveform type = (Waveform) list.get_cur_value();
   float frequency = get_frequency();
   float phase = get_phase();

   /* The amplitude could be varied too. */
   generate_wave(type, buf, samples, t, frequency, phase, 0.2);
}


float Group::get_frequency() const
{
   return freq_slider.get_cur_value();
}


float Group::get_phase() const
{
   return phase_slider.get_cur_value() / 100.0 - PI;
}


class Prog : public EventHandler {
private:
   Dialog   d;
   Group    group1;
   Group    group2;
   Group    group3;
   Group    group4;
   Group    group5;
   double   t;

public:
   Prog(const Theme & theme);
   virtual ~Prog() {}
   void run();
   void handle_event(const ALLEGRO_EVENT & event);
};


Prog::Prog(const Theme & theme) :
   d(Dialog(theme, al_get_current_display(), 30, 26)),
   t(0.0)
{
   group1.add_to_dialog(d, 1, 1);
   group2.add_to_dialog(d, 1, 6);
   group3.add_to_dialog(d, 1, 11);
   group4.add_to_dialog(d, 1, 16);
   group5.add_to_dialog(d, 1, 21);
}


void Prog::run()
{
   d.prepare();

   d.register_event_source(al_get_stream_event_source(stream));
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
   if (event.type == ALLEGRO_EVENT_STREAM_EMPTY_FRAGMENT) {
      void *buf_void;
      float *buf;
      unsigned i;

      if (!al_get_stream_fragment(stream, &buf_void)) {
         return;
      }
      buf = (float *) buf_void;

      for (i = 0; i < SAMPLES_PER_BUFFER; i++) {
         buf[i] = 0.0;
      }

      t += dt * SAMPLES_PER_BUFFER;

      group1.generate(buf, SAMPLES_PER_BUFFER, t);
      group2.generate(buf, SAMPLES_PER_BUFFER, t);
      group3.generate(buf, SAMPLES_PER_BUFFER, t);
      group4.generate(buf, SAMPLES_PER_BUFFER, t);
      group5.generate(buf, SAMPLES_PER_BUFFER, t);

      if (!al_set_stream_fragment(stream, buf)) {
         fprintf(stderr, "Error setting stream fragment.\n");
      }
   }
}


int main(void)
{
   if (!al_init()) {
      fprintf(stderr, "Could not init Allegro.\n");
      return 1;
   }
   al_install_keyboard();
   al_install_mouse();

   al_init_font_addon();
   al_init_ttf_addon();

   al_set_new_display_flags(ALLEGRO_GENERATE_EXPOSE_EVENTS);
   if (!al_create_display(800, 600)) {
      TRACE("Unable to create display\n");
      return 1;
   }
   al_set_window_title("Synthesiser of sorts");

   font_gui = al_load_ttf_font("data/DejaVuSans.ttf", 12, 0);
   if (!font_gui) {
      TRACE("Failed to load data/fixed_font.tga\n");
      return 1;
   }

   if (!al_install_audio(ALLEGRO_AUDIO_DRIVER_AUTODETECT)) {
      TRACE("Could not init sound!\n");
      return 1;
   }

   if (!al_reserve_samples(0)) {
      TRACE("Could not set up voice and mixer.\n");
      return 1;
   }

   stream = al_create_stream(8, SAMPLES_PER_BUFFER, STREAM_FREQUENCY,
      ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_1);
   if (!stream) {
      TRACE("Could not create stream.\n");
      return 1;
   }

   if (!al_attach_stream_to_mixer(stream, al_get_default_mixer())) {
      TRACE("Could not attach stream to mixer.\n");
      return 1;
   }

   al_show_mouse_cursor();

   /* Prog is destroyed at the end of this scope. */
   {
      Theme theme(font_gui);
      Prog prog(theme);
      prog.run();
   }

   al_destroy_stream(stream);
   al_uninstall_audio();

   al_destroy_font(font_gui);

   return 0;
}
END_OF_MAIN()

/* vim: set sts=3 sw=3 et: */
