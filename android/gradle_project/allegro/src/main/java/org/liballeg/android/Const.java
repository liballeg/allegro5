package org.liballeg.android;

import android.content.pm.ActivityInfo;
import android.view.Surface;

final class Const
{
   /* color.h */
   static final int A5O_PIXEL_FORMAT_ABGR_8888         = 17;
   static final int A5O_PIXEL_FORMAT_BGR_565           = 20;
   static final int A5O_PIXEL_FORMAT_RGBA_4444         = 26;
   static final int A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8  = 27;

   /* display.h */
   static final int A5O_RED_SIZE	    = 0;
   static final int A5O_GREEN_SIZE	    = 1;
   static final int A5O_BLUE_SIZE	    = 2;
   static final int A5O_ALPHA_SIZE	    = 3;
   static final int A5O_COLOR_SIZE      = 14;
   static final int A5O_DEPTH_SIZE	    = 15;
   static final int A5O_STENCIL_SIZE    = 16;
   static final int A5O_SAMPLE_BUFFERS  = 17;
   static final int A5O_SAMPLES	    = 18;

   static final int A5O_DISPLAY_ORIENTATION_UNKNOWN = 0;
   static final int A5O_DISPLAY_ORIENTATION_0_DEGREES = 1;
   static final int A5O_DISPLAY_ORIENTATION_90_DEGREES = 2;
   static final int A5O_DISPLAY_ORIENTATION_180_DEGREES = 4;
   static final int A5O_DISPLAY_ORIENTATION_270_DEGREES = 8;
   static final int A5O_DISPLAY_ORIENTATION_PORTRAIT = 5;
   static final int A5O_DISPLAY_ORIENTATION_LANDSCAPE = 10;
   static final int A5O_DISPLAY_ORIENTATION_ALL = 15;
   static final int A5O_DISPLAY_ORIENTATION_FACE_UP = 16;
   static final int A5O_DISPLAY_ORIENTATION_FACE_DOWN = 32;

   /* events.h */
   static final int A5O_EVENT_TOUCH_BEGIN  = 50;
   static final int A5O_EVENT_TOUCH_END    = 51;
   static final int A5O_EVENT_TOUCH_MOVE   = 52;
   static final int A5O_EVENT_TOUCH_CANCEL = 53;

   static int toAndroidOrientation(int alleg_orientation)
   {
      switch (alleg_orientation)
      {
         case A5O_DISPLAY_ORIENTATION_0_DEGREES:
            return ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

         case A5O_DISPLAY_ORIENTATION_90_DEGREES:
            return ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;

         case A5O_DISPLAY_ORIENTATION_180_DEGREES:
            return ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;

         case A5O_DISPLAY_ORIENTATION_270_DEGREES:
            return ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;

         case A5O_DISPLAY_ORIENTATION_PORTRAIT:
            return ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;

         case A5O_DISPLAY_ORIENTATION_LANDSCAPE:
            return ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;

         case A5O_DISPLAY_ORIENTATION_ALL:
            return ActivityInfo.SCREEN_ORIENTATION_SENSOR;
      }

      return ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;
   }

   static int toAllegroOrientation(int rotation)
   {
      switch (rotation) {
         case Surface.ROTATION_0:
            return A5O_DISPLAY_ORIENTATION_0_DEGREES;

         case Surface.ROTATION_180:
            return A5O_DISPLAY_ORIENTATION_180_DEGREES;

         /* Android device orientations are the opposite of Allegro ones.
          * Allegro orientations are the orientation of the device, with 0
          * being holding the device at normal orientation, 90 with the device
          * rotated 90 degrees clockwise and so on. Android orientations are
          * the orientations of the GRAPHICS. By rotating the device by 90
          * degrees clockwise, the graphics are actually rotated 270 degrees,
          * and that's what Android uses.
          */

         case Surface.ROTATION_90:
            return A5O_DISPLAY_ORIENTATION_270_DEGREES;

         case Surface.ROTATION_270:
            return A5O_DISPLAY_ORIENTATION_90_DEGREES;
      }

      return A5O_DISPLAY_ORIENTATION_UNKNOWN;
   }
}

/* vim: set sts=3 sw=3 et: */
