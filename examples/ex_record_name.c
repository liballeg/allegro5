/* ex_record_name 
 * 
 * This example automatically detects when you say your name and creates a
 * sample (maximum of three seconds). Pressing any key (other than ESC) will
 * play it back.
 */

#define A5O_UNSTABLE
#include "allegro5/allegro.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_font.h"
#include "allegro5/allegro_image.h"
#include "allegro5/allegro_primitives.h"

#include "common.c"

int main(int argc, char *argv[])
{
   A5O_DISPLAY *display;
   A5O_FONT *font;
   A5O_AUDIO_RECORDER *recorder;
   A5O_EVENT_QUEUE *queue;
   A5O_TIMER *timer;
   int font_height;
   
   /* Frequency is the number of samples per second. */
   const int frequency = 44100;
   
   const int channels = 2;

   /* The latency is used to determine the size of the fragment buffer.
      More accurately, it represents approximately how many seconds will
      pass between fragment events. (There may be overhead latency from
      the OS or driver the adds a fixed amount of latency on top of 
      Allegro's.) 
      
      For this example, the latency should be kept relatively low since
      each fragment is processed in its entirety. Increasing the latency
      would increase the size of the fragment, which would decrease the
      accuracy of the code that processes the fragment.
      
      But if it's too low, then it will cut out too quickly. (If the
      example were more thoroughly written, the latency setting wouldn't
      actually change how the voice detection worked.)
    */
   const float latency = 0.10;

   const int max_seconds = 3; /* number of seconds of voice recording */
   
   int16_t *name_buffer;      /* stores up to max_seconds of audio */
   int16_t *name_buffer_pos;  /* points to the current recorded position */
   int16_t *name_buffer_end;  /* points to the end of the buffer */
   
   float gain = 0.0f;         /* 0.0 (quiet) - 1.0 (loud) */
   float begin_gain = 0.3f;   /* when to begin recording */
   
   bool is_recording = false;
   
   A5O_SAMPLE *spl = NULL;
   
   (void) argc;
   (void) argv;
   
   if (!al_init()) {
      abort_example("Could not init Allegro.\n");
   }
   
   if (!al_install_audio()) {
      abort_example("Unable to initialize audio addon\n");
   }
   
   if (!al_init_acodec_addon()) {
      abort_example("Unable to initialize acoded addon\n");
   }
   
   if (!al_init_image_addon()) {
      abort_example("Unable to initialize image addon\n");
   }
   
   if (!al_init_primitives_addon()) {
      abort_example("Unable to initialize primitives addon\n");
   }
      
   al_init_font_addon();
   al_install_keyboard();
   
   font = al_load_bitmap_font("data/bmpfont.tga");
   if (!font) {
      abort_example("Unable to load data/a4_font.tga\n");
   }
   
   font_height = al_get_font_line_height(font);
   
   /* WARNING: This demo assumes an audio depth of INT16 and two channels.
      Changing those values will break the demo. Nothing here really needs to be
      changed. If you want to fiddle with things, adjust the constants at the
      beginning of the program.
    */
   
   recorder = al_create_audio_recorder(
      5 / latency,                 /* five seconds of buffer space */
      frequency * latency,         /* configure the fragment size to give us the given
                                        latency in seconds */
      frequency,                   /* samples per second (higher => better quality) */
      A5O_AUDIO_DEPTH_INT16,   /* 2-byte sample size */
      A5O_CHANNEL_CONF_2       /* stereo */
   );
   
   if (!recorder) {
      abort_example("Unable to create audio recorder\n");
   }
   
   display = al_create_display(640, 480);
   if (!display) {
      abort_example("Unable to create display\n");
   }
   
   /* Used to play back the voice recording. */
   al_reserve_samples(1);
   
   /* store up to three seconds */
   name_buffer = al_calloc(channels * frequency * max_seconds, sizeof(int16_t));
   name_buffer_pos = name_buffer;
   name_buffer_end = name_buffer + channels * frequency * max_seconds;
   
   queue = al_create_event_queue();
   timer = al_create_timer(1 / 60.0);
   
   al_register_event_source(queue, al_get_display_event_source(display));
   al_register_event_source(queue, al_get_audio_recorder_event_source(recorder));
   al_register_event_source(queue, al_get_timer_event_source(timer));
   al_register_event_source(queue, al_get_keyboard_event_source());
   
   al_start_timer(timer);
   al_start_audio_recorder(recorder);
   
   while (true) {
      A5O_EVENT event;
      bool do_draw = false;
      
      al_wait_for_event(queue, &event);
      
      if (event.type == A5O_EVENT_DISPLAY_CLOSE || 
         (event.type == A5O_EVENT_KEY_UP && event.keyboard.keycode == A5O_KEY_ESCAPE)) {
         break;
      }
      else if (event.type == A5O_EVENT_KEY_CHAR) {
         if (spl && event.keyboard.unichar != 27) {
            al_play_sample(spl, 1.0, 0.0, 1.0, A5O_PLAYMODE_ONCE, NULL);
         }
      }
      else if (event.type == A5O_EVENT_TIMER) {
         do_draw = true;
      }
      else if (event.type == A5O_EVENT_AUDIO_RECORDER_FRAGMENT && recorder != NULL) {
         /* Because the recording happens in a different thread (and simply because we are
            queuing up events), it's quite possible to receive (process) a fragment event
            after the recorder has been stopped or destroyed. Thus, it is important to
            somehow check that the recorder is still valid, as we are doing above.
          */
         A5O_AUDIO_RECORDER_EVENT *re = al_get_audio_recorder_event(&event);
         int16_t *buffer = re->buffer;
         int16_t low = 0, high = 0;
         unsigned int i;
         
         /* Calculate the volume by comparing the highest and lowest points. This entire
            section assumes we are using fairly small fragment size (low latency). If a 
            large fragment size were used, then we'd have to inspect smaller portions 
            of it at a time to more accurately deterine when recording started and
            stopped. */
         for (i = 0; i < channels * re->samples; ++i) {
            if (buffer[i] < low)
               low = buffer[i];
            else if (buffer[i] > high)
               high = buffer[i];
         }
         
         gain = gain * 0.25 + ((float) (high - low) / 0xffff) * 0.75;
         
         /* Set arbitrary thresholds for beginning and stopping recording. This probably
            should be calibrated by determining how loud the ambient noise is.
          */
         if (!is_recording && gain >= begin_gain && name_buffer_pos == name_buffer)
            is_recording = true;
         else if (is_recording && gain <= 0.10)
            is_recording = false;
         
         if (is_recording) {
            /* Copy out of the fragment buffer into our own buffer that holds the
               name. */
            int samples_to_copy = channels * re->samples;
            
            /* Don't overfill up our name buffer... */
            if (samples_to_copy > name_buffer_end - name_buffer_pos)
               samples_to_copy = name_buffer_end - name_buffer_pos;
            
            if (samples_to_copy) {
               /* must multiply by two, since we are using 16-bit samples */
               memcpy(name_buffer_pos, re->buffer, samples_to_copy * 2);
            }
            
            name_buffer_pos += samples_to_copy;
            if (name_buffer_pos >= name_buffer_end) {
               is_recording = false;
            }
         }
         
         if (!is_recording && name_buffer_pos != name_buffer && !spl) {
            /* finished recording, but haven't created the sample yet */
            spl = al_create_sample(name_buffer, name_buffer_pos - name_buffer, frequency, 
               A5O_AUDIO_DEPTH_INT16, A5O_CHANNEL_CONF_2, false);
            
            /* We no longer need the recorder. Destroying it is the only way to unlock the device. */
            al_destroy_audio_recorder(recorder);
            recorder = NULL;
         }
      }
      
      if (do_draw) {
         al_clear_to_color(al_map_rgb(0,0,0));
         if (!spl) {
            const char *msg = "Say Your Name";
            int width = al_get_text_width(font, msg);
            
            al_draw_text(font, al_map_rgb(255,255,255),
               320, 240 - font_height / 2, A5O_ALIGN_CENTRE, msg
            );
            
            /* draw volume meter */
            al_draw_filled_rectangle(320 - width / 2, 242 + font_height / 2,
               (320 - width / 2) + (gain * width), 242 + font_height, 
               al_map_rgb(0,255,0)
            );
            
            /* draw target line that triggers recording */
            al_draw_line((320 - width / 2) + (begin_gain * width), 242 + font_height / 2,
               (320 - width / 2) + (begin_gain * width), 242 + font_height,
               al_map_rgb(255,255,0), 1.0
            );
         }
         else {
            al_draw_text(font, al_map_rgb(255,255,255), 320, 240 - font_height / 2,
               A5O_ALIGN_CENTRE, "Press Any Key");
         }
         al_flip_display();
      }
   }
   
   if (recorder) {
      al_destroy_audio_recorder(recorder);
   }
   
   al_free(name_buffer);
   
   return 0;
}
