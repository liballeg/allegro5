package org.liballeg.android;

import android.app.Activity;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

class ScreenLock
{
   private static final String TAG = "ScreenLock";

   private Activity activity;

   ScreenLock(Activity activity)
   {
      this.activity = activity;
   }

   boolean inhibitScreenLock(boolean inhibit)
   {
      try {
         if (inhibit) {
            acquire();
         }
         else if (!inhibit) {
            release();
         }
         return true;
      }
      catch (Exception e) {
         Log.d(TAG, "Got exception in inhibitScreenLock: " + e.getMessage());
         return false;
      }
   }

   private void acquire()
   {
      activity.runOnUiThread(new Runnable() {
         @Override
         public void run()
         {
            activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
         }
      });
   }

   private void release()
   {
      activity.runOnUiThread(new Runnable() {
         @Override
         public void run()
         {
            activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
         }
      });
   }
}

/* vim: set sts=3 sw=3 et: */
