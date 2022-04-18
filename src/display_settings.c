/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *   Additional display settings (like multisample).
 *
 *   Original code from AllegroGL.
 *
 *   Heavily modified by Elias Pschernig.
 *
 *   See readme.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_display.h"
#include <math.h>

ALLEGRO_DEBUG_CHANNEL("display")


/* Function: al_set_new_display_option
 */
void al_set_new_display_option(int option, int value, int importance)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = _al_get_new_display_settings();
   switch (importance) {
      case ALLEGRO_REQUIRE:
         extras->required |= (int64_t)1 << option;
         extras->suggested &= ~((int64_t)1 << option);
         break;
      case ALLEGRO_SUGGEST:
         extras->suggested |= (int64_t)1 << option;
         extras->required &= ~((int64_t)1 << option);
         break;
      case ALLEGRO_DONTCARE:
         extras->required &= ~((int64_t)1 << option);
         extras->suggested &= ~((int64_t)1 << option);
         break;
   }
   extras->settings[option] = value;
}


int _al_get_suggested_display_option(ALLEGRO_DISPLAY *d,
   int option, int default_value)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *s = &d->extra_settings;
   uint64_t flags = s->required | s->suggested;
   if (flags & ((uint64_t)1 << option))
      return s->settings[option];
   return default_value;
}


/* Function: al_get_new_display_option
 */
int al_get_new_display_option(int option, int *importance)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = _al_get_new_display_settings();
   if (extras->required & ((int64_t)1 << option)) {
      if (importance) *importance = ALLEGRO_REQUIRE;
      return extras->settings[option];
   }
   if (extras->suggested & ((int64_t)1 << option)) {
      if (importance) *importance = ALLEGRO_SUGGEST;
      return extras->settings[option];
   }
   if (importance) *importance = ALLEGRO_DONTCARE;
   return 0;
}

/* Function: al_set_display_option
 */
void al_set_display_option(ALLEGRO_DISPLAY *display, int option, int value)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = &display->extra_settings;
   extras->settings[option] = value;
   if (display->vt->set_display_option) {
      display->vt->set_display_option(display, option, value);
   }
}

/* Function: al_get_display_option
 */
int al_get_display_option(ALLEGRO_DISPLAY *display, int option)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = &display->extra_settings;
   return extras->settings[option];
}


/* Function: al_reset_new_display_options
 */
void al_reset_new_display_options(void)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS *extras;
   extras = _al_get_new_display_settings();
   _al_fill_display_settings(extras);
}


#define req ref->required
#define sug ref->suggested


/* _al_fill_display_settings()
 * Will fill in missing settings by 'guessing' what the user intended.
 */
void _al_fill_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref)
{
   int all_components = (1<<ALLEGRO_RED_SIZE)
                      | (1<<ALLEGRO_GREEN_SIZE)
                      | (1<<ALLEGRO_BLUE_SIZE)
                      | (1<<ALLEGRO_ALPHA_SIZE);

   ASSERT(ref);

   /* If all color components were set, but not the color depth */
   if ((((req | sug) & (1<<ALLEGRO_COLOR_SIZE)) == 0)
    && (((req | sug) & all_components) == all_components)) {

       ref->settings[ALLEGRO_COLOR_SIZE] = ref->settings[ALLEGRO_RED_SIZE]
                                         + ref->settings[ALLEGRO_GREEN_SIZE]
                                         + ref->settings[ALLEGRO_BLUE_SIZE]
                                         + ref->settings[ALLEGRO_ALPHA_SIZE];

      /* Round depth to 8 bits */
      ref->settings[ALLEGRO_COLOR_SIZE] = (ref->settings[ALLEGRO_COLOR_SIZE] + 7) / 8;
   }
   /* If only some components were set, guess the others */
   else if ((req | sug) & all_components) {
      int avg = ((req | sug) & (1<<ALLEGRO_RED_SIZE)   ? ref->settings[ALLEGRO_RED_SIZE]: 0)
              + ((req | sug) & (1<<ALLEGRO_GREEN_SIZE) ? ref->settings[ALLEGRO_GREEN_SIZE]: 0)
              + ((req | sug) & (1<<ALLEGRO_BLUE_SIZE)  ? ref->settings[ALLEGRO_BLUE_SIZE]: 0)
              + ((req | sug) & (1<<ALLEGRO_ALPHA_SIZE) ? ref->settings[ALLEGRO_ALPHA_SIZE]: 0);

      int num = ((req | sug) & (1<<ALLEGRO_RED_SIZE)   ? 1 : 0)
              + ((req | sug) & (1<<ALLEGRO_GREEN_SIZE) ? 1 : 0)
              + ((req | sug) & (1<<ALLEGRO_BLUE_SIZE)  ? 1 : 0)
              + ((req | sug) & (1<<ALLEGRO_ALPHA_SIZE) ? 1 : 0);

      avg /= (num ? num : 1);

      if (((req | sug) & (1<<ALLEGRO_RED_SIZE) )== 0) {
         sug |= (1<<ALLEGRO_RED_SIZE);
         ref->settings[ALLEGRO_RED_SIZE] = avg;
      }
      if (((req | sug) & (1<<ALLEGRO_GREEN_SIZE)) == 0) {
         sug |= (1<<ALLEGRO_GREEN_SIZE);
         ref->settings[ALLEGRO_GREEN_SIZE] = avg;
      }
      if (((req | sug) & (1<<ALLEGRO_BLUE_SIZE)) == 0) {
         sug |= (1<<ALLEGRO_BLUE_SIZE);
         ref->settings[ALLEGRO_BLUE_SIZE] = avg;
      }
      if (((req | sug) & (1<<ALLEGRO_ALPHA_SIZE)) == 0) {
         sug |= (1<<ALLEGRO_ALPHA_SIZE);
         ref->settings[ALLEGRO_ALPHA_SIZE] = avg;
      }

      /* If color depth wasn't defined, figure it out */
      if (((req | sug) & (1<<ALLEGRO_COLOR_SIZE)) == 0) {
         _al_fill_display_settings(ref);
      }
   }

   /* Require double-buffering */
   if (!((req | sug) & (1<<ALLEGRO_SINGLE_BUFFER))) {
      al_set_new_display_option(ALLEGRO_SINGLE_BUFFER, 0, ALLEGRO_REQUIRE);
   }

   /* Prefer no multisamping */
   if (!((req | sug) & ((1<<ALLEGRO_SAMPLE_BUFFERS) | (1<<ALLEGRO_SAMPLES)))) {
      al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 0, ALLEGRO_SUGGEST);
      al_set_new_display_option(ALLEGRO_SAMPLES, 0, ALLEGRO_SUGGEST);
   }

   /* Prefer monoscopic */
   if (!((req | sug) & (1<<ALLEGRO_STEREO))) {
      al_set_new_display_option(ALLEGRO_STEREO, 0, ALLEGRO_SUGGEST);
   }

   /* Prefer accelerated */
   if (!((req | sug) & (1<<ALLEGRO_RENDER_METHOD))) {
      al_set_new_display_option(ALLEGRO_RENDER_METHOD, 1, ALLEGRO_SUGGEST);
   }

   /* Prefer unsigned normalized buffers */
   if (!((req | sug) & ((1<<ALLEGRO_FLOAT_COLOR) | (1<<ALLEGRO_FLOAT_DEPTH)))) {
      al_set_new_display_option(ALLEGRO_FLOAT_DEPTH, 0, ALLEGRO_SUGGEST);
      al_set_new_display_option(ALLEGRO_FLOAT_COLOR, 0, ALLEGRO_SUGGEST);
   }

   /* The display must meet Allegro requrements. */
   if (!((req | sug) & (1<<ALLEGRO_COMPATIBLE_DISPLAY))) {
      al_set_new_display_option(ALLEGRO_COMPATIBLE_DISPLAY, 1, ALLEGRO_REQUIRE);
   }

   al_set_new_display_option(ALLEGRO_DEFAULT_SHADER_PLATFORM, ALLEGRO_SHADER_AUTO, ALLEGRO_REQUIRE);
}

static void debug_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   ALLEGRO_DEBUG("color: %d (rgba %d%d%d%d), depth: %d, stencil: %d, "
         "acc: %d%d%d%d, samples: %d/%d\n",
      eds->settings[ALLEGRO_COLOR_SIZE],
      eds->settings[ALLEGRO_RED_SIZE],
      eds->settings[ALLEGRO_GREEN_SIZE],
      eds->settings[ALLEGRO_BLUE_SIZE],
      eds->settings[ALLEGRO_ALPHA_SIZE],
      eds->settings[ALLEGRO_DEPTH_SIZE],
      eds->settings[ALLEGRO_STENCIL_SIZE],
      eds->settings[ALLEGRO_ACC_RED_SIZE],
      eds->settings[ALLEGRO_ACC_RED_SIZE],
      eds->settings[ALLEGRO_ACC_RED_SIZE],
      eds->settings[ALLEGRO_ACC_RED_SIZE],
      eds->settings[ALLEGRO_SAMPLES],
      eds->settings[ALLEGRO_SAMPLE_BUFFERS]);
}

int _al_score_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds,
                               ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref)
{
   int score = 0;

   debug_display_settings(eds);

   if (eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] != ref->settings[ALLEGRO_COMPATIBLE_DISPLAY]) {
      if (req & (1<<ALLEGRO_COMPATIBLE_DISPLAY)) {
         ALLEGRO_DEBUG("Display not compatible with Allegro.\n");
         return -1;
      }
   }
   else {
      score += 128;
   }

   if (eds->settings[ALLEGRO_VSYNC] != ref->settings[ALLEGRO_VSYNC]) {
      if (req & (1<<ALLEGRO_VSYNC)) {
         ALLEGRO_DEBUG("Vsync requirement not met.\n");
         return -1;
      }
   }
   else {
      score += 128;
   }

   if (eds->settings[ALLEGRO_COLOR_SIZE] != ref->settings[ALLEGRO_COLOR_SIZE]) {
      if (req & (1<<ALLEGRO_COLOR_SIZE)) {
         ALLEGRO_DEBUG("Color depth requirement not met.\n");
         return -1;
      }
   }
   else {
      /* If requested color depths agree */
      score += 128;
   }

   if (sug & (1<<ALLEGRO_COLOR_SIZE)) {
      if (eds->settings[ALLEGRO_COLOR_SIZE] < ref->settings[ALLEGRO_COLOR_SIZE])
         score += (96 * eds->settings[ALLEGRO_COLOR_SIZE]) / ref->settings[ALLEGRO_COLOR_SIZE];
      else
         score += 96 + 96 / (1 + eds->settings[ALLEGRO_COLOR_SIZE] - ref->settings[ALLEGRO_COLOR_SIZE]);
   }

   /* check colour component widths here and Allegro formatness */
   if ((req & (1<<ALLEGRO_RED_SIZE))
    && (eds->settings[ALLEGRO_RED_SIZE] != ref->settings[ALLEGRO_RED_SIZE])) {
      ALLEGRO_DEBUG("Red depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_RED_SIZE)) {
      if (eds->settings[ALLEGRO_RED_SIZE] < ref->settings[ALLEGRO_RED_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_RED_SIZE]) / ref->settings[ALLEGRO_RED_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_RED_SIZE] - ref->settings[ALLEGRO_RED_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_GREEN_SIZE))
    && (eds->settings[ALLEGRO_GREEN_SIZE] != ref->settings[ALLEGRO_GREEN_SIZE])) {
      ALLEGRO_DEBUG("Green depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_GREEN_SIZE)) {
      if (eds->settings[ALLEGRO_GREEN_SIZE] < ref->settings[ALLEGRO_GREEN_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_GREEN_SIZE]) / ref->settings[ALLEGRO_GREEN_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_GREEN_SIZE] - ref->settings[ALLEGRO_GREEN_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_BLUE_SIZE))
    && (eds->settings[ALLEGRO_BLUE_SIZE] != ref->settings[ALLEGRO_BLUE_SIZE])) {
      ALLEGRO_DEBUG("Blue depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_BLUE_SIZE)) {
      if (eds->settings[ALLEGRO_BLUE_SIZE] < ref->settings[ALLEGRO_BLUE_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_BLUE_SIZE]) / ref->settings[ALLEGRO_BLUE_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_BLUE_SIZE] - ref->settings[ALLEGRO_BLUE_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_ALPHA_SIZE))
    && (eds->settings[ALLEGRO_ALPHA_SIZE] != ref->settings[ALLEGRO_ALPHA_SIZE])) {
      ALLEGRO_DEBUG("Alpha depth requirement not met (%d instead of %d).\n",
         eds->settings[ALLEGRO_ALPHA_SIZE], ref->settings[ALLEGRO_ALPHA_SIZE]);
      return -1;
   }

   if (sug & (1<<ALLEGRO_ALPHA_SIZE)) {
      if (eds->settings[ALLEGRO_ALPHA_SIZE] < ref->settings[ALLEGRO_ALPHA_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_ALPHA_SIZE]) / ref->settings[ALLEGRO_ALPHA_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_ALPHA_SIZE] - ref->settings[ALLEGRO_ALPHA_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_ACC_RED_SIZE))
    && (eds->settings[ALLEGRO_ACC_RED_SIZE] != ref->settings[ALLEGRO_ACC_RED_SIZE])) {
      ALLEGRO_DEBUG("Accumulator Red depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_ACC_RED_SIZE)) {
      if (eds->settings[ALLEGRO_ACC_RED_SIZE] < ref->settings[ALLEGRO_ACC_RED_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_ACC_RED_SIZE]) / ref->settings[ALLEGRO_ACC_RED_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_ACC_RED_SIZE] - ref->settings[ALLEGRO_ACC_RED_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_ACC_GREEN_SIZE))
    && (eds->settings[ALLEGRO_ACC_GREEN_SIZE] != ref->settings[ALLEGRO_ACC_GREEN_SIZE])) {
      ALLEGRO_DEBUG("Accumulator Green depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_ACC_GREEN_SIZE)) {
      if (eds->settings[ALLEGRO_ACC_GREEN_SIZE] < ref->settings[ALLEGRO_ACC_GREEN_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_ACC_GREEN_SIZE]) / ref->settings[ALLEGRO_ACC_GREEN_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_ACC_GREEN_SIZE] - ref->settings[ALLEGRO_ACC_GREEN_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_ACC_BLUE_SIZE))
    && (eds->settings[ALLEGRO_ACC_BLUE_SIZE] != ref->settings[ALLEGRO_ACC_BLUE_SIZE])) {
      ALLEGRO_DEBUG("Accumulator Blue depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_ACC_BLUE_SIZE)) {
      if (eds->settings[ALLEGRO_ACC_BLUE_SIZE] < ref->settings[ALLEGRO_ACC_BLUE_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_ACC_BLUE_SIZE]) / ref->settings[ALLEGRO_ACC_BLUE_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_ACC_BLUE_SIZE] - ref->settings[ALLEGRO_ACC_BLUE_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_ACC_ALPHA_SIZE))
    && (eds->settings[ALLEGRO_ACC_ALPHA_SIZE] != ref->settings[ALLEGRO_ACC_ALPHA_SIZE])) {
      ALLEGRO_DEBUG("Accumulator Alpha depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_ACC_ALPHA_SIZE)) {
      if (eds->settings[ALLEGRO_ACC_ALPHA_SIZE] < ref->settings[ALLEGRO_ACC_ALPHA_SIZE]) {
         score += (16 * eds->settings[ALLEGRO_ACC_ALPHA_SIZE]) / ref->settings[ALLEGRO_ACC_ALPHA_SIZE];
      }
      else {
         score += 16 + 16 / (1 + eds->settings[ALLEGRO_ACC_ALPHA_SIZE] - ref->settings[ALLEGRO_ACC_ALPHA_SIZE]);
      }
   }

   if (!eds->settings[ALLEGRO_SINGLE_BUFFER] != !ref->settings[ALLEGRO_SINGLE_BUFFER]) {
      if (req & (1<<ALLEGRO_SINGLE_BUFFER)) {
         ALLEGRO_DEBUG("Single Buffer requirement not met.\n");
         return -1;
      }
   }
   else {
      score += (sug & (1<<ALLEGRO_SINGLE_BUFFER)) ? 256 : 1;
   }

   if (!eds->settings[ALLEGRO_STEREO] != !ref->settings[ALLEGRO_STEREO]) {
      if (req & (1<<ALLEGRO_STEREO)) {
         ALLEGRO_DEBUG("Stereo Buffer requirement not met.\n");
         return -1;
      }
   }
   else {
      if (sug & (1<<ALLEGRO_STEREO)) {
         score += 128;
      }
   }

   if ((req & (1<<ALLEGRO_AUX_BUFFERS)) &&
      (eds->settings[ALLEGRO_AUX_BUFFERS] < ref->settings[ALLEGRO_AUX_BUFFERS])) {
      ALLEGRO_DEBUG("Aux Buffer requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_AUX_BUFFERS)) {
      if (eds->settings[ALLEGRO_AUX_BUFFERS] < ref->settings[ALLEGRO_AUX_BUFFERS]) {
         score += (64 * eds->settings[ALLEGRO_AUX_BUFFERS]) / ref->settings[ALLEGRO_AUX_BUFFERS];
      }
      else {
         score += 64 + 64 / (1 + eds->settings[ALLEGRO_AUX_BUFFERS] - ref->settings[ALLEGRO_AUX_BUFFERS]);
      }
   }

   if ((req & (1<<ALLEGRO_DEPTH_SIZE)) &&
      (eds->settings[ALLEGRO_DEPTH_SIZE] != ref->settings[ALLEGRO_DEPTH_SIZE])) {
      ALLEGRO_DEBUG("Z-Buffer requirement not met.\n");
      return -1;
   }
   if (sug & (1<<ALLEGRO_DEPTH_SIZE)) {
      if (eds->settings[ALLEGRO_DEPTH_SIZE] < ref->settings[ALLEGRO_DEPTH_SIZE]) {
         score += (64 * eds->settings[ALLEGRO_DEPTH_SIZE]) / ref->settings[ALLEGRO_DEPTH_SIZE];
      }
      else {
         score += 64 + 64 / (1 + eds->settings[ALLEGRO_DEPTH_SIZE] - ref->settings[ALLEGRO_DEPTH_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_STENCIL_SIZE))
    && (eds->settings[ALLEGRO_STENCIL_SIZE] != ref->settings[ALLEGRO_STENCIL_SIZE])) {
      ALLEGRO_DEBUG("Stencil depth requirement not met.\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_STENCIL_SIZE)) {
      if (eds->settings[ALLEGRO_STENCIL_SIZE] < ref->settings[ALLEGRO_STENCIL_SIZE]) {
         score += (64 * eds->settings[ALLEGRO_STENCIL_SIZE]) / ref->settings[ALLEGRO_STENCIL_SIZE];
      }
      else {
         score += 64 + 64 / (1 + eds->settings[ALLEGRO_STENCIL_SIZE] - ref->settings[ALLEGRO_STENCIL_SIZE]);
      }
   }

   if ((req & (1<<ALLEGRO_RENDER_METHOD))
     && ((eds->settings[ALLEGRO_RENDER_METHOD] != ref->settings[ALLEGRO_RENDER_METHOD])
     || (ref->settings[ALLEGRO_RENDER_METHOD] == 2))) {
      ALLEGRO_DEBUG("Render Method requirement not met.\n");
      return -1;
   }

   if ((sug & (1<<ALLEGRO_RENDER_METHOD))
      && (eds->settings[ALLEGRO_RENDER_METHOD] == ref->settings[ALLEGRO_RENDER_METHOD])) {
      score += 1024;
   }
   else if (eds->settings[ALLEGRO_RENDER_METHOD] == 1) {
      score++; /* Add 1 for hw accel */
   }

   if ((req & (1<<ALLEGRO_SAMPLE_BUFFERS))
    && (eds->settings[ALLEGRO_SAMPLE_BUFFERS] != ref->settings[ALLEGRO_SAMPLE_BUFFERS])) {
         ALLEGRO_DEBUG("Multisample Buffers requirement not met\n");
         return -1;
   }
   else if (sug & (1<<ALLEGRO_SAMPLE_BUFFERS)) {
      if (eds->settings[ALLEGRO_SAMPLE_BUFFERS] == ref->settings[ALLEGRO_SAMPLE_BUFFERS]) {
         score += 128;
      }
   }

   if ((req & (1<<ALLEGRO_SAMPLES))
      && (eds->settings[ALLEGRO_SAMPLES] != ref->settings[ALLEGRO_SAMPLES])) {
      ALLEGRO_DEBUG("Multisample Samples requirement not met\n");
      return -1;
   }

   if (sug & (1<<ALLEGRO_SAMPLES)) {
      if (eds->settings[ALLEGRO_SAMPLES] < ref->settings[ALLEGRO_SAMPLES]) {
         score += (64 * eds->settings[ALLEGRO_SAMPLES]) / ref->settings[ALLEGRO_SAMPLES];
      }
      else {
         score += 64 + 64 / (1 + eds->settings[ALLEGRO_SAMPLES] - ref->settings[ALLEGRO_SAMPLES]);
      }
   }

   if (!eds->settings[ALLEGRO_FLOAT_COLOR] != !ref->settings[ALLEGRO_FLOAT_COLOR]) {
      if (req & (1<<ALLEGRO_FLOAT_COLOR)) {
         ALLEGRO_DEBUG("Float Color requirement not met.\n");
         return -1;
      }
   }
   else {
      if (sug & (1<<ALLEGRO_FLOAT_COLOR)) {
         score += 128;
      }
   }

   if (!eds->settings[ALLEGRO_FLOAT_DEPTH] != !ref->settings[ALLEGRO_FLOAT_DEPTH]) {
      if (req & (1<<ALLEGRO_FLOAT_DEPTH)) {
         ALLEGRO_DEBUG("Float Depth requirement not met.\n");
         return -1;
      }
   }
   else {
      if (sug & (1<<ALLEGRO_FLOAT_DEPTH)) {
         score += 128;
      }
   }

   ALLEGRO_DEBUG("Score is : %i\n", score);
   return score;
}

#undef req
#undef sug


/* Helper function for sorting pixel formats by score */
int _al_display_settings_sorter(const void *p0, const void *p1)
{
   const ALLEGRO_EXTRA_DISPLAY_SETTINGS *f0 = *((ALLEGRO_EXTRA_DISPLAY_SETTINGS **)p0);
   const ALLEGRO_EXTRA_DISPLAY_SETTINGS *f1 = *((ALLEGRO_EXTRA_DISPLAY_SETTINGS **)p1);

   if (!f0 && !f1)
      return 0;
   if (!f0)
      return 1;
   if (!f1)
      return -1;
   if (f0->score == f1->score)
      return f0->index - f1->index; /* lower better */
   else
      return f1->score - f0->score; /* higher better */
}


int _al_deduce_color_format(ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds)
{
   /* dummy value to check if the format was detected */
   int format = ALLEGRO_PIXEL_FORMAT_ANY;

   if (eds->settings[ALLEGRO_RED_SIZE]   == 8 &&
       eds->settings[ALLEGRO_GREEN_SIZE] == 8 &&
       eds->settings[ALLEGRO_BLUE_SIZE]  == 8) {
      if (eds->settings[ALLEGRO_ALPHA_SIZE] == 8 &&
          eds->settings[ALLEGRO_COLOR_SIZE] == 32) {
         if (eds->settings[ALLEGRO_ALPHA_SHIFT] == 0 &&
             eds->settings[ALLEGRO_BLUE_SHIFT]  == 8 &&
             eds->settings[ALLEGRO_GREEN_SHIFT] == 16 &&
             eds->settings[ALLEGRO_RED_SHIFT]   == 24) {
            format = ALLEGRO_PIXEL_FORMAT_RGBA_8888;
         }
         else if (eds->settings[ALLEGRO_ALPHA_SHIFT] == 24 &&
                  eds->settings[ALLEGRO_RED_SHIFT]   == 0 &&
                  eds->settings[ALLEGRO_GREEN_SHIFT] == 8 &&
                  eds->settings[ALLEGRO_BLUE_SHIFT] == 16) {
            format = ALLEGRO_PIXEL_FORMAT_ABGR_8888;
         }
         else if (eds->settings[ALLEGRO_ALPHA_SHIFT] == 24 &&
                  eds->settings[ALLEGRO_RED_SHIFT] == 16 &&
                  eds->settings[ALLEGRO_GREEN_SHIFT] == 8 &&
                  eds->settings[ALLEGRO_BLUE_SHIFT] == 0) {
            format = ALLEGRO_PIXEL_FORMAT_ARGB_8888;
         }
      }
      else if (eds->settings[ALLEGRO_ALPHA_SIZE] == 0 &&
               eds->settings[ALLEGRO_COLOR_SIZE] == 24) {
         if (eds->settings[ALLEGRO_BLUE_SHIFT] == 0 &&
             eds->settings[ALLEGRO_GREEN_SHIFT] == 8 &&
             eds->settings[ALLEGRO_RED_SHIFT] == 16) {
            format = ALLEGRO_PIXEL_FORMAT_RGB_888;
         }
         else if (eds->settings[ALLEGRO_RED_SHIFT] == 0 &&
                  eds->settings[ALLEGRO_GREEN_SHIFT] == 8 &&
                  eds->settings[ALLEGRO_BLUE_SHIFT] == 16) {
            format = ALLEGRO_PIXEL_FORMAT_BGR_888;
         }
      }
      else if (eds->settings[ALLEGRO_ALPHA_SIZE] == 0 &&
               eds->settings[ALLEGRO_COLOR_SIZE] == 32) {
         if (eds->settings[ALLEGRO_BLUE_SHIFT] == 0 &&
             eds->settings[ALLEGRO_GREEN_SHIFT] == 8 &&
             eds->settings[ALLEGRO_RED_SHIFT] == 16) {
            format = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
         }
         else if (eds->settings[ALLEGRO_RED_SHIFT] == 0 &&
                  eds->settings[ALLEGRO_GREEN_SHIFT] == 8 &&
                  eds->settings[ALLEGRO_BLUE_SHIFT] == 16) {
            format = ALLEGRO_PIXEL_FORMAT_XBGR_8888;
         }
         else if (eds->settings[ALLEGRO_RED_SHIFT] == 24 &&
                  eds->settings[ALLEGRO_GREEN_SHIFT] == 16 &&
                  eds->settings[ALLEGRO_BLUE_SHIFT] == 8) {
            format = ALLEGRO_PIXEL_FORMAT_RGBX_8888;
         }
      }
   }
   else if (eds->settings[ALLEGRO_RED_SIZE] == 5 &&
            eds->settings[ALLEGRO_GREEN_SIZE] == 6 &&
            eds->settings[ALLEGRO_BLUE_SIZE] == 5) {
      if (eds->settings[ALLEGRO_RED_SHIFT] == 0 &&
          eds->settings[ALLEGRO_GREEN_SHIFT] == 5 &&
          eds->settings[ALLEGRO_BLUE_SHIFT] == 11) {
         format = ALLEGRO_PIXEL_FORMAT_BGR_565;
      }
      else if (eds->settings[ALLEGRO_BLUE_SHIFT] == 0 &&
               eds->settings[ALLEGRO_GREEN_SHIFT] == 5 &&
               eds->settings[ALLEGRO_RED_SHIFT] == 11) {
         format = ALLEGRO_PIXEL_FORMAT_RGB_565;
      }
   }
   else if (eds->settings[ALLEGRO_RED_SIZE] == 5 &&
            eds->settings[ALLEGRO_GREEN_SIZE] == 5 &&
            eds->settings[ALLEGRO_BLUE_SIZE] == 5) {
      if (eds->settings[ALLEGRO_ALPHA_SIZE] == 1 &&
          eds->settings[ALLEGRO_COLOR_SIZE] == 16) {
         if (eds->settings[ALLEGRO_ALPHA_SHIFT] == 0 &&
             eds->settings[ALLEGRO_BLUE_SHIFT]  == 1 &&
             eds->settings[ALLEGRO_GREEN_SHIFT] == 6 &&
             eds->settings[ALLEGRO_RED_SHIFT]   == 11) {
            format = ALLEGRO_PIXEL_FORMAT_RGBA_5551;
         }
         if (eds->settings[ALLEGRO_ALPHA_SHIFT] == 15 &&
             eds->settings[ALLEGRO_BLUE_SHIFT]  == 0 &&
             eds->settings[ALLEGRO_GREEN_SHIFT] == 5 &&
             eds->settings[ALLEGRO_RED_SHIFT]   == 10) {
            format = ALLEGRO_PIXEL_FORMAT_ARGB_1555;
         }
      }
   }
   else if (eds->settings[ALLEGRO_RED_SIZE] == 4 &&
            eds->settings[ALLEGRO_GREEN_SIZE] == 4 &&
            eds->settings[ALLEGRO_BLUE_SIZE] == 4) {
      if (eds->settings[ALLEGRO_ALPHA_SIZE] == 4 &&
          eds->settings[ALLEGRO_COLOR_SIZE] == 16) {
         if (eds->settings[ALLEGRO_ALPHA_SHIFT] == 12 &&
             eds->settings[ALLEGRO_BLUE_SHIFT]  == 0 &&
             eds->settings[ALLEGRO_GREEN_SHIFT] == 4 &&
             eds->settings[ALLEGRO_RED_SHIFT]   == 8) {
            format = ALLEGRO_PIXEL_FORMAT_ARGB_4444;
         }
         else if (eds->settings[ALLEGRO_ALPHA_SHIFT] == 12 &&
             eds->settings[ALLEGRO_BLUE_SHIFT]  == 8 &&
             eds->settings[ALLEGRO_GREEN_SHIFT] == 4 &&
             eds->settings[ALLEGRO_RED_SHIFT]   == 0) {
            format = ALLEGRO_PIXEL_FORMAT_RGBA_4444;
         }
      }
   }

   if (format == ALLEGRO_PIXEL_FORMAT_ANY) {
      ALLEGRO_WARN(
         "Could not deduce color format, sizes = (%d,%d,%d,%d,%d), shifts = (%d,%d,%d,%d)\n",
          eds->settings[ALLEGRO_RED_SIZE],
          eds->settings[ALLEGRO_GREEN_SIZE],
          eds->settings[ALLEGRO_BLUE_SIZE],
          eds->settings[ALLEGRO_ALPHA_SIZE],
          eds->settings[ALLEGRO_COLOR_SIZE],
          eds->settings[ALLEGRO_RED_SHIFT],
          eds->settings[ALLEGRO_GREEN_SHIFT],
          eds->settings[ALLEGRO_BLUE_SHIFT],
          eds->settings[ALLEGRO_ALPHA_SHIFT]);
   }

   return format;
}


void _al_set_color_components(int format, ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds,
                              int importance)
{
   ALLEGRO_EXTRA_DISPLAY_SETTINGS old_eds;
   memcpy(&old_eds, _al_get_new_display_settings(), sizeof(old_eds));
   _al_set_new_display_settings(eds);

   al_set_new_display_option(ALLEGRO_RED_SIZE,    0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_RED_SHIFT,   0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_GREEN_SIZE,  0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_BLUE_SIZE,   0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_ALPHA_SIZE,  0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, ALLEGRO_DONTCARE);
   al_set_new_display_option(ALLEGRO_COLOR_SIZE,  0, ALLEGRO_DONTCARE);

   switch (format) {
      case ALLEGRO_PIXEL_FORMAT_ANY_NO_ALPHA:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE,   0, importance);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_WITH_ALPHA:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE,   8, importance);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_NO_ALPHA:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE,   0, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE,   16, importance);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_16_WITH_ALPHA:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE,   1, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE,   16, importance);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE,   0, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE,   24, importance);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_NO_ALPHA:
         /* With OpenGL drivers, we never "know" the actual pixel
          * format. We use glReadPixels when we lock the screen, so
          * we can always lock the screen in any format we want. There
          * is no "display format".
          * 
          * Therefore it makes no sense to fail display creation
          * if either an RGB or RGBX format was requested but the
          * other seems available only in WGL/GLX (those really report
          * the number of bits used for red/green/blue/alpha to us only.
          * They never report any "X bits".).
          */
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE,   0, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE,   32, ALLEGRO_SUGGEST);
         break;
      case ALLEGRO_PIXEL_FORMAT_ANY_32_WITH_ALPHA:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE,   8, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE,   32, importance);
         break;
   }

   switch (format) {
      case ALLEGRO_PIXEL_FORMAT_RGBA_8888:
      case ALLEGRO_PIXEL_FORMAT_ABGR_8888:
      case ALLEGRO_PIXEL_FORMAT_ARGB_8888:
      case ALLEGRO_PIXEL_FORMAT_RGB_888:
      case ALLEGRO_PIXEL_FORMAT_BGR_888:
      case ALLEGRO_PIXEL_FORMAT_RGBX_8888:
      case ALLEGRO_PIXEL_FORMAT_XRGB_8888:
      case ALLEGRO_PIXEL_FORMAT_XBGR_8888:
         al_set_new_display_option(ALLEGRO_RED_SIZE,   8, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SIZE, 8, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SIZE,  8, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_BGR_565:
      case ALLEGRO_PIXEL_FORMAT_RGB_565:
         al_set_new_display_option(ALLEGRO_RED_SIZE,   5, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SIZE, 6, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SIZE,  5, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_RGBA_5551:
      case ALLEGRO_PIXEL_FORMAT_ARGB_1555:
         al_set_new_display_option(ALLEGRO_RED_SIZE,   5, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SIZE, 5, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SIZE,  5, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_ARGB_4444:
      case ALLEGRO_PIXEL_FORMAT_RGBA_4444:
         al_set_new_display_option(ALLEGRO_RED_SIZE,   4, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SIZE, 4, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SIZE,  4, importance);
      break;
   }

   switch (format) {
      case ALLEGRO_PIXEL_FORMAT_RGBA_8888:
      case ALLEGRO_PIXEL_FORMAT_ABGR_8888:
      case ALLEGRO_PIXEL_FORMAT_ARGB_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE, 8, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_RGB_888:
      case ALLEGRO_PIXEL_FORMAT_BGR_888:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE, 0, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE, 24, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_RGBX_8888:
      case ALLEGRO_PIXEL_FORMAT_XRGB_8888:
      case ALLEGRO_PIXEL_FORMAT_XBGR_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE, 0, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE, 32, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_BGR_565:
      case ALLEGRO_PIXEL_FORMAT_RGB_565:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE, 0, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE, 16, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_RGBA_5551:
      case ALLEGRO_PIXEL_FORMAT_ARGB_1555:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE, 1, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE, 16, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_ARGB_4444:
      case ALLEGRO_PIXEL_FORMAT_RGBA_4444:
         al_set_new_display_option(ALLEGRO_ALPHA_SIZE, 4, importance);
         al_set_new_display_option(ALLEGRO_COLOR_SIZE, 16, importance);
      break;
   }

   switch (format) {
      case ALLEGRO_PIXEL_FORMAT_RGBA_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  8, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 16, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   24, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_ABGR_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 24, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  16, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 8, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   0, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_ARGB_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 24, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  0, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 8, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   16, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_RGB_888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  0, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 8, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   16, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_BGR_888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  16, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 8, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   0, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_XRGB_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  0, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 8, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   16, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_RGBX_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  8, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 16, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   24, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_XBGR_8888:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  16, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 8, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   0, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_BGR_565:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  11, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 5, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   0, importance);
      break;
      case ALLEGRO_PIXEL_FORMAT_RGB_565:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  0, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 5, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   11, importance);      
      break;
      case ALLEGRO_PIXEL_FORMAT_RGBA_5551:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  1, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 6, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   11, importance);      
      break;
      case ALLEGRO_PIXEL_FORMAT_ARGB_1555:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 15, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  0, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 5, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   10, importance);      
      break;
      case ALLEGRO_PIXEL_FORMAT_ARGB_4444:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 12, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  0, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 4, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   8, importance);      
      break;
      case ALLEGRO_PIXEL_FORMAT_RGBA_4444:
         al_set_new_display_option(ALLEGRO_ALPHA_SHIFT, 0, importance);
         al_set_new_display_option(ALLEGRO_BLUE_SHIFT,  4, importance);
         al_set_new_display_option(ALLEGRO_GREEN_SHIFT, 8, importance);
         al_set_new_display_option(ALLEGRO_RED_SHIFT,   12, importance);      
      break;
   }

   memcpy(eds, _al_get_new_display_settings(), sizeof(*eds));
   _al_set_new_display_settings(&old_eds);
}
