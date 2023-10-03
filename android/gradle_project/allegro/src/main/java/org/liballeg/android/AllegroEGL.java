package org.liballeg.android;

import android.content.Context;
import android.util.Log;
import java.util.ArrayList;
import java.util.HashMap;
import javax.microedition.khronos.egl.*;

class AllegroEGL
{
   private static final String TAG = "AllegroEGL";

   private static final int EGL_CONTEXT_MAJOR_VERSION = 0x3098;
   private static final int EGL_CONTEXT_MINOR_VERSION = 0x30fb;
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
   private HashMap<Integer, Integer> attribMap;
   private EGLConfig[] matchingConfigs;
   private EGLConfig chosenConfig;

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

      Log.d(TAG, "egl_Init OpenGL ES " + egl_version[0] + "." + egl_version[1]);
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

   void egl_initRequiredAttribs()
   {
      attribMap = new HashMap();
   }

   void egl_setRequiredAttrib(int attr, int value)
   {
      int egl_attr = eglAttrib(attr);
      if (egl_attr >= 0) {
         attribMap.put(egl_attr, value);
      }
   }

   private int eglAttrib(int al_attr)
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();
      final int[] mapping = attribMapping(egl);

      for (int i = 0; i < mapping.length; i += 2) {
         if (al_attr == mapping[i + 1])
            return mapping[i];
      }

      assert(false);
      return -1;
   }

   private final int[] attribMapping(EGL10 egl)
   {
      return new int[] {
         egl.EGL_RED_SIZE,       Const.ALLEGRO_RED_SIZE,
         egl.EGL_GREEN_SIZE,     Const.ALLEGRO_GREEN_SIZE,
         egl.EGL_BLUE_SIZE,      Const.ALLEGRO_BLUE_SIZE, 
         egl.EGL_ALPHA_SIZE,     Const.ALLEGRO_ALPHA_SIZE, 
         egl.EGL_BUFFER_SIZE,    Const.ALLEGRO_COLOR_SIZE,
         egl.EGL_DEPTH_SIZE,     Const.ALLEGRO_DEPTH_SIZE, 
         egl.EGL_STENCIL_SIZE,   Const.ALLEGRO_STENCIL_SIZE, 
         egl.EGL_SAMPLE_BUFFERS, Const.ALLEGRO_SAMPLE_BUFFERS, 
         egl.EGL_SAMPLES,        Const.ALLEGRO_SAMPLES
      };
   }

   int egl_chooseConfig(boolean programmable_pipeline)
   {
      EGL10 egl = (EGL10)EGLContext.getEGL();

      if (programmable_pipeline) {
         attribMap.put(EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT);
      } else {
         attribMap.put(EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT);
      }

      /* Populate the matchingConfigs array. */
      matchingConfigs = new EGLConfig[20];
      int[] num = new int[1];
      boolean ok = egl.eglChooseConfig(egl_Display, requiredAttribsArray(),
         matchingConfigs, matchingConfigs.length, num);
      if (!ok || num[0] < 1) {
         Log.e(TAG, "No matching config");
         return 0;
      }

      Log.d(TAG, "eglChooseConfig returned " + num[0] + " configurations.");
      return num[0];
   }

   private int[] requiredAttribsArray()
   {
      final int n = attribMap.size();
      final int[] arr = new int[n * 2 + 1];
      int i = 0;

      for (int attrib : attribMap.keySet()) {
         arr[i++] = attrib;
         arr[i++] = attribMap.get(attrib);
      }
      arr[i] = EGL10.EGL_NONE; /* sentinel */
      return arr;
   }

   void egl_getConfigAttribs(int index, int ret[])
   {
      Log.d(TAG, "Getting attribs for config at index " + index);

      for (int i = 0; i < ret.length; i++) {
         ret[i] = 0;
      }

      final EGL10 egl = (EGL10)EGLContext.getEGL();
      final EGLConfig config = matchingConfigs[index];
      final int[] mapping = attribMapping(egl);
      final int box[] = new int[1];

      for (int i = 0; i < mapping.length; i += 2) {
         int egl_attr = mapping[i];
         int al_attr = mapping[i + 1];

         if (egl.eglGetConfigAttrib(egl_Display, config, egl_attr, box)) {
            ret[al_attr] = box[0];
         } else {
            Log.e(TAG, "eglGetConfigAttrib(" + egl_attr + ") failed\n");
         }
      }
   }

   private int[] versionAttribList(int major, int minor)
   {
      return new int[] {
         EGL_CONTEXT_MAJOR_VERSION, major,
         EGL_CONTEXT_MINOR_VERSION, minor,
         EGL10.EGL_NONE
      };
   }

   private int versionCode(int major, int minor)
   {
      return (major << 8) | minor;
   }

   /* Return values:
    * 0 - failure
    * 1 - success
    */
   int egl_createContext(int configIndex, boolean programmable_pipeline,
      int major, int minor, boolean isRequiredMajor, boolean isRequiredMinor)
   {
      Log.d(TAG, "egl_createContext");

      EGL10 egl = (EGL10)EGLContext.getEGL();

      chosenConfig = matchingConfigs[configIndex];
      matchingConfigs = null;
      attribMap = null;

      // we'll attempt to create a GLES context of version major.minor.
      // if major == minor == 0, then the user did not request a specific version.
      // minMajor.minMinor is the minimum acceptable version.

      int minMajor = (programmable_pipeline || major >= 2) ? 2 : 1;
      int minMinor = 0;

      if (isRequiredMajor && major > minMajor)
         minMajor = major;
      if (isRequiredMinor && minor > minMinor)
         minMinor = minor;

      int minVersion = versionCode(minMajor, minMinor);
      int wantedVersion = versionCode(major, minor);
      if (wantedVersion < minVersion) {
         if(isRequiredMajor || isRequiredMinor) {
            Log.d(TAG, "Can't require OpenGL ES version " + major + "." + minor);
            return 0;
         }

         major = minMajor;
         minor = minMinor;
         wantedVersion = minVersion;
      }

      Log.d(TAG, "egl_createContext: requesting OpenGL ES " + major + "." + minor);

      // request a GLES context version major.minor
      EGLContext ctx = egl.eglCreateContext(egl_Display, chosenConfig,
         EGL10.EGL_NO_CONTEXT, versionAttribList(major, minor));
      if (ctx == EGL10.EGL_NO_CONTEXT) {
         // failed to create a GLES context of the requested version
         checkEglError("eglCreateContext", egl);
         Log.d(TAG, "egl_createContext failed. min version is " +
            minMajor + "." + minMinor);

         // try the min version instead, unless the user required the failed version
         if ((wantedVersion == minVersion) || (EGL10.EGL_NO_CONTEXT == (ctx =
            egl.eglCreateContext(egl_Display, chosenConfig, EGL10.EGL_NO_CONTEXT,
            versionAttribList(minMajor, minMinor))
         ))) {
            // failed again
            checkEglError("eglCreateContext", egl);
            Log.d(TAG, "egl_createContext no context");
            return 0;
         }
      }

      Log.d(TAG, "egl_createContext: success");

      egl_Context = ctx;
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
         chosenConfig, parent, null);
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
