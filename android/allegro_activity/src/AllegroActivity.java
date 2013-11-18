package org.liballeg.android;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.PowerManager;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.lang.Runnable;
import java.lang.String;
import java.util.List;

public class AllegroActivity extends Activity implements SensorEventListener
{
   /* properties */
   private static SensorManager sensorManager;
   private List<Sensor> sensors;

   private static AllegroSurface surface;
   private Handler handler;

   private Configuration currentConfig;

   /* native methods we call */
   public native boolean nativeOnCreate();
   public native void nativeOnPause();
   public native void nativeOnResume();
   public native void nativeOnDestroy();
   public native void nativeOnAccel(int id, float x, float y, float z);

   public native void nativeOnOrientationChange(int orientation, boolean init);

   private boolean exitedMain = false;

   /* methods native code calls */

   private boolean inhibit_screen_lock = false;
   private PowerManager.WakeLock wake_lock;

   public boolean inhibitScreenLock(boolean inhibit)
   {
      boolean last_state = inhibit_screen_lock;
      inhibit_screen_lock = inhibit;

      try {
         if (inhibit) {
            if (last_state) {
               // Already there
            }
            else {
               // Disable lock
               PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
               wake_lock = pm.newWakeLock(PowerManager.SCREEN_BRIGHT_WAKE_LOCK, "Allegro Wake Lock");
               wake_lock.acquire();
            }
         }
         else {
            if (last_state) {
               // Turn lock back on
               wake_lock.release();
               wake_lock = null;
            }
            else {
               // Already there
            }
         }
      }
      catch (Exception e) {
         Log.d("AllegroActivity", "Got exception in inhibitScreenLock: " + e.getMessage());
      }

      return true;
   }

   public String getLibraryDir()
   {
      /* Android 1.6 doesn't have .nativeLibraryDir :( */
      /* FIXME: use reflection here to detect the capabilities of the device */
      return getApplicationInfo().dataDir + "/lib";
   }

   public String getAppName()
   {
      try {
         return getPackageManager()
            .getActivityInfo(getComponentName(),
               android.content.pm.PackageManager.GET_META_DATA)
            .metaData.getString("org.liballeg.app_name");
      } catch (PackageManager.NameNotFoundException ex) {
         return new String();
      }
   }

   public String getResourcesDir()
   {
      //return getApplicationInfo().dataDir + "/assets";
      //return getApplicationInfo().sourceDir + "/assets/";
      return getFilesDir().getAbsolutePath();
   }

   public String getPubDataDir()
   {
      return getFilesDir().getAbsolutePath();
   }

   public String getApkPath()
   {
      return getApplicationInfo().sourceDir;
   }

   public String getModel()
   {
      return android.os.Build.MODEL;
   }

   public void postRunnable(Runnable runme)
   {
      try {
         Log.d("AllegroActivity", "postRunnable");
         handler.post( runme );
      } catch (Exception x) {
         Log.d("AllegroActivity", "postRunnable exception: " + x.getMessage());
      }
   }

   public void createSurface()
   {
      try {
         Log.d("AllegroActivity", "createSurface");
         surface = new AllegroSurface(getApplicationContext(),
               getWindowManager().getDefaultDisplay());

         SurfaceHolder holder = surface.getHolder();
         holder.addCallback(surface); 
         holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
         //setContentView(surface);
         Window win = getWindow();
         win.setContentView(surface);
         Log.d("AllegroActivity", "createSurface end");  
      } catch (Exception x) {
         Log.d("AllegroActivity", "createSurface exception: " + x.getMessage());
      }
   }

   public void postCreateSurface()
   {
      try {
         Log.d("AllegroActivity", "postCreateSurface");

         handler.post(new Runnable() {
            public void run() {
               createSurface();
            }
         });
      } catch (Exception x) {
         Log.d("AllegroActivity", "postCreateSurface exception: " + x.getMessage());
      }
   }

   public void destroySurface()
   {
      Log.d("AllegroActivity", "destroySurface");

      ViewGroup vg = (ViewGroup)(surface.getParent());
      vg.removeView(surface);
      surface = null;
   }

   public void postDestroySurface()
   {
      try {
         Log.d("AllegroActivity", "postDestroySurface");

         handler.post(new Runnable() {
            public void run() {
               destroySurface();
            }
         });
      } catch (Exception x) {
         Log.d("AllegroActivity", "postDestroySurface exception: " + x.getMessage());
      }
   }

   public int getNumSensors()
   {
      return sensors.size();
   }

   /*
    * load passes in the buffer, will return the decoded bytebuffer
    * then I want to change lock/unlock_bitmap to accept a raw pointer
    * rather than allocating another buffer and making you copy shit again
    * images are loaded as ARGB_8888 by android by default
    */
   private Bitmap decodedBitmap;

   public Bitmap decodeBitmapByteArray(byte[] array)
   {
      Log.d("AllegroActivity", "decodeBitmapByteArray");
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         Bitmap bmp = BitmapFactory.decodeByteArray(array, 0, array.length, options);
         return bmp;
      }
      catch (Exception ex) {
         Log.e("AllegroActivity", "decodeBitmapByteArray exception: " + ex.getMessage());
      }

      return null;
   }

   public int[] getPixels(Bitmap bmp)
   {
      int width = bmp.getWidth();
      int height = bmp.getHeight();
      int[] pixels = new int[width*height];
      bmp.getPixels(pixels, 0, width, 0, 0, width, height);
      return pixels;
   }

   public Bitmap decodeBitmap(final String filename)
   {
      Log.d("AllegroActivity", "decodeBitmap begin");
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         InputStream is = getResources().getAssets().open(filename);;
         decodedBitmap = BitmapFactory.decodeStream(is, null, options);
         is.close();
         Log.d("AllegroActivity", "done waiting for decodeStream");
      } catch (Exception ex) {
         Log.e("AllegroActivity", "decodeBitmap exception: " + ex.getMessage());
      }
      Log.d("AllegroActivity", "decodeBitmap end");
      return decodedBitmap;
   }

   public Bitmap decodeBitmap_f(final AllegroInputStream is)
   {
      Log.d("AllegroActivity", "decodeBitmap_f begin");
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         decodedBitmap = BitmapFactory.decodeStream(is, null, options);
         Log.d("AllegroActivity", "done waiting for decodeStream");

      } catch (Exception ex) {
         Log.e("AllegroActivity", "decodeBitmap_f exception: " + ex.getMessage());
      }
      Log.d("AllegroActivity", "decodeBitmap_f end");
      return decodedBitmap;
   }

   public void postFinish()
   {
      exitedMain = true;

      try {
         Log.d("AllegroActivity", "posting finish!");
         handler.post(new Runnable() {
            public void run() {
               try {
                  AllegroActivity.this.finish();
                  System.exit(0);
               } catch (Exception x) {
                  Log.d("AllegroActivity", "inner exception: " + x.getMessage());
               }
            }
         });
      } catch (Exception x) {
         Log.d("AllegroActivity", "exception: " + x.getMessage());
      }
   }

   public boolean getMainReturned()
   {
      return exitedMain;
   }

   /* end of functions native code calls */

   /** Called when the activity is first created. */
   @Override
   public void onCreate(Bundle savedInstanceState)
   {
      super.onCreate(savedInstanceState);

      Log.d("AllegroActivity", "onCreate");

      Log.d("AllegroActivity", "Files Dir: " + getFilesDir());
      File extdir = Environment.getExternalStorageDirectory();

      boolean mExternalStorageAvailable = false;
      boolean mExternalStorageWriteable = false;
      String state = Environment.getExternalStorageState();

      if (Environment.MEDIA_MOUNTED.equals(state)) {
         // We can read and write the media
         mExternalStorageAvailable = mExternalStorageWriteable = true;
      } else if (Environment.MEDIA_MOUNTED_READ_ONLY.equals(state)) {
         // We can only read the media
         mExternalStorageAvailable = true;
         mExternalStorageWriteable = false;
      } else {
         // Something else is wrong. It may be one of many other states, but
         // all we need to know is we can neither read nor write
         mExternalStorageAvailable = mExternalStorageWriteable = false;
      }

      Log.d("AllegroActivity", "External Storage Dir: " + extdir.getAbsolutePath());
      Log.d("AllegroActivity", "External Files Dir: " + getExternalFilesDir(null));

      Log.d("AllegroActivity", "external: avail = " + mExternalStorageAvailable +
            " writable = " + mExternalStorageWriteable);

      Log.d("AllegroActivity", "sourceDir: " + getApplicationInfo().sourceDir);
      Log.d("AllegroActivity", "publicSourceDir: " + getApplicationInfo().publicSourceDir);

      handler = new Handler();
      initSensors();

      currentConfig = new Configuration(getResources().getConfiguration());

      Log.d("AllegroActivity", "before nativeOnCreate");
      if (!nativeOnCreate()) {
         finish();
         Log.d("AllegroActivity", "onCreate fail");
         return;
      }

      nativeOnOrientationChange(0, true);

      requestWindowFeature(Window.FEATURE_NO_TITLE);
      this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
            WindowManager.LayoutParams.FLAG_FULLSCREEN);

      Log.d("AllegroActivity", "onCreate end");
   }

   @Override
   public void onStart()
   {
      super.onStart();
      Log.d("AllegroActivity", "onStart.");
   }

   @Override
   public void onRestart()
   {
      super.onRestart();
      Log.d("AllegroActivity", "onRestart.");
   }

   @Override
   public void onStop()
   {
      super.onStop();
      Log.d("AllegroActivity", "onStop.");
   }

   /** Called when the activity is paused. */
   @Override
   public void onPause()
   {
      super.onPause();
      Log.d("AllegroActivity", "onPause");

      disableSensors();

      nativeOnPause();
      Log.d("AllegroActivity", "onPause end");
   }

   /** Called when the activity is resumed/unpaused */
   @Override
   public void onResume()
   {
      Log.d("AllegroActivity", "onResume");
      super.onResume();

      enableSensors();

      nativeOnResume();

      Log.d("AllegroActivity", "onResume end");
   }

   /** Called when the activity is destroyed */
   @Override
   public void onDestroy()
   {
      super.onDestroy();
      Log.d("AllegroActivity", "onDestroy");

      nativeOnDestroy();
      Log.d("AllegroActivity", "onDestroy end");
   }

   /** Called when config has changed */
   @Override
   public void onConfigurationChanged(Configuration conf)
   {
      super.onConfigurationChanged(conf);
      Log.d("AllegroActivity", "onConfigurationChanged");
      // compare conf.orientation with some saved value

      int changes = currentConfig.diff(conf);
      Log.d("AllegroActivity", "changes: " + Integer.toBinaryString(changes));

      if ((changes & ActivityInfo.CONFIG_FONT_SCALE) != 0)
         Log.d("AllegroActivity", "font scale changed");

      if ((changes & ActivityInfo.CONFIG_MCC) != 0)
         Log.d("AllegroActivity", "mcc changed");

      if ((changes & ActivityInfo.CONFIG_MNC) != 0)
         Log.d("AllegroActivity", " changed");

      if ((changes & ActivityInfo.CONFIG_LOCALE) != 0)
         Log.d("AllegroActivity", "locale changed");

      if ((changes & ActivityInfo.CONFIG_TOUCHSCREEN) != 0)
         Log.d("AllegroActivity", "touchscreen changed");

      if ((changes & ActivityInfo.CONFIG_KEYBOARD) != 0)
         Log.d("AllegroActivity", "keyboard changed");

      if ((changes & ActivityInfo.CONFIG_NAVIGATION) != 0)
         Log.d("AllegroActivity", "navigation changed");

      if ((changes & ActivityInfo.CONFIG_ORIENTATION) != 0) {
         Log.d("AllegroActivity", "orientation changed");
         nativeOnOrientationChange(getAllegroOrientation(), false);
      }

      if ((changes & ActivityInfo.CONFIG_SCREEN_LAYOUT) != 0)
         Log.d("AllegroActivity", "screen layout changed");

      if ((changes & ActivityInfo.CONFIG_UI_MODE) != 0)
         Log.d("AllegroActivity", "ui mode changed");

      if (currentConfig.screenLayout != conf.screenLayout) {
         Log.d("AllegroActivity", "screenLayout changed!");
      }

      Log.d("AllegroActivity",
            "old orientation: " + currentConfig.orientation
            + ", new orientation: " + conf.orientation);

      currentConfig = new Configuration(conf);
   }

   /** Called when app is frozen **/
   @Override
   public void onSaveInstanceState(Bundle state)
   {
      Log.d("AllegroActivity", "onSaveInstanceState");
      /* do nothing? */
      /* This should get rid of the following warning:
       *  couldn't save which view has focus because the focused view has no id.
       */
   }

   /* sensors */

   private boolean initSensors()
   {
      sensorManager = (SensorManager)getApplicationContext().getSystemService("sensor");

      /* Only check for Accelerometers for now, not sure how we should utilize
       * other types.
       */
      sensors = sensorManager.getSensorList(Sensor.TYPE_ACCELEROMETER);
      return (sensors != null);
   }

   private void enableSensors()
   {
      for(int i = 0; i < sensors.size(); i++) {
         sensorManager.registerListener(this, sensors.get(i), SensorManager.SENSOR_DELAY_GAME);
      }
   }

   private void disableSensors()
   {
      for(int i = 0; i < sensors.size(); i++) {
         sensorManager.unregisterListener(this, sensors.get(i));
      }
   }

   public void onAccuracyChanged(Sensor sensor, int accuracy) { /* what to do? */ }

   public void onSensorChanged(SensorEvent event)
   {
      int idx = sensors.indexOf(event.sensor);

      if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
         nativeOnAccel(idx, event.values[0], event.values[1], event.values[2]);
      }
   }

   public int getAndroidOrientation(int alleg_orientation)
   {
      int android_orientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;

      switch (alleg_orientation)
      {
         case Const.ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
            break;

         case Const.ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
            break;

         case Const.ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
            break;

         case Const.ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
            break;

         case Const.ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
            break;

         case Const.ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
            break;

         case Const.ALLEGRO_DISPLAY_ORIENTATION_ALL:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR;
            break;
      }

      return android_orientation;
   }

   private int getAllegroOrientation()
   {
      int allegro_orientation = Const.ALLEGRO_DISPLAY_ORIENTATION_UNKNOWN;
      int rotation;

      if (Utils.methodExists(getWindowManager().getDefaultDisplay(), "getRotation")) {
         /* 2.2+ */
         rotation = getWindowManager().getDefaultDisplay().getRotation();
      }
      else {
         rotation = getWindowManager().getDefaultDisplay().getOrientation();
      }

      switch (rotation) {
         case Surface.ROTATION_0:
            allegro_orientation = Const.ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES;
            break;

         case Surface.ROTATION_180:
            allegro_orientation = Const.ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES;
            break;

         /* Android device orientations are the opposite of Allegro ones.
          * Allegro orientations are the orientation of the device, with 0
          * being holding the device at normal orientation, 90 with the device
          * rotated 90 degrees clockwise and so on. Android orientations are
          * the orientations of the GRAPHICS. By rotating the device by 90
          * degrees clockwise, the graphics are actually rotated 270 degrees,
          * and that's what Android uses.
          */

         case Surface.ROTATION_90:
            allegro_orientation = Const.ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES;
            break;

         case Surface.ROTATION_270:
            allegro_orientation = Const.ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES;
            break;
      }

      return allegro_orientation;
   }

   public String getOsVersion()
   {
      return android.os.Build.VERSION.RELEASE;
   }
}

/* vim: set sts=3 sw=3 et: */
