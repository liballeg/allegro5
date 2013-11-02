package org.liballeg.example;

import org.liballeg.app.AllegroActivity;

public class ExampleActivity extends AllegroActivity {

   /* Load Allegro and other shared libraries in the lib directory of the apk
    * bundle.  You must load libraries which are required by later libraries
    * first.
    */
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
