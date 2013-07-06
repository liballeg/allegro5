package org.liballeg.example;

import org.liballeg.app.AllegroActivity;

public class ExampleActivity extends AllegroActivity {

   /* load allegro */
   static {
      /* FIXME: see if we can't load the allegro library name, or type from the manifest here */
      System.loadLibrary("allegro-debug");
      System.loadLibrary("allegro_primitives-debug");
      System.loadLibrary("allegro_image-debug");
      //System.loadLibrary("allegro_font-debug");
      //System.loadLibrary("allegro_ttf-debug");
      //System.loadLibrary("allegro_audio-debug");
      //System.loadLibrary("allegro_acodec-debug");
      //System.loadLibrary("allegro_monolith-debug");
   }

}
