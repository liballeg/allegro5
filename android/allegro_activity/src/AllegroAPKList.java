package org.liballeg.android;
import java.io.IOException;
import android.util.Log;
import java.io.InputStream;

class AllegroAPKList
{
   static String list(AllegroActivity activity, String path)
   {
      /* The getAssets().list() method is *very* finicky about asset
       * names, there must be no leading, trailing or double slashes
       * or it fails.
       */
      while (path.startsWith("/"))
         path = path.substring(1);
      while (path.endsWith("/"))
         path = path.substring(0, path.length() - 1);
      try {
         String[] files = activity.getResources().getAssets().list(path);
         String ret = "";
         for (String file : files) {
            if (!ret.isEmpty())
               ret += ";";
            ret += file;

            /* We cannot use openfs as that causes an exception on any
             * compressed files. The only thing we can do is open a
             * stream - if that fails we assume it is a directory,
             * otherwise a normal file.
             */
            try {
               file = path + "/" + file;
               while (file.startsWith("/"))
                  file = file.substring(1);
               InputStream s = activity.getResources().getAssets().open(file);
               s.close();
            }
            catch (IOException e) {
               ret += "/";
            }
         }
         return ret;
      }
      catch (IOException e) {
         Log.d("APK", e.toString());
         return "";
      }
   }
}
