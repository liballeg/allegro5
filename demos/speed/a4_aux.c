/*
 *    SPEED - by Shawn Hargreaves, 1999
 *
 *    Allegro 5 port by Peter Wang, 2010
 *
 *    The functions in this file resemble Allegro 4 functions but do not
 *    necessarily emulate the behaviour precisely.
 */

#include <math.h>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_primitives.h>

#include "a4_aux.h"

A5O_AUDIO_STREAM *_midi_stream;


/*
 * Configuration files
 */



/* emulate get_config_string() */
const char *get_config_string(const A5O_CONFIG *cfg, const char *section,
			      const char *name, const char *def)
{
   const char *v = al_get_config_value(cfg, section, name);

   return (v) ? v : def;
}



/* emulate get_config_int() */
int get_config_int(const A5O_CONFIG *cfg, const char *section,
		   const char *name, int def)
{
   const char *v = al_get_config_value(cfg, section, name);

   return (v) ? atoi(v) : def;
}



/* emulate set_config_int() */
void set_config_int(A5O_CONFIG *cfg, const char *section, const char *name,
		    int val)
{
   char buf[32];

   sprintf(buf, "%d", val);
   al_set_config_value(cfg, section, name, buf);
}



/*
 * Input routines
 */


#define MAX_KEYBUF   16


int key[A5O_KEY_MAX];

int joy_left;
int joy_right;
int joy_b1;

static int keybuf[MAX_KEYBUF];
static int keybuf_len = 0;
static A5O_MUTEX *keybuf_mutex;

static A5O_EVENT_QUEUE *input_queue;



/* initialises the input emulation */
void init_input()
{
   A5O_JOYSTICK *joy;

   keybuf_len = 0;
   keybuf_mutex = al_create_mutex();

   input_queue = al_create_event_queue();
   al_register_event_source(input_queue, al_get_keyboard_event_source());
   al_register_event_source(input_queue, al_get_display_event_source(screen));

   if (al_get_num_joysticks() > 0) {
      joy = al_get_joystick(0);
      if (joy)
	 al_register_event_source(input_queue, al_get_joystick_event_source());
   }
}



/* closes down the input emulation */
void shutdown_input()
{
   al_destroy_mutex(keybuf_mutex);
   keybuf_mutex = NULL;

   al_destroy_event_queue(input_queue);
   input_queue = NULL;
}



/* helper function to add a keypress to a buffer */
static void add_key(A5O_KEYBOARD_EVENT *event)
{
   if ((event->unichar == 0) || (event->unichar > 255))
      return;

   al_lock_mutex(keybuf_mutex);

   if (keybuf_len < MAX_KEYBUF) {
      keybuf[keybuf_len] = event->unichar | ((event->keycode << 8) & 0xff00);
      keybuf_len++;
   }

   al_unlock_mutex(keybuf_mutex);
}



/* emulate poll_keyboard() and poll_joystick() combined */
void poll_input()
{
   A5O_EVENT event;

   while (al_get_next_event(input_queue, &event)) {

      switch (event.type) {

    case A5O_EVENT_DISPLAY_RESIZE:
       al_acknowledge_resize(event.display.source);
       break;

	 case A5O_EVENT_KEY_DOWN:
	    key[event.keyboard.keycode] = 1;
	    break;

	 case A5O_EVENT_KEY_UP:
	    key[event.keyboard.keycode] = 0;
	    break;

	 case A5O_EVENT_KEY_CHAR:
	    add_key(&event.keyboard);
	    break;

	 case A5O_EVENT_JOYSTICK_BUTTON_DOWN:
	    if (event.joystick.button == 0)
	       joy_b1 = 1;
	    break;

	 case A5O_EVENT_JOYSTICK_BUTTON_UP:
	    if (event.joystick.button == 0)
	       joy_b1 = 0;
	    break;

	 case A5O_EVENT_JOYSTICK_AXIS:
	    if (event.joystick.stick == 0 && event.joystick.axis == 0) {
	       float pos = event.joystick.pos;
	       joy_left = (pos < 0.0);
	       joy_right = (pos > 0.0);
	    }
	    break;

	 case A5O_EVENT_TIMER:
	    /* retrace_count incremented */
	    break;

	 case A5O_EVENT_DISPLAY_EXPOSE:
	    break;
      }
   }
}



/* blocking version of poll_input(), also wakes on retrace_count */
void poll_input_wait()
{
   al_wait_for_event(input_queue, NULL);
   poll_input();
}



/* emulate keypressed() */
int keypressed()
{
   poll_input();

   return keybuf_len > 0;
}



/* emulate readkey(), except this version never blocks */
int readkey()
{
   int c = 0;

   poll_input();

   al_lock_mutex(keybuf_mutex);

   if (keybuf_len > 0) {
      c = keybuf[0];
      keybuf_len--;
      memmove(keybuf, keybuf + 1, sizeof(keybuf[0]) * keybuf_len);
   }

   al_unlock_mutex(keybuf_mutex);

   return c;
}



/* emulate clear_keybuf() */
void clear_keybuf()
{
   al_lock_mutex(keybuf_mutex);

   keybuf_len = 0;

   al_unlock_mutex(keybuf_mutex);
}



/*
 * Graphics routines
 */


#define MAX_POLYGON_VERTICES	 6


A5O_DISPLAY *screen;
A5O_FONT *font;
A5O_FONT *font_video;



/* like create_bitmap() */
A5O_BITMAP *create_memory_bitmap(int w, int h)
{
   int oldflags;
   int newflags;
   A5O_BITMAP *bmp;

   oldflags = al_get_new_bitmap_flags();
   newflags = (oldflags &~ A5O_VIDEO_BITMAP) | A5O_MEMORY_BITMAP;
   al_set_new_bitmap_flags(newflags);
   bmp = al_create_bitmap(w, h);
   al_set_new_bitmap_flags(oldflags);
   return bmp;
}



/* used to clone a video bitmap from a memory bitmap; no such function in A4 */
A5O_BITMAP *replace_bitmap(A5O_BITMAP *bmp)
{
   A5O_BITMAP *tmp = al_clone_bitmap(bmp);
   al_destroy_bitmap(bmp);
   return tmp;
}



void stretch_blit(BITMAP *bitmap, int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2) {
   al_draw_scaled_bitmap(bitmap, x1, y1, w1, h1, x2, y2, w2, h2, 0);
}



void blit(BITMAP *bitmap, int x1, int y1, int x2, int y2, int w, int h) {
   stretch_blit(bitmap, x1, y1, w, h, x2, y2, w, h);
}



void draw_sprite(BITMAP *bitmap, int x, int y) {
   al_draw_bitmap(bitmap, x, y, 0);
}



void rotate_sprite(BITMAP *bitmap, int x, int y, int angle) {
   al_draw_rotated_bitmap(bitmap, 0, 0, x, y, angle, 0);
}



/* approximate solid_mode() function, but we we use alpha for transparent
 * pixels instead of a mask color
 */
void solid_mode()
{
   al_set_blender(A5O_ADD, A5O_ONE, A5O_INVERSE_ALPHA);
}



/* emulate makecol() */
A5O_COLOR makecol(int r, int g, int b)
{
   return al_map_rgb(r, g, b);
}



/* emulate hline() */
void hline(int x1, int y, int x2, A5O_COLOR c)
{
   al_draw_line(x1+0.5, y+0.5, x2+0.5, y+0.5, c, 1);
}



/* emulate vline() */
void vline(int x, int y1, int y2, A5O_COLOR c)
{
   al_draw_line(x+0.5, y1+0.5, x+0.5, y2+0.5, c, 1);
}



/* emulate line() */
void line(int x1, int y1, int x2, int y2, A5O_COLOR color)
{
   al_draw_line(x1+0.5, y1+0.5, x2+0.5, y2+0.5, color, 1);
}



/* emulate rectfill() */
void rectfill(int x1, int y1, int x2, int y2, A5O_COLOR color)
{
   al_draw_filled_rectangle(x1, y1, x2+1, y2+1, color);
}



/* emulate circle() */
void circle(int x, int y, int radius, A5O_COLOR color)
{
   al_draw_circle(x+0.5, y+0.5, radius, color, 1);
}



/* emulate circlefill() */
void circlefill(int x, int y, int radius, A5O_COLOR color)
{
   al_draw_filled_circle(x+0.5, y+0.5, radius, color);
}



/* emulate stretch_sprite() */
void stretch_sprite(A5O_BITMAP *bmp, A5O_BITMAP *sprite,
		    int x, int y, int w, int h)
{
   A5O_STATE state;

   al_store_state(&state, A5O_STATE_TARGET_BITMAP);

   al_set_target_bitmap(bmp);
   al_draw_scaled_bitmap(sprite,
      0, 0, al_get_bitmap_width(sprite), al_get_bitmap_height(sprite),
      x, y, w, h, 0);

   al_restore_state(&state);
}



/* emulate polygon() for convex polygons only */
void polygon(int vertices, const int *points, A5O_COLOR color)
{
   A5O_VERTEX vtxs[MAX_POLYGON_VERTICES + 2];
   int i;

   assert(vertices <= MAX_POLYGON_VERTICES);

   vtxs[0].x = 0.0;
   vtxs[0].y = 0.0;
   vtxs[0].z = 0.0;
   vtxs[0].color = color;
   vtxs[0].u = 0;
   vtxs[0].v = 0;

   for (i = 0; i < vertices; i++) {
      vtxs[0].x += points[i*2];
      vtxs[0].y += points[i*2 + 1];
   }

   vtxs[0].x /= vertices;
   vtxs[0].y /= vertices;

   for (i = 1; i <= vertices; i++) {
      vtxs[i].x = points[0];
      vtxs[i].y = points[1];
      vtxs[i].z = 0.0;
      vtxs[i].color = color;
      vtxs[i].u = 0;
      vtxs[i].v = 0;
      points += 2;
   }

   vtxs[vertices + 1] = vtxs[1];

   al_draw_prim(vtxs, NULL, NULL, 0, vertices + 2, A5O_PRIM_TRIANGLE_FAN);
}



/* emulate textout() */
void textout(const A5O_FONT *font, const char *s, int x, int y, A5O_COLOR c)
{
   al_draw_text(font, c, x, y, A5O_ALIGN_LEFT, s);
}



/* emulate textout_centre() */
void textout_centre(const A5O_FONT *font, const char *s, int x, int y, A5O_COLOR c)
{
   al_draw_text(font, c, x, y, A5O_ALIGN_CENTRE, s);
}



/* emulate textprintf() */
void textprintf(const A5O_FONT *f, int x, int y, A5O_COLOR color, const char *fmt, ...)
{
   va_list ap;
   char buf[256];

   va_start(ap, fmt);
#if defined(__WATCOMC__) || defined(_MSC_VER)
   _vsnprintf(buf, sizeof(buf), fmt, ap);
   buf[sizeof(buf)-1] = '\0';
#else
   vsnprintf(buf, sizeof(buf), fmt, ap);
#endif
   va_end(ap);

   textout(f, buf, x, y, color);
}



/*
 * Matrix routines
 */



static const MATRIX_f identity_matrix_f = 
{
   {
      /* 3x3 identity */
      { 1.0, 0.0, 0.0 },
      { 0.0, 1.0, 0.0 },
      { 0.0, 0.0, 1.0 },
   },

   /* zero translation */
   { 0.0, 0.0, 0.0 }
};



/* get_scaling_matrix_f:
 *  Floating point version of get_scaling_matrix().
 */
void get_scaling_matrix_f(MATRIX_f *m, float x, float y, float z)
{
   *m = identity_matrix_f;

   m->v[0][0] = x;
   m->v[1][1] = y;
   m->v[2][2] = z;
}



/* get_z_rotate_matrix_f:
 *  Floating point version of get_z_rotate_matrix().
 */
void get_z_rotate_matrix_f(MATRIX_f *m, float r)
{
   float c, s;

   s = sin(r * A5O_PI / 128.0);
   c = cos(r * A5O_PI / 128.0);
   *m = identity_matrix_f;

   m->v[0][0] = c;
   m->v[0][1] = -s;

   m->v[1][0] = s;
   m->v[1][1] = c;
}



/* qtranslate_matrix_f:
 *  Floating point version of qtranslate_matrix().
 */
void qtranslate_matrix_f(MATRIX_f *m, float x, float y, float z)
{
   m->t[0] += x;
   m->t[1] += y;
   m->t[2] += z;
}



/* matrix_mul_f:
 *  Floating point version of matrix_mul().
 */
void matrix_mul_f(const MATRIX_f *m1, const MATRIX_f *m2, MATRIX_f *out)
{
   MATRIX_f temp;
   int i, j;

   if (m1 == out) {
      temp = *m1;
      m1 = &temp;
   }
   else if (m2 == out) {
      temp = *m2;
      m2 = &temp;
   }

   for (i=0; i<3; i++) {
      for (j=0; j<3; j++) {
	 out->v[i][j] = (m1->v[0][j] * m2->v[i][0]) +
			(m1->v[1][j] * m2->v[i][1]) +
			(m1->v[2][j] * m2->v[i][2]);
      }

      out->t[i] = (m1->t[0] * m2->v[i][0]) +
		  (m1->t[1] * m2->v[i][1]) +
		  (m1->t[2] * m2->v[i][2]) +
		  m2->t[i];
   }
}



/* apply_matrix_f:
 *  Floating point vector by matrix multiplication routine.
 */
void apply_matrix_f(const MATRIX_f *m, float x, float y, float z,
		    float *xout, float *yout, float *zout)
{
#define CALC_ROW(n) (x * m->v[(n)][0] + y * m->v[(n)][1] + z * m->v[(n)][2] + m->t[(n)])
   *xout = CALC_ROW(0);
   *yout = CALC_ROW(1);
   *zout = CALC_ROW(2);
#undef CALC_ROW
}



/*
 * Timing routines
 */


static A5O_TIMER *retrace_counter = NULL;



/* start incrementing retrace_count */
void start_retrace_count()
{
   retrace_counter = al_create_timer(1/70.0);
   al_register_event_source(input_queue, al_get_timer_event_source(retrace_counter));
   al_start_timer(retrace_counter);
}



/* stop incrementing retrace_count */
void stop_retrace_count()
{
   al_destroy_timer(retrace_counter);
   retrace_counter = NULL;
}



/* emulate 'retrace_count' variable */
int64_t retrace_count()
{
   return al_get_timer_count(retrace_counter);
}



/* emulate rest() */
void rest(unsigned int time)
{
   al_rest(0.001 * time);
}



void install_timer()
{
   init_input();
   start_retrace_count();
}



/*
 * Sound routines
 */



/* emulate create_sample(), for unsigned 8-bit mono samples */
A5O_SAMPLE *create_sample_u8(int freq, int len)
{
   char *buf = al_malloc(freq * len);

   return al_create_sample(buf, len, freq, A5O_AUDIO_DEPTH_UINT8,
			   A5O_CHANNEL_CONF_1, true);
}



/* emulate play_sample() */
void play_sample(A5O_SAMPLE *spl, int vol, int pan, int freq, int loop)
{
   int playmode = loop ? A5O_PLAYMODE_LOOP : A5O_PLAYMODE_ONCE;

   al_play_sample(spl, vol/255.0, (pan - 128)/128.0, freq/1000.0,
		  playmode, NULL);
}



int install_sound(void)
{
   if (!al_install_audio())
      return 0;
   al_reserve_samples(100);
   return 1;
}



void play_midi(char const *midi, int loop)
{
   stop_midi();
   _midi_stream = al_load_audio_stream(midi, 2, 4096);
   al_attach_audio_stream_to_mixer(_midi_stream, al_get_default_mixer());
   if (loop) {
      al_set_audio_stream_playmode(_midi_stream, A5O_PLAYMODE_LOOP);
   }
}



void stop_midi(void)
{
   if (_midi_stream) {
      al_destroy_audio_stream(_midi_stream);
      _midi_stream = NULL;
   }
}



int AL_RAND() {
   return rand();
}
