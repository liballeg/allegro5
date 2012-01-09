#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_acodec.h>

/* How many samples do we want to process at one time?
   Let's pick a multiple of the width of the screen so that the
   visualization graph is easy to draw. */
#define SAMPLES_PER_FRAGMENT  320 * 4

/* Frequency is the quality of audio. (Samples per second.) Higher
   quality consumes more memory. For speech, numbers as low as 8000
   can be good enough. 44100 is often used for high quality recording. */      
#define FREQ 11025

int main()
{
   ALLEGRO_AUDIO_RECORDER *r;
   ALLEGRO_EVENT_QUEUE *q;
   ALLEGRO_DISPLAY *d;
   ALLEGRO_FILE *fp = NULL;
   ALLEGRO_PATH *tmp_path = NULL;
   ALLEGRO_SAMPLE *samp = NULL;
   
   /* The buffer is where the audio driver will place the recording. If processing
      the data in realtime, you only need a buffer big enough to hold the number of
      samples you wish to record per fragment. The driver will automatically buffer 
      any audio that comes in while you are prcessing data (assuming you don't take
      too much time).
      
      It's important to note that since we are using mono, 8-bit audio, the sample
      size is 1 byte. Thus we can use sample size and buffer size interchangeably.
      If the audio depth or channels were increased, then we'd need to compensate
      by making the buffers bigger.
      */
   const int buffer_size = SAMPLES_PER_FRAGMENT;
   uint8_t *buffer;
   
   /* n => number of samples written to the tmp WAV file */
   int n = 0;
   
   buffer = malloc(buffer_size);
   if (!buffer)
     return 0;

   al_init();
   al_init_primitives_addon();
   al_install_keyboard();
   
   al_install_audio();
   al_init_acodec_addon();
   
   /* Note: changing the depth to more than 8-bit or increasing the number of channels will break
            this demo, since it assumes 1 byte per sample. */
   r = al_create_audio_recorder(1000, buffer_size, FREQ, ALLEGRO_AUDIO_DEPTH_UINT8, ALLEGRO_CHANNEL_CONF_1);
   if (!r) {
      return 1;
   }
   
   /* The recorded wav file is played back as a sample */
   al_reserve_samples(10);
   q = al_create_event_queue();
   
   d = al_create_display(320, 240);
   
   al_set_window_title(d, "SPACE to record. P to playback.");
   
   al_register_event_source(q, al_get_audio_recorder_event_source(r));
   al_register_event_source(q, al_get_display_event_source(d));
   al_register_event_source(q, al_get_keyboard_event_source());
   
   while (true) {
      ALLEGRO_EVENT e;

      al_wait_for_event(q, &e);
       
      if (e.type == ALLEGRO_EVENT_AUDIO_RECORDER_FRAGMENT) {
         ALLEGRO_AUDIO_RECORDER_EVENT *re = al_get_audio_recorder_event(&e);
         uint8_t *input = (uint8_t *) re->buffer;
         int sample_count = re->samples; 
         int x;
         int R = sample_count / 320;
         al_clear_to_color(al_map_rgb(0,0,0));
        
         if (al_is_audio_recorder_recording(r)) {
            /* We may get this event after we've stopped recording, so
               that's why the check to see if it's playing is there. 
               
               Save the first 1000 fragments to wav file. (The limit is only
               to prevent you from accidentally filling up your hard drive!)
               */
            if (fp && n < 1000) {
               al_fwrite(fp, input, sample_count);
               ++n;
            }

            /* Draw a pathetic visualization. */
            for (x = 0; x < 320; ++x) {
               int i, c = 0;
               
               for (i = x * R; i < x * R + R && i < sample_count; ++i) {
                  c += input[i] - 127;
               }
               c /= R;
               al_draw_line(x + 0.5, 120 - c, x + 0.5, 120 + c, al_map_rgb(255,255,255), 1.0);
            }
         }
         al_flip_display();
      }
      else if (e.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
         break;
      }
      else if (e.type == ALLEGRO_EVENT_KEY_CHAR) {
         if (e.keyboard.unichar == 27) {
            break;
         }
         else if (e.keyboard.unichar == ' ') {
            if (!al_is_audio_recorder_recording(r)) {
               /* Start the recording */
               
               if (!tmp_path) {
                  fp = al_make_temp_file("alrecXXX.wav", &tmp_path);
               }
               else if (fp) {
                  al_fseek(fp, ALLEGRO_SEEK_SET, 0);
               }
               
               if (fp) {
                  /* save to a wav file in realtime */
                  al_fputs(fp, "RIFF");
                  al_fwrite32le(fp, 0); /* fill in the size later */
                  al_fputs(fp, "WAVEfmt ");
                  al_fwrite32le(fp, 16); /* chunk size */
                  al_fwrite16le(fp, 1);  /* PCM */
                  al_fwrite16le(fp, 1);  /* channels */
                  al_fwrite32le(fp, FREQ); /* frequency */
                  al_fwrite32le(fp, FREQ * 1 * 1); /* bytes/sec */
                  al_fwrite16le(fp, 1 * 1); /* block align */
                  al_fwrite16le(fp, 1 * 1 * 8); /* bits per sample */
                  al_fputs(fp, "data");
                  al_fwrite32le(fp, 0); /* fill in the size later */
               }
               
               al_start_audio_recorder(r);
               n = 0;
            }
            else {
               /* Stop the recording, update the wav file. */
               int sample_size = 1;
               int size = sample_size * (n * SAMPLES_PER_FRAGMENT);
      
               if (size & 1) al_fputc(fp, 0x80);

               al_fseek(fp, 4, ALLEGRO_SEEK_SET);
               al_fwrite32le(fp, 44 + size + (size & 1)); 
      
               al_fseek(fp, 40, ALLEGRO_SEEK_SET);
               al_fwrite32le(fp, size);
               al_fflush(fp);
               
               al_stop_audio_recorder(r);
               
               al_clear_to_color(al_map_rgb(0,0,0));
               al_flip_display();
            }

         }
         else if (e.keyboard.unichar == 'p') {
            /* Play the previously recorded wav file */
            if (!al_is_audio_recorder_recording(r)) {
               if (fp) {
                  if (samp) {
                     al_destroy_sample(samp);
                  }
                  al_fseek(fp, ALLEGRO_SEEK_SET, 0);
                  samp = al_load_sample_f(fp, ".wav");
                  if (samp) {
                     al_play_sample(samp, 1.0, 0.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
                  }
               }
            }
         }
      }
   }
   
   /* clean up */
   al_destroy_audio_recorder(r);
   
   if (samp)
      al_destroy_sample(samp);
      
   if (fp)
      al_fclose(fp);
      
   if (tmp_path) {
      al_remove_filename(al_path_cstr(tmp_path, '/'));
      al_destroy_path(tmp_path);
   }
   
   free(buffer);
   
   return 0;
}