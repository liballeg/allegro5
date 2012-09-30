#include "cosmic_protector.hpp"

void StreamResource::destroy(void)
{
   if (!stream)
      return;
   al_destroy_audio_stream(stream);
   stream = 0;
}

bool StreamResource::load(void)
{
   if (!al_is_audio_installed()) {
      debug_message("Skipped loading stream %s\n", filename.c_str());
      return true;
   }

   stream = al_load_audio_stream(filename.c_str(), 4, 1024);
   if (!stream) {
       debug_message("Error creating stream\n");
       return false;
   }

   al_set_audio_stream_playing(stream, false);
   al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_LOOP);
   al_attach_audio_stream_to_mixer(stream, al_get_default_mixer());

   return true;
}

void* StreamResource::get(void)
{
   return stream;
}

StreamResource::StreamResource(const char* filename) :
   stream(0),
   filename(filename)
{
}

