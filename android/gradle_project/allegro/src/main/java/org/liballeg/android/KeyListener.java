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
      switch (keyCode) {
         // gamepad buttons
         case KeyEvent.KEYCODE_BUTTON_A:
            return AllegroActivity.JS_A;

         case KeyEvent.KEYCODE_BUTTON_B:
            return AllegroActivity.JS_B;

         case KeyEvent.KEYCODE_BUTTON_C:
            return AllegroActivity.JS_C;

         case KeyEvent.KEYCODE_BUTTON_X:
            return AllegroActivity.JS_X;

         case KeyEvent.KEYCODE_BUTTON_Y:
            return AllegroActivity.JS_Y;

         case KeyEvent.KEYCODE_BUTTON_Z:
            return AllegroActivity.JS_Z;

         case KeyEvent.KEYCODE_MENU:
         case KeyEvent.KEYCODE_BUTTON_START:
            return AllegroActivity.JS_START;

         case KeyEvent.KEYCODE_BACK:
         case KeyEvent.KEYCODE_BUTTON_SELECT:
            return AllegroActivity.JS_SELECT;

         case KeyEvent.KEYCODE_BUTTON_MODE:
            return AllegroActivity.JS_MODE;

         case KeyEvent.KEYCODE_BUTTON_THUMBL:
            return AllegroActivity.JS_THUMBL;

         case KeyEvent.KEYCODE_BUTTON_THUMBR:
            return AllegroActivity.JS_THUMBR;

         case KeyEvent.KEYCODE_BUTTON_L1:
            return AllegroActivity.JS_L1;

         case KeyEvent.KEYCODE_BUTTON_R1:
            return AllegroActivity.JS_R1;

         case KeyEvent.KEYCODE_BUTTON_L2:
            return AllegroActivity.JS_L2;

         case KeyEvent.KEYCODE_BUTTON_R2:
            return AllegroActivity.JS_R2;

         // D-Pad
         case KeyEvent.KEYCODE_DPAD_CENTER:
            return AllegroActivity.JS_DPAD_CENTER;

         case KeyEvent.KEYCODE_DPAD_LEFT:
            if (event.getAction() == KeyEvent.ACTION_DOWN)
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_L, true);
            else
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_L, false);
            return -2;

         case KeyEvent.KEYCODE_DPAD_RIGHT:
            if (event.getAction() == KeyEvent.ACTION_DOWN)
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_R, true);
            else
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_R, false);
            return -2;

         case KeyEvent.KEYCODE_DPAD_UP:
            if (event.getAction() == KeyEvent.ACTION_DOWN)
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_U, true);
            else
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_U, false);
            return -2;

         case KeyEvent.KEYCODE_DPAD_DOWN:
            if (event.getAction() == KeyEvent.ACTION_DOWN)
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_D, true);
            else
               nativeOnJoystickButton(index1, AllegroActivity.JS_DPAD_D, false);
            return -2;

         // generic gamepad buttons
         case KeyEvent.KEYCODE_BUTTON_1:
         case KeyEvent.KEYCODE_BUTTON_2:
         case KeyEvent.KEYCODE_BUTTON_3:
         case KeyEvent.KEYCODE_BUTTON_4:
         case KeyEvent.KEYCODE_BUTTON_5:
         case KeyEvent.KEYCODE_BUTTON_6:
         case KeyEvent.KEYCODE_BUTTON_7:
         case KeyEvent.KEYCODE_BUTTON_8:
         case KeyEvent.KEYCODE_BUTTON_9:
         case KeyEvent.KEYCODE_BUTTON_10:
         case KeyEvent.KEYCODE_BUTTON_11:
         case KeyEvent.KEYCODE_BUTTON_12:
         case KeyEvent.KEYCODE_BUTTON_13:
         case KeyEvent.KEYCODE_BUTTON_14:
         case KeyEvent.KEYCODE_BUTTON_15:
         case KeyEvent.KEYCODE_BUTTON_16:
            return AllegroActivity.JS_BUTTON_1 + (keyCode - KeyEvent.KEYCODE_BUTTON_1);

         // not a joystick button
         default:
            return -1;
      }
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
      if (code == -1) {
         return onKeyboardKey(v, keyCode, event);
      }
      else if (code == -2) {
         return true;
      }
      if (event.getAction() == KeyEvent.ACTION_DOWN) {
         if (event.getRepeatCount() == 0) {
            nativeOnJoystickButton(index, code, true);
         }
      }
      else {
         nativeOnJoystickButton(index, code, false);
      }
      return true;
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
