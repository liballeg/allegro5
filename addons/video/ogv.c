/* Ogg Theora/Vorbis video backend
 *
 * TODO:
 * - seeking
 * - generate video frame events
 * - better ycbcr->rgb
 * - improve frame skipping
 * - Ogg Skeleton support
 * - pass Theora test suite
 *
 * NOTE: we treat Ogg timestamps as the beginning of frames.  In reality they
 * are end timestamps.  Normally this should not be noticeable, but can make a
 * difference.
 */

#include <stdio.h>
#include "allegro5/allegro5.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_video.h"
#include "allegro5/internal/aintern_dtor.h"
#include "allegro5/internal/aintern_vector.h"
#include "allegro5/internal/aintern_video.h"

#include <ogg/ogg.h>
#include <theora/theora.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>

ALLEGRO_DEBUG_CHANNEL("video")


/* XXX probably should be based on stream parameters */
static const int NUM_FRAGS    = 2;
static const int FRAG_SAMPLES = 4096;
static const int RGB_PIXEL_FORMAT = ALLEGRO_PIXEL_FORMAT_ABGR_8888;


typedef struct OGG_VIDEO OGG_VIDEO;
typedef struct STREAM STREAM;
typedef struct THEORA_STREAM THEORA_STREAM;
typedef struct VORBIS_STREAM VORBIS_STREAM;
typedef struct PACKET_NODE PACKET_NODE;

enum {
   STREAM_TYPE_UNKNOWN = 0,
   STREAM_TYPE_THEORA,
   STREAM_TYPE_VORBIS
};

struct PACKET_NODE {
   PACKET_NODE *next;
   ogg_packet pkt;
};

struct THEORA_STREAM {
   th_info info;
   th_comment comment;
   th_setup_info *setup;
   th_dec_ctx *ctx;
   ogg_int64_t prev_framenum;
   double frame_duration;
};

struct VORBIS_STREAM {
   vorbis_info info;
   vorbis_comment comment;
   bool inited_for_data;
   vorbis_dsp_state dsp;
   vorbis_block block;
   int channels;
   float *next_fragment;            /* channels * FRAG_SAMPLES elements */
   int next_fragment_pos;
};

struct STREAM {
   int stream_type;
   bool active;
   bool headers_done;
   ogg_stream_state state;
   PACKET_NODE *packet_queue;
   union {
      THEORA_STREAM theora;
      VORBIS_STREAM vorbis;
   } u;
};

struct OGG_VIDEO {
   ALLEGRO_FILE *fp;
   bool reached_eof;
   ogg_sync_state sync_state;
   _AL_VECTOR streams;              /* vector of STREAM pointers */
   STREAM *selected_video_stream;   /* one of the streams */
   STREAM *selected_audio_stream;   /* one of the streams */
   int seek_counter;

   /* Video output. */
   th_pixel_fmt pixel_fmt;
   th_ycbcr_buffer buffer;
   bool buffer_dirty;
   unsigned char* rgb_data;
   ALLEGRO_BITMAP *frame_bmp;
   ALLEGRO_BITMAP *pic_bmp;         /* frame_bmp, or subbitmap thereof */

   ALLEGRO_EVENT_SOURCE evtsrc;
   ALLEGRO_EVENT_QUEUE *queue;
   ALLEGRO_MUTEX *mutex;
   ALLEGRO_COND *cond;
   ALLEGRO_THREAD *thread;
};


/* forward declarations */
static bool ogv_close_video(ALLEGRO_VIDEO *video);


/* Packet queue. */

static PACKET_NODE *create_packet_node(ogg_packet *packet)
{
   PACKET_NODE *node = al_malloc(sizeof(PACKET_NODE));

   node->next = NULL;
   node->pkt = *packet;
   node->pkt.packet = al_malloc(packet->bytes);
   memcpy(node->pkt.packet, packet->packet, packet->bytes);

   return node;
}

static void free_packet_node(PACKET_NODE *node)
{
   ASSERT(node->next == NULL);

   al_free(node->pkt.packet);
   al_free(node);
}

static void add_tail_packet(STREAM *stream, PACKET_NODE *node)
{
   PACKET_NODE *cur;

   ASSERT(node->next == NULL);

   for (cur = stream->packet_queue; cur != NULL; cur = cur->next) {
      if (cur->next == NULL) {
         cur->next = node;
         ASSERT(cur->pkt.packetno < node->pkt.packetno);
         return;
      }
   }

   ASSERT(stream->packet_queue == NULL);
   stream->packet_queue = node;
}

static void add_head_packet(STREAM *stream, PACKET_NODE *node)
{
   ASSERT(node->next == NULL);

   node->next = stream->packet_queue;
   stream->packet_queue = node;

   if (node->next) {
      ASSERT(node->pkt.packetno < node->next->pkt.packetno);
   }
}

static PACKET_NODE *take_head_packet(STREAM *stream)
{
   PACKET_NODE *cur;

   cur = stream->packet_queue;
   if (!cur) {
      return NULL;
   }
   if (cur->next) {
      ASSERT(cur->pkt.packetno < cur->next->pkt.packetno);
   }
   stream->packet_queue = cur->next;
   cur->next = NULL;
   return cur;
}

static void free_packet_queue(STREAM *stream)
{
   while (stream->packet_queue) {
      PACKET_NODE *node = stream->packet_queue;
      stream->packet_queue = node->next;
      node->next = NULL;
      free_packet_node(node);
   }
}

static void deactivate_stream(STREAM *stream)
{
   stream->active = false;
   free_packet_queue(stream);
}


/* Logical streams. */

static STREAM *create_stream(OGG_VIDEO *ogv, int serial)
{
   STREAM *stream;
   STREAM **slot;

   stream = al_calloc(1, sizeof(STREAM));
   stream->stream_type = STREAM_TYPE_UNKNOWN;
   stream->active = true;
   stream->headers_done = false;
   ogg_stream_init(&stream->state, serial);
   stream->packet_queue = NULL;

   slot = _al_vector_alloc_back(&ogv->streams);
   (*slot) = stream;

   return stream;
}

static STREAM *find_stream(OGG_VIDEO *ogv, int serial)
{
   unsigned i;

   for (i = 0; i < _al_vector_size(&ogv->streams); i++) {
      STREAM **slot = _al_vector_ref(&ogv->streams, i);
      STREAM *stream = *slot;

      if (stream->state.serialno == serial) {
         return stream;
      }
   }

   return NULL;
}

static void free_stream(STREAM *stream)
{
   ASSERT(stream);

   ogg_stream_clear(&stream->state);

   free_packet_queue(stream);

   switch (stream->stream_type) {
      case STREAM_TYPE_UNKNOWN:
         break;

      case STREAM_TYPE_THEORA:
         {
            THEORA_STREAM *tstream = &stream->u.theora;

            ALLEGRO_DEBUG("Clean up Theora.\n");
            th_info_clear(&tstream->info);
            th_comment_clear(&tstream->comment);
            if (tstream->setup) {
               th_setup_free(tstream->setup);
            }
            if (tstream->ctx) {
               th_decode_free(tstream->ctx);
            }
         }
         break;

      case STREAM_TYPE_VORBIS:
         {
            VORBIS_STREAM *vstream = &stream->u.vorbis;

            ALLEGRO_DEBUG("Clean up Vorbis.\n");
            vorbis_info_clear(&vstream->info);
            vorbis_comment_clear(&vstream->comment);
            if (vstream->inited_for_data) {
               vorbis_block_clear(&vstream->block);
               vorbis_dsp_clear(&vstream->dsp);
            }
            al_free(vstream->next_fragment);
         }
         break;
   }

   al_free(stream);
}

/* Returns true if got a page. */
static bool read_page(OGG_VIDEO *ogv, ogg_page *page)
{
   const int buffer_size = 4096;

   if (al_feof(ogv->fp) || al_ferror(ogv->fp)) {
      ogv->reached_eof = true;
      return ogg_sync_pageout(&ogv->sync_state, page) == 1;
   }

   while (ogg_sync_pageout(&ogv->sync_state, page) != 1) {
      char *buffer;
      size_t bytes;
      int rc;

      buffer = ogg_sync_buffer(&ogv->sync_state, buffer_size);
      bytes = al_fread(ogv->fp, buffer, buffer_size);
      if (bytes == 0) {
         ALLEGRO_DEBUG("End of file.\n");
         return false;
      }

      rc = ogg_sync_wrote(&ogv->sync_state, bytes);
      ASSERT(rc == 0);
   }

   return true;
}

/* Return true if got a packet for the stream. */
static bool read_packet(OGG_VIDEO *ogv, STREAM *stream, ogg_packet *packet)
{
   ogg_page page;
   int rc;

   for (;;) {
      rc = ogg_stream_packetout(&stream->state, packet);
      if (rc == 1) {
         /* Got a packet for stream. */
         return true;
      }

      if (read_page(ogv, &page)) {
         STREAM *page_stream = find_stream(ogv, ogg_page_serialno(&page));

         if (page_stream && page_stream->active) {
            rc = ogg_stream_pagein(&page_stream->state, &page);
            ASSERT(rc == 0);
         }
      }
      else {
         return false;
      }
   }
}


/* Header decoding. */

static bool try_decode_theora_header(STREAM *stream, ogg_packet *packet)
{
   int rc;

   switch (stream->stream_type) {
      case STREAM_TYPE_UNKNOWN:
         th_info_init(&stream->u.theora.info);
         th_comment_init(&stream->u.theora.comment);
         break;
      case STREAM_TYPE_THEORA:
         break;
      case STREAM_TYPE_VORBIS:
         return false;
   }

   if (stream->headers_done) {
      add_tail_packet(stream, create_packet_node(packet));
      return true;
   }

   rc = th_decode_headerin(&stream->u.theora.info, &stream->u.theora.comment,
      &stream->u.theora.setup, packet);

   if (rc > 0) {
      /* Successfully parsed a Theora header. */
      if (stream->stream_type == STREAM_TYPE_UNKNOWN) {
         ALLEGRO_DEBUG("Found Theora stream.\n");
         stream->stream_type = STREAM_TYPE_THEORA;
      }
      return true;
   }

   if (rc == 0) {
      /* First Theora data packet. */
      add_tail_packet(stream, create_packet_node(packet));
      stream->headers_done = true;
      return true;
   }

   th_info_clear(&stream->u.theora.info);
   th_comment_clear(&stream->u.theora.comment);
   return false;
}

static int try_decode_vorbis_header(STREAM *stream, ogg_packet *packet)
{
   int rc;

   switch (stream->stream_type) {
      case STREAM_TYPE_UNKNOWN:
         vorbis_info_init(&stream->u.vorbis.info);
         vorbis_comment_init(&stream->u.vorbis.comment);
         break;
      case STREAM_TYPE_VORBIS:
         break;
      case STREAM_TYPE_THEORA:
         return false;
   }

   rc = vorbis_synthesis_headerin(&stream->u.vorbis.info,
      &stream->u.vorbis.comment, packet);

   if (rc == 0) {
      /* Successfully parsed a Vorbis header. */
      if (stream->stream_type == STREAM_TYPE_UNKNOWN) {
         ALLEGRO_INFO("Found Vorbis stream.\n");
         stream->stream_type = STREAM_TYPE_VORBIS;
         stream->u.vorbis.inited_for_data = false;
      }
      return true;
   }

   if (stream->stream_type == STREAM_TYPE_VORBIS && rc == OV_ENOTVORBIS) {
      /* First data packet. */
      add_tail_packet(stream, create_packet_node(packet));
      stream->headers_done = true;
      return true;
   }

   vorbis_info_clear(&stream->u.vorbis.info);
   vorbis_comment_clear(&stream->u.vorbis.comment);
   return false;
}

static bool all_headers_done(OGG_VIDEO *ogv)
{
   bool have_something = false;
   unsigned i;

   for (i = 0; i < _al_vector_size(&ogv->streams); i++) {
      STREAM **slot = _al_vector_ref(&ogv->streams, i);
      STREAM *stream = *slot;

      switch (stream->stream_type) {
         case STREAM_TYPE_THEORA:
         case STREAM_TYPE_VORBIS:
            have_something = true;
            if (!stream->headers_done)
               return false;
            break;
         case STREAM_TYPE_UNKNOWN:
            break;
      }
   }

   return have_something;
}

static void read_headers(OGG_VIDEO *ogv)
{
   ogg_page page;
   ogg_packet packet;
   STREAM *stream;
   int serial;
   int rc;

   ALLEGRO_DEBUG("Begin reading headers.\n");

   do {
      /* Read a page of data. */
      if (!read_page(ogv, &page)) {
         break;
      }

      /* Which stream is this page for? */
      serial = ogg_page_serialno(&page);

      /* Start a new stream or find an existing one. */
      if (ogg_page_bos(&page)) {
         stream = create_stream(ogv, serial);
      }
      else {
         stream = find_stream(ogv, serial);
      }

      if (!stream) {
         ALLEGRO_WARN("No stream for serial: %x\n", serial);
         continue;
      }

      if (!stream->active) {
         continue;
      }

      /* Add the page to the stream. */
      rc = ogg_stream_pagein(&stream->state, &page);
      ASSERT(rc == 0);

      /* Look for a complete packet. */
      rc = ogg_stream_packetpeek(&stream->state, &packet);
      if (rc == 0) {
         continue;
      }
      if (rc == -1) {
         ALLEGRO_WARN("No packet due to lost sync or hole in data.\n");
         continue;
      }

      /* Try to decode the packet as a Theora or Vorbis header. */
      if (!try_decode_theora_header(stream, &packet)) {
         if (!try_decode_vorbis_header(stream, &packet)) {
            ALLEGRO_DEBUG("Unknown packet type ignored.\n");
         }
      }

      /* Consume the packet. */
      rc = ogg_stream_packetout(&stream->state, &packet);
      ASSERT(rc == 1);

   } while (!all_headers_done(ogv));

   ALLEGRO_DEBUG("End reading headers.\n");
}


/* Vorbis streams. */

static void setup_vorbis_stream_decode(ALLEGRO_VIDEO *video, STREAM *stream)
{
   VORBIS_STREAM * const vstream = &stream->u.vorbis;
   int rc;

   rc = vorbis_synthesis_init(&vstream->dsp, &vstream->info);
   ASSERT(rc == 0);

   rc = vorbis_block_init(&vstream->dsp, &vstream->block);
   ASSERT(rc == 0);

   vstream->inited_for_data = true;

   video->audio_rate = vstream->info.rate;
   vstream->channels = vstream->info.channels;

   vstream->next_fragment =
      al_calloc(vstream->channels * FRAG_SAMPLES, sizeof(float));

   ALLEGRO_INFO("Audio rate: %f\n", video->audio_rate);
   ALLEGRO_INFO("Audio channels: %d\n", vstream->channels);
}

static void handle_vorbis_data(VORBIS_STREAM *vstream, ogg_packet *packet)
{
   int rc;

   rc = vorbis_synthesis(&vstream->block, packet);
   if (rc != 0) {
      ALLEGRO_ERROR("vorbis_synthesis returned %d\n", rc);
      return;
   }

   rc = vorbis_synthesis_blockin(&vstream->dsp, &vstream->block);
   if (rc != 0) {
      ALLEGRO_ERROR("vorbis_synthesis_blockin returned %d\n", rc);
      return;
   }
}

static bool generate_next_audio_fragment(VORBIS_STREAM *vstream)
{
   float **pcm = NULL;
   float *p;
   int samples;
   int i, ch;
   int rc;

   samples = vorbis_synthesis_pcmout(&vstream->dsp, &pcm);
   if (samples == 0) {
      return false;
   }

   if (samples > FRAG_SAMPLES - vstream->next_fragment_pos) {
      samples = FRAG_SAMPLES - vstream->next_fragment_pos;
   }

   ASSERT(vstream->next_fragment);
   p = &vstream->next_fragment[
      vstream->channels * vstream->next_fragment_pos];

   if (vstream->channels == 2) {
      for (i = 0; i < samples; i++) {
         *p++ = pcm[0][i];
         *p++ = pcm[1][i];
      }
   }
   else if (vstream->channels == 1) {
      for (i = 0; i < samples; i++) {
         *p++ = pcm[0][i];
      }
   }
   else {
      for (i = 0; i < samples; i++) {
         for (ch = 0; ch < vstream->channels; ch++) {
            *p++ = pcm[ch][i];
         }
      }
   }

   vstream->next_fragment_pos += samples;

   rc = vorbis_synthesis_read(&vstream->dsp, samples);
   ASSERT(rc == 0);
   return true;
}

static void poll_vorbis_decode(OGG_VIDEO *ogv, STREAM *vstream_outer)
{
   VORBIS_STREAM * const vstream = &vstream_outer->u.vorbis;

   while (vstream->next_fragment_pos < FRAG_SAMPLES
      && generate_next_audio_fragment(vstream))
   {
   }

   while (vstream->next_fragment_pos < FRAG_SAMPLES) {
      PACKET_NODE *node;
      ogg_packet packet;

      node = take_head_packet(vstream_outer);
      if (node) {
         handle_vorbis_data(vstream, &node->pkt);
         generate_next_audio_fragment(vstream);
         free_packet_node(node);
      }
      else if (read_packet(ogv, vstream_outer, &packet)) {
         handle_vorbis_data(vstream, &packet);
         generate_next_audio_fragment(vstream);
      }
      else {
         break;
      }
   }
}

static ALLEGRO_AUDIO_STREAM *create_audio_stream(const ALLEGRO_VIDEO *video,
   const STREAM *vstream_outer)
{
   const VORBIS_STREAM *vstream = &vstream_outer->u.vorbis;
   ALLEGRO_AUDIO_STREAM *audio;
   int chanconf;
   int rc;

   switch (vstream->channels) {
      case 1: chanconf = ALLEGRO_CHANNEL_CONF_1; break;
      case 2: chanconf = ALLEGRO_CHANNEL_CONF_2; break;
      case 3: chanconf = ALLEGRO_CHANNEL_CONF_3; break;
      case 4: chanconf = ALLEGRO_CHANNEL_CONF_4; break;
      case 6: chanconf = ALLEGRO_CHANNEL_CONF_5_1; break;
      case 7: chanconf = ALLEGRO_CHANNEL_CONF_6_1; break;
      case 8: chanconf = ALLEGRO_CHANNEL_CONF_7_1; break;
      default:
         ALLEGRO_WARN("Unsupported number of channels: %d\n",
            vstream->channels);
         return NULL;
   }

   audio = al_create_audio_stream(NUM_FRAGS, FRAG_SAMPLES,
      vstream->info.rate, ALLEGRO_AUDIO_DEPTH_FLOAT32, chanconf);
   if (!audio) {
      ALLEGRO_ERROR("Could not create audio stream.\n");
      return NULL;
   }

   if (video->mixer) {
      rc = al_attach_audio_stream_to_mixer(audio, video->mixer);
   }
   else if (video->voice) {
      rc = al_attach_audio_stream_to_voice(audio, video->voice);
   }
   else {
      rc = al_attach_audio_stream_to_mixer(audio, al_get_default_mixer());
   }

   if (rc) {
      ALLEGRO_DEBUG("Audio stream ready.\n");
   }
   else {
      ALLEGRO_ERROR("Could not attach audio stream.\n");
      al_destroy_audio_stream(audio);
      audio = NULL;
   }

   return audio;
}

static void update_audio_fragment(ALLEGRO_AUDIO_STREAM *audio_stream,
   VORBIS_STREAM *vstream, bool paused, bool reached_eof)
{
   float *frag;

   frag = al_get_audio_stream_fragment(audio_stream);
   if (!frag)
      return;

   if (paused || vstream->next_fragment_pos < FRAG_SAMPLES) {
      if (!paused && !reached_eof) {
         ALLEGRO_WARN("Next fragment not ready.\n");
      }
      memset(frag, 0, vstream->channels * FRAG_SAMPLES * sizeof(float));
   }
   else {
      memcpy(frag, vstream->next_fragment,
         vstream->channels * FRAG_SAMPLES * sizeof(float));
      vstream->next_fragment_pos = 0;
   }

   al_set_audio_stream_fragment(audio_stream, frag);
}


/* Theora streams. */

static void setup_theora_stream_decode(ALLEGRO_VIDEO *video, OGG_VIDEO *ogv,
   STREAM *tstream_outer)
{
   THEORA_STREAM * const tstream = &tstream_outer->u.theora;
   int frame_w = tstream->info.frame_width;
   int frame_h = tstream->info.frame_height;
   int pic_x = tstream->info.pic_x;
   int pic_y = tstream->info.pic_y;
   int pic_w = tstream->info.pic_width;
   int pic_h = tstream->info.pic_height;
   float aspect_ratio = 1.0;

   tstream->ctx = th_decode_alloc(&tstream->info, tstream->setup);
   ASSERT(tstream->ctx);
   th_setup_free(tstream->setup);
   tstream->setup = NULL;

   ogv->pixel_fmt = tstream->info.pixel_fmt;
   ogv->frame_bmp = al_create_bitmap(frame_w, frame_h);
   if (pic_x == 0 && pic_y == 0 && pic_w == frame_w && pic_h == frame_h) {
      ogv->pic_bmp = ogv->frame_bmp;
   }
   else {
      ogv->pic_bmp = al_create_sub_bitmap(ogv->frame_bmp,
         pic_x, pic_y, pic_w, pic_h);
   }
   ogv->rgb_data =
      al_malloc(al_get_pixel_size(RGB_PIXEL_FORMAT) * frame_w * frame_h);

   video->fps =
      (double)tstream->info.fps_numerator /
      (double)tstream->info.fps_denominator;
   tstream->frame_duration =
      (double)tstream->info.fps_denominator /
      (double)tstream->info.fps_numerator;

   if (tstream->info.aspect_denominator != 0) {
      aspect_ratio =
         (double)(pic_w * tstream->info.aspect_numerator) /
         (double)(pic_h * tstream->info.aspect_denominator);
   }

   _al_compute_scaled_dimensions(pic_w, pic_h, aspect_ratio, &video->scaled_width,
      &video->scaled_height);

   tstream->prev_framenum = -1;

   ALLEGRO_INFO("Frame size: %dx%d\n", frame_w, frame_h);
   ALLEGRO_INFO("Picture size: %dx%d\n", pic_w, pic_h);
   ALLEGRO_INFO("Scaled size: %fx%f\n", video->scaled_width, video->scaled_height);
   ALLEGRO_INFO("FPS: %f\n", video->fps);
   ALLEGRO_INFO("Frame_duration: %f\n", tstream->frame_duration);
}

static int64_t get_theora_framenum(THEORA_STREAM *tstream, ogg_packet *packet)
{
   if (packet->granulepos > 0) {
      return th_granule_frame(&tstream->info, packet->granulepos);
   }

   return tstream->prev_framenum + 1;
}

static bool handle_theora_data(ALLEGRO_VIDEO *video, THEORA_STREAM *tstream,
   ogg_packet *packet, bool *ret_new_frame)
{
   int64_t expected_framenum;
   int64_t framenum;
   int rc;

   expected_framenum = tstream->prev_framenum + 1;
   framenum = get_theora_framenum(tstream, packet);

   if (framenum > expected_framenum) {
      /* Packet is for a later frame, don't decode it yet. */
      ALLEGRO_DEBUG("Expected frame %ld, got %ld\n",
         (long)expected_framenum, (long)framenum);
      video->video_position += tstream->frame_duration;
      tstream->prev_framenum++;
      return false;
   }

   if (framenum < expected_framenum) {
      ALLEGRO_DEBUG("Expected frame %ld, got %ld (decoding anyway)\n",
         (long)expected_framenum, (long)framenum);
   }

   rc = th_decode_packetin(tstream->ctx, packet, NULL);
   if (rc != TH_EBADPACKET) {
      /* HACK: When we seek to beginning, the first few packets are actually
       * headers. To properly fix this, those packets should be ignored when we
       * do the seeking (see the XXX there). */
      ASSERT(rc == 0 || rc == TH_DUPFRAME);
   }

   if (rc == 0) {
      *ret_new_frame = true;

      video->video_position = framenum * tstream->frame_duration;
      tstream->prev_framenum = framenum;
   }

   return true;
}

/* Y'CrCb to RGB conversion. */
/* XXX simple slow implementation */

static unsigned char clamp(int x)
{
   if (x < 0)
      return 0;
   if (x > 255)
      return 255;
   return x;
}

static INLINE void ycbcr_to_rgb(th_ycbcr_buffer buffer,
   unsigned char* rgb_data, int pixel_size, int pitch, int xshift, int yshift)
{
   const int w = buffer[0].width;
   const int h = buffer[0].height;
   int x, y;

   for (y = 0; y < h; y++) {
      const int y2 = y >> yshift;
      for (x = 0; x < w; x++) {
         const int x2 = x >> xshift;
         unsigned char * const data = rgb_data + y * pitch + x * pixel_size;
         const int yp = buffer[0].data[y  * buffer[0].stride + x ];
         const int cb = buffer[1].data[y2 * buffer[1].stride + x2];
         const int cr = buffer[2].data[y2 * buffer[2].stride + x2];
         const int C = yp - 16;
         const int D = cb - 128;
         const int E = cr - 128;

         data[0] = clamp((298*C         + 409*E + 128) >> 8);
         data[1] = clamp((298*C - 100*D - 208*E + 128) >> 8);
         data[2] = clamp((298*C + 516*D         + 128) >> 8);
         data[3] = 0xff;
      }
   }
}

static void convert_buffer_to_rgba(OGG_VIDEO *ogv)
{
   const int pixel_size = al_get_pixel_size(RGB_PIXEL_FORMAT);
   const int pitch = pixel_size * al_get_bitmap_width(ogv->frame_bmp);

   switch (ogv->pixel_fmt) {
      case TH_PF_420:
         ycbcr_to_rgb(ogv->buffer, ogv->rgb_data, pixel_size, pitch, 1, 1);
         break;
      case TH_PF_422:
         ycbcr_to_rgb(ogv->buffer, ogv->rgb_data, pixel_size, pitch, 1, 0);
         break;
      case TH_PF_444:
         ycbcr_to_rgb(ogv->buffer, ogv->rgb_data, pixel_size, pitch, 0, 0);
         break;
      default:
         ALLEGRO_ERROR("Unsupported pixel format.\n");
         break;
   }
}

static int poll_theora_decode(ALLEGRO_VIDEO *video, STREAM *tstream_outer)
{
   OGG_VIDEO * const ogv = video->data;
   THEORA_STREAM * const tstream = &tstream_outer->u.theora;
   bool new_frame = false;
   int num_frames = 0;
   int rc;

   while (video->video_position < video->position) {
      PACKET_NODE *node;
      ogg_packet packet;

      node = take_head_packet(tstream_outer);
      if (node) {
         if (handle_theora_data(video, tstream, &node->pkt, &new_frame)) {
            free_packet_node(node);
            num_frames++;
         }
         else {
            add_head_packet(tstream_outer, node);
         }
      }
      else if (read_packet(ogv, tstream_outer, &packet)) {
         if (handle_theora_data(video, tstream, &packet, &new_frame)) {
            num_frames++;
         }
         else {
            add_head_packet(tstream_outer, create_packet_node(&packet));
         }
      }
      else {
         break;
      }

      /* Only skip frames if we are really falling behind, not just slightly
       * ahead of the target position.
       * XXX improve frame skipping algorithm
       */
      if (video->video_position
            >= video->position - 3.0*tstream->frame_duration) {
         break;
      }
   }

   if (new_frame) {
      ALLEGRO_EVENT event;
      al_lock_mutex(ogv->mutex);

      rc = th_decode_ycbcr_out(tstream->ctx, ogv->buffer);
      ASSERT(rc == 0);

      convert_buffer_to_rgba(ogv);

      ogv->buffer_dirty = true;

      event.type = ALLEGRO_EVENT_VIDEO_FRAME_SHOW;
      event.user.data1 = (intptr_t)video;
      al_emit_user_event(&video->es, &event, NULL);

      al_unlock_mutex(ogv->mutex);
   }

   return num_frames;
}


/* Seeking. */

static void seek_to_beginning(ALLEGRO_VIDEO *video, OGG_VIDEO *ogv,
   THEORA_STREAM *tstream)
{
   unsigned i;
   int rc;
   bool seeked;

   for (i = 0; i < _al_vector_size(&ogv->streams); i++) {
      STREAM **slot = _al_vector_ref(&ogv->streams, i);
      STREAM *stream = *slot;

      ogg_stream_reset(&stream->state);
      free_packet_queue(stream);
   }

   if (tstream) {
      ogg_int64_t granpos = 0;

      rc = th_decode_ctl(tstream->ctx, TH_DECCTL_SET_GRANPOS, &granpos,
         sizeof(granpos));
      ASSERT(rc == 0);

      tstream->prev_framenum = -1;
   }

   rc = ogg_sync_reset(&ogv->sync_state);
   ASSERT(rc == 0);

   seeked = al_fseek(ogv->fp, 0, SEEK_SET);
   ASSERT(seeked);
   /* XXX read enough file data to get into position */

   ogv->reached_eof = false;
   video->audio_position = 0.0;
   video->video_position = 0.0;
   video->position = 0.0;

   /* XXX maybe clear backlog of time and stream fragment events */
}

/* Decode thread. */

static void *decode_thread_func(ALLEGRO_THREAD *thread, void *_video)
{
   ALLEGRO_VIDEO * const video = _video;
   OGG_VIDEO * const ogv = video->data;
   STREAM *tstream_outer;
   THEORA_STREAM *tstream = NULL;
   STREAM *vstream_outer;
   VORBIS_STREAM *vstream = NULL;
   ALLEGRO_TIMER *timer;
   double audio_pos_step = 0.0;
   double timer_dur;

   ALLEGRO_DEBUG("Thread started.\n");

   /* Destruction is handled by al_close_video. */
   _al_push_destructor_owner();
   tstream_outer = ogv->selected_video_stream;
   if (tstream_outer) {
      ASSERT(tstream_outer->stream_type == STREAM_TYPE_THEORA);
      tstream = &tstream_outer->u.theora;
   }

   vstream_outer = ogv->selected_audio_stream;
   if (vstream_outer) {
      ASSERT(vstream_outer->stream_type == STREAM_TYPE_VORBIS);

      video->audio = create_audio_stream(video, vstream_outer);
      if (video->audio) {
         vstream = &vstream_outer->u.vorbis;
         audio_pos_step = (double)FRAG_SAMPLES / vstream->info.rate;
      }
      else {
         deactivate_stream(vstream_outer);
         vstream_outer = NULL;
      }
   }

   if (!tstream_outer && !vstream_outer) {
      ALLEGRO_WARN("No audio or video stream found.\n");
      _al_pop_destructor_owner();
      return NULL;
   }

   timer_dur = 1.0;
   if (audio_pos_step != 0.0) {
      timer_dur = audio_pos_step / NUM_FRAGS;
   }
   if (tstream && tstream->frame_duration < timer_dur) {
      timer_dur = tstream->frame_duration;
   }
   timer = al_create_timer(timer_dur);
   al_register_event_source(ogv->queue, al_get_timer_event_source(timer));

   if (video->audio) {
      al_register_event_source(ogv->queue,
      al_get_audio_stream_event_source(video->audio));
   }
   _al_pop_destructor_owner();

   ALLEGRO_DEBUG("Begin decode loop.\n");

   al_start_timer(timer);

   while (!al_get_thread_should_stop(thread)) {
      ALLEGRO_EVENT ev;

      al_wait_for_event(ogv->queue, &ev);

      if (ev.type == _ALLEGRO_EVENT_VIDEO_SEEK) {
         double seek_to = ev.user.data1 / 1.0e6;
         /* XXX we only know how to seek to start of video */
         ASSERT(seek_to <= 0.0);
         al_lock_mutex(ogv->mutex);
         seek_to_beginning(video, ogv, tstream);
         ogv->seek_counter++;
         al_broadcast_cond(ogv->cond);
         al_unlock_mutex(ogv->mutex);
         continue;
      }

      if (ev.type == ALLEGRO_EVENT_TIMER) {
         if (vstream_outer && video->playing) {
            poll_vorbis_decode(ogv, vstream_outer);
         }

         /* If no audio then video is master. */
         if (!video->audio && video->playing && !ogv->reached_eof) {
            video->position += tstream->frame_duration;
         }

         if (tstream_outer) {
            poll_theora_decode(video, tstream_outer);
         }

         if (video->playing && ogv->reached_eof) {
            ALLEGRO_EVENT event;
            video->playing = false;

            event.type = ALLEGRO_EVENT_VIDEO_FINISHED;
            event.user.data1 = (intptr_t)video;
            al_emit_user_event(&video->es, &event, NULL);
         }
      }

      if (ev.type == ALLEGRO_EVENT_AUDIO_STREAM_FRAGMENT) {
         /* Audio clock is master when it exists. */
         /* XXX This doesn't work well when the process is paused then resumed,
          * due to a problem with the audio addon.  We get a flood of
          * fragment events which pushes the position field ahead of the
          * real audio position.
          */
         if (video->playing && !ogv->reached_eof) {
            video->audio_position += audio_pos_step;
            video->position = video->audio_position - NUM_FRAGS * audio_pos_step;
         }
         update_audio_fragment(video->audio, vstream, !video->playing,
            ogv->reached_eof);
      }
   }

   ALLEGRO_DEBUG("End decode loop.\n");

   if (video->audio) {
      al_drain_audio_stream(video->audio);
      al_destroy_audio_stream(video->audio);
      video->audio = NULL;
   }
   al_destroy_timer(timer);

   ALLEGRO_DEBUG("Thread exit.\n");

   return NULL;
}


static bool update_frame_bmp(OGG_VIDEO *ogv)
{
   ALLEGRO_LOCKED_REGION *lr;
   int y;
   int pitch = al_get_pixel_size(RGB_PIXEL_FORMAT) * al_get_bitmap_width(ogv->frame_bmp);

   lr = al_lock_bitmap(ogv->frame_bmp, RGB_PIXEL_FORMAT,
      ALLEGRO_LOCK_WRITEONLY);
   if (!lr) {
      ALLEGRO_ERROR("Failed to lock bitmap.\n");
      return false;
   }

   for (y = 0; y < al_get_bitmap_height(ogv->frame_bmp); y++) {
      memcpy((unsigned char*)lr->data + y * lr->pitch, ogv->rgb_data + y * pitch, pitch);
   }

   al_unlock_bitmap(ogv->frame_bmp);
   return true;
}


/* Video interface. */

static bool do_open_video(ALLEGRO_VIDEO *video, OGG_VIDEO *ogv)
{
   unsigned i;

   read_headers(ogv);

   /* Select the first Theora and Vorbis tracks. */
   for (i = 0; i < _al_vector_size(&ogv->streams); i++) {
      STREAM **slot = _al_vector_ref(&ogv->streams, i);
      STREAM *stream = *slot;

      if (stream->stream_type == STREAM_TYPE_THEORA &&
         !ogv->selected_video_stream)
      {
         setup_theora_stream_decode(video, ogv, stream);
         ogv->selected_video_stream = stream;
      }
      else if (stream->stream_type == STREAM_TYPE_VORBIS &&
         !ogv->selected_audio_stream)
      {
         setup_vorbis_stream_decode(video, stream);
         ogv->selected_audio_stream = stream;
      }
      else {
         deactivate_stream(stream);
      }
   }

   return ogv->selected_video_stream || ogv->selected_audio_stream;
}

static bool ogv_open_video(ALLEGRO_VIDEO *video)
{
   ALLEGRO_FILE *fp;
   OGG_VIDEO *ogv;
   int rc;

   fp = video->file;
   if (!fp) {
      ALLEGRO_WARN("Failed to open video file from file interface.\n");
      return false;
   }

   ogv = al_calloc(1, sizeof(OGG_VIDEO));
   if (!ogv) {
      ALLEGRO_ERROR("Out of memory.\n");
      al_fclose(fp);
      return false;
   }
   ogv->fp = fp;
   rc = ogg_sync_init(&ogv->sync_state);
   ASSERT(rc == 0);
   _al_vector_init(&ogv->streams, sizeof(STREAM *));

   if (!do_open_video(video, ogv)) {
      ALLEGRO_ERROR("No audio or video stream found.\n");
      ogv_close_video(video);
      return false;
   }

   /* ogv->mutex and ogv->thread are created in ogv_start_video. */

   video->data = ogv;
   return true;
}
static bool ogv_close_video(ALLEGRO_VIDEO *video)
{
   OGG_VIDEO *ogv;
   unsigned i;

   ogv = video->data;
   if (ogv) {
      if (ogv->thread) {
         al_join_thread(ogv->thread, NULL);
         al_destroy_user_event_source(&ogv->evtsrc);
         al_destroy_event_queue(ogv->queue);
         al_destroy_mutex(ogv->mutex);
         al_destroy_cond(ogv->cond);
         al_destroy_thread(ogv->thread);
      }

      al_fclose(ogv->fp);
      ogg_sync_clear(&ogv->sync_state);
      for (i = 0; i < _al_vector_size(&ogv->streams); i++) {
         STREAM **slot = _al_vector_ref(&ogv->streams, i);
         free_stream(*slot);
      }
      _al_vector_free(&ogv->streams);
      if (ogv->pic_bmp != ogv->frame_bmp) {
         al_destroy_bitmap(ogv->pic_bmp);
      }
      al_destroy_bitmap(ogv->frame_bmp);

      al_free(ogv->rgb_data);

      al_free(ogv);
   }
   video->data = NULL;

   return true;
}

static bool ogv_start_video(ALLEGRO_VIDEO *video)
{
   OGG_VIDEO *ogv = video->data;

   if (ogv->thread != NULL) {
      ALLEGRO_ERROR("Thread already created.\n");
      return false;
   }

   ogv->thread = al_create_thread(decode_thread_func, video);
   if (!ogv->thread) {
      ALLEGRO_ERROR("Could not create thread.\n");
      return false;
   }

   al_init_user_event_source(&ogv->evtsrc);
   ogv->queue = al_create_event_queue();
   ogv->mutex = al_create_mutex();
   ogv->cond = al_create_cond();

   al_register_event_source(ogv->queue, &ogv->evtsrc);

   al_start_thread(ogv->thread);
   return true;
}

static bool ogv_set_video_playing(ALLEGRO_VIDEO *video)
{
   OGG_VIDEO * const ogv = video->data;
   if (ogv->reached_eof) {
      video->playing = false;
   }
   return true;
}

static bool ogv_seek_video(ALLEGRO_VIDEO *video, double seek_to)
{
   OGG_VIDEO *ogv = video->data;
   ALLEGRO_EVENT ev;
   int seek_counter;

   /* XXX we only know how to seek to beginning */
   if (seek_to > 0.0) {
      return false;
   }

   al_lock_mutex(ogv->mutex);

   seek_counter = ogv->seek_counter;

   ev.user.type = _ALLEGRO_EVENT_VIDEO_SEEK;
   ev.user.data1 = seek_to * 1.0e6;
   ev.user.data2 = 0;
   ev.user.data3 = 0;
   ev.user.data4 = 0;
   al_emit_user_event(&ogv->evtsrc, &ev, NULL);

   while (seek_counter == ogv->seek_counter) {
      al_wait_cond(ogv->cond, ogv->mutex);
   }

   al_unlock_mutex(ogv->mutex);

   return true;
}

static bool ogv_update_video(ALLEGRO_VIDEO *video)
{
   OGG_VIDEO *ogv = video->data;
   int w, h;
   bool ret;

   al_lock_mutex(ogv->mutex);

   w = ogv->buffer[0].width;
   h = ogv->buffer[0].height;

   if (w > 0 && h && h > 0 && ogv->frame_bmp) {
      ASSERT(w == al_get_bitmap_width(ogv->frame_bmp));
      ASSERT(h == al_get_bitmap_height(ogv->frame_bmp));

      if (ogv->buffer_dirty) {
         ret = update_frame_bmp(ogv);
         ogv->buffer_dirty = false;
      }
      else {
         ret = true;
      }

      video->current_frame = ogv->pic_bmp;
   }
   else {
      /* No frame ready yet. */
      ret = false;
   }

   al_unlock_mutex(ogv->mutex);

   return ret;
}

static ALLEGRO_VIDEO_INTERFACE ogv_vtable = {
   ogv_open_video,
   ogv_close_video,
   ogv_start_video,
   ogv_set_video_playing,
   ogv_seek_video,
   ogv_update_video,
};

ALLEGRO_VIDEO_INTERFACE *_al_video_ogv_vtable(void)
{
   return &ogv_vtable;
}

bool _al_video_identify_ogv(ALLEGRO_FILE *f)
{
   uint8_t x[4];
   if (al_fread(f, x, 4) < 4)
      return false;
   /* TODO: This technically only verifies that this is an OGG container,
    * saying nothing of the contents. Maybe refactor read_headers and make
    * use of it here. */
   if (memcmp(x, "OggS", 4) == 0)
      return true;
   return false;
}

/* vim: set sts=3 sw=3 et: */
