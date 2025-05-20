/* This is just a quick hack. But good enough for our use of displaying
 * a short intro video when our game starts up - so might as well share
 * it.
 *
 * Known bugs:
 *
 * - Only very crude synching. Audio is slightly delayed and some
 *   videos seem to constantly drift off and then the audio gets all
 *   distorted...
 *
 * - Seeking/Pausing doesn't really work.
 *
 * - Memory leaks. Easy to fix but don't have time right now.
 *
 * Missing features:
 *
 * - Stream information. For example allow selection of one of several
 *   audio streams or subtitle overlay streams.
 *
 * - Non audio/video streams. For example something like:
 *   ALLEGRO_USTR *al_get_video_subtitle(float *x, float *y);
 *
 * - Buffering. Right now buffering is hardcoded to a fixed size which
 *   seemed enough for streaming 720p from disk in my tests. Obviously
 *   when streaming higher bandwidth or from a source with high
 *   fluctuation like an internet stream this won't work at all.
 *
 * - Provide an audio stream for the audio. Then could use this to
 *   stream audio files. Right now opening an .mp3 with the video
 *   addon will play it but only with the video API instead of Allegro's
 *   normal audio streaming API...
 *
 * - Audio/Video sync. For a game user-controlled sync is probably not
 *   too important as it can just ship with a properly synchronizeded
 *   video. However right now the audio delay is completely ignored.
 *
 * - Additional drivers. Also redo the API a bit so not everything
 *   has to be done by the driver.
 */
#define ALLEGRO_INTERNAL_UNSTABLE

#include "allegro5/allegro5.h"
#include "allegro5/allegro_video.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_exitfunc.h"
#include "allegro5/internal/aintern_system.h"
#include "allegro5/internal/aintern_video.h"
#include "allegro5/internal/aintern_video_cfg.h"
#include "allegro5/internal/aintern_vector.h"

ALLEGRO_DEBUG_CHANNEL("video")


/* globals */
static bool video_inited = false;

typedef struct VideoHandler {
   const char *extension;
   ALLEGRO_VIDEO_INTERFACE *vtable;
   ALLEGRO_VIDEO_IDENTIFIER_FUNCTION identifier;
} VideoHandler;

static _AL_VECTOR handlers = _AL_VECTOR_INITIALIZER(VideoHandler);

static const char* identify_video(ALLEGRO_FILE *f)
{
   size_t i;
   for (i = 0; i < _al_vector_size(&handlers); i++) {
      VideoHandler *l = _al_vector_ref(&handlers, i);
      if (l->identifier(f)) {
         return l->extension;
      }
   }
   return NULL;
}

static ALLEGRO_VIDEO_INTERFACE *find_handler(const char *extension)
{
   size_t i;
   for (i = 0; i < _al_vector_size(&handlers); i++) {
      VideoHandler *l = _al_vector_ref(&handlers, i);
      if (0 == _al_stricmp(extension, l->extension)) {
         return l->vtable;
      }
   }
   return NULL;
}

static void add_handler(const char *extension, ALLEGRO_VIDEO_INTERFACE *vtable,
                        ALLEGRO_VIDEO_IDENTIFIER_FUNCTION identifier)
{
   VideoHandler *v = _al_vector_alloc_back(&handlers);
   v->extension = extension;
   v->vtable = vtable;
   v->identifier = identifier;
}

/* Function: al_open_video
 */
ALLEGRO_VIDEO *al_open_video(char const *filename)
{
   ALLEGRO_VIDEO *video;
   const char *ext;
   video = al_calloc(1, sizeof *video);

   ASSERT(filename);
   ext = al_identify_video(filename);
   if (!ext) {
      ext = strrchr(filename, '.');
      if (!ext) {
         ALLEGRO_ERROR("Could not identify video %s!\n", filename);
      }
   }

   video->vtable = find_handler(ext);
   if (video->vtable == NULL) {
      ALLEGRO_ERROR("No handler for video extension %s - "
         "therefore not trying to load %s.\n", ext, filename);
      al_free(video);
      return NULL;
   }

   video->file = al_fopen(filename, "rb");

   if (!video->vtable->open_video(video)) {
      ALLEGRO_ERROR("Could not open %s.\n", filename);
      al_free(video);
      return NULL;
   }

   al_init_user_event_source(&video->es);
   video->es_inited = true;
   video->dtor_item = _al_register_destructor(_al_dtor_list, "video", video, (void (*)(void *)) al_close_video);

   return video;
}

/* Function: al_open_video_f
*/
ALLEGRO_VIDEO* al_open_video_f(ALLEGRO_FILE* fp, const char* ident)
{
   ALLEGRO_VIDEO* video;
   const char* ext;
   video = al_calloc(1, sizeof * video);

   ASSERT(fp);
   if (!ident) {
      ext = al_identify_video_f(fp);
      al_fseek(fp, 0, ALLEGRO_SEEK_SET);  // rewind to zero after al_identify_video_f
   }
   else
      ext = ident;
   if (!ext) {
      ALLEGRO_ERROR("Could not identify video %s!\n", "file from file interface");
   }
     
   video->vtable = find_handler(ext);
   if (video->vtable == NULL) {
      ALLEGRO_ERROR("No handler for video extension %s - "
         "therefore not trying to load %s.\n", ext, "video file from file interface");
      al_free(video);
      return NULL;
   }

   video->file = fp;

   if (!video->vtable->open_video(video)) {
      ALLEGRO_ERROR("Could not open %s.\n", "video from from file interface");
      al_free(video);
      return NULL;
   }

   al_init_user_event_source(&video->es);
   video->es_inited = true;
   video->dtor_item = _al_register_destructor(_al_dtor_list, "video", video, (void (*)(void*)) al_close_video);

   return video;
}

/* Function: al_close_video
 */
void al_close_video(ALLEGRO_VIDEO *video)
{
   if (video) {
      video->vtable->close_video(video);
      if (video->es_inited) {
         al_destroy_user_event_source(&video->es);
      }
      al_fclose(video->file);
      _al_unregister_destructor(_al_dtor_list, video->dtor_item);
      al_free(video);
   }
}

/* Function: al_get_video_event_source
 */
ALLEGRO_EVENT_SOURCE *al_get_video_event_source(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return &video->es;
}

/* Function: al_start_video
 */
void al_start_video(ALLEGRO_VIDEO *video, ALLEGRO_MIXER *mixer)
{
   ASSERT(video);

   /* XXX why is this not just a parameter? */
   video->mixer = mixer;
   video->playing = true;
   /* Destruction is handled by al_close_video. */
   _al_push_destructor_owner();
   video->vtable->start_video(video);
   _al_pop_destructor_owner();
}

/* Function: al_start_video_with_voice
 */
void al_start_video_with_voice(ALLEGRO_VIDEO *video, ALLEGRO_VOICE *voice)
{
   ASSERT(video);

   /* XXX why is voice not just a parameter? */
   video->voice = voice;
   video->vtable->start_video(video);
}

/* Function: al_set_video_playing
 */
void al_set_video_playing(ALLEGRO_VIDEO *video, bool play)
{
   ASSERT(video);

   if (play != video->playing) {
      video->playing = play;
      video->vtable->set_video_playing(video);
   }
}

/* Function: al_is_video_playing
 */
bool al_is_video_playing(ALLEGRO_VIDEO *video)
{
   ASSERT(video);

   return video->playing;
}

/* Function: al_get_video_frame
 */
ALLEGRO_BITMAP *al_get_video_frame(ALLEGRO_VIDEO *video)
{
   ASSERT(video);

   video->vtable->update_video(video);
   return video->current_frame;
}

/* Function: al_get_video_position
 */
double al_get_video_position(ALLEGRO_VIDEO *video, ALLEGRO_VIDEO_POSITION_TYPE which)
{
   ASSERT(video);

   if (which == ALLEGRO_VIDEO_POSITION_VIDEO_DECODE)
      return video->video_position;
   if (which == ALLEGRO_VIDEO_POSITION_AUDIO_DECODE)
      return video->audio_position;
   return video->position;
}

/* Function: al_seek_video
 */
bool al_seek_video(ALLEGRO_VIDEO *video, double pos_in_seconds)
{
   ASSERT(video);

   return video->vtable->seek_video(video, pos_in_seconds);
}

/* Function: al_get_video_audio_rate
 */
double al_get_video_audio_rate(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return video->audio_rate;
}

/* Function: al_get_video_fps
 */
double al_get_video_fps(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return video->fps;
}

/* Function: al_get_video_scaled_width
 */
float al_get_video_scaled_width(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return video->scaled_width;
}

/* Function: al_get_video_scaled_height
 */
float al_get_video_scaled_height(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return video->scaled_height;
}

/* Function: al_init_video_addon
 */
bool al_init_video_addon(void)
{
   if (video_inited)
      return true;

#ifdef ALLEGRO_CFG_VIDEO_HAVE_OGV
   add_handler(".ogv", _al_video_ogv_vtable(), _al_video_identify_ogv);
#endif

   if (_al_vector_size(&handlers) == 0) {
      ALLEGRO_WARN("No video handlers available!\n");
      return false;
   }

   _al_add_exit_func(al_shutdown_video_addon, "al_shutdown_video_addon");
   video_inited = true;
   return true;
}


/* Function: al_is_video_addon_initialized
 */
bool al_is_video_addon_initialized(void)
{
   return video_inited;
}


/* Function: al_shutdown_video_addon
 */
void al_shutdown_video_addon(void)
{
   if (!video_inited)
      return;

   _al_vector_free(&handlers);

   video_inited = false;
}


/* Function: al_get_allegro_video_version
 */
uint32_t al_get_allegro_video_version(void)
{
   return ALLEGRO_VERSION_INT;
}


/* Function: al_identify_video_f
 */
char const *al_identify_video_f(ALLEGRO_FILE *fp)
{
   return identify_video(fp);
}


/* Function: al_identify_video
 */
char const *al_identify_video(char const *filename)
{
   char const *ext;
   ALLEGRO_FILE *fp = al_fopen(filename, "rb");
   if (!fp)
      return NULL;
   ext = identify_video(fp);
   al_fclose(fp);
   return ext;
}

/* The returned width and height are always greater than or equal to the frame
 * width and height. */
void _al_compute_scaled_dimensions(int frame_w, int frame_h, float aspect_ratio,
                                   float *scaled_w, float *scaled_h)
{
   if (aspect_ratio > 1.0) {
      *scaled_w = frame_h * aspect_ratio;
      *scaled_h = frame_h;
   }
   else {
      *scaled_w = frame_w;
      *scaled_h = frame_w / aspect_ratio;
   }
}

/* vim: set sts=3 sw=3 et: */
