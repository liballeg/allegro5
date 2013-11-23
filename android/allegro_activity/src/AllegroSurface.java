package org.liballeg.android;

import android.content.Context;
import android.graphics.Canvas;
import android.util.Log;
import android.view.Display;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import java.util.ArrayList;
import java.util.HashMap;
import javax.microedition.khronos.egl.*;

class AllegroSurface extends SurfaceView implements SurfaceHolder.Callback
{
   static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098; 

   /** native functions we call */
   public native void nativeOnCreate();
   public native boolean nativeOnDestroy();
   public native void nativeOnChange(int format, int width, int height);

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

   private KeyListener key_listener;
   private TouchListener touch_listener;

   public AllegroSurface(Context context, Display display)
   {
      super(context);

      Log.d("AllegroSurface", "PixelFormat=" + display.getPixelFormat());
      getHolder().setFormat(display.getPixelFormat());
      getHolder().addCallback(this); 

      this.key_listener = new KeyListener(context);
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

   /* XXX not exposed in C API yet */
   void setCaptureVolumeKeys(boolean onoff)
   {
      key_listener.setCaptureVolumeKeys(onoff);
   }
}

/* vim: set sts=3 sw=3 et: */
