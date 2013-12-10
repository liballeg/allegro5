package org.liballeg.android;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import java.io.File;
import java.lang.Runnable;
import java.lang.String;

public class AllegroActivity extends Activity
{
   /* properties */
   private String userLibName = "libapp.so";
   private Handler handler;
   private Sensors sensors;
   private Configuration currentConfig;
   private AllegroSurface surface;
   private ScreenLock screenLock;
   private boolean exitedMain = false;

   /* native methods we call */
   native boolean nativeOnCreate();
   native void nativeOnPause();
   native void nativeOnResume();
   native void nativeOnDestroy();
   native void nativeOnOrientationChange(int orientation, boolean init);

   /* methods native code calls */

   String getUserLibName()
   {
      ApplicationInfo appInfo = getApplicationInfo();
      String libDir = Reflect.getField(appInfo, "nativeLibraryDir");
      /* Android < 2.3 doesn't have .nativeLibraryDir */
      if (libDir == null) {
         libDir = appInfo.dataDir + "/lib";
      }
      return libDir + "/" + userLibName;
   }

   String getResourcesDir()
   {
      //return getApplicationInfo().dataDir + "/assets";
      //return getApplicationInfo().sourceDir + "/assets/";
      return getFilesDir().getAbsolutePath();
   }

   String getPubDataDir()
   {
      return getFilesDir().getAbsolutePath();
   }

   String getApkPath()
   {
      return getApplicationInfo().sourceDir;
   }

   String getModel()
   {
      return android.os.Build.MODEL;
   }

   void postRunnable(Runnable runme)
   {
      try {
         Log.d("AllegroActivity", "postRunnable");
         handler.post( runme );
      } catch (Exception x) {
         Log.d("AllegroActivity", "postRunnable exception: " + x.getMessage());
      }
   }

   void createSurface()
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

   void postCreateSurface()
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

   void destroySurface()
   {
      Log.d("AllegroActivity", "destroySurface");

      ViewGroup vg = (ViewGroup)(surface.getParent());
      vg.removeView(surface);
      surface = null;
   }

   void postDestroySurface()
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

   void postFinish()
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

   boolean getMainReturned()
   {
      return exitedMain;
   }

   boolean inhibitScreenLock(boolean inhibit)
   {
      if (screenLock == null) {
         screenLock = new ScreenLock(this);
      }
      return screenLock.inhibitScreenLock(inhibit);
   }

   /* end of functions native code calls */

   public AllegroActivity(String userLibName)
   {
      super();
      this.userLibName = userLibName;
   }

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
      sensors = new Sensors(getApplicationContext());

      currentConfig = new Configuration(getResources().getConfiguration());

      Log.d("AllegroActivity", "before nativeOnCreate");
      if (!nativeOnCreate()) {
         finish();
         Log.d("AllegroActivity", "nativeOnCreate failed");
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

      sensors.unlisten();

      nativeOnPause();
      Log.d("AllegroActivity", "onPause end");
   }

   /** Called when the activity is resumed/unpaused */
   @Override
   public void onResume()
   {
      Log.d("AllegroActivity", "onResume");
      super.onResume();

      sensors.listen();

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

   void setAllegroOrientation(int alleg_orientation)
   {
      setRequestedOrientation(Const.toAndroidOrientation(alleg_orientation));
   }

   int getAllegroOrientation()
   {
      int rotation;
      if (Reflect.methodExists(getWindowManager().getDefaultDisplay(),
            "getRotation"))
      {
         /* 2.2+ */
         rotation = getWindowManager().getDefaultDisplay().getRotation();
      }
      else {
         rotation = getWindowManager().getDefaultDisplay().getOrientation();
      }
      return Const.toAllegroOrientation(rotation);
   }

   String getOsVersion()
   {
      return android.os.Build.VERSION.RELEASE;
   }
}

/* vim: set sts=3 sw=3 et: */
