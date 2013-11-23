package org.liballeg.android;

import android.content.Context;
import android.graphics.Canvas;
import android.media.AudioManager;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import java.util.ArrayList;
import java.util.HashMap;
import javax.microedition.khronos.egl.*;
import javax.microedition.khronos.opengles.GL10;

class AllegroSurface extends SurfaceView implements SurfaceHolder.Callback,
   View.OnKeyListener, View.OnTouchListener
{
   static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098; 

   /** native functions we call */
   public native void nativeOnCreate();
   public native boolean nativeOnDestroy();
   public native void nativeOnChange(int format, int width, int height);
   public native void nativeOnKeyDown(int key);
   public native void nativeOnKeyUp(int key);
   public native void nativeOnKeyChar(int key, int unichar);
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

   boolean egl_Init()
   {
      Log.d("AllegroSurface", "egl_Init");
      EGL10 egl = (EGL10)EGLContext.getEGL();

      EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

      if (!egl.eglInitialize(dpy, egl_Version)) {
         Log.d("AllegroSurface", "egl_Init fail");
         return false;
      }

      egl_Display = dpy;

      Log.d("AllegroSurface", "egl_Init end");
      return true;
   }

   void egl_setConfigAttrib(int attr, int value)
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();

      int egl_attr = attr;

      switch (attr) {
         case Const.ALLEGRO_RED_SIZE:
            egl_attr = egl.EGL_RED_SIZE;
            break;

         case Const.ALLEGRO_GREEN_SIZE:
            egl_attr = egl.EGL_GREEN_SIZE;
            break;

         case Const.ALLEGRO_BLUE_SIZE:
            egl_attr = egl.EGL_BLUE_SIZE;
            break;

         case Const.ALLEGRO_ALPHA_SIZE:
            egl_attr = egl.EGL_ALPHA_SIZE;
            break;

         case Const.ALLEGRO_DEPTH_SIZE:
            egl_attr = egl.EGL_DEPTH_SIZE;
            break;

         case Const.ALLEGRO_STENCIL_SIZE:
            egl_attr = egl.EGL_STENCIL_SIZE;
            break;

         case Const.ALLEGRO_SAMPLE_BUFFERS:
            egl_attr = egl.EGL_SAMPLE_BUFFERS;
            break;

         case Const.ALLEGRO_SAMPLES:
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

   private boolean checkGL20Support(Context context)
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

   private static HashMap<Integer, String> eglErrors;
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
   int egl_createContext(int version)
   {
      Log.d("AllegroSurface", "egl_createContext");
      EGL10 egl = (EGL10)EGLContext.getEGL();
      int ret = 1;

      es2_attrib[1] = version;

      egl_setConfigAttrib(EGL10.EGL_RENDERABLE_TYPE,
         (version == 2) ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT);

      boolean color_size_specified = false;
      for (int i = 0; i < egl_attribWork.size(); i++) {
         Log.d("AllegroSurface", "egl_attribs[" + i + "] = " + egl_attribWork.get(i));
         if (i % 2 == 0) {
            if (egl_attribWork.get(i) == EGL10.EGL_RED_SIZE ||
               egl_attribWork.get(i) == EGL10.EGL_GREEN_SIZE ||
               egl_attribWork.get(i) == EGL10.EGL_BLUE_SIZE)
            {
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
      boolean retval = egl.eglChooseConfig(egl_Display, egl_attribs,
         egl_Config, 1, num);
      if (retval == false || num[0] < 1) {
         Log.e("AllegroSurface", "No matching config");
         return 0;
      }

      EGLContext ctx = egl.eglCreateContext(egl_Display, egl_Config[0],
         EGL10.EGL_NO_CONTEXT, es2_attrib);
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

   private void egl_destroyContext()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      Log.d("AllegroSurface", "destroying egl_Context");
      egl.eglDestroyContext(egl_Display, egl_Context);
      egl_Context = EGL10.EGL_NO_CONTEXT;
   }

   boolean egl_createSurface()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      EGLSurface surface = egl.eglCreateWindowSurface(egl_Display,
         egl_Config[0], this, null);
      if (surface == EGL10.EGL_NO_SURFACE) {
         Log.d("AllegroSurface", "egl_createSurface can't create surface (" +  egl.eglGetError() + ")");
         return false;
      }

      if (!egl.eglMakeCurrent(egl_Display, surface, surface, egl_Context)) {
         egl.eglDestroySurface(egl_Display, surface);
         Log.d("AllegroSurface", "egl_createSurface can't make current");
         return false;
      }

      egl_Surface = surface;

      Log.d("AllegroSurface", "created new surface: " + surface);

      return true;
   }

   private void egl_destroySurface()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if (!egl.eglMakeCurrent(egl_Display, EGL10.EGL_NO_SURFACE,
            EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT))
      {
         Log.d("AllegroSurface", "could not clear current context");
      }

      Log.d("AllegroSurface", "destroying egl_Surface");
      egl.eglDestroySurface(egl_Display, egl_Surface);
      egl_Surface = EGL10.EGL_NO_SURFACE;
   }

   void egl_clearCurrent()
   {
      Log.d("AllegroSurface", "egl_clearCurrent");
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if (!egl.eglMakeCurrent(egl_Display, EGL10.EGL_NO_SURFACE,
               EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT))
      {
         Log.d("AllegroSurface", "could not clear current context");
      }
      Log.d("AllegroSurface", "egl_clearCurrent done");
   }

   void egl_makeCurrent()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if (!egl.eglMakeCurrent(egl_Display, egl_Surface, egl_Surface, egl_Context)) {
         // egl.eglDestroySurface(egl_Display, surface);
         // egl.eglTerminate(egl_Display);
         // egl_Display = null;
         Log.d("AllegroSurface", "can't make thread current: ");
         checkEglError("eglMakeCurrent", egl);
      }  
   }

   void egl_SwapBuffers()
   {
      try {
         EGL10 egl = (EGL10)EGLContext.getEGL();

         // FIXME: Pretty sure flush is implicit with SwapBuffers
         //egl.eglWaitNative(EGL10.EGL_NATIVE_RENDERABLE, null);
         //egl.eglWaitGL();

         egl.eglSwapBuffers(egl_Display, egl_Surface);
         checkEglError("eglSwapBuffers", egl);

      } catch (Exception x) {
         Log.d("AllegroSurface", "inner exception: " + x.getMessage());
      }
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

   private void grabFocus()
   {
      Log.d("AllegroSurface", "Grabbing focus");

      setFocusable(true);
      setFocusableInTouchMode(true);
      requestFocus();
      setOnKeyListener(this); 
      setOnTouchListener(this);
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

      egl_makeCurrent();

      egl_destroySurface();
      egl_destroyContext();

      EGL10 egl = (EGL10)EGLContext.getEGL();
      egl.eglTerminate(egl_Display);
      egl_Display = null;

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

   /* Maybe dump a stacktrace and die, rather than logging and ignoring errors.
    * All this fancyness is so we work on as many versions of Android as
    * possible, and gracefully degrade, rather than just outright failing.
    */

   public void setCaptureVolumeKeys(boolean onoff)
   {
      captureVolume = onoff;
   }

   private void volumeChange(int inc)
   {
      AudioManager mAudioManager =
         (AudioManager)context.getApplicationContext()
         .getSystemService(Context.AUDIO_SERVICE);

      int curr = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
      int max = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
      int vol = curr + inc;

      if (0 <= vol && vol <= max) {
         // Set a new volume level manually with the FLAG_SHOW_UI flag.
         mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, vol,
            AudioManager.FLAG_SHOW_UI);
      }
   }

   @Override
   public boolean onKey(View v, int keyCode, KeyEvent event)
   {
      int unichar;
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
         if (Key.alKey(keyCode) == Key.ALLEGRO_KEY_BACKSPACE) {
            unichar = '\b';
         }
         else if (Key.alKey(keyCode) == Key.ALLEGRO_KEY_ENTER) {
            unichar = '\r';
         }
         else {
            unichar = event.getUnicodeChar();
         }

         if (event.getRepeatCount() == 0) {
            nativeOnKeyDown(Key.alKey(keyCode));
            nativeOnKeyChar(Key.alKey(keyCode), unichar);
         }
         else {
            nativeOnKeyChar(Key.alKey(keyCode), unichar);
         }
         return true;
      }
      else if (event.getAction() == KeyEvent.ACTION_UP) {
         nativeOnKeyUp(Key.alKey(keyCode));
         return true;
      }

      return false;
   }

   // FIXME: Pull out android version detection into the setup and just check
   // some flags here, rather than checking for the existance of the fields and
   // methods over and over.
   @Override
   public boolean onTouch(View v, MotionEvent event)
   {
      //Log.d("AllegroSurface", "onTouch");
      int action = 0;
      int pointer_id = 0;

      Class[] no_args = new Class[0];
      Class[] int_arg = new Class[1];
      int_arg[0] = int.class;

      if (Reflect.methodExists(event, "getActionMasked")) { // android-8 / 2.2.x
         action = Reflect.<Integer>callMethod(event, "getActionMasked", no_args);
         int ptr_idx = Reflect.<Integer>callMethod(event, "getActionIndex", no_args);
         pointer_id = Reflect.<Integer>callMethod(event, "getPointerId", int_arg, ptr_idx);
      } else {
         int raw_action = event.getAction();

         if (Reflect.fieldExists(event, "ACTION_MASK")) { // android-5 / 2.0
            int mask = Reflect.<Integer>getField(event, "ACTION_MASK");
            action = raw_action & mask;

            int ptr_id_mask = Reflect.<Integer>getField(event, "ACTION_POINTER_ID_MASK");
            int ptr_id_shift = Reflect.<Integer>getField(event, "ACTION_POINTER_ID_SHIFT");

            pointer_id = event.getPointerId((raw_action & ptr_id_mask) >> ptr_id_shift);
         }
         else { // android-4 / 1.6
            /* no ACTION_MASK? no multi touch, no pointer_id, */
            action = raw_action;
         }
      }

      boolean primary = false;

      if (action == MotionEvent.ACTION_DOWN) {
         primary = true;
         action = Const.ALLEGRO_EVENT_TOUCH_BEGIN;
      }
      else if (action == MotionEvent.ACTION_UP) {
         primary = true;
         action = Const.ALLEGRO_EVENT_TOUCH_END;
      }
      else if (action == MotionEvent.ACTION_MOVE) {
         action = Const.ALLEGRO_EVENT_TOUCH_MOVE;
      }
      else if (action == MotionEvent.ACTION_CANCEL) {
         action = Const.ALLEGRO_EVENT_TOUCH_CANCEL;
      }
      // android-5 / 2.0
      else if (Reflect.fieldExists(event, "ACTION_POINTER_DOWN") &&
         action == Reflect.<Integer>getField(event, "ACTION_POINTER_DOWN"))
      {
         action = Const.ALLEGRO_EVENT_TOUCH_BEGIN;
      }
      else if (Reflect.fieldExists(event, "ACTION_POINTER_UP") &&
         action == Reflect.<Integer>getField(event, "ACTION_POINTER_UP"))
      {
         action = Const.ALLEGRO_EVENT_TOUCH_END;
      }
      else {
         Log.v("AllegroSurface", "unknown MotionEvent type: " + action);
         Log.d("AllegroSurface", "onTouch endf");
         return false;
      }

      if (Reflect.methodExists(event, "getPointerCount")) { // android-5 / 2.0
         int pointer_count = Reflect.<Integer>callMethod(event, "getPointerCount", no_args);
         for(int i = 0; i < pointer_count; i++) {
            float x = Reflect.<Float>callMethod(event, "getX", int_arg, i);
            float y = Reflect.<Float>callMethod(event, "getY", int_arg, i);
            int  id = Reflect.<Integer>callMethod(event, "getPointerId", int_arg, i);

            /* not entirely sure we need to report move events for non primary touches here
             * but examples I've see say that the ACTION_[POINTER_][UP|DOWN]
             * report all touches and they can change between the last MOVE
             * and the UP|DOWN event */
            if (id == pointer_id) {
               nativeOnTouch(id, action, x, y, primary);
            } else {
               nativeOnTouch(id, Const.ALLEGRO_EVENT_TOUCH_MOVE, x, y, primary);
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
