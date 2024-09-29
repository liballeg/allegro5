package org.liballeg.android;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;
import java.io.InputStream;

class ImageLoader
{
   private static final String TAG = "ImageLoader";

   static Bitmap decodeBitmapAsset(Activity activity, final String filename)
   {
      Bitmap decodedBitmap;
      Log.d(TAG, "decodeBitmapAsset begin");
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         // Only added in API level 19, avoid for now.
         // options.inPremultiplied = premul;
         InputStream is = activity.getResources().getAssets().open(
               Path.simplifyPath(filename));
         decodedBitmap = BitmapFactory.decodeStream(is, null, options);
         is.close();
         Log.d(TAG, "done waiting for decodeStream");
      } catch (Exception ex) {
         Log.e(TAG,
            "decodeBitmapAsset exception: " + ex.getMessage());
         decodedBitmap = null;
      }
      Log.d(TAG, "decodeBitmapAsset end");
      return decodedBitmap;
   }

   static Bitmap decodeBitmapStream(final InputStream is)
   {
      Bitmap decodedBitmap;
      Log.d(TAG, "decodeBitmapStream begin");
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         // Only added in API level 19, avoid for now.
         // options.inPremultiplied = premul;
         decodedBitmap = BitmapFactory.decodeStream(is, null, options);
         Log.d(TAG, "done waiting for decodeStream");
      } catch (Exception ex) {
         Log.e(TAG, "decodeBitmapStream exception: " +
               ex.getMessage());
         decodedBitmap = null;
      }
      Log.d(TAG, "decodeBitmapStream end");
      return decodedBitmap;
   }

   /* Unused yet */
   /*
   static Bitmap decodeBitmapByteArray(byte[] array)
   {
      Bitmap decodedBitmap;
      Log.d(TAG, "decodeBitmapByteArray");
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         Bitmap bmp = BitmapFactory.decodeByteArray(array, 0, array.length, options);
         return bmp;
      }
      catch (Exception ex) {
         Log.e(TAG, "decodeBitmapByteArray exception: " +
               ex.getMessage());
      }
      return null;
   }
   */

   static int getBitmapFormat(Bitmap bitmap)
   {
      switch (bitmap.getConfig()) {
         case ALPHA_8:
            return Const.A5O_PIXEL_FORMAT_SINGLE_CHANNEL_8; // not really
         case ARGB_4444:
            return Const.A5O_PIXEL_FORMAT_RGBA_4444;
         case ARGB_8888:
            return Const.A5O_PIXEL_FORMAT_ABGR_8888;
         case RGB_565:
            return Const.A5O_PIXEL_FORMAT_BGR_565; // untested
         default:
            assert(false);
            return -1;
      }
   }

   static int[] getPixels(Bitmap bmp)
   {
      int width = bmp.getWidth();
      int height = bmp.getHeight();
      int[] pixels = new int[width * height];
      bmp.getPixels(pixels, 0, width, 0, 0, width, height);
      return pixels;
   }
}

/* vim: set sts=3 sw=3 et: */
