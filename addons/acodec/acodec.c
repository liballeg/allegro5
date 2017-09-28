#include "allegro5/allegro_acodec.h"
#include "allegro5/allegro_audio.h"
#include "allegro5/internal/aintern_acodec_cfg.h"
#include "acodec.h"

#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
#include <dumb.h>
#endif

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

   /* buil-in VOC loader */
   ret &= al_register_sample_loader(".voc", _al_load_voc);
   ret &= al_register_sample_loader_f(".voc", _al_load_voc_f);

#ifdef ALLEGRO_CFG_ACODEC_FLAC
   ret &= al_register_sample_loader(".flac", _al_load_flac);
   ret &= al_register_audio_stream_loader(".flac", _al_load_flac_audio_stream);
   ret &= al_register_sample_loader_f(".flac", _al_load_flac_f);
   ret &= al_register_audio_stream_loader_f(".flac", _al_load_flac_audio_stream_f);
#endif

#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
#if (DUMB_MAJOR_VERSION) >= 2
   /*
    * DUMB 2.0 offers a single loader for at least 13 formats, see their
    * readme. Amiga NoiseTracker isn't listed there, but it's DUMB-supported.
    * It merely has no common extensions, see:
    * https://github.com/kode54/dumb/issues/53
    */
   ret &= al_register_audio_stream_loader(".669", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".669", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".amf", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".amf", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".asy", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".asy", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".it", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".it", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".mod", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mod", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".mtm", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mtm", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".okt", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".okt", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".psm", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".psm", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".ptm", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".ptm", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".riff", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".riff", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".s3m", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".s3m", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".stm", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".stm", _al_load_dumb_audio_stream_f);
   ret &= al_register_audio_stream_loader(".xm", _al_load_dumb_audio_stream);
   ret &= al_register_audio_stream_loader_f(".xm", _al_load_dumb_audio_stream_f);
#else
   /*
    * DUMB 0.9.3 supported only these 4 formats and had no *_any loader.
    * Avoid DUMB 1.0 because of versioning problems: dumb.h from git tag 1.0
    * reports 0.9.3 in its version numbers.
    */
   ret &= al_register_audio_stream_loader(".xm", _al_load_xm_audio_stream);
   ret &= al_register_audio_stream_loader_f(".xm", _al_load_xm_audio_stream_f);
   ret &= al_register_audio_stream_loader(".it", _al_load_it_audio_stream);
   ret &= al_register_audio_stream_loader_f(".it", _al_load_it_audio_stream_f);
   ret &= al_register_audio_stream_loader(".mod", _al_load_mod_audio_stream);
   ret &= al_register_audio_stream_loader_f(".mod", _al_load_mod_audio_stream_f);
   ret &= al_register_audio_stream_loader(".s3m", _al_load_s3m_audio_stream);
   ret &= al_register_audio_stream_loader_f(".s3m", _al_load_s3m_audio_stream_f);
#endif // DUMB_MAJOR_VERSION
#endif // ALLEGRO_CFG_ACODEC_MODAUDIO

#ifdef ALLEGRO_CFG_ACODEC_VORBIS
   ret &= al_register_sample_loader(".ogg", _al_load_ogg_vorbis);
   ret &= al_register_audio_stream_loader(".ogg", _al_load_ogg_vorbis_audio_stream);
   ret &= al_register_sample_loader_f(".ogg", _al_load_ogg_vorbis_f);
   ret &= al_register_audio_stream_loader_f(".ogg", _al_load_ogg_vorbis_audio_stream_f);
#endif

#ifdef ALLEGRO_CFG_ACODEC_OPUS
   ret &= al_register_sample_loader(".opus", _al_load_ogg_opus);
   ret &= al_register_audio_stream_loader(".opus", _al_load_ogg_opus_audio_stream);
   ret &= al_register_sample_loader_f(".opus", _al_load_ogg_opus_f);
   ret &= al_register_audio_stream_loader_f(".opus", _al_load_ogg_opus_audio_stream_f);
#endif

   return ret;
}


/* vim: set sts=3 sw=3 et: */
