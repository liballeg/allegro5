package org.liballeg.android;

import android.content.Context;
import android.graphics.Canvas;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnGenericMotionListener;
import android.view.MotionEvent;
import android.view.InputDevice;

class AllegroSurface extends SurfaceView implements SurfaceHolder.Callback, OnGenericMotionListener
{
   /** native functions we call */
   public native void nativeOnCreate();
   public native boolean nativeOnDestroy();
   public native void nativeOnChange(int format, int width, int height);
   public native void nativeOnJoystickAxis(int index, int stick, int axis, float value);
   public native void nativeOnJoystickButton(int index, int button, boolean down);

   private AllegroActivity activity;

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

   int egl_createContext(int configIndex, boolean programmable_pipeline)
   {
      return egl.egl_createContext(configIndex, programmable_pipeline);
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
      setOnGenericMotionListener(this);
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

   private float axis0_x = 0.0f;
   private float axis0_y = 0.0f;
   private float axis0_hat_x = 0.0f;
   private float axis0_hat_y = 0.0f;
   private float axis1_x = 0.0f;
   private float axis1_y = 0.0f;

   private void handleHat(int index1, float old, float cur, int button1, int button2) {
      if (old == cur)
         return;

      if (old == 0) {
         if (cur < 0)
            nativeOnJoystickButton(index1, button1, true);
         else
            nativeOnJoystickButton(index1, button2, true);
      }
      else if (old < 0) {
         nativeOnJoystickButton(index1, button1, false);
         if (cur > 0) {
            nativeOnJoystickButton(index1, button2, true);
         }
      }
      else if (old > 0) {
         nativeOnJoystickButton(index1, button2, false);
         if (cur < 0) {
            nativeOnJoystickButton(index1, button1, true);
         }
      }
   }

   @Override
   public boolean onGenericMotion(View v, MotionEvent event) {
      if (activity.joystickActive == false) {
         return false;
      }

      int id = event.getDeviceId();
      int index = activity.indexOfJoystick(id);
      if (index >= 0) {
         float ax = event.getAxisValue(MotionEvent.AXIS_X, 0);
         float ay = event.getAxisValue(MotionEvent.AXIS_Y, 0);
         float ahx = event.getAxisValue(MotionEvent.AXIS_HAT_X, 0);
         float ahy = event.getAxisValue(MotionEvent.AXIS_HAT_Y, 0);
         float az = event.getAxisValue(MotionEvent.AXIS_Z, 0);
         float arz = event.getAxisValue(MotionEvent.AXIS_RZ, 0);
         if (ax != axis0_x || ay != axis0_y) {
            nativeOnJoystickAxis(index, 0, 0, ax);
            nativeOnJoystickAxis(index, 0, 1, ay);
            axis0_x = ax;
            axis0_y = ay;
         }
         else if (ahx != axis0_hat_x || ahy != axis0_hat_y) {
            handleHat(index, axis0_hat_x, ahx, AllegroActivity.JS_DPAD_L, AllegroActivity.JS_DPAD_R);
            handleHat(index, axis0_hat_y, ahy, AllegroActivity.JS_DPAD_U, AllegroActivity.JS_DPAD_D);
            axis0_hat_x = ahx;
            axis0_hat_y = ahy;
         }
         if (az != axis1_x || arz != axis1_y) {
            nativeOnJoystickAxis(index, 1, 0, az);
            nativeOnJoystickAxis(index, 1, 1, arz);
            axis1_x = az;
            axis1_y = arz;
         }
         return true;
      }
      return false;
   }
}

/* vim: set sts=3 sw=3 et: */
