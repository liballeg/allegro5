package org.liballeg.app;

import android.app.Activity;
import android.os.Bundle;
import android.os.Handler;
import android.os.Environment;
import android.os.PowerManager;

import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.OrientationEventListener;
import android.view.WindowManager;
import android.view.Display;

import android.hardware.*;
import android.content.res.Configuration;
import android.content.res.AssetManager;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.PixelFormat;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ActivityInfo;
import android.util.Log;

import java.lang.String;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.lang.Runnable;

import java.util.List;
import java.util.BitSet;
import java.util.ArrayList;
import java.util.HashMap;

import java.io.File;
import java.io.InputStream;
import java.io.FileOutputStream;

import java.nio.ByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.*;

import org.liballeg.app.AllegroInputStream;
import android.media.AudioManager;

class Utils
{
   public static boolean methodExists(Object obj, String methName)
   {
      try {
         Class cls = obj.getClass();
         Method m = cls.getMethod(methName);
         return true;
      } catch(Exception x) {
         return false;
      }
   }
   
}

public class AllegroActivity extends Activity implements SensorEventListener
{
   /* properties */

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

   public native void nativeCreateDisplay();
   
   public native void nativeOnOrientationChange(int orientation, boolean init);

   public static AllegroActivity Self;

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
         return getPackageManager().getActivityInfo(getComponentName(), android.content.pm.PackageManager.GET_META_DATA).metaData.getString("org.liballeg.app_name");
      } catch(PackageManager.NameNotFoundException ex) {
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
         surface = new AllegroSurface(getApplicationContext(), getWindowManager().getDefaultDisplay());
         
         SurfaceHolder holder = surface.getHolder();
         holder.addCallback(surface); 
         holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
         //setContentView(surface);
         Window win = getWindow();
         win.setContentView(surface);
         Log.d("AllegroActivity", "createSurface end");  
      } catch(Exception x) {
         Log.d("AllegroActivity", "createSurface exception: " + x.getMessage());
      }
   }
   
   public void postCreateSurface()
   {
      try {
         Log.d("AllegroActivity", "postCreateSurface");
         
         handler.post( new Runnable() {
            public void run() {
               createSurface();
            }
         } );
      } catch(Exception x) {
         Log.d("AllegroActivity", "postCreateSurface exception: " + x.getMessage());
      }
      
      return;
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
         
         handler.post( new Runnable() {
            public void run() {
               destroySurface();
            }
         } );
      } catch(Exception x) {
         Log.d("AllegroActivity", "postDestroySurface exception: " + x.getMessage());
      }
      
      return;
   }
   
   public int getNumSensors() { return sensors.size(); }
   
   /*
	 * load passes in the buffer, will return the decoded bytebuffer
	 * then I want to change lock/unlock_bitmap to accept a raw pointer
	 * rather than allocating another buffer and making you copy shit again
	 * images are loaded as ARGB_8888 by android by default
	 */
   private Bitmap decodedBitmap;
   private boolean bitmapLoaded;
   
   public Bitmap decodeBitmapByteArray(byte[] array)
   {
      Log.d("AllegroActivity", "decodeBitmapByteArray");
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         Bitmap bmp = BitmapFactory.decodeByteArray(array, 0, array.length, options);
         //Bitmap.Config conf = bmp.getConfig();
         //switch(conf.
         return bmp;
      }
      catch(Exception ex)
      {
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
      bitmapLoaded = false;
      try {
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         InputStream is = getResources().getAssets().open(filename);;
         decodedBitmap = BitmapFactory.decodeStream(is, null, options);
         is.close();
         Log.d("AllegroActivity", "done waiting for decodeStream");
      } catch(Exception ex) {
         Log.e("AllegroActivity", "decodeBitmap exception: " + ex.getMessage());
      }
      Log.d("AllegroActivity", "decodeBitmap end");
      return decodedBitmap;
   }

   public Bitmap decodeBitmap_f(final AllegroInputStream is)
   {
      Log.d("AllegroActivity", "decodeBitmap_f begin");
      bitmapLoaded = false;
      try {
         //handler.post( new Runnable() {
         //   public void run() {
         //      Log.d("AllegroActivity", "calling decodeStream");
         //      decodedBitmap = BitmapFactory.decodeStream(is);
         //     bitmapLoaded = true;
         //      Log.d("AllegroActivity", "done decodeStream");
         //   }
         //} );
         
         //Log.d("AllegroActivity", "waiting for decodeStream");
         //while(bitmapLoaded == false) { Thread.sleep(20); }
         BitmapFactory.Options options = new BitmapFactory.Options();
         options.inPreferredConfig = Bitmap.Config.ARGB_8888;
         decodedBitmap = BitmapFactory.decodeStream(is, null, options);
         Log.d("AllegroActivity", "done waiting for decodeStream");
         
      } catch(Exception ex) {
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
         handler.post( new Runnable() {
            public void run() {
               try {
                  AllegroActivity.this.finish();
		  System.exit(0);
               } catch(Exception x) {
                  Log.d("AllegroActivity", "inner exception: " + x.getMessage());
               }
            }
         } );
      } catch(Exception x) {
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
      
      Self = this;
      
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
         // Something else is wrong. It may be one of many other states, but all we need
         //  to know is we can neither read nor write
         mExternalStorageAvailable = mExternalStorageWriteable = false;
      }
   
      Log.d("AllegroActivity", "External Storage Dir: " + extdir.getAbsolutePath());
      Log.d("AllegroActivity", "External Files Dir: " + getExternalFilesDir(null));
      
      Log.d("AllegroActivity", "external: avail = " + mExternalStorageAvailable + " writable = " + mExternalStorageWriteable);
      
      Log.d("AllegroActivity", "sourceDir: " + getApplicationInfo().sourceDir);
      Log.d("AllegroActivity", "publicSourceDir: " + getApplicationInfo().publicSourceDir);
      
      //unpackAssets("");
      unpackAssets("unpack");
      
      handler = new Handler();
      initSensors();

      currentConfig = new Configuration(getResources().getConfiguration());

      Log.d("AllegroActivity", "before nativeOnCreate");
      if(!nativeOnCreate()) {
         finish();
         Log.d("AllegroActivity", "onCreate fail");
         return;
      }

      nativeOnOrientationChange(0, true);
  
      requestWindowFeature(Window.FEATURE_NO_TITLE);
      this.getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,WindowManager.LayoutParams.FLAG_FULLSCREEN);

      Log.d("AllegroActivity", "onCreate end");
   }
   
   @Override
   public void onStart()
   {
      super.onStart();
      Log.d("AllegroActivity", "onStart.");
   } //
   
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
      
      if((changes & ActivityInfo.CONFIG_FONT_SCALE) != 0)
         Log.d("AllegroActivity", "font scale changed");

      if((changes & ActivityInfo.CONFIG_MCC) != 0)
         Log.d("AllegroActivity", "mcc changed");
      
      if((changes & ActivityInfo.CONFIG_MNC) != 0)
         Log.d("AllegroActivity", " changed");
      
      if((changes & ActivityInfo.CONFIG_LOCALE) != 0)
         Log.d("AllegroActivity", "locale changed");
      
      if((changes & ActivityInfo.CONFIG_TOUCHSCREEN) != 0)
         Log.d("AllegroActivity", "touchscreen changed");
      
      if((changes & ActivityInfo.CONFIG_KEYBOARD) != 0)
         Log.d("AllegroActivity", "keyboard changed");
      
      if((changes & ActivityInfo.CONFIG_NAVIGATION) != 0)
         Log.d("AllegroActivity", "navigation changed");
        
      if((changes & ActivityInfo.CONFIG_ORIENTATION) != 0) {
         Log.d("AllegroActivity", "orientation changed");
         nativeOnOrientationChange(getAllegroOrientation(), false);
      }
         
      if((changes & ActivityInfo.CONFIG_SCREEN_LAYOUT) != 0)
         Log.d("AllegroActivity", "screen layout changed");

      if((changes & ActivityInfo.CONFIG_UI_MODE) != 0)
         Log.d("AllegroActivity", "ui mode changed");
      
      if(currentConfig.screenLayout != conf.screenLayout) {
         Log.d("AllegroActivity", "screenLayout changed!");
      }
      
      Log.d("AllegroActivity", "old orientation: " + currentConfig.orientation + ", new orientation: " + conf.orientation);
      
      currentConfig = new Configuration(conf);
   }
   
   /** Called when app is frozen **/
   @Override
   public void onSaveInstanceState(Bundle state)
   {
      Log.d("AllegroActivity", "onSaveInstanceState");
      /* do nothing? */
      /* this should get rid of the following warning:
       *  couldn't save which view has focus because the focused view has no id. */
   }
   
   /* sensors */
   
   private boolean initSensors()
   {
      sensorManager = (SensorManager)getApplicationContext().getSystemService("sensor");
      
      /* only check for Acclerometers for now, not sure how we should utilize other types */
      sensors = sensorManager.getSensorList(Sensor.TYPE_ACCELEROMETER);
      if(sensors == null)
         return false;
      
      return true;
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
      
      if(event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
         nativeOnAccel(idx, event.values[0], event.values[1], event.values[2]);
      }
   }

   public int getAndroidOrientation(int alleg_orientation)
   {
      int android_orientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED;

      switch (alleg_orientation)
      {
         case ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
            break;

         case ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
            break;

         case ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
            break;

         case ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
            break;

         case ALLEGRO_DISPLAY_ORIENTATION_PORTRAIT:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
            break;

         case ALLEGRO_DISPLAY_ORIENTATION_LANDSCAPE:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
            break;

         case ALLEGRO_DISPLAY_ORIENTATION_ALL:
            android_orientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR;
            break;
      }

      return android_orientation;
   }
   
   private int getAllegroOrientation()
   {
      int allegro_orientation = ALLEGRO_DISPLAY_ORIENTATION_UNKNOWN;
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
            allegro_orientation = ALLEGRO_DISPLAY_ORIENTATION_0_DEGREES;
            break;

         case Surface.ROTATION_180:
            allegro_orientation = ALLEGRO_DISPLAY_ORIENTATION_180_DEGREES;
            break;

         /* Big fat notice here: Android device orientations are the opposite of Allegro ones.
          * Allegro orientations are the orientation of the device, with 0 being holding the
          * device at normal orientation, 90 with the device rotated 90 degrees clockwise and
          * so on. Android orientations are the orientations of the GRAPHICS. By rotating the
          * device by 90 degrees clockwise, the graphics are actually rotated 270 degrees, and
          * that's what Android uses.
          */

         case Surface.ROTATION_90:
            allegro_orientation = ALLEGRO_DISPLAY_ORIENTATION_270_DEGREES;
            break;

         case Surface.ROTATION_270:
            allegro_orientation = ALLEGRO_DISPLAY_ORIENTATION_90_DEGREES;
            break;
      }

      return allegro_orientation;
   }
   
   // FIXME: this is a horrible hack, needs to be replaced with something smarter
   // FIXME:  or a fshook driver to access assets.
   private void unpackAssets(String dir)
   {
      // create directories
      File f = new File(getResourcesDir()+"/"+dir);
      f.mkdir();

      try {
         AssetManager am = getResources().getAssets();
         String list[] = am.list(dir);
         for(int i = 0; i < list.length; i++) {
            String full = dir + (dir.equals("") ? "" : "/") + list[i];
            InputStream is = null;
            try {
               is = am.open(full);
            }
            catch (Exception e) {
               // Directory
               Log.d("AllegroActivity", "asset["+i+"] (directory): " + full);
               unpackAssets(full);
               continue;
            }
            Log.d("AllegroActivity", "asset["+i+"] (file): " + full);
            FileOutputStream os = new FileOutputStream(getResourcesDir()+"/"+full);
            
            byte buff[] = new byte[4096];
            while(true) {
               int read_ret = is.read(buff);
               if(read_ret > 0) {
                  os.write(buff, 0, read_ret);
               }
               else {
                  break;
               }
            }

            is.close();
            os.close();
         }
      } catch(java.io.IOException ex) {
         Log.e("AllegroActivity", "asset list exception: "+ex.getMessage());
      }
   }

   public String getOsVersion()
   {
      return android.os.Build.VERSION.RELEASE;
   }
}

class AllegroSurface extends SurfaceView implements SurfaceHolder.Callback, 
   View.OnKeyListener, View.OnTouchListener
{
   
   static final int ALLEGRO_PIXEL_FORMAT_RGBA_8888 = 10;
   static final int ALLEGRO_PIXEL_FORMAT_RGB_888 = 12;
   static final int ALLEGRO_PIXEL_FORMAT_RGB_565 = 13;
   static final int ALLEGRO_PIXEL_FORMAT_RGBA_5551 = 15;
   static final int ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE = 25;
   static final int ALLEGRO_PIXEL_FORMAT_RGBA_4444 = 26;

   static final int ALLEGRO_RED_SIZE = 0;
   static final int ALLEGRO_GREEN_SIZE = 1;
   static final int ALLEGRO_BLUE_SIZE = 2;
   static final int ALLEGRO_ALPHA_SIZE = 3;
   static final int ALLEGRO_DEPTH_SIZE = 15;
   static final int ALLEGRO_STENCIL_SIZE = 16;
   static final int ALLEGRO_SAMPLE_BUFFERS = 17;
   static final int ALLEGRO_SAMPLES = 18;

   static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098; 
   
   static final int ALLEGRO_KEY_A     = 1;
   static final int ALLEGRO_KEY_B     = 2;
   static final int ALLEGRO_KEY_C     = 3;
   static final int ALLEGRO_KEY_D     = 4;
   static final int ALLEGRO_KEY_E     = 5;
   static final int ALLEGRO_KEY_F     = 6;
   static final int ALLEGRO_KEY_G     = 7;
   static final int ALLEGRO_KEY_H     = 8;
   static final int ALLEGRO_KEY_I     = 9;
   static final int ALLEGRO_KEY_J     = 10;
   static final int ALLEGRO_KEY_K     = 11;
   static final int ALLEGRO_KEY_L     = 12;
   static final int ALLEGRO_KEY_M     = 13;
   static final int ALLEGRO_KEY_N     = 14;
   static final int ALLEGRO_KEY_O     = 15;
   static final int ALLEGRO_KEY_P     = 16;
   static final int ALLEGRO_KEY_Q     = 17;
   static final int ALLEGRO_KEY_R     = 18;
   static final int ALLEGRO_KEY_S     = 19;
   static final int ALLEGRO_KEY_T     = 20;
   static final int ALLEGRO_KEY_U     = 21;
   static final int ALLEGRO_KEY_V     = 22;
   static final int ALLEGRO_KEY_W     = 23;
   static final int ALLEGRO_KEY_X     = 24;
   static final int ALLEGRO_KEY_Y     = 25;
   static final int ALLEGRO_KEY_Z     = 26;
   
   static final int ALLEGRO_KEY_0     = 27;
   static final int ALLEGRO_KEY_1     = 28;
   static final int ALLEGRO_KEY_2     = 29;
   static final int ALLEGRO_KEY_3     = 30;
   static final int ALLEGRO_KEY_4     = 31;
   static final int ALLEGRO_KEY_5     = 32;
   static final int ALLEGRO_KEY_6     = 33;
   static final int ALLEGRO_KEY_7     = 34;
   static final int ALLEGRO_KEY_8     = 35;
   static final int ALLEGRO_KEY_9     = 36;

   static final int ALLEGRO_KEY_PAD_0    = 37;
   static final int ALLEGRO_KEY_PAD_1    = 38;
   static final int ALLEGRO_KEY_PAD_2    = 39;
   static final int ALLEGRO_KEY_PAD_3    = 40;
   static final int ALLEGRO_KEY_PAD_4    = 41;
   static final int ALLEGRO_KEY_PAD_5    = 42;
   static final int ALLEGRO_KEY_PAD_6    = 43;
   static final int ALLEGRO_KEY_PAD_7    = 44;
   static final int ALLEGRO_KEY_PAD_8    = 45;
   static final int ALLEGRO_KEY_PAD_9    = 46;

   static final int ALLEGRO_KEY_F1    = 47;
   static final int ALLEGRO_KEY_F2    = 48;
   static final int ALLEGRO_KEY_F3    = 49;
   static final int ALLEGRO_KEY_F4    = 50;
   static final int ALLEGRO_KEY_F5    = 51;
   static final int ALLEGRO_KEY_F6    = 52;
   static final int ALLEGRO_KEY_F7    = 53;
   static final int ALLEGRO_KEY_F8    = 54;
   static final int ALLEGRO_KEY_F9    = 55;
   static final int ALLEGRO_KEY_F10   = 56;
   static final int ALLEGRO_KEY_F11   = 57;
   static final int ALLEGRO_KEY_F12   = 58;

   static final int ALLEGRO_KEY_ESCAPE     = 59;
   static final int ALLEGRO_KEY_TILDE      = 60;
   static final int ALLEGRO_KEY_MINUS      = 61;
   static final int ALLEGRO_KEY_EQUALS     = 62;
   static final int ALLEGRO_KEY_BACKSPACE  = 63;
   static final int ALLEGRO_KEY_TAB        = 64;
   static final int ALLEGRO_KEY_OPENBRACE  = 65;
   static final int ALLEGRO_KEY_CLOSEBRACE = 66;
   static final int ALLEGRO_KEY_ENTER      = 67;
   static final int ALLEGRO_KEY_SEMICOLON  = 68;
   static final int ALLEGRO_KEY_QUOTE      = 69;
   static final int ALLEGRO_KEY_BACKSLASH  = 70;
   static final int ALLEGRO_KEY_BACKSLASH2 = 71; /* DirectInput calls this DIK_OEM_102: "< > | on UK/Germany keyboards" */
   static final int ALLEGRO_KEY_COMMA      = 72;
   static final int ALLEGRO_KEY_FULLSTOP   = 73;
   static final int ALLEGRO_KEY_SLASH      = 74;
   static final int ALLEGRO_KEY_SPACE      = 75;

   static final int ALLEGRO_KEY_INSERT   = 76;
   static final int ALLEGRO_KEY_DELETE   = 77;
   static final int ALLEGRO_KEY_HOME     = 78;
   static final int ALLEGRO_KEY_END      = 79;
   static final int ALLEGRO_KEY_PGUP     = 80;
   static final int ALLEGRO_KEY_PGDN     = 81;
   static final int ALLEGRO_KEY_LEFT     = 82;
   static final int ALLEGRO_KEY_RIGHT    = 83;
   static final int ALLEGRO_KEY_UP       = 84;
   static final int ALLEGRO_KEY_DOWN     = 85;

   static final int ALLEGRO_KEY_PAD_SLASH    = 86;
   static final int ALLEGRO_KEY_PAD_ASTERISK = 87;
   static final int ALLEGRO_KEY_PAD_MINUS    = 88;
   static final int ALLEGRO_KEY_PAD_PLUS     = 89;
   static final int ALLEGRO_KEY_PAD_DELETE   = 90;
   static final int ALLEGRO_KEY_PAD_ENTER    = 91;

   static final int ALLEGRO_KEY_PRINTSCREEN = 92;
   static final int ALLEGRO_KEY_PAUSE       = 93;

   static final int ALLEGRO_KEY_ABNT_C1    = 94;
   static final int ALLEGRO_KEY_YEN        = 95;
   static final int ALLEGRO_KEY_KANA       = 96;
   static final int ALLEGRO_KEY_CONVERT    = 97;
   static final int ALLEGRO_KEY_NOCONVERT  = 98;
   static final int ALLEGRO_KEY_AT         = 99;
   static final int ALLEGRO_KEY_CIRCUMFLEX = 100;
   static final int ALLEGRO_KEY_COLON2     = 101;
   static final int ALLEGRO_KEY_KANJI      = 102;

   static final int ALLEGRO_KEY_PAD_EQUALS  = 103;   /* MacOS X */
   static final int ALLEGRO_KEY_BACKQUOTE   = 104;   /* MacOS X */
   static final int ALLEGRO_KEY_SEMICOLON2  = 105;   /* MacOS X -- TODO: ask lillo what this should be */
   static final int ALLEGRO_KEY_COMMAND     = 106;   /* MacOS X */
   static final int ALLEGRO_KEY_BACK        = 107;
   static final int ALLEGRO_KEY_VOLUME_UP   = 108;
   static final int ALLEGRO_KEY_VOLUME_DOWN = 109;
   
   /* Some more standard Android keys.
    * These happen to be the ones used by the Xperia Play.
    */
   static final int ALLEGRO_KEY_SEARCH       = 110;
   static final int ALLEGRO_KEY_DPAD_CENTER  = 111;
   static final int ALLEGRO_KEY_BUTTON_X     = 112;
   static final int ALLEGRO_KEY_BUTTON_Y     = 113;
   static final int ALLEGRO_KEY_DPAD_UP      = 114;
   static final int ALLEGRO_KEY_DPAD_DOWN    = 115;
   static final int ALLEGRO_KEY_DPAD_LEFT    = 116;
   static final int ALLEGRO_KEY_DPAD_RIGHT   = 117;
   static final int ALLEGRO_KEY_SELECT       = 118;
   static final int ALLEGRO_KEY_START        = 119;
   static final int ALLEGRO_KEY_BUTTON_L1    = 120;
   static final int ALLEGRO_KEY_BUTTON_R1    = 121;
   static final int ALLEGRO_KEY_BUTTON_L2    = 122;
   static final int ALLEGRO_KEY_BUTTON_R2    = 123;
   static final int ALLEGRO_KEY_BUTTON_A     = 124;
   static final int ALLEGRO_KEY_BUTTON_B     = 125;
   static final int ALLEGRO_KEY_THUMBL       = 126;
   static final int ALLEGRO_KEY_THUMBR       = 127;

   static final int ALLEGRO_KEY_UNKNOWN      = 128;

      /* All codes up to before ALLEGRO_KEY_MODIFIERS can be freely
      * assignedas additional unknown keys, like various multimedia
      * and application keys keyboards may have.
      */

   static final int ALLEGRO_KEY_MODIFIERS = 215;

   static final int ALLEGRO_KEY_LSHIFT     = 215;
   static final int ALLEGRO_KEY_RSHIFT     = 216;
   static final int ALLEGRO_KEY_LCTRL      = 217;
   static final int ALLEGRO_KEY_RCTRL      = 218;
   static final int ALLEGRO_KEY_ALT        = 219;
   static final int ALLEGRO_KEY_ALTGR      = 220;
   static final int ALLEGRO_KEY_LWIN       = 221;
   static final int ALLEGRO_KEY_RWIN       = 222;
   static final int ALLEGRO_KEY_MENU       = 223;
   static final int ALLEGRO_KEY_SCROLLLOCK = 224;
   static final int ALLEGRO_KEY_NUMLOCK    = 225;
   static final int ALLEGRO_KEY_CAPSLOCK   = 226;
   
   static final int ALLEGRO_KEY_MAX = 227;
   
   private static int[] keyMap = {
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_UNKNOWN
      ALLEGRO_KEY_LEFT,        // KeyEvent.KEYCODE_SOFT_LEFT
      ALLEGRO_KEY_RIGHT,       // KeyEvent.KEYCODE_SOFT_RIGHT
      ALLEGRO_KEY_HOME,        // KeyEvent.KEYCODE_HOME
      ALLEGRO_KEY_BACK,        // KeyEvent.KEYCODE_BACK
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CALL
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ENDCALL
      ALLEGRO_KEY_0,           // KeyEvent.KEYCODE_0
      ALLEGRO_KEY_1,           // KeyEvent.KEYCODE_1
      ALLEGRO_KEY_2,           // KeyEvent.KEYCODE_2
      ALLEGRO_KEY_3,           // KeyEvent.KEYCODE_3
      ALLEGRO_KEY_4,           // KeyEvent.KEYCODE_4
      ALLEGRO_KEY_5,           // KeyEvent.KEYCODE_5 
      ALLEGRO_KEY_6,           // KeyEvent.KEYCODE_6 
      ALLEGRO_KEY_7,           // KeyEvent.KEYCODE_7 
      ALLEGRO_KEY_8,           // KeyEvent.KEYCODE_8 
      ALLEGRO_KEY_9,           // KeyEvent.KEYCODE_9 
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_STAR
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_POUND
      ALLEGRO_KEY_UP,          // KeyEvent.KEYCODE_DPAD_UP
      ALLEGRO_KEY_DOWN,        // KeyEvent.KEYCODE_DPAD_DOWN
      ALLEGRO_KEY_LEFT,        // KeyEvent.KEYCODE_DPAD_LEFT
      ALLEGRO_KEY_RIGHT,       // KeyEvent.KEYCODE_DPAD_RIGHT
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_DPAD_CENTER
      ALLEGRO_KEY_VOLUME_UP,   // KeyEvent.KEYCODE_VOLUME_UP
      ALLEGRO_KEY_VOLUME_DOWN, // KeyEvent.KEYCODE_VOLUME_DOWN
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_POWER
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CAMERA
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_CLEAR
      ALLEGRO_KEY_A,           // KeyEvent.KEYCODE_A
      ALLEGRO_KEY_B,           // KeyEvent.KEYCODE_B
      ALLEGRO_KEY_C,           // KeyEvent.KEYCODE_B
      ALLEGRO_KEY_D,           // KeyEvent.KEYCODE_D
      ALLEGRO_KEY_E,           // KeyEvent.KEYCODE_E
      ALLEGRO_KEY_F,           // KeyEvent.KEYCODE_F
      ALLEGRO_KEY_G,           // KeyEvent.KEYCODE_G
      ALLEGRO_KEY_H,           // KeyEvent.KEYCODE_H
      ALLEGRO_KEY_I,           // KeyEvent.KEYCODE_I
      ALLEGRO_KEY_J,           // KeyEvent.KEYCODE_J
      ALLEGRO_KEY_K,           // KeyEvent.KEYCODE_K 
      ALLEGRO_KEY_L,           // KeyEvent.KEYCODE_L
      ALLEGRO_KEY_M,           // KeyEvent.KEYCODE_M
      ALLEGRO_KEY_N,           // KeyEvent.KEYCODE_N
      ALLEGRO_KEY_O,           // KeyEvent.KEYCODE_O
      ALLEGRO_KEY_P,           // KeyEvent.KEYCODE_P
      ALLEGRO_KEY_Q,           // KeyEvent.KEYCODE_Q
      ALLEGRO_KEY_R,           // KeyEvent.KEYCODE_R
      ALLEGRO_KEY_S,           // KeyEvent.KEYCODE_S
      ALLEGRO_KEY_T,           // KeyEvent.KEYCODE_T
      ALLEGRO_KEY_U,           // KeyEvent.KEYCODE_U
      ALLEGRO_KEY_V,           // KeyEvent.KEYCODE_V
      ALLEGRO_KEY_W,           // KeyEvent.KEYCODE_W
      ALLEGRO_KEY_X,           // KeyEvent.KEYCODE_X
      ALLEGRO_KEY_Y,           // KeyEvent.KEYCODE_Y
      ALLEGRO_KEY_Z,           // KeyEvent.KEYCODE_Z
      ALLEGRO_KEY_COMMA,       // KeyEvent.KEYCODE_COMMA 
      ALLEGRO_KEY_FULLSTOP,    // KeyEvent.KEYCODE_PERIOD 
      ALLEGRO_KEY_ALT,         // KeyEvent.KEYCODE_ALT_LEFT
      ALLEGRO_KEY_ALTGR,       // KeyEvent.KEYCODE_ALT_RIGHT
      ALLEGRO_KEY_LSHIFT,      // KeyEvent.KEYCODE_SHIFT_LEFT
      ALLEGRO_KEY_RSHIFT,      // KeyEvent.KEYCODE_SHIFT_RIGHT
      ALLEGRO_KEY_TAB,         // KeyEvent.KEYCODE_TAB
      ALLEGRO_KEY_SPACE,       // KeyEvent.KEYCODE_SPACE
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_SYM
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_EXPLORER
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ENVELOPE
      ALLEGRO_KEY_ENTER,       // KeyEvent.KEYCODE_ENTER
      ALLEGRO_KEY_BACKSPACE,   // KeyEvent.KEYCODE_DEL
      ALLEGRO_KEY_TILDE,       // KeyEvent.KEYCODE_GRAVE
      ALLEGRO_KEY_MINUS,       // KeyEvent.KEYCODE_MINUS
      ALLEGRO_KEY_EQUALS,      // KeyEvent.KEYCODE_EQUALS
      ALLEGRO_KEY_OPENBRACE,   // KeyEvent.KEYCODE_LEFT_BRACKET
      ALLEGRO_KEY_CLOSEBRACE,  // KeyEvent.KEYCODE_RIGHT_BRACKET
      ALLEGRO_KEY_BACKSLASH,   // KeyEvent.KEYCODE_BACKSLASH
      ALLEGRO_KEY_SEMICOLON,   // KeyEvent.KEYCODE_SEMICOLON
      ALLEGRO_KEY_QUOTE,       // KeyEvent.KEYCODE_APOSTROPHY
      ALLEGRO_KEY_SLASH,       // KeyEvent.KEYCODE_SLASH
      ALLEGRO_KEY_AT,          // KeyEvent.KEYCODE_AT
      ALLEGRO_KEY_NUMLOCK,     // KeyEvent.KEYCODE_NUM
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_HEADSETHOOK
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_FOCUS
      ALLEGRO_KEY_PAD_PLUS,    // KeyEvent.KEYCODE_PLUS
      ALLEGRO_KEY_MENU,        // KeyEvent.KEYCODE_MENU
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_NOTIFICATION
      ALLEGRO_KEY_SEARCH,      // KeyEvent.KEYCODE_SEARCH
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_STOP
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_NEXT
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_PREVIOUS
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_REWIND
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MEDIA_FAST_FORWARD
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MUTE
      ALLEGRO_KEY_PGUP,        // KeyEvent.KEYCODE_PAGE_UP
      ALLEGRO_KEY_PGDN,        // KeyEvent.KEYCODE_PAGE_DOWN
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_PICTSYMBOLS
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_SWITCH_CHARSET
      ALLEGRO_KEY_BUTTON_A,    // KeyEvent.KEYCODE_BUTTON_A
      ALLEGRO_KEY_BUTTON_B,    // KeyEvent.KEYCODE_BUTTON_B
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_C
      ALLEGRO_KEY_BUTTON_X,    // KeyEvent.KEYCODE_BUTTON_X
      ALLEGRO_KEY_BUTTON_Y,    // KeyEvent.KEYCODE_BUTTON_Y
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_BUTTON_Z
      ALLEGRO_KEY_BUTTON_L1,   // KeyEvent.KEYCODE_BUTTON_L1
      ALLEGRO_KEY_BUTTON_R1,   // KeyEvent.KEYCODE_BUTTON_R1
      ALLEGRO_KEY_BUTTON_L2,   // KeyEvent.KEYCODE_BUTTON_L2
      ALLEGRO_KEY_BUTTON_R2,   // KeyEvent.KEYCODE_BUTTON_R2
      ALLEGRO_KEY_THUMBL,      // KeyEvent.KEYCODE_THUMBL
      ALLEGRO_KEY_THUMBR,      // KeyEvent.KEYCODE_THUMBR
      ALLEGRO_KEY_START,       // KeyEvent.KEYCODE_START
      ALLEGRO_KEY_SELECT,      // KeyEvent.KEYCODE_SELECT
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_MODE
      ALLEGRO_KEY_ESCAPE,      // KeyEvent.KEYCODE_ESCAPE (111)
      ALLEGRO_KEY_DELETE,      // KeyEvent.KEYCODE_FORWARD_DELETE (112)
      ALLEGRO_KEY_LCTRL,       // KeyEvent.KEYCODE_CTRL_LEFT (113)
      ALLEGRO_KEY_RCTRL,       // KeyEvent.KEYCODE_CONTROL_RIGHT (114)
      ALLEGRO_KEY_CAPSLOCK,    // KeyEvent.KEYCODE_CAPS_LOCK (115)
      ALLEGRO_KEY_SCROLLLOCK,  // KeyEvent.KEYCODE_SCROLL_LOCK (116)
      ALLEGRO_KEY_LWIN,        // KeyEvent.KEYCODE_META_LEFT (117)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (118)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (119)
      ALLEGRO_KEY_PRINTSCREEN, // KeyEvent.KEYCODE_SYSRQ (120)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (121)
      ALLEGRO_KEY_HOME,        // KeyEvent.KEYCODE_MOVE_HOME (122)
      ALLEGRO_KEY_END,         // KeyEvent.KEYCODE_MOVE_END (123)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (124)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (125)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (126)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (127)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (128)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (129)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (130)
      ALLEGRO_KEY_F1,          // KeyEvent.KEYCODE_ (131)
      ALLEGRO_KEY_F2,          // KeyEvent.KEYCODE_ (132)
      ALLEGRO_KEY_F3,          // KeyEvent.KEYCODE_ (133)
      ALLEGRO_KEY_F4,          // KeyEvent.KEYCODE_ (134)
      ALLEGRO_KEY_F5,          // KeyEvent.KEYCODE_ (135)
      ALLEGRO_KEY_F6,          // KeyEvent.KEYCODE_ (136)
      ALLEGRO_KEY_F7,          // KeyEvent.KEYCODE_ (137)
      ALLEGRO_KEY_F8,          // KeyEvent.KEYCODE_ (138)
      ALLEGRO_KEY_F9,          // KeyEvent.KEYCODE_ (139)
      ALLEGRO_KEY_F10,         // KeyEvent.KEYCODE_ (140)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (141)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (142)
      ALLEGRO_KEY_NUMLOCK,     // KeyEvent.KEYCODE_NUM_LOCK (143)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (144)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (145)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (146)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (147)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (148)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (149)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (150)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (151)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (152)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (153)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (154)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (155)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (156)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (157)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (158)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (159)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (160)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (161)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (162)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (163)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (164)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (165)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (166)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (167)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (168)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (169)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (170)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (171)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (172)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (173)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (174)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (175)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (176)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (177)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (178)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (179)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (180)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (181)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (182)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (183)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (184)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (185)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (186)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (187)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (188)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (189)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (190)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (191)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (192)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (193)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (194)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (195)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (196)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (197)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (198)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (199)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (200)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (201)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (202)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (203)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (204)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (205)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (206)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (207)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (208)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (209)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (210)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (211)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (212)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (213)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (214)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (215)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (216)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (217)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (218)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (219)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (220)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (221)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (222)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (223)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (224)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (225)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (226)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (227)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (228)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (229)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (230)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (231)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (232)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (233)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (234)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (235)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (236)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (237)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (238)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (239)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (240)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (241)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (242)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (243)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (244)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (245)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (246)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (247)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (248)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (249)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (250)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (251)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (252)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (253)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (254)
      ALLEGRO_KEY_UNKNOWN,     // KeyEvent.KEYCODE_ (255)
   };
   
   final int ALLEGRO_EVENT_TOUCH_BEGIN  = 50;
   final int ALLEGRO_EVENT_TOUCH_END    = 51;
   final int ALLEGRO_EVENT_TOUCH_MOVE   = 52;
   final int ALLEGRO_EVENT_TOUCH_CANCEL = 5;
   
   /** native functions we call */
   public native void nativeOnCreate();
   public native boolean nativeOnDestroy();
   public native void nativeOnChange(int format, int width, int height);
   public native void nativeOnKeyDown(int key);
   public native void nativeOnKeyUp(int key);
   public native void nativeOnKeyChar(int key);
   public native void nativeOnTouch(int id, int action, float x, float y, boolean primary);
   
   /** functions that native code calls */
   
   private int[]       egl_Version = { 0, 0 };
   private EGLContext  egl_Context;
   private EGLSurface  egl_Surface;
   private EGLDisplay  egl_Display;
   private int         egl_numConfigs = 0;
   private int[]       egl_attribs;
   ArrayList<Integer>  egl_attribWork = new ArrayList<Integer>();
   EGLConfig[]         egl_Config = new EGLConfig[] { null };
   int[]               es2_attrib = {EGL_CONTEXT_CLIENT_VERSION, 1, EGL10.EGL_NONE};

   private Context context;
   private boolean captureVolume = false;
   
   public boolean egl_Init()
   {
      Log.d("AllegroSurface", "egl_Init");
      EGL10 egl = (EGL10)EGLContext.getEGL();

      EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
      
      if(!egl.eglInitialize(dpy, egl_Version)) {
         Log.d("AllegroSurface", "egl_Init fail");
         return false;
      }
      
      egl_Display = dpy;
      
      Log.d("AllegroSurface", "egl_Init end");
      return true;
   }
   
   public int egl_getMajorVersion() { return egl_Version[0]; }
   public int egl_getMinorVersion() { return egl_Version[1]; }
   public int egl_getNumConfigs()   { return egl_numConfigs; }

   public void egl_setConfigAttrib(int attr, int value)
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();

      int egl_attr = attr;

      switch (attr) {
         case ALLEGRO_RED_SIZE:
            egl_attr = egl.EGL_RED_SIZE;
            break;
            
         case ALLEGRO_GREEN_SIZE:
            egl_attr = egl.EGL_GREEN_SIZE;
            break;
            
         case ALLEGRO_BLUE_SIZE:
            egl_attr = egl.EGL_BLUE_SIZE;
            break;

         case ALLEGRO_ALPHA_SIZE:
            egl_attr = egl.EGL_ALPHA_SIZE;
            break;

         case ALLEGRO_DEPTH_SIZE:
            egl_attr = egl.EGL_DEPTH_SIZE;
            break;

         case ALLEGRO_STENCIL_SIZE:
            egl_attr = egl.EGL_STENCIL_SIZE;
            break;

         case ALLEGRO_SAMPLE_BUFFERS:
            egl_attr = egl.EGL_SAMPLE_BUFFERS;
            break;

         case ALLEGRO_SAMPLES:
            egl_attr = egl.EGL_SAMPLES;
            break;

         /* Allow others to pass right into the array */
      }

      /* Check if it's already in the list, if so change the value */
      for (int i = 0; i < egl_attribWork.size(); i++) {
         if (i % 2 == 0) {
            if (egl_attribWork.get(i) == egl_attr) {
               egl_attribWork.set(i+1, value);
               return;
            }
         }
      }

      /* Not in the list, add it */
      egl_attribWork.add(egl_attr);
      egl_attribWork.add(value);
   }
      
   private final int EGL_OPENGL_ES_BIT = 1;
   private final int EGL_OPENGL_ES2_BIT = 4;

   private boolean checkGL20Support( Context context )
   {
      EGL10 egl = (EGL10) EGLContext.getEGL();      
      //EGLDisplay display = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);
   
      //int[] version = new int[2];
      //egl.eglInitialize(display, version);
   
      int[] configAttribs =
      {
         EGL10.EGL_RED_SIZE, 4,
         EGL10.EGL_GREEN_SIZE, 4,
         EGL10.EGL_BLUE_SIZE, 4,
         EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
         EGL10.EGL_NONE
      };
   
      EGLConfig[] configs = new EGLConfig[10];
      int[] num_config = new int[1];
      //egl.eglChooseConfig(display, configAttribs, configs, 10, num_config);    
      egl.eglChooseConfig(egl_Display, configAttribs, configs, 10, num_config);    
      //egl.eglTerminate(display);
      Log.d("AllegroSurface", "" + num_config[0] + " OpenGL ES 2 configurations found.");
      return num_config[0] > 0;
   }

   static HashMap<Integer, String> eglErrors;
   private static void checkEglError(String prompt, EGL10 egl) {
      if (eglErrors == null) {
          eglErrors = new HashMap<Integer, String>();
          eglErrors.put(EGL10.EGL_BAD_DISPLAY, "EGL_BAD_DISPLAY");
          eglErrors.put(EGL10.EGL_NOT_INITIALIZED, "EGL_NOT_INITIALIZED");
          eglErrors.put(EGL10.EGL_BAD_SURFACE, "EGL_BAD_SURFACE");
          eglErrors.put(EGL10.EGL_BAD_CONTEXT, "EGL_BAD_CONTEXT");
          eglErrors.put(EGL10.EGL_BAD_MATCH, "EGL_BAD_MATCH");
          eglErrors.put(EGL10.EGL_BAD_ACCESS, "EGL_BAD_ACCESS");
          eglErrors.put(EGL10.EGL_BAD_NATIVE_PIXMAP, "EGL_BAD_NATIVE_PIXMAP");
          eglErrors.put(EGL10.EGL_BAD_NATIVE_WINDOW, "EGL_BAD_NATIVE_WINDOW");
          eglErrors.put(EGL10.EGL_BAD_CURRENT_SURFACE, "EGL_BAD_CURRENT_SURFACE");
          eglErrors.put(EGL10.EGL_BAD_ALLOC, "EGL_BAD_ALLOC");
          eglErrors.put(EGL10.EGL_BAD_CONFIG, "EGL_BAD_CONFIG");
          eglErrors.put(EGL10.EGL_BAD_ATTRIBUTE, "EGL_BAD_ATTRIBUTE");
          eglErrors.put(EGL11.EGL_CONTEXT_LOST, "EGL_CONTEXT_LOST");
      }
      int error;
      while ((error = egl.eglGetError()) != EGL10.EGL_SUCCESS) {
         Log.e("Allegro", String.format("%s: EGL error: %s", prompt,
            eglErrors.get(error)));
      }
   }

   /* Return values:
    * 0 - failure
    * 1 - success
    * 2 - fell back to older ES version
    */
   public int egl_createContext(int version)
   {
      Log.d("AllegroSurface", "egl_createContext");
      EGL10 egl = (EGL10)EGLContext.getEGL();
      int ret = 1;

      es2_attrib[1] = version;
      
      egl_setConfigAttrib(EGL10.EGL_RENDERABLE_TYPE, version == 2 ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT);

      boolean color_size_specified = false;
      for (int i = 0; i < egl_attribWork.size(); i++) {
         Log.d("AllegroSurface", "egl_attribs[" + i + "] = " + egl_attribWork.get(i));
         if (i % 2 == 0) {
            if (egl_attribWork.get(i) == EGL10.EGL_RED_SIZE || egl_attribWork.get(i) == EGL10.EGL_GREEN_SIZE ||
                  egl_attribWork.get(i) == EGL10.EGL_BLUE_SIZE) {
               color_size_specified = true;
            }
         }
      }

      egl_attribs = new int[egl_attribWork.size()+1];
      for (int i = 0; i < egl_attribWork.size(); i++) {
         egl_attribs[i] = egl_attribWork.get(i);
      }
      egl_attribs[egl_attribWork.size()] = EGL10.EGL_NONE;
      
      int[] num = new int[1];
      boolean retval = egl.eglChooseConfig(egl_Display, egl_attribs, egl_Config, 1, num);
      if (retval == false || num[0] < 1) {
         Log.e("AllegroSurface", "No matching config");
         return 0;
      }

      EGLContext ctx = egl.eglCreateContext(egl_Display, egl_Config[0], EGL10.EGL_NO_CONTEXT, es2_attrib);
      if (ctx == EGL10.EGL_NO_CONTEXT) {
         checkEglError("eglCreateContext", egl);
         Log.d("AllegroSurface", "egl_createContext no context");
         return 0;
      }

      Log.d("AllegroSurface", "EGL context created");
      
      egl_Context = ctx;
      
      Log.d("AllegroSurface", "egl_createContext end");

      return ret;
   }
   
   public void egl_destroyContext()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      Log.d("AllegroSurface", "destroying egl_Context");
      egl.eglDestroyContext(egl_Display, egl_Context);
      egl_Context = EGL10.EGL_NO_CONTEXT;
   }
   
   public boolean egl_createSurface()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      EGLSurface surface = egl.eglCreateWindowSurface(egl_Display, egl_Config[0], this, null);
      if(surface == EGL10.EGL_NO_SURFACE) {
         Log.d("AllegroSurface", "egl_createSurface can't create surface (" +  egl.eglGetError() + ")");
         return false;
      }
      
      if(!egl.eglMakeCurrent(egl_Display, surface, surface, egl_Context)) {
         egl.eglDestroySurface(egl_Display, surface);
         Log.d("AllegroSurface", "egl_createSurface can't make current");
         return false;
      }
      
      egl_Surface = surface;
      
      Log.d("AllegroSurface", "created new surface: " + surface);
      
      return true;
   }
   
   public void egl_destroySurface()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if(!egl.eglMakeCurrent(egl_Display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT)) {
         Log.d("AllegroSurface", "could not clear current context");
      }
      
      Log.d("AllegroSurface", "destroying egl_Surface");
      egl.eglDestroySurface(egl_Display, egl_Surface);
      egl_Surface = EGL10.EGL_NO_SURFACE;
   }
   
   public void egl_clearCurrent()
   {
      Log.d("AllegroSurface", "egl_clearCurrent");
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if(!egl.eglMakeCurrent(egl_Display, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT)) {
         Log.d("AllegroSurface", "could not clear current context");
      }
      Log.d("AllegroSurface", "egl_clearCurrent done");
   }
   
   public void egl_makeCurrent()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if(!egl.eglMakeCurrent(egl_Display, egl_Surface, egl_Surface, egl_Context)) {
      //   egl.eglDestroySurface(egl_Display, surface);
      //   egl.eglTerminate(egl_Display);
      //   egl_Display = null;
         Log.d("AllegroSurface", "can't make thread current: ");
         checkEglError("eglMakeCurrent", egl);
         return;
      }  
   }
   
   public void egl_SwapBuffers()
   {
      //try {
      //   Log.d("AllegroSurface", "posting SwapBuffers!");
      //   AllegroActivity.Self.postRunnable( new Runnable() {
      //      public void run() {
               try {
                  EGL10 egl = (EGL10)EGLContext.getEGL();

                  // FIXME: Pretty sure flush is implicit with SwapBuffers
                  //egl.eglWaitNative(EGL10.EGL_NATIVE_RENDERABLE, null);
                  //egl.eglWaitGL();
                  
                  egl.eglSwapBuffers(egl_Display, egl_Surface);
                  checkEglError("eglSwapBuffers", egl);
                  
               } catch(Exception x) {
                  Log.d("AllegroSurface", "inner exception: " + x.getMessage());
               }
      //      }
      //   } );
      //} catch(Exception x) {
      //   Log.d("AllegroSurface", "exception: " + x.getMessage());
      //}
   }
   
   /** main handlers */

   public AllegroSurface(Context context, Display display)
   {
      super(context);

      this.context = context;

      Log.d("AllegroSurface", "PixelFormat=" + display.getPixelFormat());
      getHolder().setFormat(display.getPixelFormat());

      Log.d("AllegroSurface", "ctor");

      getHolder().addCallback(this); 
      
      Log.d("AllegroSurface", "ctor end");
   }

   void grabFocus()
   {
      Log.d("AllegroSurface", "Grabbing focus");

      setFocusable(true);
      setFocusableInTouchMode(true);
      requestFocus();
      setOnKeyListener(this); 
      setOnTouchListener(this);
   }
      
   public void surfaceCreated(SurfaceHolder holder)
   {
      Log.d("AllegroSurface", "surfaceCreated");
      nativeOnCreate();
      grabFocus();
      Log.d("AllegroSurface", "surfaceCreated end");
   }

   public void surfaceDestroyed(SurfaceHolder holder)
   {
      Log.d("AllegroSurface", "surfaceDestroyed");

      if (!nativeOnDestroy()) {
         Log.d("AllegroSurface", "No surface created, returning early");
         return;
      }

      egl_makeCurrent();
      
      egl_destroySurface();
      egl_destroyContext();
      
      EGL10 egl = (EGL10)EGLContext.getEGL();
      egl.eglTerminate(egl_Display);
      egl_Display = null;
      
      Log.d("AllegroSurface", "surfaceDestroyed end");
   }

   public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
   {
      Log.d("AllegroSurface", "surfaceChanged (width=" + width + " height=" + height + ")");
      nativeOnChange(0xdeadbeef, width, height);
      Log.d("AllegroSurface", "surfaceChanged end");
   }
   
   /* unused */
   public void onDraw(Canvas canvas) { }
   
   /* events */
   
   /* maybe dump a stacktrace and die, rather than loging, and ignoring errors */
   /* all this fancyness is so we work on as many versions of android as possible,
    * and gracefully degrade, rather than just outright failing */
   
   private boolean fieldExists(Object obj, String fieldName)
   {
      try {
         Class cls = obj.getClass();
         Field m = cls.getField(fieldName);
         return true;
      } catch(Exception x) {
         return false;
      }
   }
   
   private <T> T getField(Object obj, String field)
   {
      try {
         Class cls = obj.getClass();
         Field f = cls.getField(field);
         return (T)f.get(obj);
      }
      catch(NoSuchFieldException x) {
         Log.v("AllegroSurface", "field " + field + " not found in class " + obj.getClass().getCanonicalName());
         return null;
      }
      catch(IllegalArgumentException x) {
         Log.v("AllegroSurface", "this shouldn't be possible, but fetching " + field + " from class " + obj.getClass().getCanonicalName() + " failed with an illegal argument exception");
         return null;
      }
      catch(IllegalAccessException x) {
         Log.v("AllegroSurface", "field " + field + " is not accessible in class " + obj.getClass().getCanonicalName());
         return null;
      }
   }
   
   private <T> T callMethod(Object obj, String methName, Class [] types, Object... args)
   {
      try {
         Class cls = obj.getClass();
         Method m = cls.getMethod(methName, types);
         return (T)m.invoke(obj, args);
      }
      catch(NoSuchMethodException x) {
         Log.v("AllegroSurface", "method " + methName + " does not exist in class " + obj.getClass().getCanonicalName());
         return null;
      }
      catch(NullPointerException x) {
         Log.v("AllegroSurface", "can't invoke null method name");
         return null;
      }
      catch(SecurityException x) {
         Log.v("AllegroSurface", "method " + methName + " is not accessible in class " + obj.getClass().getCanonicalName());
         return null;
      }
      catch(IllegalAccessException x)
      {
         Log.v("AllegroSurface", "method " + methName + " is not accessible in class " + obj.getClass().getCanonicalName());
         return null;
      }
      catch(InvocationTargetException x)
      {
         Log.v("AllegroSurface", "method " + methName + " threw an exception :(");
         return null;
      }
   }

   public void setCaptureVolumeKeys(boolean onoff)
   {
      captureVolume = onoff;
   }

   private void volumeChange(int inc)
   {
         AudioManager mAudioManager = (AudioManager)context.getApplicationContext().getSystemService(Context.AUDIO_SERVICE);

         // get the current volume level
         int curr =  
            mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);

         curr += inc;

         if (0 <= curr && curr <= mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC))
         {
            // set a new volume level manually
            // with the FLAG_SHOW_UI flag.
            mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, curr, AudioManager.FLAG_SHOW_UI);
         }
   }

   public boolean onKey(View v, int keyCode, KeyEvent event)
   {
      if (event.getAction() == KeyEvent.ACTION_DOWN) {
         if (!captureVolume) {
            if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
               volumeChange(1);
               return true;
            }
            else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
               volumeChange(-1);
               return true;
            }
         }
         if (event.getRepeatCount() == 0) {
            nativeOnKeyDown(keyMap[keyCode]);
            nativeOnKeyChar(keyMap[keyCode]);
         }
         else {
            nativeOnKeyChar(keyMap[keyCode]);
         }
         return true;
      }
      else if (event.getAction() == KeyEvent.ACTION_UP) {
         nativeOnKeyUp(keyMap[keyCode]);
         return true;
      }
         
      return false;
   }

   // FIXME: pull out android version detection into the setup
   // and just check some flags here, rather than checking for
   // the existance of the fields and methods over and over
   public boolean onTouch(View v, MotionEvent event)
   {
      //Log.d("AllegroSurface", "onTouch");
      int action = 0;
      int pointer_id = 0;
      
      Class[] no_args = new Class[0];
      Class[] int_arg = new Class[1];
      int_arg[0] = int.class;
      
      if(Utils.methodExists(event, "getActionMasked")) { // android-8 / 2.2.x
         action = this.<Integer>callMethod(event, "getActionMasked", no_args);
         int ptr_idx = this.<Integer>callMethod(event, "getActionIndex", no_args);
         pointer_id = this.<Integer>callMethod(event, "getPointerId", int_arg, ptr_idx);
      } else {
         int raw_action = event.getAction();
         
         if(fieldExists(event, "ACTION_MASK")) { // android-5 / 2.0
            int mask = this.<Integer>getField(event, "ACTION_MASK");
            action = raw_action & mask;
            
            int ptr_id_mask = this.<Integer>getField(event, "ACTION_POINTER_ID_MASK");
            int ptr_id_shift = this.<Integer>getField(event, "ACTION_POINTER_ID_SHIFT");
            
            pointer_id = event.getPointerId((raw_action & ptr_id_mask) >> ptr_id_shift);
         }
         else { // android-4 / 1.6
            /* no ACTION_MASK? no multi touch, no pointer_id, */
            action = raw_action;
         }
      }
      
      boolean primary = false;
      
      if(action == MotionEvent.ACTION_DOWN) {
         primary = true;
         action = ALLEGRO_EVENT_TOUCH_BEGIN;
      }
      else if(action == MotionEvent.ACTION_UP) {
         primary = true;
         action = ALLEGRO_EVENT_TOUCH_END;
      }
      else if(action == MotionEvent.ACTION_MOVE) {
         action = ALLEGRO_EVENT_TOUCH_MOVE;
      }
      else if(action == MotionEvent.ACTION_CANCEL) {
         action = ALLEGRO_EVENT_TOUCH_CANCEL;
      }
      // android-5 / 2.0
      else if(fieldExists(event, "ACTION_POINTER_DOWN") &&
         action == this.<Integer>getField(event, "ACTION_POINTER_DOWN"))
      {
         action = ALLEGRO_EVENT_TOUCH_BEGIN;
      }
      else if(fieldExists(event, "ACTION_POINTER_UP") &&
         action == this.<Integer>getField(event, "ACTION_POINTER_UP"))
      {
         action = ALLEGRO_EVENT_TOUCH_END;
      }
      else {
         Log.v("AllegroSurface", "unknown MotionEvent type: " + action);
         Log.d("AllegroSurface", "onTouch endf");
         return false;
      }
      
      if(Utils.methodExists(event, "getPointerCount")) { // android-5 / 2.0
         int pointer_count = this.<Integer>callMethod(event, "getPointerCount", no_args);
         for(int i = 0; i < pointer_count; i++) {
            float x = this.<Float>callMethod(event, "getX", int_arg, i);
            float y = this.<Float>callMethod(event, "getY", int_arg, i);
            int  id = this.<Integer>callMethod(event, "getPointerId", int_arg, i);
            
            /* not entirely sure we need to report move events for non primary touches here
             * but examples I've see say that the ACTION_[POINTER_][UP|DOWN]
             * report all touches and they can change between the last MOVE
             * and the UP|DOWN event */
            if(id == pointer_id) {
               nativeOnTouch(id, action, x, y, primary);
            } else {
               nativeOnTouch(id, ALLEGRO_EVENT_TOUCH_MOVE, x, y, primary);
            }
         }
      } else {
         nativeOnTouch(pointer_id, action, event.getX(), event.getY(), primary);
      }

      //Log.d("AllegroSurface", "onTouch end");
      return true;
   }

}


/* vim: set sts=3 sw=3 et: */
