#include "allegro5/allegro.h"
#include "allegro5/internal/aintern_audio.h"
#include "allegro5/platform/allegro_internal_sdl.h"

ALLEGRO_DEBUG_CHANNEL("SDL")

typedef struct SDL_VOICE
{
   SDL_AudioDeviceID device;
   SDL_AudioSpec spec;
   ALLEGRO_VOICE *voice;
   bool is_playing;
} SDL_VOICE;

static void audio_callback(void *userdata, Uint8 *stream, int len)
{
   // TODO: Allegro has those mysterious "non-streaming" samples, but I
   // can't figure out what their purpose is and how would I play them...

   SDL_VOICE *sv = userdata;
   ALLEGRO_SAMPLE_INSTANCE *instance = sv->voice->attached_stream;
   ALLEGRO_SAMPLE *sample = &instance->spl_data;

   unsigned int frames = sv->spec.samples;
   const void *data = _al_voice_update(sv->voice, sv->voice->mutex, &frames);
   if (data) {
      // FIXME: What is frames for?
      memcpy(stream, sample->buffer.ptr, len);
   }
}

static int sdl_open(void)
{
   return 0;
}

static void sdl_close(void)
{
}

static SDL_AudioFormat allegro_format_to_sdl(ALLEGRO_AUDIO_DEPTH d)
{
   if (d == ALLEGRO_AUDIO_DEPTH_INT8) return AUDIO_S8;
   if (d == ALLEGRO_AUDIO_DEPTH_UINT8) return AUDIO_U8;
   if (d == ALLEGRO_AUDIO_DEPTH_INT16) return AUDIO_S16;
   if (d == ALLEGRO_AUDIO_DEPTH_UINT16) return AUDIO_U16;
   return AUDIO_F32;
}

static int sdl_allocate_voice(ALLEGRO_VOICE *voice)
{
   SDL_VOICE *sv = al_malloc(sizeof *sv);
   SDL_AudioSpec want;
   memset(&want, 0, sizeof want);

   want.freq = voice->frequency;
   want.format = allegro_format_to_sdl(voice->depth);
   want.channels = al_get_channel_count(voice->chan_conf);
   // TODO: Should make this configurable somehow
   want.samples = 4096;
   want.callback = audio_callback;
   want.userdata = sv;

   sv->device = SDL_OpenAudioDevice(NULL, 0, &want, &sv->spec,
      SDL_AUDIO_ALLOW_FORMAT_CHANGE);

   voice->extra = sv;
   sv->voice = voice;

   return 0;
}

static void sdl_deallocate_voice(ALLEGRO_VOICE *voice)
{
   SDL_VOICE *sv = voice->extra;
   SDL_CloseAudioDevice(sv->device);
   al_free(sv);
}

static int sdl_load_voice(ALLEGRO_VOICE *voice, const void *data)
{
   (void)data;
   voice->attached_stream->pos = 0;
   return 0;
}

static void sdl_unload_voice(ALLEGRO_VOICE *voice)
{
   (void) voice;
}

static int sdl_start_voice(ALLEGRO_VOICE *voice)
{
   SDL_VOICE *sv = voice->extra;
   sv->is_playing = true;
   SDL_PauseAudioDevice(sv->device, 0); 
   return 0;
}

static int sdl_stop_voice(ALLEGRO_VOICE *voice)
{
   SDL_VOICE *sv = voice->extra;
   sv->is_playing = false;
   SDL_PauseAudioDevice(sv->device, 1);
   return 0;
}

static bool sdl_voice_is_playing(const ALLEGRO_VOICE *voice)
{
   SDL_VOICE *sv = voice->extra;
   return sv->is_playing;
}

static unsigned int sdl_get_voice_position(const ALLEGRO_VOICE *voice)
{
   return voice->attached_stream->pos;
}

static int sdl_set_voice_position(ALLEGRO_VOICE *voice, unsigned int pos)
{
   voice->attached_stream->pos = pos;
   return 0;
}

ALLEGRO_AUDIO_DRIVER _al_kcm_sdl_driver =
{
   "SDL",
   sdl_open,
   sdl_close,
   sdl_allocate_voice,
   sdl_deallocate_voice,
   sdl_load_voice,
   sdl_unload_voice,
   sdl_start_voice,
   sdl_stop_voice,
   sdl_voice_is_playing,
   sdl_get_voice_position,
   sdl_set_voice_position,

   NULL, //sdl_allocate_recorder,
   NULL  //sdl_deallocate_recorder
};
