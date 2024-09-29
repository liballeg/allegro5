#ifndef __al_included_allegro_video_h
#define __al_included_allegro_video_h

#ifdef __cplusplus
   extern "C" {
#endif

#include "allegro5/allegro5.h"
#include "allegro5/allegro_audio.h"

#if (defined A5O_MINGW32) || (defined A5O_MSVC) || (defined A5O_BCC32)
   #ifndef A5O_STATICLINK
      #ifdef A5O_VIDEO_SRC
         #define _A5O_VIDEO_DLL __declspec(dllexport)
      #else
         #define _A5O_VIDEO_DLL __declspec(dllimport)
      #endif
   #else
      #define _A5O_VIDEO_DLL
   #endif
#endif

#if defined A5O_MSVC
   #define A5O_VIDEO_FUNC(type, name, args)      _A5O_VIDEO_DLL type __cdecl name args
#elif defined A5O_MINGW32
   #define A5O_VIDEO_FUNC(type, name, args)      extern type name args
#elif defined A5O_BCC32
   #define A5O_VIDEO_FUNC(type, name, args)      extern _A5O_VIDEO_DLL type name args
#else
   #define A5O_VIDEO_FUNC      AL_FUNC
#endif

/* Enum: A5O_VIDEO_EVENT_TYPE
 */
enum A5O_VIDEO_EVENT_TYPE
{
   A5O_EVENT_VIDEO_FRAME_SHOW   = 550,
   A5O_EVENT_VIDEO_FINISHED     = 551,
   _A5O_EVENT_VIDEO_SEEK        = 552   /* internal */
};

enum A5O_VIDEO_POSITION_TYPE
{
   A5O_VIDEO_POSITION_ACTUAL        = 0,
   A5O_VIDEO_POSITION_VIDEO_DECODE  = 1,
   A5O_VIDEO_POSITION_AUDIO_DECODE  = 2
};

/* Enum: A5O_VIDEO_POSITION_TYPE
 */
typedef enum A5O_VIDEO_POSITION_TYPE A5O_VIDEO_POSITION_TYPE;

typedef struct A5O_VIDEO A5O_VIDEO;

A5O_VIDEO_FUNC(A5O_VIDEO *, al_open_video, (char const *filename));
A5O_VIDEO_FUNC(void, al_close_video, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(void, al_start_video, (A5O_VIDEO *video, A5O_MIXER *mixer));
A5O_VIDEO_FUNC(void, al_start_video_with_voice, (A5O_VIDEO *video, A5O_VOICE *voice));
A5O_VIDEO_FUNC(A5O_EVENT_SOURCE *, al_get_video_event_source, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(void, al_set_video_playing, (A5O_VIDEO *video, bool playing));
A5O_VIDEO_FUNC(bool, al_is_video_playing, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(double, al_get_video_audio_rate, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(double, al_get_video_fps, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(float, al_get_video_scaled_width, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(float, al_get_video_scaled_height, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(A5O_BITMAP *, al_get_video_frame, (A5O_VIDEO *video));
A5O_VIDEO_FUNC(double, al_get_video_position, (A5O_VIDEO *video, A5O_VIDEO_POSITION_TYPE which));
A5O_VIDEO_FUNC(bool, al_seek_video, (A5O_VIDEO *video, double pos_in_seconds));
A5O_VIDEO_FUNC(bool, al_init_video_addon, (void));
A5O_VIDEO_FUNC(bool, al_is_video_addon_initialized, (void));
A5O_VIDEO_FUNC(void, al_shutdown_video_addon, (void));
A5O_VIDEO_FUNC(uint32_t, al_get_allegro_video_version, (void));

A5O_VIDEO_FUNC(char const *, al_identify_video_f, (A5O_FILE *fp));
A5O_VIDEO_FUNC(char const *, al_identify_video, (char const *filename));

#ifdef __cplusplus
   }
#endif

#endif
