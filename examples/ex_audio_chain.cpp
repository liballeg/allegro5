/*
 *    Example program for the Allegro library, by Peter Wang.
 *
 *    Demonstrate the audio addons.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_ttf.h>
#include <ctype.h>
#include <math.h>
#include <string>
#include <vector>

#include "common.c"

#define DISP_W 800
#define DISP_H 600

using namespace std;

class Element;
class Voice;
class Mixer;
class SampleInstance;
class Sample;
class Audiostream;

struct Context {
   A5O_FONT *font;
   A5O_COLOR bg;
   A5O_COLOR fg;
   A5O_COLOR fill;
   A5O_COLOR disabled;
   A5O_COLOR highlight;
};

class Element {
public:
   Element();
   virtual ~Element() {};
   void set_pos(int x, int y);
   void set_random_pos();
   bool attach(Element *elt);
   bool detach();
   bool is_attached_to(Element const *elt);
   virtual bool is_playing() const = 0;
   virtual bool toggle_playing() = 0;
   virtual float get_gain() const = 0;
   virtual bool adjust_gain(float d) = 0;
   void draw(Context const& ctx, bool highlight) const;
   void draw_arrow(Context const& ctx, float px, float py, bool highlight) const;
   void move_by(int dx, int dy);
   bool contains(int x, int y) const;
   void output_point(float & ox, float & oy) const;
   void input_point(float & ox, float & oy) const;
   virtual char const *get_label() const = 0;

   typedef vector<Element *>::iterator iter;

protected:
   int x, y, w, h;

private:
   virtual bool do_attach(Mixer & mixer);
   virtual bool do_attach(SampleInstance & spl);
   virtual bool do_attach(Audiostream & stream);
   virtual bool do_detach() = 0;
   void set_attached_to(Element const *elt);

   Element const *attached_to;
};

class Voice : public Element {
public:
   Voice();
   ~Voice();
   bool valid() const;
   bool is_playing() const;
   bool toggle_playing();
   float get_gain() const;
   bool adjust_gain(float d);
   char const *get_label() const;
private:
   bool do_attach(Mixer & mixer);
   bool do_attach(SampleInstance & spl);
   bool do_attach(Audiostream & stream);
   bool do_detach();

   A5O_VOICE *voice;
};

class Mixer : public Element {
public:
   Mixer();
   ~Mixer();
   operator A5O_MIXER *() const { return mixer; }
   bool is_playing() const;
   bool toggle_playing();
   float get_gain() const;
   bool adjust_gain(float d);
   char const *get_label() const;
private:
   bool do_attach(Mixer & mixer);
   bool do_attach(SampleInstance & spl);
   bool do_attach(Audiostream & stream);
   bool do_detach();

   A5O_MIXER *mixer;
};

class SampleInstance : public Element {
public:
   SampleInstance();
   ~SampleInstance();
   operator A5O_SAMPLE_INSTANCE *() const { return splinst; }
   bool is_playing() const;
   bool toggle_playing();
   float get_gain() const;
   bool adjust_gain(float d);
   char const *get_label() const;
   bool set_sample(Sample const & wav);
private:
   bool do_detach();

   A5O_SAMPLE_INSTANCE *splinst;
   Sample const *spl;
   unsigned pos;
};

class Sample {
public:
   Sample(char const *filename);
   ~Sample();
   operator A5O_SAMPLE *() const { return spl; }
   bool valid() const;
   string const& get_filename() const;
private:
   A5O_SAMPLE *spl;
   string filename;
};

class Audiostream : public Element {
public:
   Audiostream(string const& filename);
   ~Audiostream();
   operator A5O_AUDIO_STREAM *() const { return stream; }
   bool valid() const;
   bool is_playing() const;
   bool toggle_playing();
   float get_gain() const;
   bool adjust_gain(float d);
   char const *get_label() const;
private:
   bool do_detach();

   A5O_AUDIO_STREAM *stream;
   string filename;
};

/*---------------------------------------------------------------------------*/

static A5O_PATH *make_path(char const *str)
{
   static A5O_PATH *dir;
   A5O_PATH *path;

   if (!dir) {
      dir = al_get_standard_path(A5O_RESOURCES_PATH);
#ifdef A5O_MSVC
      {
         /* Hack to cope automatically with MSVC workspaces. */
         const char *last = al_get_path_component(dir, -1);
         if (0 == strcmp(last, "Debug")
            || 0 == strcmp(last, "RelWithDebInfo")
            || 0 == strcmp(last, "Release")
            || 0 == strcmp(last, "Profile")) {
            al_remove_path_component(dir, -1);
         }
      }
#endif
   }

   path = al_create_path(str);
   al_rebase_path(dir, path);
   return path;
}

static void basename(const string & filename, string & out)
{
   size_t pos = filename.find_last_of("/");
   if (pos != string::npos)
      out = filename.substr(pos + 1);
   else
      out = filename;
}

static float clamp(float lo, float mid, float hi)
{
   if (mid < lo)
      return lo;
   if (mid > hi)
      return hi;
   return mid;
}

/*---------------------------------------------------------------------------*/

Element::Element() : x(0), y(0), w(40), h(40), attached_to(0)
{
}

void Element::set_pos(int x, int y)
{
   this->x = x;
   this->y = y;
}

void Element::set_random_pos()
{
   /* Could be smarter. */
   x = rand() % (DISP_W - w);
   y = rand() % (DISP_H - h);
}

bool Element::attach(Element *elt)
{
   Mixer *mixer;
   SampleInstance *splinst;
   Audiostream *stream;
   bool rc = false;

   if ((mixer = dynamic_cast<Mixer *>(elt))) {
      rc = do_attach(*mixer);
   }
   else if ((splinst = dynamic_cast<SampleInstance *>(elt))) {
      rc = do_attach(*splinst);
   }
   else if ((stream = dynamic_cast<Audiostream *>(elt))) {
      rc = do_attach(*stream);
   }
   if (rc) {
      elt->set_attached_to(this);
   }
   return rc;
}

bool Element::do_attach(Mixer & other)
{
   (void)other;
   return false;
}

bool Element::do_attach(SampleInstance & other)
{
   (void)other;
   return false;
}

bool Element::do_attach(Audiostream & other)
{
   (void)other;
   return false;
}

bool Element::detach()
{
   bool rc = attached_to && do_detach();
   if (rc) {
      attached_to = NULL;
   }
   return rc;
}

void Element::set_attached_to(Element const *attached_to)
{
   this->attached_to = attached_to;
}

bool Element::is_attached_to(Element const *elt)
{
   return attached_to == elt;
}

void Element::draw(Context const& ctx, bool highlight) const
{
   if (attached_to) {
      float x2, y2;
      attached_to->input_point(x2, y2);
      draw_arrow(ctx, x2, y2, highlight);
   }

   A5O_COLOR textcol = ctx.fg;
   A5O_COLOR boxcol = ctx.fg;
   if (highlight)
      boxcol = ctx.highlight;
   if (!is_playing())
      textcol = ctx.disabled;

   float rad = 7.0;
   al_draw_filled_rounded_rectangle(x+0.5, y+0.5, x+w-0.5, y+h-0.5, rad, rad, ctx.fill);
   al_draw_rounded_rectangle(x+0.5, y+0.5, x+w-0.5, y+h-0.5, rad, rad, boxcol, 1.0);

   float th = al_get_font_line_height(ctx.font);
   al_draw_text(ctx.font, textcol, x + w/2, y + h/2 - th/2,
      A5O_ALIGN_CENTRE, get_label());

   float gain = get_gain();
   if (gain > 0.0) {
      float igain = 1.0 - clamp(0.0, gain, 2.0)/2.0;
      al_draw_rectangle(x+w+1.5, y+h*igain, x+w+3.5, y+h, ctx.fg, 1.0);
   }
}

void Element::draw_arrow(Context const& ctx, float x2, float y2, bool highlight) const
{
   float x1, y1;
   A5O_COLOR col;
   output_point(x1, y1);
   col = (highlight ? ctx.highlight : ctx.fg);
   al_draw_line(x1, y1, x2, y2, col, 1.0);

   float a = atan2(y1-y2, x1-x2);
   float a1 = a + 0.5;
   float a2 = a - 0.5;
   float len = 7.0;
   al_draw_line(x2, y2, x2 + len*cos(a1), y2 + len*sin(a1), col, 1.0);
   al_draw_line(x2, y2, x2 + len*cos(a2), y2 + len*sin(a2), col, 1.0);
}

void Element::move_by(int dx, int dy)
{
   x += dx;
   y += dy;

   if (x < 0) x = 0;
   if (y < 0) y = 0;
   if (x+w > DISP_W) x = DISP_W-w;
   if (y+h > DISP_H) y = DISP_H-h;
}

bool Element::contains(int px, int py) const
{
   return px >= x && px < x + w
      &&  py >= y && py < y + h;
}

void Element::output_point(float & ox, float & oy) const
{
   ox = x + w/2;
   oy = y;
}

void Element::input_point(float & ox, float & oy) const
{
   ox = x + w/2;
   oy = y + h;
}

/*---------------------------------------------------------------------------*/

Voice::Voice()
{
   set_random_pos();
   voice = al_create_voice(44100, A5O_AUDIO_DEPTH_INT16,
      A5O_CHANNEL_CONF_2);
}

Voice::~Voice()
{
   al_destroy_voice(voice);
}

bool Voice::valid() const
{
   return (voice != NULL);
}

bool Voice::do_attach(Mixer & mixer)
{
   return al_attach_mixer_to_voice(mixer, voice);
}

bool Voice::do_attach(SampleInstance & spl)
{
   return al_attach_sample_instance_to_voice(spl, voice);
}

bool Voice::do_attach(Audiostream & stream)
{
   return al_attach_audio_stream_to_voice(stream, voice);
}

bool Voice::do_detach()
{
   return false;
}

bool Voice::is_playing() const
{
   return al_get_voice_playing(voice);
}

bool Voice::toggle_playing()
{
   bool playing = al_get_voice_playing(voice);
   return al_set_voice_playing(voice, !playing);
}

float Voice::get_gain() const
{
   return 0.0;
}

bool Voice::adjust_gain(float d)
{
   (void)d;
   return false;
}

char const *Voice::get_label() const
{
   return "Voice";
}

/*---------------------------------------------------------------------------*/

Mixer::Mixer()
{
   set_random_pos();
   mixer = al_create_mixer(44100, A5O_AUDIO_DEPTH_FLOAT32,
      A5O_CHANNEL_CONF_2);
}

Mixer::~Mixer()
{
   al_destroy_mixer(mixer);
}

bool Mixer::do_attach(Mixer & other)
{
   return al_attach_mixer_to_mixer(other, mixer);
}

bool Mixer::do_attach(SampleInstance & spl)
{
   return al_attach_sample_instance_to_mixer(spl, mixer);
}

bool Mixer::do_attach(Audiostream & stream)
{
   return al_attach_audio_stream_to_mixer(stream, mixer);
}

bool Mixer::do_detach()
{
   return al_detach_mixer(mixer);
}

bool Mixer::is_playing() const
{
   return al_get_mixer_playing(mixer);
}

bool Mixer::toggle_playing()
{
   bool playing = al_get_mixer_playing(mixer);
   return al_set_mixer_playing(mixer, !playing);
}

float Mixer::get_gain() const
{
   return al_get_mixer_gain(mixer);
}

bool Mixer::adjust_gain(float d)
{
   float gain = al_get_mixer_gain(mixer) + d;
   gain = clamp(0, gain, 2);
   return al_set_mixer_gain(mixer, gain);
}

char const *Mixer::get_label() const
{
   return "Mixer";
}

/*---------------------------------------------------------------------------*/

SampleInstance::SampleInstance() : spl(NULL), pos(0)
{
   w = 150;
   set_random_pos();
   splinst = al_create_sample_instance(NULL);
}

SampleInstance::~SampleInstance()
{
   al_destroy_sample_instance(splinst);
}

bool SampleInstance::do_detach()
{
   return al_detach_sample_instance(splinst);
}

bool SampleInstance::is_playing() const
{
   return al_get_sample_instance_playing(splinst);
}

bool SampleInstance::toggle_playing()
{
   bool playing = is_playing();
   if (playing)
      pos = al_get_sample_instance_position(splinst);
   else
      al_set_sample_instance_position(splinst, pos);
   return al_set_sample_instance_playing(splinst, !playing);
}

float SampleInstance::get_gain() const
{
   return al_get_sample_instance_gain(splinst);
}

bool SampleInstance::adjust_gain(float d)
{
   float gain = al_get_sample_instance_gain(splinst) + d;
   gain = clamp(0, gain, 2);
   return al_set_sample_instance_gain(splinst, gain);
}

bool SampleInstance::set_sample(Sample const & spl)
{
   bool playing = is_playing();
   bool rc = al_set_sample(splinst, spl);
   if (rc) {
      this->spl = &spl;
      al_set_sample_instance_playmode(splinst, A5O_PLAYMODE_LOOP);
      al_set_sample_instance_playing(splinst, playing);
   }
   return rc;
}

char const *SampleInstance::get_label() const
{
   if (spl) {
      return spl->get_filename().c_str();
   }
   return "No sample";
}

/*---------------------------------------------------------------------------*/

Sample::Sample(char const *filename)
{
   spl = al_load_sample(filename);
   basename(filename, this->filename);
}

Sample::~Sample()
{
   al_destroy_sample(spl);
}

bool Sample::valid() const
{
   return spl;
}

string const& Sample::get_filename() const
{
   return filename;
}

/*---------------------------------------------------------------------------*/

Audiostream::Audiostream(string const& filename)
{
   w = 150;
   set_random_pos();
   basename(filename, this->filename);

   stream = al_load_audio_stream(filename.c_str(), 4, 2048);
   if (stream) {
      al_set_audio_stream_playmode(stream, A5O_PLAYMODE_LOOP);
   }
}

Audiostream::~Audiostream()
{
   al_destroy_audio_stream(stream);
}

bool Audiostream::valid() const
{
   return stream;
}

bool Audiostream::do_detach()
{
   return al_detach_audio_stream(stream);
}

bool Audiostream::is_playing() const
{
   return al_get_audio_stream_playing(stream);
}

bool Audiostream::toggle_playing()
{
   bool playing = al_get_audio_stream_playing(stream);
   return al_set_audio_stream_playing(stream, !playing);
}

float Audiostream::get_gain() const
{
   return al_get_audio_stream_gain(stream);
}

bool Audiostream::adjust_gain(float d)
{
   float gain = al_get_audio_stream_gain(stream) + d;
   gain = clamp(0, gain, 2);
   return al_set_audio_stream_gain(stream, gain);
}

char const *Audiostream::get_label() const
{
   return filename.c_str();
}

/*---------------------------------------------------------------------------*/

class Prog {
public:
   Prog();
   void init();
   void add_sample(char const *filename);
   void add_stream_path(char const *filename);
   void initial_config();
   void run(void);

private:
   Voice *new_voice();
   Mixer *new_mixer();
   SampleInstance *new_sample_instance();
   Audiostream *new_audiostream();
   void process_mouse_button_down(int mb, int mx, int my);
   void process_mouse_button_up(int mb, int mx, int my);
   void process_mouse_axes(int mx, int my, int dx, int dy);
   void process_mouse_wheel(int dz);
   void process_key_char(int unichar);
   Element *find_element(int x, int y);
   void delete_element(Element *elt);
   void redraw();

   A5O_DISPLAY *dpy;
   A5O_EVENT_QUEUE *queue;
   Context ctx;
   vector<Sample const *> samples;
   vector<string> stream_paths;
   vector<Element *> elements;
   int cur_button;
   Element *cur_element;
   float connect_x;
   float connect_y;
};

Prog::Prog() : cur_button(0), cur_element(NULL)
{
}

void Prog::init()
{
   if (!al_init()) {
      abort_example("Could not initialise Allegro.\n");
   }
   if (!al_init_primitives_addon()) {
      abort_example("Could not initialise primitives.\n");
   }
   al_init_font_addon();
   if (!al_init_ttf_addon()) {
      abort_example("Could not initialise TTF fonts.\n");
   }
   if (!al_install_audio()) {
      abort_example("Could not initialise audio.\n");
   }
   if (!al_init_acodec_addon()) {
      abort_example("Could not initialise audio codecs.\n");
   }
   init_platform_specific();
   if (!(dpy = al_create_display(800, 600))) {
      abort_example("Could not create display.\n");
   }
   if (!al_install_keyboard()) {
      abort_example("Could not install keyboard.\n");
   }
   if (!al_install_mouse()) {
      abort_example("Could not install mouse.\n");
   }

   ctx.font = al_load_ttf_font("data/DejaVuSans.ttf", 10, 0);
   if (!ctx.font) {
      abort_example("Could not load font.\n");
   }
   ctx.bg = al_map_rgb_f(0.9, 0.9, 0.9);
   ctx.fg = al_map_rgb_f(0, 0, 0);
   ctx.fill = al_map_rgb_f(0.85, 0.85, 0.85);
   ctx.disabled = al_map_rgb_f(0.6, 0.6, 0.6);
   ctx.highlight = al_map_rgb_f(1, 0.1, 0.1);

   queue = al_create_event_queue();
   al_register_event_source(queue, al_get_mouse_event_source());
   al_register_event_source(queue, al_get_keyboard_event_source());
   al_register_event_source(queue, al_get_display_event_source(dpy));
}

void Prog::add_sample(char const *filename)
{
   A5O_PATH *path = make_path(filename);
   Sample *spl = new Sample(al_path_cstr(path, '/'));
   if (spl) {
      samples.push_back(spl);
   } else {
      delete spl;
   }
   al_destroy_path(path);
}

void Prog::add_stream_path(char const *filename)
{
   A5O_PATH *path = make_path(filename);
   stream_paths.push_back(al_path_cstr(path, '/'));
   al_destroy_path(path);
}

void Prog::initial_config()
{
   Voice *voice = new_voice();
   if (!voice) {
      abort_example("Could not create initial voice.\n");
   }
   voice->set_pos(300, 50);

   Mixer *mixer = new_mixer();
   mixer->set_pos(300, 150);
   voice->attach(mixer);

   SampleInstance *splinst = new_sample_instance();
   splinst->set_pos(220, 300);
   mixer->attach(splinst);
   splinst->toggle_playing();

   SampleInstance *splinst2 = new_sample_instance();
   splinst2->set_pos(120, 240);
   mixer->attach(splinst2);
   splinst2->toggle_playing();

   Mixer *mixer2 = new_mixer();
   mixer2->set_pos(500, 250);
   mixer->attach(mixer2);

   Audiostream *stream;
   if ((stream = new_audiostream())) {
      stream->set_pos(450, 350);
      mixer2->attach(stream);
   }
}

Voice *Prog::new_voice()
{
   Voice *voice = new Voice();
   if (voice->valid()) {
      elements.push_back(voice);
   }
   else {
      delete voice;
      voice = NULL;
   }
   return voice;
}

Mixer *Prog::new_mixer()
{
   Mixer *mixer = new Mixer();
   elements.push_back(mixer);
   return mixer;
}

SampleInstance *Prog::new_sample_instance()
{
   SampleInstance *splinst = new SampleInstance();
   if (samples.size() > 0) {
      static unsigned i = 0;
      unsigned n = (i++) % samples.size();
      splinst->set_sample(*samples[n]);
      i++;
   }
   elements.push_back(splinst);
   return splinst;
}

Audiostream *Prog::new_audiostream()
{
   if (stream_paths.size() > 0) {
      static unsigned i = 0;
      unsigned n = (i++) % stream_paths.size();
      Audiostream *stream = new Audiostream(stream_paths[n]);
      if (stream->valid()) {
         elements.push_back(stream);
         return stream;
      }
      delete stream;
   }
   return NULL;
}

void Prog::run()
{
   for (;;) {
      if (al_is_event_queue_empty(queue)) {
         redraw();
      }

      A5O_EVENT ev;
      al_wait_for_event(queue, &ev);
      switch (ev.type) {
         case A5O_EVENT_DISPLAY_CLOSE:
            return;
         case A5O_EVENT_MOUSE_BUTTON_DOWN:
            process_mouse_button_down(ev.mouse.button, ev.mouse.x, ev.mouse.y);
            break;
         case A5O_EVENT_MOUSE_BUTTON_UP:
            process_mouse_button_up(ev.mouse.button, ev.mouse.x, ev.mouse.y);
            break;
         case A5O_EVENT_MOUSE_AXES:
            if (ev.mouse.dz) {
               process_mouse_wheel(ev.mouse.dz);
            }
            else {
               process_mouse_axes(ev.mouse.x, ev.mouse.y,
                  ev.mouse.dx, ev.mouse.dy);
            }
            break;
         case A5O_EVENT_KEY_CHAR:
            if (ev.keyboard.unichar == 27) {
               return;
            }
            process_key_char(ev.keyboard.unichar);
            break;
      }
   }
}

void Prog::process_mouse_button_down(int mb, int mx, int my)
{
   if (cur_button == 0) {
      cur_element = find_element(mx, my);
      if (cur_element) {
         cur_button = mb;
      }
      if (cur_button == 2) {
         connect_x = mx;
         connect_y = my;
      }
   }
}

void Prog::process_mouse_button_up(int mb, int mx, int my)
{
   if (mb != cur_button)
      return;

   if (cur_button == 2 && cur_element) {
      Element *sink = find_element(mx, my);
      cur_element->detach();
      if (sink && sink != cur_element) {
         sink->attach(cur_element);
      }
      cur_element = NULL;
   }

   cur_button = 0;
}

void Prog::process_mouse_axes(int mx, int my, int dx, int dy)
{
   if (cur_button == 1 && cur_element) {
      cur_element->move_by(dx, dy);
   }
   if (cur_button == 2 && cur_element) {
      connect_x = mx;
      connect_y = my;
   }
}

void Prog::process_mouse_wheel(int dz)
{
   if (cur_element) {
      cur_element->adjust_gain(dz * 0.1);
   }
}

void Prog::process_key_char(int unichar)
{
   int upper = toupper(unichar);
   if (upper == 'V') {
      new_voice();
   }
   else if (upper == 'M') {
      new_mixer();
   }
   else if (upper == 'S') {
      new_sample_instance();
   }
   else if (upper == 'A') {
      new_audiostream();
   }
   else if (upper == ' ') {
      if (cur_element) {
         cur_element->toggle_playing();
      }
   }
   else if (upper == 'X') {
      if (cur_element) {
         delete_element(cur_element);
         cur_element = NULL;
      }
   }
   else if (upper >= '1' && upper <= '9') {
      unsigned n = upper - '1';
      SampleInstance *splinst;
      if (n < samples.size() &&
         (splinst = dynamic_cast<SampleInstance *>(cur_element)))
      {
         splinst->set_sample(*samples.at(n));
      }
   }
}

Element *Prog::find_element(int x, int y)
{
   for (Element::iter it = elements.begin(); it != elements.end(); it++) {
      if ((*it)->contains(x, y))
         return *it;
   }
   return NULL;
}

void Prog::delete_element(Element *elt)
{
   for (Element::iter it = elements.begin(); it != elements.end(); it++) {
      if ((*it)->is_attached_to(elt)) {
         (*it)->detach();
      }
   }
   for (Element::iter it = elements.begin(); it != elements.end(); it++) {
      if (*it == elt) {
         (*it)->detach();
         delete *it;
         elements.erase(it);
         break;
      }
   }
}

void Prog::redraw()
{
   al_clear_to_color(ctx.bg);

   for (Element::iter it = elements.begin(); it != elements.end(); it++) {
      bool highlight = (*it == cur_element);
      (*it)->draw(ctx, highlight);

      if (highlight && cur_button == 2) {
         (*it)->draw_arrow(ctx, connect_x, connect_y, highlight);
      }
   }

   float y = al_get_display_height(dpy);
   float th = al_get_font_line_height(ctx.font);
   al_draw_textf(ctx.font, ctx.fg, 0, y-th*2, A5O_ALIGN_LEFT,
      "Create [v]oices, [m]ixers, [s]ample instances, [a]udiostreams.   "
      "[SPACE] pause playback.    "
      "[1]-[9] set sample.    "
      "[x] delete.");
   al_draw_textf(ctx.font, ctx.fg, 0, y-th*1, A5O_ALIGN_LEFT,
      "Mouse: [LMB] select element.   "
      "[RMB] attach sources to sinks "
      "(sample->mixer, mixer->mixer, mixer->voice, sample->voice)");

   al_flip_display();
}

int main(int argc, char **argv)
{
   Prog prog;
   prog.init();
   prog.add_sample("data/haiku/air_0.ogg");
   prog.add_sample("data/haiku/air_1.ogg");
   prog.add_sample("data/haiku/earth_0.ogg");
   prog.add_sample("data/haiku/earth_1.ogg");
   prog.add_sample("data/haiku/earth_2.ogg");
   prog.add_sample("data/haiku/fire_0.ogg");
   prog.add_sample("data/haiku/fire_1.ogg");
   prog.add_sample("data/haiku/water_0.ogg");
   prog.add_sample("data/haiku/water_1.ogg");
   prog.add_stream_path("../demos/cosmic_protector/data/sfx/game_music.ogg");
   prog.add_stream_path("../demos/cosmic_protector/data/sfx/title_music.ogg");
   prog.initial_config();
   prog.run();
   /* Let Allegro handle the cleanup. */
   return 0;

   (void)argc;
   (void)argv;
}

/* vim: set sts=3 sw=3 et: */
