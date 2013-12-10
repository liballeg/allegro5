package org.liballeg.android;

import android.content.Context;
import android.util.Log;
import java.util.ArrayList;
import java.util.HashMap;
import javax.microedition.khronos.egl.*;

class AllegroEGL
{
   private static final String TAG = "AllegroEGL";

   private static final int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
   private static final int EGL_OPENGL_ES_BIT = 1;
   private static final int EGL_OPENGL_ES2_BIT = 4;

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

   /* instance members */
   private EGLContext egl_Context;
   private EGLSurface egl_Surface;
   private EGLDisplay egl_Display;
   private ArrayList<Integer> egl_attribWork = new ArrayList<Integer>();
   private EGLConfig[] egl_Config = new EGLConfig[] { null };

   boolean egl_Init()
   {
      Log.d(TAG, "egl_Init");
      EGL10 egl = (EGL10)EGLContext.getEGL();

      EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

      int[] egl_version = { 0, 0 };
      if (!egl.eglInitialize(dpy, egl_version)) {
         Log.d(TAG, "egl_Init fail");
         return false;
      }

      egl_Display = dpy;

      Log.d(TAG, "egl_Init end");
      return true;
   }

   void egl_Terminate()
   {
      egl_makeCurrent();

      egl_destroySurface();
      egl_destroyContext();

      EGL10 egl = (EGL10)EGLContext.getEGL();
      egl.eglTerminate(egl_Display);
      egl_Display = null;
   }

   void egl_setConfigAttrib(int attr, int value)
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();

      int egl_attr;
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
         default:
            /* Allow others to pass right into the array. */
            egl_attr = attr;
            break;
      }

      /* Check if it's already in the list, if so change the value. */
      for (int i = 0; i < egl_attribWork.size(); i += 2) {
         if (egl_attribWork.get(i) == egl_attr) {
            egl_attribWork.set(i + 1, value);
            return;
         }
      }

      /* Not in the list, add it. */
      egl_attribWork.add(egl_attr);
      egl_attribWork.add(value);
   }

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
      Log.d(TAG, "" + num_config[0] + " OpenGL ES 2 configurations found.");
      return num_config[0] > 0;
   }

   /* Return values:
    * 0 - failure
    * 1 - success
    * 2 - fell back to older ES version
    */
   int egl_createContext(int version)
   {
      Log.d(TAG, "egl_createContext");
      EGL10 egl = (EGL10)EGLContext.getEGL();

      egl_setConfigAttrib(EGL10.EGL_RENDERABLE_TYPE,
         (version == 2) ? EGL_OPENGL_ES2_BIT : EGL_OPENGL_ES_BIT);

      int[] egl_attribs = new int[egl_attribWork.size() + 1];
      for (int i = 0; i < egl_attribWork.size(); i++) {
         egl_attribs[i] = egl_attribWork.get(i);
      }
      egl_attribs[egl_attribWork.size()] = EGL10.EGL_NONE;

      int[] num = new int[1];
      boolean retval = egl.eglChooseConfig(egl_Display, egl_attribs,
         egl_Config, 1, num);
      if (!retval || num[0] < 1) {
         Log.e(TAG, "No matching config");
         return 0;
      }

      int[] es2_attrib = {
         EGL_CONTEXT_CLIENT_VERSION, version,
         EGL10.EGL_NONE
      };
      EGLContext ctx = egl.eglCreateContext(egl_Display, egl_Config[0],
         EGL10.EGL_NO_CONTEXT, es2_attrib);
      if (ctx == EGL10.EGL_NO_CONTEXT) {
         checkEglError("eglCreateContext", egl);
         Log.d(TAG, "egl_createContext no context");
         return 0;
      }

      Log.d(TAG, "EGL context created");

      egl_Context = ctx;

      Log.d(TAG, "egl_createContext end");

      return 1;
   }

   private void egl_destroyContext()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      Log.d(TAG, "destroying egl_Context");
      egl.eglDestroyContext(egl_Display, egl_Context);
      egl_Context = EGL10.EGL_NO_CONTEXT;
   }

   boolean egl_createSurface(AllegroSurface parent)
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      EGLSurface surface = egl.eglCreateWindowSurface(egl_Display,
         egl_Config[0], parent, null);
      if (surface == EGL10.EGL_NO_SURFACE) {
         Log.d(TAG, "egl_createSurface can't create surface: " +
               egl.eglGetError());
         return false;
      }

      if (!egl.eglMakeCurrent(egl_Display, surface, surface, egl_Context)) {
         egl.eglDestroySurface(egl_Display, surface);
         Log.d(TAG, "egl_createSurface can't make current");
         return false;
      }

      egl_Surface = surface;

      Log.d(TAG, "created new surface: " + surface);

      return true;
   }

   private void egl_destroySurface()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if (!egl.eglMakeCurrent(egl_Display, EGL10.EGL_NO_SURFACE,
            EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT))
      {
         Log.d(TAG, "could not clear current context");
      }

      Log.d(TAG, "destroying egl_Surface");
      egl.eglDestroySurface(egl_Display, egl_Surface);
      egl_Surface = EGL10.EGL_NO_SURFACE;
   }

   void egl_clearCurrent()
   {
      Log.d(TAG, "egl_clearCurrent");
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if (!egl.eglMakeCurrent(egl_Display, EGL10.EGL_NO_SURFACE,
               EGL10.EGL_NO_SURFACE, EGL10.EGL_NO_CONTEXT))
      {
         Log.d(TAG, "could not clear current context");
      }
      Log.d(TAG, "egl_clearCurrent done");
   }

   void egl_makeCurrent()
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      if (!egl.eglMakeCurrent(egl_Display, egl_Surface, egl_Surface, egl_Context)) {
         // egl.eglDestroySurface(egl_Display, surface);
         // egl.eglTerminate(egl_Display);
         // egl_Display = null;
         Log.d(TAG, "can't make thread current: ");
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
         Log.d(TAG, "inner exception: " + x.getMessage());
      }
   }
}

/* vim: set sts=3 sw=3 et: */
