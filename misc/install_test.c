/*
 * A small test program that you can compile and run to verify that Allegro was
 * installed correctly. It assumes you installed all the addons.
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_video.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_physfs.h>
#include <allegro5/allegro_memfile.h>

#include <stdio.h>

int main() {
   uint32_t version = al_get_allegro_version();
   int major = version >> 24;
   int minor = (version >> 16) & 255;
   int revision = (version >> 8) & 255;
   int release = version & 255;

   fprintf(stderr, "Library version: %d.%d.%d.%d\n", major, minor, revision, release);
   fprintf(stderr, "Header version: %d.%d.%d.%d\n", A5O_VERSION, A5O_SUB_VERSION, A5O_WIP_VERSION, A5O_RELEASE_NUMBER);
   fprintf(stderr, "Header version string: %s\n", A5O_VERSION_STR);

   if (!al_init()) {
      fprintf(stderr, "Failed to initialize Allegro, probably a header/shared library version mismatch.\n");
      return -1;
   }

#define INIT_CHECK(init_function, addon_name) do { if (!init_function()) { fprintf(stderr, "Failed to initialize the " addon_name " addon.\n"); return -1; } } while (0)

   INIT_CHECK(al_init_font_addon, "font");
   INIT_CHECK(al_init_ttf_addon, "TTF");
   INIT_CHECK(al_init_image_addon, "image");
   INIT_CHECK(al_install_audio, "audio");
   INIT_CHECK(al_init_acodec_addon, "acodec");
   INIT_CHECK(al_init_native_dialog_addon, "native dialog");
   INIT_CHECK(al_init_primitives_addon, "primitives");
   INIT_CHECK(al_init_video_addon, "video");

   fprintf(stderr, "Everything looks good!\n");
   return 0;
}
