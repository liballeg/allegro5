package org.liballeg.app;

final class Const
{
   /* display.h */
   static final int ALLEGRO_RED_SIZE	    = 0;
   static final int ALLEGRO_GREEN_SIZE	    = 1;
   static final int ALLEGRO_BLUE_SIZE	    = 2;
   static final int ALLEGRO_ALPHA_SIZE	    = 3;
   static final int ALLEGRO_DEPTH_SIZE	    = 15;
   static final int ALLEGRO_STENCIL_SIZE    = 16;
   static final int ALLEGRO_SAMPLE_BUFFERS  = 17;
   static final int ALLEGRO_SAMPLES	    = 18;

   static final int ALLEGRO_DISPLAY_ORIENTATION_UNKNOWN = 0;
   static final int ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES = 1;
   static final int ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES = 2;
   static final int ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES = 4;
   static final int ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES = 8;
   static final int ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT = 5;
   static final int ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE = 10;
   static final int ALLEGRO_DISPLAY_ORIENTATION_ALL = 15;
   static final int ALLEGRO_DISPLAY_ORIENTATION_FACE_UP = 16;
   static final int ALLEGRO_DISPLAY_ORIENTATION_FACE_DOWN = 32;

   /* events.h */
   static final int ALLEGRO_EVENT_TOUCH_BEGIN  = 50;
   static final int ALLEGRO_EVENT_TOUCH_END    = 51;
   static final int ALLEGRO_EVENT_TOUCH_MOVE   = 52;
   static final int ALLEGRO_EVENT_TOUCH_CANCEL = 53;
}

/* vim: set sts=3 sw=3 et: */
