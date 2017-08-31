package org.liballeg.example;

import org.liballeg.android.AllegroActivity;

public class ExampleActivity extends AllegroActivity {

   /* Load Allegro and other shared libraries in the lib directory of the apk
    * bundle.  You must load libraries which are required by later libraries
    * first.
    */
   static {
      System.loadLibrary("allegro-debug");
      System.loadLibrary("allegro_primitives-debug");
      System.loadLibrary("allegro_image-debug");
      //System.loadLibrary("allegro_font-debug");
      //System.loadLibrary("allegro_ttf-debug");
      //System.loadLibrary("allegro_audio-debug");
      //System.loadLibrary("allegro_acodec-debug");
      //System.loadLibrary("allegro_monolith-debug");
   }

   /* By default, AllegroActivity.onCreate will cause Allegro to load the
    * shared object `libapp.so'.  You can specify another library name by
    * overriding the constructor.
    */
   public ExampleActivity() {
      super("libexample.so");
   }
}
