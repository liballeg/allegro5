package org.liballeg.app;

import java.io.InputStream;
import android.util.Log;

public class AllegroInputStream extends InputStream
{
   private int mark_pos;
   private int handle;
   
   public native int nativeRead(int hdnl, byte[] buffer, int offset, int length);
   public native void nativeClose(int hdnl);
   
   public AllegroInputStream(int hdnl)
   {
      super();
      handle = hdnl;
      Log.d("AllegroInputStream", "ctor handle:" + handle);
   }
   
   public int available()
   {
      Log.d("AllegroInputStream", "available");
      return 0;
   }
   
   public void close()
   {
      Log.d("AllegroInputStream", "close");
      nativeClose(handle);
   }
   
   public void mark(int limit)
   {
      Log.d("AllegroInputStream", "mark " + limit);
   }
   
   public boolean markSupported()
   {
      Log.d("AllegroInputStream", "markSupported");
      return false;
   }
   
   public int read(byte[] buffer)
   {
      return read(buffer, 0, buffer.length);
   }
   
   public int read()
   {
      byte buffer[] = new byte[1];
      int ret = read(buffer, 0, buffer.length);
      if(ret != -1)
         return buffer[0];
      
      return -1;
   }
   
   public int read(byte[] buffer, int offset, int length)
   {
      Log.d("AllegroInputStream", "handle " + handle + " read " + length + " bytes at offset " + offset);
      int ret = nativeRead(handle, buffer, offset, length);
      Log.d("AllegroInputStream", "read end");
      return ret;
   }
}
