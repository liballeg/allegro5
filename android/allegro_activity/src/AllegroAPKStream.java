package org.liballeg.android;

import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.text.TextUtils;
import android.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;

public class AllegroAPKStream
{
   AllegroActivity activity;
   String fn;
   InputStream in;
   private long pos;
   long fsize;
   boolean at_eof;

   public AllegroAPKStream(AllegroActivity activity, String filename)
   {
      this.activity = activity;
      // AssetManager does not interpret the path(!) so we do it here.
      fn = simplifyPath(filename);
      if (!fn.equals(filename)) {
         Log.d("APK", filename + " simplified to: " + fn);
      }
      pos = 0;
      at_eof = false;
   }

   private String simplifyPath(final String path)
   {
      final String[] pieces = path.split("/");
      ArrayList<String> keep = new ArrayList<String>();
      for (String piece : pieces) {
         if (piece.equals(""))
            continue;
         if (piece.equals("."))
            continue;
         if (piece.equals("..")) {
            if (keep.size() > 0)
               keep.remove(keep.size() - 1);
            continue;
         }
         keep.add(piece);
      }
      return TextUtils.join("/", keep);
   }

   private boolean reopen()
   {
      try {
         if (in != null) {
            in.close();
         }
         in = activity.getResources().getAssets().open(fn, AssetManager.ACCESS_RANDOM);
         in.mark((int)Math.pow(2, 31));
         if (in == null)
            return false;
         return true;
      }
      catch (IOException e) {
         Log.d("APK", "Got IOException in reopen. fn='" + fn + "'");
         return false;
      }
   }

   public boolean open()
   {
      try {
         AssetFileDescriptor fd = activity.getResources().getAssets().openFd(fn);
         fsize = fd.getLength();
         fd.close();
      }
      catch (IOException e) {
         fsize = 0;
      }

      boolean res = reopen();
      if (!res)
         return false;

      return true;
   }

   public void close()
   {
      try {
         in.close();
      }
      catch (IOException e) {
         Log.d("APK", "IOException in close");
      }
   }

   private void force_skip(long n)
   {
      if (n <= 0)
         return;

      try {
         /* NOTE: in.skip doesn't work here! */
         byte[] b = new byte[(int)n];
         while (n > 0) {
            long res = in.read(b, 0, (int)n);
            if (res <= 0)
               break;
            pos += res;
            n -= res;
         }
      }
      catch (IOException e) {
      }
   }

   public boolean seek(long seekto)
   {
      at_eof = false;

      if (seekto >= pos) {
         long seek_ahead = seekto - pos;
         force_skip(seek_ahead);
      }
      else {
         try {
            in.reset();
         }
         catch (IOException e) {
            reopen();
         }
         pos = 0;
         force_skip(seekto);
      }
      return true;
   }

   public long tell()
   {
      return pos;
   }

   public int read(byte[] b)
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
         Log.d("APK", "IOException in read");
         return -1;
      }
   }

   public long size()
   {
      return fsize;
   }

   public boolean eof()
   {
      return at_eof;
   }

   public void ungetc(int c)
   {
      /* XXX unimplemented? */
   }
}

/* vim: set sts=3 sw=3 et: */
