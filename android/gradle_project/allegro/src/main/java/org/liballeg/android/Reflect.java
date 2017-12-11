package org.liballeg.android;

import android.util.Log;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;

final class Reflect
{
   static final String TAG = "Reflect";

   static boolean fieldExists(Object obj, String fieldName)
   {
      try {
         Class cls = obj.getClass();
         Field m = cls.getField(fieldName);
         return true;
      } catch (Exception x) {
         return false;
      }
   }

   static boolean methodExists(Object obj, String methName)
   {
      try {
         Class cls = obj.getClass();
         Method m = cls.getMethod(methName);
         return true;
      } catch (Exception x) {
         return false;
      }
   }

   static <T> T getField(Object obj, String field)
   {
      try {
         Class cls = obj.getClass();
         Field f = cls.getField(field);
         return (T)f.get(obj);
      }
      catch (NoSuchFieldException x) {
         Log.v(TAG, "field " + field + " not found in class " +
               obj.getClass().getCanonicalName());
         return null;
      }
      catch (IllegalArgumentException x) {
         Log.v(TAG, "IllegalArgumentException when getting field " + field +
               " from class " + obj.getClass().getCanonicalName());
         return null;
      }
      catch (IllegalAccessException x) {
         Log.v(TAG, "field " + field + " is not accessible in class " +
               obj.getClass().getCanonicalName());
         return null;
      }
   }

   static <T>
   T callMethod(Object obj, String methName, Class [] types, Object... args)
   {
      try {
         Class cls = obj.getClass();
         Method m = cls.getMethod(methName, types);
         return (T)m.invoke(obj, args);
      }
      catch (NoSuchMethodException x) {
         Log.v(TAG, "method " + methName + " does not exist in class " +
               obj.getClass().getCanonicalName());
         return null;
      }
      catch (NullPointerException x) {
         Log.v(TAG, "can't invoke null method name");
         return null;
      }
      catch (SecurityException x) {
         Log.v(TAG, "method " + methName + " is inaccessible in class " +
               obj.getClass().getCanonicalName());
         return null;
      }
      catch (IllegalAccessException x) {
         Log.v(TAG, "method " + methName + " is inaccessible in class " +
               obj.getClass().getCanonicalName());
         return null;
      }
      catch (InvocationTargetException x) {
         Log.v(TAG, "method " + methName + " threw an exception");
         return null;
      }
   }
}

/* vim: set sts=3 sw=3 et: */
