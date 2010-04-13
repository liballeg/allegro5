#ifndef __al_included_ex_acodec_h
#define __al_included_ex_acodec_h

/* acodec.h
 *
 * Selectively includes available audio codecs; to be used
 * by any Allegro example that loads samples or streams.
 *
 * The ALLEGRO_CFG_ACODEC_* flags are only available while
 * compiling Allegro. If the examples are compiled outside the
 * normal Allegro CMake build, the flags will need to be set
 * manually. 
 *
 * Likewise, these flags are not available in user programs.
 */

#ifdef ALLEGRO_CFG_ACODEC_FLAC
#include "allegro5/allegro_flac.h"
#endif
#ifdef ALLEGRO_CFG_ACODEC_VORBIS
#include "allegro5/allegro_vorbis.h"
#endif
#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
#include "allegro5/allegro_modaudio.h"
#endif

/* init_acodecs()
 *
 * Returns true if all audio codecs are successfully initialized.
 */

static bool init_acodecs(void)
{
   bool ret = true;
   
#ifdef ALLEGRO_CFG_ACODEC_FLAC
   ret &= al_init_flac_addon();
#endif

#ifdef ALLEGRO_CFG_ACODEC_MODAUDIO
   ret &= al_init_modaudio_addon();
#endif

#ifdef ALLEGRO_CFG_ACODEC_VORBIS
   ret &= al_init_ogg_vorbis_addon();
#endif

   return ret;
}

#endif
