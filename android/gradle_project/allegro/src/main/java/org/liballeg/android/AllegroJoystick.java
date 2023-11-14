package org.liballeg.android;

import android.view.View.OnGenericMotionListener;
import android.view.MotionEvent;
import android.view.InputDevice;
import android.view.View;

class AllegroJoystick implements OnGenericMotionListener
{
   private AllegroActivity activity;
   private AllegroSurface surface;

   public AllegroJoystick(AllegroActivity activity, AllegroSurface surface) {
      this.activity = activity;
      this.surface = surface;
   }

   private float axis0_x = 0.0f;
   private float axis0_y = 0.0f;
   private float axis0_hat_x = 0.0f;
   private float axis0_hat_y = 0.0f;
   private float axis1_x = 0.0f;
   private float axis1_y = 0.0f;
   private float axis_lt = 0.0f;
   private float axis_rt = 0.0f;

   private void handleHat(int index1, float old, float cur, int button1, int button2) {
      if (old == cur)
         return;

      if (old == 0) {
         if (cur < 0)
            surface.nativeOnJoystickButton(index1, button1, true);
         else
            surface.nativeOnJoystickButton(index1, button2, true);
      }
      else if (old < 0) {
         surface.nativeOnJoystickButton(index1, button1, false);
         if (cur > 0) {
            surface.nativeOnJoystickButton(index1, button2, true);
         }
      }
      else if (old > 0) {
         surface.nativeOnJoystickButton(index1, button2, false);
         if (cur < 0) {
            surface.nativeOnJoystickButton(index1, button1, true);
         }
      }
   }

   @Override
   public boolean onGenericMotion(View v, MotionEvent event) {
      if (activity.joystickActive == false) {
         return false;
      }

      int id = event.getDeviceId();
      int index = activity.indexOfJoystick(id);
      if (index >= 0) {
         float ax = event.getAxisValue(MotionEvent.AXIS_X, 0);
         float ay = event.getAxisValue(MotionEvent.AXIS_Y, 0);
         float ahx = event.getAxisValue(MotionEvent.AXIS_HAT_X, 0);
         float ahy = event.getAxisValue(MotionEvent.AXIS_HAT_Y, 0);
         float az = event.getAxisValue(MotionEvent.AXIS_Z, 0);
         float arz = event.getAxisValue(MotionEvent.AXIS_RZ, 0);
         float alt = event.getAxisValue(MotionEvent.AXIS_BRAKE, 0);
         float art = event.getAxisValue(MotionEvent.AXIS_GAS, 0);
         if (ax != axis0_x || ay != axis0_y) {
            surface.nativeOnJoystickAxis(index, 0, 0, ax);
            surface.nativeOnJoystickAxis(index, 0, 1, ay);
            axis0_x = ax;
            axis0_y = ay;
         }
         else if (ahx != axis0_hat_x || ahy != axis0_hat_y) {
            handleHat(index, axis0_hat_x, ahx, AllegroActivity.JS_DPAD_L, AllegroActivity.JS_DPAD_R);
            handleHat(index, axis0_hat_y, ahy, AllegroActivity.JS_DPAD_U, AllegroActivity.JS_DPAD_D);
            axis0_hat_x = ahx;
            axis0_hat_y = ahy;
         }
         if (az != axis1_x || arz != axis1_y) {
            surface.nativeOnJoystickAxis(index, 1, 0, az);
            surface.nativeOnJoystickAxis(index, 1, 1, arz);
            axis1_x = az;
            axis1_y = arz;
         }
         if (alt != axis_lt) {
            surface.nativeOnJoystickAxis(index, 2, 0, alt);
            axis_lt = alt;
         }
         if (art != axis_rt) {
            surface.nativeOnJoystickAxis(index, 3, 0, art);
            axis_rt = art;
         }
         return true;
      }
      return false;
   }
}
