package org.liballeg.android;

import java.io.InputStream;
import android.util.Log;

public class AllegroInputStream extends InputStream
{
   private static final String TAG = "AllegroInputStream";

   private int handle;

   public native int nativeRead(int handle, byte[] buffer, int offset, int length);
   public native void nativeClose(int handle);

   public AllegroInputStream(int handle)
   {
      super();
      this.handle = handle;
      Log.d(TAG, "ctor handle:" + handle);
   }

   @Override
   public int available()
   {
      Log.d(TAG, "available");
      return 0;
   }

   @Override
   public void close()
   {
      Log.d(TAG, "close");
      nativeClose(handle);
   }

   @Override
   public void mark(int limit)
   {
      Log.d(TAG, "mark " + limit);
   }

   @Override
   public boolean markSupported()
   {
      Log.d(TAG, "markSupported");
      return false;
   }

   @Override
   public int read()
   {
      byte buffer[] = new byte[1];
      int ret = read(buffer, 0, buffer.length);
      if (ret != -1)
         return buffer[0];
      else
         return -1;
   }

   @Override
   public int read(byte[] buffer)
   {
      return read(buffer, 0, buffer.length);
   }

   @Override
   public int read(byte[] buffer, int offset, int length)
   {
      Log.d(TAG, "read handle: " + handle + ", offset: " + offset +
            ", length: " + length);
      int ret = nativeRead(handle, buffer, offset, length);
      Log.d(TAG, "read end: ret = " + ret);
      return ret;
   }
}

/* vim: set sts=3 sw=3 et: */
