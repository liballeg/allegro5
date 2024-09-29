
typedef struct A5O_VIDEO_INTERFACE {
   bool (*open_video)(A5O_VIDEO *video);
   bool (*close_video)(A5O_VIDEO *video);
   bool (*start_video)(A5O_VIDEO *video);
   bool (*set_video_playing)(A5O_VIDEO *video);
   bool (*seek_video)(A5O_VIDEO *video, double seek_to);
   bool (*update_video)(A5O_VIDEO *video);
} A5O_VIDEO_INTERFACE;

struct A5O_VIDEO {
   A5O_VIDEO_INTERFACE *vtable;
   
   /* video */
   A5O_BITMAP *current_frame;
   double video_position;
   double fps;
   float scaled_width;
   float scaled_height;

   /* audio */
   A5O_MIXER *mixer;
   A5O_VOICE *voice;
   A5O_AUDIO_STREAM *audio;
   double audio_position;
   double audio_rate;

   /* general */
   bool es_inited;
   A5O_EVENT_SOURCE es;
   A5O_PATH *filename;
   bool playing;
   double position;

   /* implementation specific */
   void *data;
};

typedef bool (*A5O_VIDEO_IDENTIFIER_FUNCTION)(A5O_FILE *f);

void _al_compute_scaled_dimensions(int frame_w, int frame_h, float aspect_ratio, float *scaled_w, float *scaled_h);

A5O_VIDEO_INTERFACE *_al_video_ogv_vtable(void);
bool _al_video_identify_ogv(A5O_FILE *f);
