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
 
#include "allegro5/allegro5.h"
#include "allegro5/allegro_video.h"
#include "allegro5/internal/aintern_video.h"
#include "allegro5/internal/aintern_video_cfg.h"
#include "allegro5/internal/aintern_exitfunc.h"

ALLEGRO_DEBUG_CHANNEL("video")


/* globals */
static bool video_inited = false;

typedef struct VideoHandler {
   struct VideoHandler *next;
   const char *extension;
   ALLEGRO_VIDEO_INTERFACE *vtable;
} VideoHandler;

static VideoHandler *handlers;

static ALLEGRO_VIDEO_INTERFACE *find_handler(const char *extension)
{
   VideoHandler *v = handlers;
   while (v) {
      if (!strcmp(extension, v->extension)) {
         return v->vtable;
      }
      v = v->next;
   }
   return NULL;
}

static void add_handler(const char *extension, ALLEGRO_VIDEO_INTERFACE *vtable)
{
   VideoHandler *v;
   if (handlers == NULL) {
      handlers = al_calloc(1, sizeof(VideoHandler));
      v = handlers;
   }
   else {
      v = handlers;
      while (v->next) {
         v = v->next;
      }
      v->next = al_calloc(1, sizeof(VideoHandler));
      v = v->next;
   }
   v->extension = extension;
   v->vtable = vtable;
}

/* Function: al_open_video
 */
ALLEGRO_VIDEO *al_open_video(char const *filename)
{
   ALLEGRO_VIDEO *video;
   const char *extension = filename + strlen(filename) - 1;

   while ((extension >= filename) && (*extension != '.'))
      extension--;
   video = al_calloc(1, sizeof *video);
   
   video->vtable = find_handler(extension);

   if (video->vtable == NULL) {
      al_free(video);
      return NULL;
   }
   
   video->filename = al_create_path(filename);
   video->playing = true;

   if (!video->vtable->open_video(video)) {
      al_destroy_path(video->filename);
      al_free(video);
      return NULL;
   }
   
   al_init_user_event_source(&video->es);
   video->es_inited = true;
   
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
      al_destroy_path(video->filename);
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
   video->vtable->start_video(video);
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
   add_handler(".ogv", _al_video_ogv_vtable());
#endif

   if (handlers == NULL) {
      ALLEGRO_WARN("No video handlers available!\n");
      return false;
   }

   _al_add_exit_func(al_shutdown_video_addon, "al_shutdown_video_addon");

   return true;
}


/* Function: al_shutdown_video_addon
 */
void al_shutdown_video_addon(void)
{
   if (!video_inited)
      return;

   VideoHandler *v = handlers;
   while (v) {
      VideoHandler *next = v->next;
      al_free(v);
      v = next;
   }
   video_inited = false;
   handlers = NULL;
}


/* Function: al_get_allegro_video_version
 */
uint32_t al_get_allegro_video_version(void)
{
   return ALLEGRO_VERSION_INT;
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
