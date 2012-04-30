/* OpenSL: The Standard for Embedded Audio Acceleration
 * http://www.khronos.org/opensles/
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"

ALLEGRO_DEBUG_CHANNEL("opensl")

/* 4/29/2012: dummy implementation */

static int _opensl_open(void)
{
    /* TODO */
    return 1;
}

static void _opensl_close(void)
{
    /* TODO */
}

static int _opensl_allocate_voice(ALLEGRO_VOICE *voice)
{
    /* TODO */
    return 1;
}

static void _opensl_deallocate_voice(ALLEGRO_VOICE *voice)
{
    /* TODO */
}

static int _opensl_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
    /* TODO */
    return 1;
}

static void _opensl_unload_voice(ALLEGRO_VOICE *voice)
{
    /* TODO */
}

static int _opensl_start_voice(ALLEGRO_VOICE *voice)
{
    /* TODO */
    return 1;
}

static int _opensl_stop_voice(ALLEGRO_VOICE* voice)
{
    /* TODO */
    return 1;
}

static bool _opensl_voice_is_playing(const ALLEGRO_VOICE *voice)
{
    /* TODO */
    return false;
}

static unsigned int _opensl_get_voice_position(const ALLEGRO_VOICE *voice)
{
    /* TODO */
    return 0;
}

static int _opensl_set_voice_position(ALLEGRO_VOICE *voice, unsigned int val)
{
    /* TODO */
    return 1;
}

ALLEGRO_AUDIO_DRIVER _al_kcm_opensl_driver = {
   "OpenSL",

   _opensl_open,
   _opensl_close,

   _opensl_allocate_voice,
   _opensl_deallocate_voice,

   _opensl_load_voice,
   _opensl_unload_voice,

   _opensl_start_voice,
   _opensl_stop_voice,

   _opensl_voice_is_playing,

   _opensl_get_voice_position,
   _opensl_set_voice_position,

   NULL,
   NULL
};
