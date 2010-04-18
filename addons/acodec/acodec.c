#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_acodec_cfg.h"
#include "acodec.h"


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
   ret &= al_register_audio_stream_loader(".wav", _al_load_wav_audio_stream);

   ret &= al_register_sample_loader_f(".wav", _al_load_wav_f);
   ret &= al_register_sample_saver_f(".wav", _al_save_wav_f);
   ret &= al_register_audio_stream_loader_f(".wav", _al_load_wav_audio_stream_f);

#ifdef ALLEGRO_CFG_ACODEC_FLAC
   ret &= al_register_sample_loader(".flac", _al_load_flac);
   ret &= al_register_audio_stream_loader(".flac", _al_load_flac_audio_stream);
   ret &= al_register_sample_loader_f(".flac", _al_load_flac_f);
   ret &= al_register_audio_stream_loader_f(".flac", _al_load_flac_audio_stream_f);
#endif

#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
   ret &= al_register_audio_stream_loader(".xm", _al_load_xm_audio_stream);
   ret &= al_register_audio_stream_loader_f(".xm", _al_load_xm_audio_stream_f);
   ret &= al_register_audio_stream_loader(".it", _al_load_it_audio_stream);
   ret &= al_register_audio_stream_loader_f(".it", _al_load_it_audio_stream_f);
   ret &= al_register_audio_stream_loader(".mod", _al_load_mod_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mod", _al_load_mod_audio_stream_f);
   ret &= al_register_audio_stream_loader(".s3m", _al_load_s3m_audio_stream);
   ret &= al_register_audio_stream_loader_f(".s3m", _al_load_s3m_audio_stream_f);
#endif

#ifdef ALLEGRO_CFG_ACODEC_VORBIS
   ret &= al_register_sample_loader(".ogg", _al_load_ogg_vorbis);
   ret &= al_register_audio_stream_loader(".ogg", _al_load_ogg_vorbis_audio_stream);
   ret &= al_register_sample_loader_f(".ogg", _al_load_ogg_vorbis_f);
   ret &= al_register_audio_stream_loader_f(".ogg", _al_load_ogg_vorbis_audio_stream_f);
#endif

   return ret;
}


/* vim: set sts=3 sw=3 et: */
