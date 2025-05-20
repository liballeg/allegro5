#ifndef __al_included_allegro_aintern_video_h
#define __al_included_allegro_aintern_video_h

#include "allegro5/internal/aintern_list.h"

typedef struct ALLEGRO_VIDEO_INTERFACE {
   bool (*open_video)(ALLEGRO_VIDEO *video);
   bool (*close_video)(ALLEGRO_VIDEO *video);
   bool (*start_video)(ALLEGRO_VIDEO *video);
   bool (*set_video_playing)(ALLEGRO_VIDEO *video);
   bool (*seek_video)(ALLEGRO_VIDEO *video, double seek_to);
   bool (*update_video)(ALLEGRO_VIDEO *video);
} ALLEGRO_VIDEO_INTERFACE;

struct ALLEGRO_VIDEO {
   ALLEGRO_VIDEO_INTERFACE *vtable;

   /* video */
   ALLEGRO_BITMAP *current_frame;
   double video_position;
   double fps;
   float scaled_width;
   float scaled_height;

   /* audio */
   ALLEGRO_MIXER *mixer;
   ALLEGRO_VOICE *voice;
   ALLEGRO_AUDIO_STREAM *audio;
   double audio_position;
   double audio_rate;

   /* general */
   bool es_inited;
   ALLEGRO_EVENT_SOURCE es;
   ALLEGRO_FILE *file;
   bool playing;
   double position;

   _AL_LIST_ITEM *dtor_item;

   /* implementation specific */
   void *data;
};

typedef bool (*ALLEGRO_VIDEO_IDENTIFIER_FUNCTION)(ALLEGRO_FILE *f);

void _al_compute_scaled_dimensions(int frame_w, int frame_h, float aspect_ratio, float *scaled_w, float *scaled_h);

ALLEGRO_VIDEO_INTERFACE *_al_video_ogv_vtable(void);
bool _al_video_identify_ogv(ALLEGRO_FILE *f);

#endif
