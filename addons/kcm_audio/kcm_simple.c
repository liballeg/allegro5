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
 *      Simple audio interface.
 *
 *      By Peter Wang.
 *
 *      See LICENSE.txt for copyright information.
 */

/* Title: Simple audio interface
 */

#include "allegro5/allegro5.h"
#include "allegro5/kcm_audio.h"
#include "allegro5/internal/aintern_vector.h"


static ALLEGRO_VOICE *simp_voice;
static ALLEGRO_MIXER *simp_mixer;
static _AL_VECTOR simp_samples = _AL_VECTOR_INITIALIZER(ALLEGRO_SAMPLE *);


static bool do_play_sample_data(ALLEGRO_SAMPLE *spl, ALLEGRO_SAMPLE_DATA *data);


/* Function: al_setup_simple_audio
 *  Set up the easy audio layer.  <al_install_audio> must have been called
 *  first.  This function will create a single voice with an attached mixer, as
 *  well as 'reserve_samples' number of samples attached to the mixer.
 *  Returns true on success, false on error.
 */
bool al_setup_simple_audio(int reserve_samples)
{
   int i;

   simp_voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
      ALLEGRO_CHANNEL_CONF_2);
   if (!simp_voice) {
      TRACE("al_create_voice failed\n");
      goto Error;
   }

   simp_mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
      ALLEGRO_CHANNEL_CONF_2);
   if (!simp_mixer) {
      TRACE("al_create_voice failed\n");
      goto Error;
   }

   if (al_attach_mixer_to_voice(simp_voice, simp_mixer) != 0) {
      TRACE("al_attach_mixer_to_voice failed\n");
      goto Error;
   }

   for (i = 0; i < reserve_samples; i++) {
      ALLEGRO_SAMPLE **slot = _al_vector_alloc_back(&simp_samples);
      *slot = al_create_sample(NULL);
      if (!*slot) {
         TRACE("al_create_sample failed\n");
         goto Error;
      }
      if (al_attach_sample_to_mixer(simp_mixer, *slot) != 0) {
         TRACE("al_attach_mixer_to_sample failed\n");
         goto Error;
      }
   }

   return true;

 Error:

   al_shutdown_simple_audio();
   return false;
}


/* Function: al_shutdown_simple_audio
 *  Shut down the easy audio layer.
 *  XXX should it be enough to call <al_uninstall_audio>?
 */
void al_shutdown_simple_audio(void)
{
   unsigned int i;

   al_stop_all_simple_samples();

   for (i = 0; i < _al_vector_size(&simp_samples); i++) {
      ALLEGRO_SAMPLE **slot = _al_vector_ref(&simp_samples, i);
      al_destroy_sample(*slot);
   }
   _al_vector_free(&simp_samples);

   al_destroy_mixer(simp_mixer);
   simp_mixer = NULL;

   al_destroy_voice(simp_voice);
   simp_voice = NULL;
}


/* Function: al_get_simple_audio_voice
 *  Return the voice created by <al_setup_simple_audio>.
 */
ALLEGRO_VOICE *al_get_simple_audio_voice(void)
{
   return simp_voice;
}


/* Function: al_get_simple_audio_mixer
 *  Return the mixer created by <al_setup_simple_audio>.
 */
ALLEGRO_MIXER *al_get_simple_audio_mixer(void)
{
   return simp_mixer;
}


/* Function: al_play_sample_data
 *  Play an instance of a sample data, using one of the samples created by
 *  <al_setup_simple_audio>.  Returns true on success, false on failure.
 *  Playback may fail because all the reserved samples are currently used.
 */
bool al_play_sample_data(ALLEGRO_SAMPLE_DATA *data)
{
   unsigned int i;
   ASSERT(data);

   for (i = 0; i < _al_vector_size(&simp_samples); i++) {
      ALLEGRO_SAMPLE **slot = _al_vector_ref(&simp_samples, i);
      ALLEGRO_SAMPLE *spl = (*slot);
      bool playing;

      if (al_get_sample_bool(spl, ALLEGRO_AUDIOPROP_PLAYING, &playing) == 0) {
         if (playing == false) {
            return do_play_sample_data(spl, data);
         }
      }
   }

   return false;
}

static bool do_play_sample_data(ALLEGRO_SAMPLE *spl, ALLEGRO_SAMPLE_DATA *data)
{
   if (al_set_sample_data(spl, data) != 0) {
      TRACE("al_set_sample_data failed\n");
      return false;
   }

   if (al_play_sample(spl) != 0) {
      TRACE("al_play_sample failed\n");
      return false;
   }

   return true;
}


/* Function: al_stop_all_simple_samples
 *  Stop all samples started by the easy audio layer.
 */
void al_stop_all_simple_samples(void)
{
   unsigned int i;

   for (i = 0; i < _al_vector_size(&simp_samples); i++) {
      ALLEGRO_SAMPLE **slot = _al_vector_ref(&simp_samples, i);
      ALLEGRO_SAMPLE *spl = (*slot);
      al_stop_sample(spl);
   }
}


/* vim: set sts=3 sw=3 et: */
