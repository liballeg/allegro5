#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_acodec_cfg.h"
#include "acodec.h"


/* globals */
static bool acodec_inited = false;


/* Function: al_get_allegro_acodec_version
 */
uint32_t al_get_allegro_acodec_version(void)
{
   return ALLEGRO_VERSION_INT;
}


/* Function: al_init_acodec_addon
 */
bool al_init_acodec_addon(void)
{
   bool ret = true;

   ret &= al_register_sample_loader(".wav", _al_load_wav);
   ret &= al_register_sample_saver(".wav", _al_save_wav);
   ret &= al_register_sample_loader_f(".wav", _al_load_wav_f);
   ret &= al_register_sample_saver_f(".wav", _al_save_wav_f);
   ret &= al_register_audio_stream_loader(".wav", _al_load_wav_audio_stream);
   ret &= al_register_audio_stream_loader_f(".wav", _al_load_wav_audio_stream_f);
   ret &= al_register_sample_identifier(".wav", _al_identify_wav);

   /* buil-in VOC loader */
   ret &= al_register_sample_loader(".voc", _al_load_voc);
   ret &= al_register_sample_loader_f(".voc", _al_load_voc_f);
   ret &= al_register_sample_identifier(".voc", _al_identify_voc);

#ifdef ALLEGRO_CFG_ACODEC_FLAC
   ret &= al_register_sample_loader(".flac", _al_load_flac);
   ret &= al_register_audio_stream_loader(".flac", _al_load_flac_audio_stream);
   ret &= al_register_sample_loader_f(".flac", _al_load_flac_f);
   ret &= al_register_audio_stream_loader_f(".flac", _al_load_flac_audio_stream_f);
   ret &= al_register_sample_identifier(".flac", _al_identify_flac);
#endif

#ifdef ALLEGRO_CFG_ACODEC_VORBIS
   ret &= al_register_sample_loader(".ogg", _al_load_ogg_vorbis);
   ret &= al_register_audio_stream_loader(".ogg", _al_load_ogg_vorbis_audio_stream);
   ret &= al_register_sample_loader_f(".ogg", _al_load_ogg_vorbis_f);
   ret &= al_register_audio_stream_loader_f(".ogg", _al_load_ogg_vorbis_audio_stream_f);
   ret &= al_register_sample_identifier(".ogg", _al_identify_ogg_vorbis);
#endif

#ifdef ALLEGRO_CFG_ACODEC_OPUS
   ret &= al_register_sample_loader(".opus", _al_load_ogg_opus);
   ret &= al_register_audio_stream_loader(".opus", _al_load_ogg_opus_audio_stream);
   ret &= al_register_sample_loader_f(".opus", _al_load_ogg_opus_f);
   ret &= al_register_audio_stream_loader_f(".opus", _al_load_ogg_opus_audio_stream_f);
   ret &= al_register_sample_identifier(".opus", _al_identify_ogg_opus);
#endif

#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
   ret &= _al_register_dumb_loaders();
#endif

   /* MP3 will mis-identify a lot of mod files, so put its identifier last */
#ifdef ALLEGRO_CFG_ACODEC_MP3
   ret &= al_register_sample_loader(".mp3", _al_load_mp3);
   ret &= al_register_audio_stream_loader(".mp3", _al_load_mp3_audio_stream);
   ret &= al_register_sample_loader_f(".mp3", _al_load_mp3_f);
   ret &= al_register_audio_stream_loader_f(".mp3", _al_load_mp3_audio_stream_f);
   ret &= al_register_sample_identifier(".mp3", _al_identify_mp3);
#endif

   acodec_inited = ret;

   return ret;
}


/* Function: al_is_acodec_addon_initialized
 */
bool al_is_acodec_addon_initialized(void)
{
   return acodec_inited;
}


/* vim: set sts=3 sw=3 et: */
