package org.liballeg.android;

import android.app.Activity;
import android.content.Context;
import android.os.PowerManager;
import android.util.Log;

class ScreenLock
{
   private static final String TAG = "ScreenLock";

   private Activity activity;
   private PowerManager.WakeLock wake_lock = null;

   ScreenLock(Activity activity)
   {
      this.activity = activity;
   }

   boolean inhibitScreenLock(boolean inhibit)
   {
      try {
         if (inhibit && wake_lock == null) {
            acquire();
         }
         else if (!inhibit && wake_lock != null) {
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
      PowerManager pm = (PowerManager)
         activity.getSystemService(Context.POWER_SERVICE);
      wake_lock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK,
         "Allegro Wake Lock");
      wake_lock.acquire();
   }

   private void release()
   {
      wake_lock.release();
      wake_lock = null;
   }
}

/* vim: set sts=3 sw=3 et: */
