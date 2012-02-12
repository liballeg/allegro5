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

/* Function: al_open_video
 */
ALLEGRO_VIDEO *al_open_video(char const *filename)
{
   ALLEGRO_VIDEO *video;

   video = al_calloc(1, sizeof *video);
   
   video->vtable = _al_video_vtable;
   
   video->filename = al_create_path(filename);

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

/* Function: al_pause_video
 */
void al_pause_video(ALLEGRO_VIDEO *video, bool paused)
{
   ASSERT(video);

   if (paused != video->paused) {
      video->paused = paused;
      video->vtable->pause_video(video);
   }
}

/* Function: al_is_video_paused
 */
bool al_is_video_paused(ALLEGRO_VIDEO *video)
{
   ASSERT(video);

   return video->paused;
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
double al_get_video_position(ALLEGRO_VIDEO *video, int which)
{
   ASSERT(video);

   /* XXX magic constants */
   if (which == 1)
      return video->video_position;
   if (which == 2)
      return video->audio_position;
   return video->position;
}

/* Function: al_seek_video
 */
void al_seek_video(ALLEGRO_VIDEO *video, double pos_in_seconds)
{
   ASSERT(video);

   /* XXX why is seek_to not just a parameter? */
   video->seek_to = pos_in_seconds;
   video->vtable->seek_video(video);
}

/* Function: al_get_video_aspect_ratio
 */
double al_get_video_aspect_ratio(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return video->aspect_ratio;
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

/* Function: al_get_video_width
 */
int al_get_video_width(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return video->width;
}

/* Function: al_get_video_height
 */
int al_get_video_height(ALLEGRO_VIDEO *video)
{
   ASSERT(video);
   return video->height;
}

/* vim: set sts=3 sw=3 et: */
