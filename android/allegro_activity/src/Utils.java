package org.liballeg.app;

import java.lang.reflect.Method;

final class Utils
{
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
}

/* vim: set sts=3 sw=3 et: */
