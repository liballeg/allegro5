package org.liballeg.android;

import android.app.Activity;

import android.content.Context;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.util.Log;


class Clipboard
{
   private static final String TAG = "Clipboard";

   private Activity  activity;
   private boolean   clip_thread_done = false;
   private String    clipdata;

   Clipboard(Activity activity)
   {
      this.activity = activity;
   }

   public boolean setText(String text)
   {
      final String ss = text;

      Runnable runnable = new Runnable() {
         public void run() {
            ClipboardManager clipboard = (ClipboardManager)
               activity.getSystemService(Context.CLIPBOARD_SERVICE);
            ClipData clip = ClipData.newPlainText("allegro", ss);
            clipboard.setPrimaryClip(clip);
            clip_thread_done = true;
         };
      };

      activity.runOnUiThread(runnable);

      while (!clip_thread_done)
         ;
      clip_thread_done = false;

      return true;
   }

   public String getText()
   {
      Runnable runnable = new Runnable() {
         public void run() {
            ClipboardManager clipboard = (ClipboardManager)
               activity.getSystemService(Context.CLIPBOARD_SERVICE);

            ClipData clip        = clipboard.getPrimaryClip();

            if (clip == null) {
               clipdata = null;
            } else {
               ClipData.Item item   = clip.getItemAt(0);

               if (item == null) {
                  clipdata = null;
               } else {
                  String text = item.coerceToText(activity.getApplicationContext()).toString();
                  clipdata    = text;
               }
            }
            clip_thread_done = true;
         }
      };

      activity.runOnUiThread(runnable);

      while (!clip_thread_done);
      clip_thread_done = false;

      return clipdata;
   }


   boolean hasText()
   {
      /* Lazy implementation... */
      return this.getText() != null;
   }
}

/* vim: set sts=3 sw=3 et: */
