/* OpenSL: The Standard for Embedded Audio Acceleration
 * http://www.khronos.org/opensles/
 * http://www.khronos.org/registry/sles/specs/OpenSL_ES_Specification_1.1.pdf
 */

#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"

#include <SLES/OpenSLES.h>

/* Not sure if this one is needed, yet */
#include <SLES/OpenSLES_Android.h>

ALLEGRO_DEBUG_CHANNEL("opensl")

static const char * opensl_get_error_string(SLresult result)
{
    switch (result){
        case SL_RESULT_PRECONDITIONS_VIOLATED: return "Preconditions violated";
        case SL_RESULT_PARAMETER_INVALID: return "Invalid parameter";
        case SL_RESULT_MEMORY_FAILURE: return "Memory failure";
        case SL_RESULT_RESOURCE_ERROR: return "Resource error";
        case SL_RESULT_RESOURCE_LOST: return "Resource lost";
        case SL_RESULT_IO_ERROR: return "IO error";
        case SL_RESULT_BUFFER_INSUFFICIENT: return "Insufficient buffer";
        case SL_RESULT_CONTENT_CORRUPTED: return "Content corrupted";
        case SL_RESULT_CONTENT_UNSUPPORTED: return "Content unsupported";
        case SL_RESULT_CONTENT_NOT_FOUND: return "Content not found";
        case SL_RESULT_PERMISSION_DENIED: return "Permission denied";
        case SL_RESULT_FEATURE_UNSUPPORTED: return "Feature unsupported";
        case SL_RESULT_INTERNAL_ERROR: return "Internal error";
        case SL_RESULT_UNKNOWN_ERROR: return "Unknown error";
        case SL_RESULT_OPERATION_ABORTED: return "Operation aborted";
        case SL_RESULT_CONTROL_LOST: return "Control lost";
    }
    return "Unknown OpenSL error";
}

/* TODO: not really working yet */
static int _opensl_open(void)
{
    SLresult result;
    SLObjectItf opensl;
    SLEngineOption options[] = {
        (SLuint32) SL_ENGINEOPTION_THREADSAFE,
        (SLuint32) SL_BOOLEAN_TRUE,
        /*
        (SLuint32) SL_ENGINEOPTION_MAJORVERSION, (SLuint32) 1,
        (SLuint32) SL_ENGINEOPTION_MINORVERSION, (SLuint32) 1,
        */
    };

    result = slCreateEngine(&opensl, 1, options, 0, NULL, NULL);
    if (result != SL_RESULT_SUCCESS){
        ALLEGRO_ERROR("Could not open audio device: %s\n",
                      opensl_get_error_string(result));
    }
    (*opensl)->Destroy(opensl);

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
