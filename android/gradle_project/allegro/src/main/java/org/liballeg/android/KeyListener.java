package org.liballeg.android;

import android.media.AudioManager;
import android.content.Context;
import android.view.KeyEvent;
import android.view.View;

class KeyListener implements View.OnKeyListener
{
   private Context context;
   private boolean captureVolume = false;
   private AllegroActivity activity;

   native void nativeOnKeyDown(int key);
   native void nativeOnKeyUp(int key);
   native void nativeOnKeyChar(int key, int unichar);
   native void nativeOnJoystickButton(int index, int button, boolean down);

   KeyListener(Context context, AllegroActivity activity)
   {
      this.context = context;
      this.activity = activity;
   }

   void setCaptureVolumeKeys(boolean onoff)
   {
      captureVolume = onoff;
   }

   public boolean onKeyboardKey(View v, int keyCode, KeyEvent event)
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

   private int getCode(int keyCode, KeyEvent event, int index1) {
      int code = -1;
      if (keyCode == KeyEvent.KEYCODE_BUTTON_A) {
         code = AllegroActivity.JS_A;
      }
      else if (keyCode == KeyEvent.KEYCODE_BUTTON_B) {
         code = AllegroActivity.JS_B;
      }
      else if (keyCode == KeyEvent.KEYCODE_BUTTON_X) {
         code = AllegroActivity.JS_X;
      }
      else if (keyCode == KeyEvent.KEYCODE_BUTTON_Y) {
         code = AllegroActivity.JS_Y;
      }
      else if (keyCode == KeyEvent.KEYCODE_BUTTON_L1) {
         code = AllegroActivity.JS_L1;
      }
      else if (keyCode == KeyEvent.KEYCODE_BUTTON_R1) {
         code = AllegroActivity.JS_R1;
      }
      else if (keyCode == KeyEvent.KEYCODE_MENU) {
         code = AllegroActivity.JS_MENU;
      }
      else if (keyCode == KeyEvent.KEYCODE_DPAD_LEFT) {
         if (event.getAction() == KeyEvent.ACTION_DOWN)
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_L, true);
         else
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_L, false);
         return -2;
      }
      else if (keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) {
         if (event.getAction() == KeyEvent.ACTION_DOWN)
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_R, true);
         else
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_R, false);
         return -2;
      }
      else if (keyCode == KeyEvent.KEYCODE_DPAD_UP) {
         if (event.getAction() == KeyEvent.ACTION_DOWN)
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_U, true);
         else
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_U, false);
         return -2;
      }
      else if (keyCode == KeyEvent.KEYCODE_DPAD_DOWN) {
         if (event.getAction() == KeyEvent.ACTION_DOWN)
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_D, true);
         else
            nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_D, false);
         return -2;
      }
      return code;
   }

   public boolean onKey(View v, int keyCode, KeyEvent event) {
      /* We want media player controls to keep working in background apps */
      if (keyCode == KeyEvent.KEYCODE_MEDIA_REWIND || keyCode == KeyEvent.KEYCODE_MEDIA_PLAY_PAUSE || keyCode == KeyEvent.KEYCODE_MEDIA_FAST_FORWARD) {
         return false;
      }

      if (activity.joystickActive == false) {
         return onKeyboardKey(v, keyCode, event);
      }

      int id = event.getDeviceId();
      int index = activity.indexOfJoystick(id);
      int code = -1;
      if (index >= 0) {
         code = getCode(keyCode, event, index);
      }
      if (code >= 0) {
         if (event.getAction() == KeyEvent.ACTION_DOWN) {
            if (event.getRepeatCount() == 0) {
               nativeOnJoystickButton(index, code, true);
            }
         }
         else {
            nativeOnJoystickButton(index, code, false);
         }
      }
      return onKeyboardKey(v, keyCode, event);
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
