package org.liballeg.android;

import android.app.Activity;
import android.content.Context;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.content.res.Configuration;
import android.hardware.display.DisplayManager;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.graphics.Rect;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import java.io.File;
import java.lang.Runnable;
import java.lang.String;
import android.view.InputDevice;
import java.util.Vector;
import android.os.Build;
import android.view.View;
import android.view.KeyEvent;
import android.window.OnBackInvokedCallback;
import android.window.OnBackInvokedDispatcher;

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
   private boolean joystickReconfigureNotified = false;
   private Vector<Integer> joysticks;
   private Clipboard clipboard;
   private DisplayManager.DisplayListener displayListener;

   public final static int JS_A = 0;
   public final static int JS_B = 1;
   public final static int JS_X = 2;
   public final static int JS_Y = 3;
   public final static int JS_L1 = 4;
   public final static int JS_R1 = 5;
   public final static int JS_DPAD_L = 6;
   public final static int JS_DPAD_R = 7;
   public final static int JS_DPAD_U = 8;
   public final static int JS_DPAD_D = 9;
   public final static int JS_START = 10; // middle right (start, menu)
   public final static int JS_SELECT = 11; // middle left (select, back)
   public final static int JS_MODE = 12; // middle (mode, guide)
   public final static int JS_THUMBL = 13;
   public final static int JS_THUMBR = 14;
   public final static int JS_L2 = 15;
   public final static int JS_R2 = 16;
   public final static int JS_C = 17;
   public final static int JS_Z = 18;
   public final static int JS_DPAD_CENTER = 19;
   public final static int JS_BUTTON_1 = 20; // generic gamepad buttons
   public final static int JS_BUTTON_2 = 21;
   public final static int JS_BUTTON_3 = 22;
   public final static int JS_BUTTON_4 = 23;
   public final static int JS_BUTTON_5 = 24;
   public final static int JS_BUTTON_6 = 25;
   public final static int JS_BUTTON_7 = 26;
   public final static int JS_BUTTON_8 = 27;
   public final static int JS_BUTTON_9 = 28;
   public final static int JS_BUTTON_10 = 29;
   public final static int JS_BUTTON_11 = 30;
   public final static int JS_BUTTON_12 = 31;
   public final static int JS_BUTTON_13 = 32;
   public final static int JS_BUTTON_14 = 33;
   public final static int JS_BUTTON_15 = 34;
   public final static int JS_BUTTON_16 = 35;

   public boolean joystickActive = false;

   /* native methods we call */
   native boolean nativeOnCreate();
   native void nativeOnPause();
   native void nativeOnResume();
   native void nativeOnDestroy();
   native void nativeOnOrientationChange(int orientation, boolean init);
   native void nativeSendJoystickConfigurationEvent();

   /* methods native code calls */

   String getUserLibName()
   {
      //ApplicationInfo appInfo = getApplicationInfo();
      //String libDir = Reflect.getField(appInfo, "nativeLibraryDir");
      ///* Android < 2.3 doesn't have .nativeLibraryDir */
      //if (libDir == null) {
         //libDir = appInfo.dataDir + "/lib";
      //}
      //return libDir + "/" + userLibName;
      return userLibName;
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

   String getTempDir()
   {
      return getCacheDir().getAbsolutePath();
   }

   String getApkPath()
   {
      return getApplicationInfo().sourceDir;
   }

   String getModel()
   {
      return android.os.Build.MODEL;
   }

   String getManufacturer()
   {
      return android.os.Build.MANUFACTURER;
   }

   Rect getDisplaySize()
   {
      Display display = getWindowManager().getDefaultDisplay();
      Rect size = new Rect();

      if (android.os.Build.VERSION.SDK_INT >= 13) {
         display.getRectSize(size);
      }
      else {
         size.left = 0;
         size.top = 0;
         size.right = display.getWidth();
         size.bottom = display.getHeight();
      }

      return size;
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
               getWindowManager().getDefaultDisplay(), this);

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

      joysticks = new Vector<Integer>();

      reconfigureJoysticks();

      Thread t = new Thread() {
         public void run() {
            while (true) {
               try {
                  Thread.sleep(100);
               }
               catch (Exception e) {
               }

               if (joystickReconfigureNotified) {
                  continue;
               }

               int[] all = InputDevice.getDeviceIds();

               boolean doConfigure = false;
               int count = 0;

               for (int i = 0; i < all.length; i++) {
                  if (isJoystick(all[i])) {
                     if (!joysticks.contains(all[i])) {
                        doConfigure = true;
                        break;
                     }
                     else {
                        count++;
                     }
                  }
               }

               if (!doConfigure) {
                  if (count != joysticks.size()) {
                     doConfigure = true;
                  }
               }

               if (doConfigure) {
                  Log.d("AllegroActivity", "Sending joystick reconfigure event");
                  joystickReconfigureNotified = true;
                  nativeSendJoystickConfigurationEvent();
               }
            }
         }
      };
      t.start();
   }

   public void updateOrientation() {
      nativeOnOrientationChange(getAllegroOrientation(), false);
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
      clipboard = new Clipboard(this);

      currentConfig = new Configuration(getResources().getConfiguration());

      Log.d("AllegroActivity", "before nativeOnCreate");
      if (!nativeOnCreate()) {
         finish();
         Log.d("AllegroActivity", "nativeOnCreate failed");
         return;
      }

      requestWindowFeature(Window.FEATURE_NO_TITLE);
      this.getWindow().addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);

      if(Build.VERSION.SDK_INT >= 33) {
         // handle the back button / gesture on API level 33+
         getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
            OnBackInvokedDispatcher.PRIORITY_DEFAULT, new OnBackInvokedCallback() {
               @Override
               public void onBackInvoked() {
                  // these will be mapped to ALLEGRO_KEY_BACK
                  KeyEvent keyDown = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_BACK);
                  KeyEvent keyUp = new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_BACK);
                  dispatchKeyEvent(keyDown);
                  dispatchKeyEvent(keyUp);
               }
            }
         );
      }

      Log.d("AllegroActivity", "onCreate end");
   }

   @Override
   public void onStart()
   {
      super.onStart();
      Log.d("AllegroActivity", "onStart");

      final AllegroActivity activity = this;
      displayListener = new DisplayManager.DisplayListener() {
         @Override
         public void onDisplayAdded(int displayId) {}
         @Override
         public void onDisplayRemoved(int displayId) {}
         @Override
         public void onDisplayChanged(int displayId) {
            activity.updateOrientation();
         }
      };
      DisplayManager displayManager = (DisplayManager) getApplicationContext().getSystemService(getApplicationContext().DISPLAY_SERVICE);
      displayManager.registerDisplayListener(displayListener, handler);

      nativeOnOrientationChange(getAllegroOrientation(), true);

      Log.d("AllegroActivity", "onStart end");
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

      DisplayManager displayManager = (DisplayManager) getApplicationContext().getSystemService(getApplicationContext().DISPLAY_SERVICE);
      displayManager.unregisterDisplayListener(displayListener);

      // TODO: Should we destroy the surface here?
      // onStop is paired with onRestart and onCreate with onDestroy -
      // if we destroy the surface here we need to handle onRestart to
      // recreate it but we currently do not.
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

      /* This is needed to get joysticks working again */
      reconfigureJoysticks();

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
         updateOrientation();
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

   private boolean isJoystick(int id) {
         InputDevice input = InputDevice.getDevice(id);
         int sources = input.getSources();

         // the device is a game controller if it has gamepad buttons, control sticks, or both
         boolean hasAnalogSticks = ((sources & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK);
         boolean hasGamepadButtons = ((sources & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD);

         return hasGamepadButtons || hasAnalogSticks;
   }

   public void reconfigureJoysticks() {
      joysticks.clear();

      int[] all = InputDevice.getDeviceIds();

      Log.d("AllegroActivity", "Number of input devices: " + all.length);

      for (int i = 0; i < all.length; i++) {
         if (isJoystick(all[i])) {
            joysticks.add(all[i]);
            Log.d("AllegroActivity", "Found joystick. Index=" + (joysticks.size()-1) + " id=" + all[i]);
         }
      }

      joystickReconfigureNotified = false;
   }

   public int getNumJoysticks() {
      return joysticks.size();
   }

   public int indexOfJoystick(int id) {
      return joysticks.indexOf(id, 0);
   }

   public String getJoystickName(int index) {
      if (index >= 0 && index < joysticks.size()) {
         int id = joysticks.get(index);
         InputDevice input = InputDevice.getDevice(id);

         if (input != null)
            return input.getName();
      }

      return "";
   }

   public void setJoystickActive() {
      joystickActive = true;
   }

   public void setJoystickInactive() {
      joystickActive = false;
   }

   private boolean got_clip = false;
   private String clip_text;
   private boolean set_clip = false;
   private boolean set_clip_result;

   public String getClipboardText() {
      got_clip = false;
      try {
         runOnUiThread(new Runnable() {
            @Override public void run() {
               clip_text = clipboard.getText();
               got_clip = true;
            }
         });
      }
      catch (Exception e) {
         Log.d("AllegroActivity", "getClipboardText failed");
         clip_text = "";
         got_clip = true;
      }
      while (got_clip == false);
      return clip_text;
   }

   public boolean hasClipboardText() {
      return clipboard.hasText();
   }

   public boolean setClipboardText(String text) {
      final String t = text;
      set_clip = false;
      try {
         runOnUiThread(new Runnable() {
            @Override public void run() {
               set_clip_result = clipboard.setText(t);
               set_clip = true;
            }
         });
      }
      catch (Exception e) {
         Log.d("AllegroActivity", "setClipboardText failed");
         set_clip_result = false;
         set_clip = true;
      }
      while (set_clip == false);
      return set_clip_result;
   }

   public void setAllegroFrameless(final boolean on) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
         runOnUiThread(new Runnable() {
            @Override public void run() {
               View view = AllegroActivity.this.getWindow().getDecorView();
               int flags = view.getSystemUiVisibility();
               int bits = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;
               if (on) flags |= bits; else flags &= ~bits;
               view.setSystemUiVisibility(flags);
            }
         });
      }
   }
}

/* vim: set sts=3 sw=3 et: */
