/* TODO: Thsi is just a quick ffmpeg video player test. Eventually this
 * doesn't have to be ffmpeg only though, could also use things like
 * theora.
 * 
 * Timing:
 * The video is always timed off wall clock time.
 * 
 * Video:
 * For each frame, we have its presentation time stamp (pts). That is
 * the time at which it should be displayed. When we get our first
 * frame, we can check its pts and also look at the clock. Then when
 * the next frame appears, we can use the difference of its pts and the
 * pts of the first frame to know exactly at which time we should
 * display it.
 * 
 * t1 = t0 + (pts1 - pts0)
 * t2 = t0 + (pts2 - pts0)
 * ...
 * 
 * Audio:
 * Audio frames also have a pts. So just like with video frames we know
 * when audio should be played. It's a bit more difficult here since
 * we don't know precisely when the audio data we put in the
 * audio stream buffer are sent to the soundcard.
 * 
 * TODO: al_get_audio_stream_fragments actually might be useful for
 * estimating the delay.
 * 
 */

#include "allegro5/allegro5.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_video.h"
#include "allegro5/internal/aintern_video.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/mathematics.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_video.h>
#include <stdio.h>
#include <math.h>

#if LIBAVCODEC_VERSION_MAJOR >= 53
   #define FFMPEG_0_8 1
#endif

ALLEGRO_DEBUG_CHANNEL("video")

#define AUDIO_BUFFER_SIZE (1024 * 8)
#define MAX_AUDIOQ_SIZE (5 * 16 * 1024)
#define MAX_VIDEOQ_SIZE (5 * 256 * 1024)
#define AV_SYNC_THRESHOLD 0.10
#define AV_NOSYNC_THRESHOLD 10.0
#define SAMPLE_CORRECTION_PERCENT_MAX 10
#define AUDIO_DIFF_AVG_NB 20
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define DEFAULT_AV_SYNC_TYPE AV_SYNC_EXTERNAL_MASTER
#define AUDIO_BUF_SIZE ((AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2)
#define AUDIO_BUF(vs) ((uint8_t *)((intptr_t)((vs)->audio_buf_unaligned + 15) & ~0xf))

#define NOPTS_VALUE ((int64_t)AV_NOPTS_VALUE)
int64_t FIXME_global_video_pkt_pts = NOPTS_VALUE;

typedef struct PacketQueue {
   AVPacketList *first_pkt, *last_pkt;
   int nb_packets;
   int size;
   ALLEGRO_MUTEX *mutex;
   ALLEGRO_COND *cond;
} PacketQueue;

typedef struct VideoPicture {
   AVFrame *frame;
   ALLEGRO_BITMAP *bmp;
   int width, height;          
   int allocated;
   double pts;
   bool dropped;
} VideoPicture;

typedef struct VideoState {
   ALLEGRO_VIDEO *video;
   ALLEGRO_MUTEX *pictq_mutex;
   ALLEGRO_COND *pictq_cond;
   ALLEGRO_THREAD *parse_thread;
   ALLEGRO_THREAD *video_thread;
   ALLEGRO_THREAD *audio_thread;
   
   /* Timing. */
   ALLEGRO_THREAD *timer_thread;
   ALLEGRO_MUTEX *timer_mutex;
   ALLEGRO_COND *timer_cond;

   AVFormatContext *format_context;
   int videoStream, audioStream;
   int video_index, audio_index;

   int av_sync_type;
   int seek_req;
   int seek_flags;
   bool after_seek_sync;
   int64_t seek_pos;
   double audio_clock;
   AVStream *audio_st;
   PacketQueue audioq;
   uint8_t audio_buf_unaligned[15 + AUDIO_BUF_SIZE];
   unsigned int audio_buf_size;
   unsigned int audio_buf_index;
   AVPacket audio_pkt;
   uint8_t *audio_pkt_data;
   int audio_pkt_size;
   int audio_hw_buf_size;
   double audio_diff_cum;
   double audio_diff_avg_coef;
   double audio_diff_threshold;
   int audio_diff_avg_count;
   double frame_timer;
   double frame_last_pts;
   double frame_last_delay;
   double video_clock;      
   double video_current_pts;    
   int64_t video_current_pts_time;
   int64_t external_clock_start;
   AVStream *video_st;
   PacketQueue videoq;
   
   double show_next;
   bool first;
   int got_picture;
   int dropped_count;
   bool paused;

   VideoPicture pictq[VIDEO_PICTURE_QUEUE_SIZE];
   VideoPicture shown;
   int pictq_size, pictq_rindex, pictq_windex;
   char filename[1024];
   bool quit;
} VideoState;

enum
{
   AV_SYNC_AUDIO_MASTER,
   AV_SYNC_VIDEO_MASTER,
   AV_SYNC_EXTERNAL_MASTER,
};

static AVPacket flush_pkt;
static bool init_once;

static void packet_queue_init(PacketQueue * q)
{
   memset(q, 0, sizeof(PacketQueue));
   q->mutex = al_create_mutex();
   q->cond = al_create_cond();
}

static int packet_queue_put(PacketQueue * q, AVPacket * pkt)
{
   AVPacketList *pkt1;
   if (pkt != &flush_pkt && av_dup_packet(pkt) < 0) {
      return -1;
   }
   pkt1 = av_malloc(sizeof(AVPacketList));
   if (!pkt1)
      return -1;
   pkt1->pkt = *pkt;
   pkt1->next = NULL;

   al_lock_mutex(q->mutex);

   if (!q->last_pkt)
      q->first_pkt = pkt1;
   else
      q->last_pkt->next = pkt1;
   q->last_pkt = pkt1;
   q->nb_packets++;
   q->size += pkt1->pkt.size;
   al_signal_cond(q->cond);
   al_unlock_mutex(q->mutex);
   return 0;
}

static int packet_queue_get(VideoState *is, PacketQueue * q, AVPacket * pkt, int block)
{
   AVPacketList *pkt1;
   int ret;

   al_lock_mutex(q->mutex);

   for (;;) {

      if (is->quit) {
         ret = -1;
         break;
      }

      pkt1 = q->first_pkt;
      if (pkt1) {
         q->first_pkt = pkt1->next;
         if (!q->first_pkt)
            q->last_pkt = NULL;
         q->nb_packets--;
         q->size -= pkt1->pkt.size;
         *pkt = pkt1->pkt;
         av_free(pkt1);
         ret = 1;
         break;
      }
      else if (!block) {
         ret = 0;
         break;
      }
      else {
         al_wait_cond(q->cond, q->mutex);
      }
   }
   al_unlock_mutex(q->mutex);
   return ret;
}

static void packet_queue_flush(PacketQueue * q)
{
   AVPacketList *pkt, *pkt1;

   al_lock_mutex(q->mutex);
   for (pkt = q->first_pkt; pkt != NULL; pkt = pkt1) {
      pkt1 = pkt->next;
      av_free_packet(&pkt->pkt);
      av_freep(&pkt);
   }
   q->last_pkt = NULL;
   q->first_pkt = NULL;
   q->nb_packets = 0;
   q->size = 0;
   al_unlock_mutex(q->mutex);
}

static double get_audio_clock(VideoState *is)
{
   double pts;
   int hw_buf_size, bytes_per_sec, n;

   pts = is->audio_clock;       /* maintained in the audio thread */
   hw_buf_size = is->audio_buf_size - is->audio_buf_index;
   bytes_per_sec = 0;
   n = is->audio_st->codec->channels * 2;
   if (is->audio_st) {
      bytes_per_sec = is->audio_st->codec->sample_rate * n;
   }
   if (bytes_per_sec) {
      pts -= (double)hw_buf_size / bytes_per_sec;
   }
   return pts;
}

static double get_video_clock(VideoState * is)
{
   double delta;

   delta = (av_gettime() - is->video_current_pts_time) / 1000000.0;
   return is->video_current_pts + delta;
}

static double get_external_clock(VideoState * is)
{
   return (av_gettime() - is->external_clock_start) / 1000000.0;
}

static double get_master_clock(VideoState * is)
{
   if (is->av_sync_type == AV_SYNC_VIDEO_MASTER) {
      return get_video_clock(is);
   }
   else if (is->av_sync_type == AV_SYNC_AUDIO_MASTER) {
      return get_audio_clock(is);
   }
   else {
      return get_external_clock(is);
   }
}

/* Add or subtract samples to get a better sync, return new
   audio buffer size */
static int synchronize_audio(VideoState * is, short *samples,
                      int samples_size, double pts)
{
   int n;
   double ref_clock;
   (void)pts;
   double diff;

   if (is->after_seek_sync) {
      /* Audio seems to be off for me after seeking, but skipping
       * video is less annoying than audio noise after the seek
       * when synching to the external clock.
       */
      is->external_clock_start = av_gettime() - get_audio_clock(is) * 1000000.0;
      is->after_seek_sync = false;
   }

   n = 2 * is->audio_st->codec->channels;
   
   if (is->av_sync_type != AV_SYNC_AUDIO_MASTER) {
      double avg_diff;
      int wanted_size, min_size, max_size;
      
      ref_clock = get_master_clock(is);
      diff = get_audio_clock(is) - ref_clock;
      
      //printf("%f, %f\n", diff, get_audio_clock(is));

      if (diff < AV_NOSYNC_THRESHOLD) {
         // accumulate the diffs
         is->audio_diff_cum = diff + is->audio_diff_avg_coef
             * is->audio_diff_cum;
         if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
            is->audio_diff_avg_count++;
         }
         else {
            avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);
            if (fabs(avg_diff) >= is->audio_diff_threshold) {
               //printf("AV_NOSYNC_THRESHOLD %f\n", avg_diff);
               wanted_size =
                   samples_size +
                   ((int)(avg_diff * is->audio_st->codec->sample_rate) * n);
               min_size = samples_size * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100;
               min_size &= ~3;
               max_size = samples_size * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100;
               max_size &= ~3;

               if (wanted_size < min_size) {
                  wanted_size = min_size;
               }
               else if (wanted_size > max_size) {
                  wanted_size = max_size;
               }
               if (wanted_size < samples_size) {
                  /* remove samples */
                  samples_size = wanted_size;
               }
               else if (wanted_size > samples_size) {
                  uint8_t *samples_end, *q;
                  int nb;
                  /* add samples by copying final sample */
                  nb = (samples_size - wanted_size);
                  samples_end = (uint8_t *) samples + samples_size - n;
                  q = samples_end + n;
                  while (nb > 0) {
                     memcpy(q, samples_end, n);
                     q += n;
                     nb -= n;
                  }
                  samples_size = wanted_size;
               }
            }
         }
      }
      else {
         /* difference is TOO big; reset diff stuff */
         is->audio_diff_avg_count = 0;
         is->audio_diff_cum = 0;
      }
   }
   return samples_size;
}

static int audio_decode_frame(VideoState * is, uint8_t * audio_buf, int buf_size,
                       double *pts_ptr)
{
   int len1, data_size, n;
   AVPacket *pkt = &is->audio_pkt;
   double pts;

   for (;;) {
      while (is->audio_pkt_size > 0) {
         data_size = buf_size;
#ifdef FFMPEG_0_8
         len1 = avcodec_decode_audio3(is->audio_st->codec,
                                      (int16_t *) audio_buf, &data_size, pkt);
#else
         len1 = avcodec_decode_audio2(is->audio_st->codec,
                                      (int16_t *) audio_buf, &data_size,
                                      is->audio_pkt_data, is->audio_pkt_size);
#endif
         if (len1 < 0) {
            /* if error, skip frame */
            is->audio_pkt_size = 0;
            break;
         }
         is->audio_pkt_data += len1;
         is->audio_pkt_size -= len1;
         if (data_size <= 0) {
            /* No data yet, get more frames */
            continue;
         }
         pts = is->audio_clock;
         *pts_ptr = pts;
         n = 2 * is->audio_st->codec->channels;
         is->audio_clock += (double)data_size /
             (double)(n * is->audio_st->codec->sample_rate);

         /* We have data, return it and come back for more later */
         return data_size;
      }
      if (pkt->data)
         av_free_packet(pkt);

      if (is->quit) {
         return -1;
      }
      /* next packet */
      if (packet_queue_get(is, &is->audioq, pkt, 1) < 0) {
         return -1;
      }
      if (pkt->data == flush_pkt.data) {
         avcodec_flush_buffers(is->audio_st->codec);
         continue;
      }
      is->audio_pkt_data = pkt->data;
      is->audio_pkt_size = pkt->size;
      /* if update, update the audio clock w/pts */
      if (pkt->pts != NOPTS_VALUE) {
         is->audio_clock = av_q2d(is->audio_st->time_base) * pkt->pts;
         //printf("audio_clock: %f\n", is->audio_clock);
      }
   }
}

static void audio_callback(void *userdata, uint8_t * stream, int len)
{
   VideoState *is = (VideoState *) userdata;
   int len1, audio_size;
   double pts;
   
   if (!is->first) return;

   while (len > 0) {
      if (is->audio_buf_index >= is->audio_buf_size) {

         /* We have already sent all our data; get more */
         audio_size = -1;
         if (!is->paused) {
            audio_size = audio_decode_frame(is, AUDIO_BUF(is),
               AUDIO_BUF_SIZE, &pts);
         }
            
         if (audio_size < 0) {
            /* If error, output silence */
            is->audio_buf_size = 1024;
            memset(AUDIO_BUF(is), 0, is->audio_buf_size);
         }
         else {
            audio_size = synchronize_audio(is, (int16_t *) AUDIO_BUF(is),
                                           audio_size, pts);
            is->audio_buf_size = audio_size;
         }
         is->audio_buf_index = 0;
      }
      len1 = is->audio_buf_size - is->audio_buf_index;
      if (len1 > len)
         len1 = len;
      memcpy(stream, (uint8_t *) AUDIO_BUF(is) + is->audio_buf_index, len1);
      len -= len1;
      stream += len1;
      is->audio_buf_index += len1;
   }
}

static void video_refresh_timer(void *userdata)
{

   VideoState *is = (VideoState *) userdata;
   VideoPicture *vp;
   double actual_delay, delay, sync_threshold, ref_clock, diff;

   if (!is->first)
      return;
   if (!is->video_st)
      return;
   if (is->pictq_size == 0)
      return;
   if (is->paused) return;
   
   if (get_master_clock(is) < is->show_next) return;

   vp = &is->pictq[is->pictq_rindex];

   is->video_current_pts = vp->pts;
   is->video_current_pts_time = av_gettime();

   delay = vp->pts - is->frame_last_pts;        /* the pts from last time */
   if (delay <= 0 || delay >= 1.0) {
      /* if incorrect delay, use previous one */
      delay = is->frame_last_delay;
   }
   /* save for next time */
   is->frame_last_delay = delay;
   is->frame_last_pts = vp->pts;

   /* update delay to sync to audio if not master source */
   if (is->av_sync_type != AV_SYNC_VIDEO_MASTER) {
      ref_clock = get_master_clock(is);
      diff = vp->pts - ref_clock;

      /* Skip or repeat the frame. Take delay into account */
      sync_threshold = (delay > AV_SYNC_THRESHOLD) ? delay : AV_SYNC_THRESHOLD;
      if (fabs(diff) < AV_NOSYNC_THRESHOLD) {
         if (diff <= -sync_threshold) {
            delay = 0;
         }
         else if (diff >= sync_threshold) {
            delay = 2 * delay;
         }
      }
   }

   is->frame_timer += delay;
   /* computer the REAL delay */
   actual_delay = is->frame_timer - (av_gettime() / 1000000.0);

   if (!vp->dropped && vp->bmp) {
      is->video->current_frame = vp->bmp;
      
      /* Can be NULL or wrong size, will be (re-)allocated as needed. */
      vp->bmp = is->shown.bmp;
      /* That way it won't be overwritten. */
      is->shown.bmp = is->video->current_frame;

      is->video->position = get_master_clock(is);
      is->video->video_position = get_video_clock(is);
      is->video->audio_position = get_audio_clock(is);
      
      is->show_next = is->video->position + actual_delay;
      al_signal_cond(is->timer_cond);

      if (is->video_st->codec->sample_aspect_ratio.num == 0) {
         is->video->aspect_ratio = 0;
      }
      else {
         is->video->aspect_ratio = av_q2d(is->video_st->codec->sample_aspect_ratio) *
             is->video_st->codec->width / is->video_st->codec->height;
      }
      if (is->video->aspect_ratio <= 0.0) {
         is->video->aspect_ratio = (float)is->video_st->codec->width /
             (float)is->video_st->codec->height;
      }
   }
   else
      is->dropped_count++;
   
   //printf("[%d] %f %s\n", is->pictq_rindex,
   //   actual_delay, vp->dropped ? "dropped" : "shown");

   /* update queue for next picture! */
   if (++is->pictq_rindex == VIDEO_PICTURE_QUEUE_SIZE) {
      is->pictq_rindex = 0;
   }
   
   al_lock_mutex(is->pictq_mutex);
   is->pictq_size--;
   al_signal_cond(is->pictq_cond);
   al_unlock_mutex(is->pictq_mutex);

   /* We skipped a frame... let's grab more until we catch up. */
   if (actual_delay < 0)
      video_refresh_timer(userdata);
}

static void alloc_picture(VideoState *is)
{

   VideoPicture *vp;

   vp = &is->pictq[is->pictq_windex];
   if (vp->bmp) {
      // we already have one make another, bigger/smaller
      al_destroy_bitmap(vp->bmp);
   }

   al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_BGR_888);
   vp->bmp = al_create_bitmap(is->video_st->codec->width,
                              is->video_st->codec->height);
   vp->width = is->video_st->codec->width;
   vp->height = is->video_st->codec->height;
}

/* Must be called from the user thread. */
static void retrieve_picture(VideoState *is)
{
   VideoPicture *vp;
   int dst_pix_fmt;
   AVPicture pict;
   static struct SwsContext *img_convert_ctx;
   AVFrame *pFrame;
   
   if (!is->got_picture) return;

   vp = &is->pictq[is->pictq_windex];
   
   pFrame = vp->frame;
   
   if (!vp->bmp ||
       vp->width != is->video_st->codec->width ||
       vp->height != is->video_st->codec->height) {
       alloc_picture(is);
   }
   
   /* YUV->RGB conversion. */
   if (vp->bmp) {
      /* Don't waste CPU on an outdated frame. */
      if (get_video_clock(is) >= get_external_clock(is) - 0.25) {
         ALLEGRO_LOCKED_REGION *lock;
         lock = al_lock_bitmap(vp->bmp, 0, ALLEGRO_LOCK_WRITEONLY);

         dst_pix_fmt = PIX_FMT_RGB24;

         pict.data[0] = lock->data;
         pict.linesize[0] = lock->pitch;

         if (img_convert_ctx == NULL) {
            int w = is->video_st->codec->width;
            int h = is->video_st->codec->height;
            img_convert_ctx = sws_getContext(w, h,
                                             is->video_st->codec->pix_fmt, w, h,
                                             dst_pix_fmt, SWS_BICUBIC, NULL, NULL,
                                             NULL);
            if (img_convert_ctx == NULL) {
               ALLEGRO_ERROR("Cannot initialize the conversion context!\n");
               return;
            }
         }
         sws_scale(img_convert_ctx, (uint8_t const *const*)pFrame->data, pFrame->linesize,
                   0, is->video_st->codec->height, pict.data, pict.linesize);

         al_unlock_bitmap(vp->bmp);
      }
      else {
         vp->dropped = true;
      }
      //printf("[%d] %f %f %f %s\n", is->pictq_windex,
      //   vp->pts, get_video_clock(is), get_external_clock(is), vp->dropped ? "dropped" : "shown");

      if (++is->pictq_windex == VIDEO_PICTURE_QUEUE_SIZE) {
         is->pictq_windex = 0;
      }
   }

   al_lock_mutex(is->pictq_mutex);
   is->pictq_size++;
   is->got_picture--;
   vp->allocated = 1;
   al_signal_cond(is->pictq_cond);
   al_unlock_mutex(is->pictq_mutex);
}

static int queue_picture(VideoState * is, AVFrame * pFrame, double pts)
{
   VideoPicture *vp;
   
   if (!is->first) {
      is->first = true;
      is->external_clock_start = av_gettime();
   }

   /* wait until we have space for a new pic */
   al_lock_mutex(is->pictq_mutex);
   while (is->pictq_size >= VIDEO_PICTURE_QUEUE_SIZE && !is->quit) {
      al_wait_cond(is->pictq_cond, is->pictq_mutex);
   }
   al_unlock_mutex(is->pictq_mutex);

   if (is->quit)
      return -1;

   // windex is set to 0 initially
   vp = &is->pictq[is->pictq_windex];
   vp->dropped = false;

   {
      ALLEGRO_EVENT event;

      vp->frame = pFrame;
      vp->pts = pts;
      vp->allocated = 0;
      /* we have to do it in the main thread */
      //printf("allocate %d (%4.1f ms)\n", is->pictq_windex,
      //   get_master_clock(is) * 1000);
      event.type = ALLEGRO_EVENT_VIDEO_FRAME_ALLOC;
      event.user.data1 = (intptr_t)is->video;
      al_emit_user_event(&is->video->es, &event, NULL);

      /* wait until we have a picture allocated */
      al_lock_mutex(is->pictq_mutex);
      is->got_picture++;
      while (!vp->allocated && !is->quit) {
         al_wait_cond(is->pictq_cond, is->pictq_mutex);
      }
      al_unlock_mutex(is->pictq_mutex);
      if (is->quit) {
         return -1;
      }
   }

   return 0;
}

// FIXME: Need synching! but right now it's all broken...
#if 0
static double synchronize_video(VideoState * is, AVFrame * src_frame,
   double pts)
{

   double frame_delay;

   if (pts != 0) {
      /* if we have pts, set video clock to it */
      is->video_clock = pts;
   }
   else {
      /* if we aren't given a pts, set it to the clock */
      pts = is->video_clock;
   }
   /* update the video clock */
   frame_delay = av_q2d(is->video_st->codec->time_base);
   /* if we are repeating a frame, adjust clock accordingly */
   frame_delay += src_frame->repeat_pict * (frame_delay * 0.5);
   is->video_clock += frame_delay;
   return pts;
}
#endif

/* These are called whenever we allocate a frame
 * buffer. We use this to store the global_pts in
 * a frame at the time it is allocated.
 */
static int our_get_buffer(struct AVCodecContext *c, AVFrame * pic)
{
   int ret = avcodec_default_get_buffer(c, pic);
   uint64_t *pts = av_malloc(sizeof(uint64_t));
   *pts = FIXME_global_video_pkt_pts;
   pic->opaque = pts;
   return ret;
}

static void our_release_buffer(struct AVCodecContext *c, AVFrame * pic)
{
   if (pic)
      av_freep(&pic->opaque);
   avcodec_default_release_buffer(c, pic);
}

static void *video_thread(ALLEGRO_THREAD * t, void *arg)
{
   VideoState *is = (VideoState *) arg;
   AVPacket pkt1, *packet = &pkt1;
   int frameFinished;
   AVFrame *pFrame;
   double pts;
   (void)t;

   pFrame = avcodec_alloc_frame();

   for (;;) {
      if (packet_queue_get(is, &is->videoq, packet, 1) < 0) {
         // means we quit getting packets
         break;
      }
      if (packet->data == flush_pkt.data) {
         avcodec_flush_buffers(is->video_st->codec);
         continue;
      }
      pts = 0;

      // Save global pts to be stored in pFrame
      FIXME_global_video_pkt_pts = packet->pts;
   
      // Decode video frame
#ifdef FFMPEG_0_8
      avcodec_decode_video2(is->video_st->codec, pFrame, &frameFinished,
                            packet);
#else
      avcodec_decode_video(is->video_st->codec, pFrame, &frameFinished,
                           packet->data, packet->size);
#endif
                                  

      if (packet->dts == NOPTS_VALUE
          && pFrame->opaque && *(int64_t *) pFrame->opaque != NOPTS_VALUE) {
         pts = 0;//*(uint64_t *) pFrame->opaque;
      }
      else if (packet->dts != NOPTS_VALUE) {
         pts = packet->dts;
      }
      else {
         pts = 0;
      }
      pts *= av_q2d(is->video_st->time_base);

      // Did we get a video frame?
      if (frameFinished) {
         //pts = synchronize_video(is, pFrame, pts);
         if (queue_picture(is, pFrame, pts) < 0) {
            break;
         }
      }
      av_free_packet(packet);
   }
   av_free(pFrame);
   return NULL;
}

static void *stream_audio(ALLEGRO_THREAD *thread, void *data)
{
   ALLEGRO_EVENT_QUEUE *queue;
   VideoState *is = data;
   ALLEGRO_AUDIO_STREAM *audio_stream = is->video->audio;

   queue = al_create_event_queue();
   
   if (is->video->mixer)
      al_attach_audio_stream_to_mixer(audio_stream, is->video->mixer);
   else if (is->video->voice)
      al_attach_audio_stream_to_voice(audio_stream, is->video->voice);
   else
      al_attach_audio_stream_to_mixer(audio_stream, al_get_default_mixer());

   al_register_event_source(queue,
      al_get_audio_stream_event_source(audio_stream));

   while (1) {
      ALLEGRO_EVENT event;
      al_wait_for_event(queue, &event);

      if (event.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
         void *buf = al_get_audio_stream_fragment(audio_stream);
         if (!buf)
            continue;
         audio_callback(is, buf, AUDIO_BUFFER_SIZE);
         al_set_audio_stream_fragment(audio_stream, buf);
      }
   }

   return thread;
}

static int stream_component_open(VideoState * is, int stream_index)
{

   AVFormatContext *format_context = is->format_context;
   AVCodecContext *codecCtx;
   AVCodec *codec;

   if (stream_index < 0 || stream_index >= (int)format_context->nb_streams) {
      return -1;
   }

   // Get a pointer to the codec context for the video stream
   codecCtx = format_context->streams[stream_index]->codec;


   if (codecCtx->codec_type == AVMEDIA_TYPE_AUDIO) {
      // Set audio settings from codec info
      is->video->audio =
          al_create_audio_stream(4, AUDIO_BUFFER_SIZE / 4,
                                 codecCtx->sample_rate,
                                 ALLEGRO_AUDIO_DEPTH_INT16,
                                 ALLEGRO_CHANNEL_CONF_1 + codecCtx->channels -
                                 1);

      if (!is->video->audio) {
         ALLEGRO_ERROR("al_create_audio_stream failed\n");
         return -1;
      }

      is->audio_thread = al_create_thread(stream_audio, is);
      al_start_thread(is->audio_thread);

      is->audio_hw_buf_size = AUDIO_BUFFER_SIZE;
   }
   codec = avcodec_find_decoder(codecCtx->codec_id);
   if (codec) {
      #ifdef FFMPEG_0_8
      if (avcodec_open2(codecCtx, codec, NULL) < 0) codec = NULL;
      #else
      if (avcodec_open(codecCtx, codec) < 0) codec = NULL;
      #endif
   }
   if (!codec) {
      ALLEGRO_ERROR("Unsupported codec!\n");
      return -1;
   }

   switch (codecCtx->codec_type) {
      case AVMEDIA_TYPE_AUDIO:
         is->audioStream = stream_index;
         is->audio_st = format_context->streams[stream_index];
         is->audio_buf_size = 0;
         is->audio_buf_index = 0;

         /* averaging filter for audio sync */
         is->audio_diff_avg_coef = exp(log(0.01 / AUDIO_DIFF_AVG_NB));
         is->audio_diff_avg_count = 0;
         /* Correct audio only if larger error than this */
         is->audio_diff_threshold = 0.1;

         memset(&is->audio_pkt, 0, sizeof(is->audio_pkt));
         packet_queue_init(&is->audioq);

         break;
      case AVMEDIA_TYPE_VIDEO:
         is->videoStream = stream_index;
         is->video_st = format_context->streams[stream_index];

         is->frame_timer = (double)av_gettime() / 1000000.0;
         is->frame_last_delay = 40e-3;
         is->video_current_pts_time = av_gettime();

         packet_queue_init(&is->videoq);
         is->video_thread = al_create_thread(video_thread, is);
         al_start_thread(is->video_thread);
         codecCtx->get_buffer = our_get_buffer;
         codecCtx->release_buffer = our_release_buffer;

         break;
      default:
         break;
   }

   return 0;
}

static void *decode_thread(ALLEGRO_THREAD *t, void *arg)
{
   VideoState *is = (VideoState *) arg;
   AVFormatContext *format_context = is->format_context;
   AVPacket pkt1, *packet = &pkt1;

   is->videoStream = -1;
   is->audioStream = -1;

   if (is->audio_index >= 0) {
      stream_component_open(is, is->audio_index);
   }
   if (is->video_index >= 0) {
      stream_component_open(is, is->video_index);
   }

   if (is->videoStream < 0 && is->audioStream < 0) {
      ALLEGRO_ERROR("%s: could not open codecs\n", is->filename);
      goto fail;
   }

   for (;;) {
      if (is->quit) {
         break;
      }

      if (is->seek_req) {
         int stream_index = -1;
         int64_t seek_target = is->seek_pos;

         if (is->videoStream >= 0)
            stream_index = is->videoStream;
         else if (is->audioStream >= 0)
            stream_index = is->audioStream;

         if (stream_index >= 0) {
            seek_target =
                av_rescale_q(seek_target, AV_TIME_BASE_Q,
                             format_context->streams[stream_index]->time_base);
         }
         
         if (av_seek_frame(is->format_context, stream_index, seek_target,
            is->seek_flags) < 0) {
            ALLEGRO_WARN("%s: error while seeking (%d, %lu)\n",
                    is->format_context->filename, stream_index, seek_target);
         }
         else {
            if (is->audioStream >= 0) {
               packet_queue_flush(&is->audioq);
               packet_queue_put(&is->audioq, &flush_pkt);
            }
            if (is->videoStream >= 0) {
               packet_queue_flush(&is->videoq);
               packet_queue_put(&is->videoq, &flush_pkt);
            }
         }
         is->seek_req = 0;
         is->after_seek_sync = true;
         
      }
      if (is->audioq.size > MAX_AUDIOQ_SIZE ||
          is->videoq.size > MAX_VIDEOQ_SIZE) {
         al_rest(0.01);
         continue;
      }
      if (av_read_frame(is->format_context, packet) < 0) {
         #ifdef FFMPEG_0_8
         if (!format_context->pb->eof_reached && !format_context->pb->error) {
         #else
         if (url_ferror((void *)&format_context->pb) == 0) {
         #endif
            al_rest(0.01);
            continue;
         }
         else {
            break;
         }
      }
      // Is this a packet from the video stream?
      if (packet->stream_index == is->videoStream) {
         packet_queue_put(&is->videoq, packet);
      }
      else if (packet->stream_index == is->audioStream) {
         packet_queue_put(&is->audioq, packet);
      }
      else {
         av_free_packet(packet);
      }
   }
   /* all done - wait for it */
   while (!is->quit) {
      al_rest(0.1);
   }

 fail:
   return t;
}

/* We want to be able to send an event to the user exactly at the time
 * a new video frame should be displayed.
 */
static void *timer_thread(ALLEGRO_THREAD *t, void *arg)
{
   VideoState *is = (VideoState *) arg;
   //double ot = 0, nt = 0;
   while (!is->quit) {
      ALLEGRO_EVENT event;
      double d;

      /* Wait here until someone signals to us when a new frame was
       * scheduled at is->show_next.
       */
      al_lock_mutex(is->timer_mutex);
      al_wait_cond(is->timer_cond, is->timer_mutex);
      al_unlock_mutex(is->timer_mutex);
      
      if (is->quit) break;
      
      /* Wait until that time. This wait is why we have our own thread
       * here so the user doesn't need to do it.
       */
      while (1) {
         d = is->show_next - get_master_clock(is);
         if (d <= 0) break;
         //printf("waiting %4.1f ms\n", d * 1000);
         al_rest(d);
      }

      //nt = get_master_clock(is);
      //printf("event after %4.1f ms\n", (nt - ot) * 1000);
      //ot = nt;
      /* Now is the time. */
      event.type = ALLEGRO_EVENT_VIDEO_FRAME_SHOW;
      event.user.data1 = (intptr_t)is->video;
      al_emit_user_event(&is->video->es, &event, NULL);
   }
   return t;
}

static void init(void)
{
   if (init_once) return;
   init_once = true;
   // Register all formats and codecs
   av_register_all();
   
   av_init_packet(&flush_pkt);
   flush_pkt.data = (void *)"FLUSH";
}

static bool open_video(ALLEGRO_VIDEO *video)
{
   VideoState *is = av_mallocz(sizeof *is);
   int i;
   AVRational fps;

   is->video = video;
   
   init();
   
   video->data = is;
   strncpy(is->filename, al_path_cstr(video->filename, '/'),
      sizeof(is->filename));
   
   is->av_sync_type = DEFAULT_AV_SYNC_TYPE;

   // Open video file
   #ifdef FFMPEG_0_8
   if (avformat_open_input(&is->format_context, is->filename, NULL,
      NULL) != 0) { 	
   #else
   if (av_open_input_file(&is->format_context, is->filename, NULL, 0,
      NULL) != 0) {
   #endif
      av_free(is);
      return false;
   }

   if (av_find_stream_info(is->format_context) < 0) {
      av_free(is);
      return false;
   }

   is->video_index = -1;
   is->audio_index = -1;
   for (i = 0; i < (int)is->format_context->nb_streams; i++) {
      if (is->format_context->streams[i]->codec->codec_type
         == AVMEDIA_TYPE_VIDEO && is->video_index < 0)
      {
         is->video_index = i;
      }
      if (is->format_context->streams[i]->codec->codec_type
         == AVMEDIA_TYPE_AUDIO && is->audio_index < 0)
      {
         is->audio_index = i;
      }
   }
   
   fps = is->format_context->streams[is->video_index]->r_frame_rate;
   video->fps = (double)fps.num / fps.den;
   video->audio_rate = is->format_context->streams[is->audio_index]->
      codec->sample_rate;
   video->width = is->format_context->streams[is->video_index]->codec->width;
   video->height = is->format_context->streams[is->video_index]->codec->height;

   is->pictq_mutex = al_create_mutex();
   is->pictq_cond = al_create_cond();
   
   is->timer_mutex = al_create_mutex();
   is->timer_cond = al_create_cond();

   return true;
}

static bool close_video(ALLEGRO_VIDEO *video)
{
   VideoState *is = video->data;
   
   is->quit = true;
   
   if (is->timer_thread) {
   
      al_lock_mutex(is->timer_mutex);
      al_signal_cond(is->timer_cond);
      al_unlock_mutex(is->timer_mutex);
   
      al_join_thread(is->timer_thread, NULL);
   }
   
   if (is->parse_thread) {
      al_join_thread(is->parse_thread, NULL);
   }

   al_destroy_mutex(is->timer_mutex);
   al_destroy_cond(is->timer_cond);

   av_free(is);
   return true;
}

static bool start_video(ALLEGRO_VIDEO *video)
{
   VideoState *is = video->data;

   is->timer_thread = al_create_thread(timer_thread, is);
   al_start_thread(is->timer_thread);

   is->parse_thread = al_create_thread(decode_thread, is);
   if (!is->parse_thread) {
      return false;
   }
   al_start_thread(is->parse_thread);
   return true;
}

static bool pause_video(ALLEGRO_VIDEO *video)
{
   VideoState *is = video->data;
   is->paused = video->paused;
   if (!is->paused) {
      is->after_seek_sync = true;
   }
   return true;
}

static bool seek_video(ALLEGRO_VIDEO *video)
{
   VideoState *is = video->data;
   if (!is->seek_req) {
      is->seek_pos = video->seek_to * AV_TIME_BASE;
      is->seek_flags = video->seek_to < video->position ? AVSEEK_FLAG_BACKWARD : 0;
      is->seek_req = 1;
      return true;
   }
   return false;
}

static bool update_video(ALLEGRO_VIDEO *video)
{
   VideoState *is = video->data;
   retrieve_picture(is); 
   video_refresh_timer(is);   
   return true;
}

static ALLEGRO_VIDEO_INTERFACE ffmpeg_vtable = {
   open_video,
   close_video,
   start_video,
   pause_video,
   seek_video,
   update_video
};

ALLEGRO_VIDEO_INTERFACE *_al_video_vtable = &ffmpeg_vtable;
