package org.liballeg.android;

import android.content.Context;
import android.graphics.Canvas;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

class AllegroSurface extends SurfaceView implements SurfaceHolder.Callback
{
   /** native functions we call */
   public native void nativeOnCreate();
   public native boolean nativeOnDestroy();
   public native void nativeOnChange(int format, int width, int height);
   public native void nativeOnJoystickAxis(int index, int stick, int axis, float value);
   public native void nativeOnJoystickButton(int index, int button, boolean down);

   private AllegroActivity activity;
   private AllegroJoystick joystick_listener;

   /** functions that native code calls */

   boolean egl_Init()
   {
      return egl.egl_Init();
   }

   void egl_initRequiredAttribs()
   {
      egl.egl_initRequiredAttribs();
   }

   void egl_setRequiredAttrib(int attr, int value)
   {
      egl.egl_setRequiredAttrib(attr, value);
   }

   int egl_chooseConfig(boolean programmable_pipeline)
   {
      return egl.egl_chooseConfig(programmable_pipeline);
   }

   void egl_getConfigAttribs(int index, int ret[])
   {
      egl.egl_getConfigAttribs(index, ret);
   }

   int egl_createContext(int configIndex, boolean programmable_pipeline,
      int major, int minor, boolean isRequiredMajor, boolean isRequiredMinor)
   {
      return egl.egl_createContext(configIndex, programmable_pipeline,
         major, minor, isRequiredMajor, isRequiredMinor);
   }

   boolean egl_createSurface()
   {
      return egl.egl_createSurface(this);
   }

   void egl_clearCurrent()
   {
      egl.egl_clearCurrent();
   }

   void egl_makeCurrent()
   {
      egl.egl_makeCurrent();
   }

   void egl_SwapBuffers()
   {
      egl.egl_SwapBuffers();
   }

   /** main handlers */

   private AllegroEGL egl;
   private KeyListener key_listener;
   private TouchListener touch_listener;

   public AllegroSurface(Context context, Display display, AllegroActivity activity)
   {
      super(context);

      Log.d("AllegroSurface", "PixelFormat=" + display.getPixelFormat());
      getHolder().setFormat(display.getPixelFormat());
      getHolder().addCallback(this); 

      this.activity = activity;
      this.egl = new AllegroEGL();
      this.key_listener = new KeyListener(context, activity);
      this.touch_listener = new TouchListener();
   }

   private void grabFocus()
   {
      Log.d("AllegroSurface", "Grabbing focus");

      setFocusable(true);
      setFocusableInTouchMode(true);
      requestFocus();
      setOnKeyListener(key_listener);
      setOnTouchListener(touch_listener);

      if (android.os.Build.VERSION.SDK_INT >= 12) {
         joystick_listener = new AllegroJoystick(activity, this);
         setOnGenericMotionListener(joystick_listener);
      }
   }

   @Override
   public void surfaceCreated(SurfaceHolder holder)
   {
      Log.d("AllegroSurface", "surfaceCreated");
      nativeOnCreate();
      grabFocus();
      Log.d("AllegroSurface", "surfaceCreated end");
   }

   @Override
   public void surfaceDestroyed(SurfaceHolder holder)
   {
      Log.d("AllegroSurface", "surfaceDestroyed");

      if (!nativeOnDestroy()) {
         Log.d("AllegroSurface", "No surface created, returning early");
         return;
      }

      egl.egl_Terminate();

      Log.d("AllegroSurface", "surfaceDestroyed end");
   }

   @Override
   public void surfaceChanged(SurfaceHolder holder, int format, int width, int height)
   {
      Log.d("AllegroSurface", "surfaceChanged (width=" + width + " height=" + height + ")");
      nativeOnChange(0xdeadbeef, width, height);
      Log.d("AllegroSurface", "surfaceChanged end");
   }

   /* unused */
   @Override
   public void onDraw(Canvas canvas)
   {
   }

   /* Events */

   /* XXX not exposed in C API yet */
   void setCaptureVolumeKeys(boolean onoff)
   {
      key_listener.setCaptureVolumeKeys(onoff);
   }
}

/* vim: set sts=3 sw=3 et: */
