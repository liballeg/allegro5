package org.liballeg.android;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.util.Log;
import java.io.IOException;
import java.io.InputStream;

class AllegroAPKStream
{
   private static final String TAG = "AllegroAPKStream";

   private AllegroActivity activity;
   private String fn;
   private InputStream in;
   private long pos = 0;
   private long fsize = -1;
   private boolean at_eof = false;

   AllegroAPKStream(AllegroActivity activity, String filename)
   {
      this.activity = activity;
      fn = Path.simplifyPath(filename);
      if (!fn.equals(filename)) {
         Log.d(TAG, filename + " simplified to: " + fn);
      }
   }

   boolean open()
   {
      try {
         AssetFileDescriptor fd;
         fd = activity.getResources().getAssets().openFd(fn);
         fsize = fd.getLength();
         fd.close();
      }
      catch (IOException e) {
         Log.w(TAG, "could not get file size: " + e.toString());
         fsize = -1;
      }

      return reopen();
   }

   boolean reopen()
   {
      if (in != null) {
         close();
         in = null;
      }

      try {
         in = activity.getResources().getAssets().open(fn,
            AssetManager.ACCESS_RANDOM);
      }
      catch (IOException e) {
         Log.d(TAG, "Got IOException in reopen. fn='" + fn + "'");
         return false;
      }

      in.mark((int)Math.pow(2, 31));
      pos = 0;
      at_eof = false;
      return true;
   }

   void close()
   {
      try {
         in.close();
         in = null;
      }
      catch (IOException e) {
         Log.d(TAG, "IOException in close");
      }
   }

   boolean seek(long seekto)
   {
      at_eof = false;

      if (seekto >= pos) {
         long seek_ahead = seekto - pos;
         return force_skip(seek_ahead);
      }

      /* Seek backwards by rewinding to start of file first. */
      try {
         in.reset();
         pos = 0;
      }
      catch (IOException e) {
         if (!reopen()) {
            /* Leaves pos wherever it lands! */
            return false;
         }
      }
      return force_skip(seekto);
   }

   private boolean force_skip(long n)
   {
      if (n <= 0)
         return true;

      /* NOTE: in.skip doesn't work here! */
      byte[] b = new byte[(int)n];
      while (n > 0) {
         int res;
         try {
            res = in.read(b, 0, (int)n);
         } catch (IOException e) {
            Log.d(TAG, "IOException: " + e.toString());
            return false;
         }
         if (res <= 0)
            break;
         pos += res;
         n -= res;
      }
      return true;
   }

   long tell()
   {
      return pos;
   }

   int read(byte[] b)
   {
      try {
         int ret = in.read(b);
         if (ret > 0)
            pos += ret;
         else if (ret == -1) {
            at_eof = true;
         }
         return ret;
      }
      catch (IOException e) {
         Log.d(TAG, "IOException in read");
         return -1;
      }
   }

   long size()
   {
      return fsize;
   }

   boolean eof()
   {
      return at_eof;
   }
}

/* vim: set sts=3 sw=3 et: */
