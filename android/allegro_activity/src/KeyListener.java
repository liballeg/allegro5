package org.liballeg.android;

import android.media.AudioManager;
import android.content.Context;
import android.view.KeyEvent;
import android.view.View;

class KeyListener implements View.OnKeyListener
{
   private Context context;
   private boolean captureVolume = false;

   native void nativeOnKeyDown(int key);
   native void nativeOnKeyUp(int key);
   native void nativeOnKeyChar(int key, int unichar);

   KeyListener(Context context)
   {
      this.context = context;
   }

   void setCaptureVolumeKeys(boolean onoff)
   {
      captureVolume = onoff;
   }

   @Override
   public boolean onKey(View v, int keyCode, KeyEvent event)
   {
      int unichar;
      if (event.getAction() == KeyEvent.ACTION_DOWN) {
         if (!captureVolume) {
            if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
               volumeChange(1);
               return true;
            }
            else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
               volumeChange(-1);
               return true;
            }
         }
         if (Key.alKey(keyCode) == Key.ALLEGRO_KEY_BACKSPACE) {
            unichar = '\b';
         }
         else if (Key.alKey(keyCode) == Key.ALLEGRO_KEY_ENTER) {
            unichar = '\r';
         }
         else {
            unichar = event.getUnicodeChar();
         }

         if (event.getRepeatCount() == 0) {
            nativeOnKeyDown(Key.alKey(keyCode));
            nativeOnKeyChar(Key.alKey(keyCode), unichar);
         }
         else {
            nativeOnKeyChar(Key.alKey(keyCode), unichar);
         }
         return true;
      }
      else if (event.getAction() == KeyEvent.ACTION_UP) {
         nativeOnKeyUp(Key.alKey(keyCode));
         return true;
      }

      return false;
   }

   private void volumeChange(int inc)
   {
      AudioManager mAudioManager =
         (AudioManager)this.context.getApplicationContext()
         .getSystemService(Context.AUDIO_SERVICE);

      int curr = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
      int max = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
      int vol = curr + inc;

      if (0 <= vol && vol <= max) {
         // Set a new volume level manually with the FLAG_SHOW_UI flag.
         mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, vol,
            AudioManager.FLAG_SHOW_UI);
      }
   }
}

/* vim: set sts=3 sw=3 et: */
