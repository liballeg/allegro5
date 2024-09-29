package org.liballeg.android;

import android.util.Log;
import android.view.MotionEvent;
import android.view.View;

class TouchListener implements View.OnTouchListener
{
   private static final String TAG = "TouchListener";

   native void nativeOnTouch(int id, int action, float x, float y,
      boolean primary);

   TouchListener()
   {
   }

   // FIXME: Pull out android version detection into the setup and just check
   // some flags here, rather than checking for the existance of the fields and
   // methods over and over.
   @Override
   public boolean onTouch(View v, MotionEvent event)
   {
      int action = 0;
      int pointer_id = 0;

      Class[] no_args = new Class[0];
      Class[] int_arg = new Class[1];
      int_arg[0] = int.class;

      if (Reflect.methodExists(event, "getActionMasked")) { // android-8 / 2.2.x
         action = Reflect.<Integer>callMethod(event, "getActionMasked", no_args);
         int ptr_idx = Reflect.<Integer>callMethod(event, "getActionIndex", no_args);
         pointer_id = Reflect.<Integer>callMethod(event, "getPointerId", int_arg, ptr_idx);
      } else {
         int raw_action = event.getAction();

         if (Reflect.fieldExists(event, "ACTION_MASK")) { // android-5 / 2.0
            int mask = Reflect.<Integer>getField(event, "ACTION_MASK");
            action = raw_action & mask;

            int ptr_id_mask = Reflect.<Integer>getField(event, "ACTION_POINTER_ID_MASK");
            int ptr_id_shift = Reflect.<Integer>getField(event, "ACTION_POINTER_ID_SHIFT");

            pointer_id = event.getPointerId((raw_action & ptr_id_mask) >> ptr_id_shift);
         }
         else { // android-4 / 1.6
            /* no ACTION_MASK? no multi touch, no pointer_id, */
            action = raw_action;
         }
      }

      boolean primary = false;

      if (action == MotionEvent.ACTION_DOWN) {
         primary = true;
         action = Const.A5O_EVENT_TOUCH_BEGIN;
      }
      else if (action == MotionEvent.ACTION_UP) {
         primary = true;
         action = Const.A5O_EVENT_TOUCH_END;
      }
      else if (action == MotionEvent.ACTION_MOVE) {
         action = Const.A5O_EVENT_TOUCH_MOVE;
      }
      else if (action == MotionEvent.ACTION_CANCEL) {
         action = Const.A5O_EVENT_TOUCH_CANCEL;
      }
      // android-5 / 2.0
      else if (Reflect.fieldExists(event, "ACTION_POINTER_DOWN") &&
         action == Reflect.<Integer>getField(event, "ACTION_POINTER_DOWN"))
      {
         action = Const.A5O_EVENT_TOUCH_BEGIN;
      }
      else if (Reflect.fieldExists(event, "ACTION_POINTER_UP") &&
         action == Reflect.<Integer>getField(event, "ACTION_POINTER_UP"))
      {
         action = Const.A5O_EVENT_TOUCH_END;
      }
      else {
         Log.v(TAG, "unknown MotionEvent type: " + action);
         return false;
      }

      if (Reflect.methodExists(event, "getPointerCount")) { // android-5 / 2.0
         int pointer_count = Reflect.<Integer>callMethod(event,
            "getPointerCount", no_args);
         for (int i = 0; i < pointer_count; i++) {
            float x = Reflect.<Float>callMethod(event, "getX", int_arg, i);
            float y = Reflect.<Float>callMethod(event, "getY", int_arg, i);
            int id = Reflect.<Integer>callMethod(event, "getPointerId",
                  int_arg, i);

            /* Not entirely sure we need to report move events for non-primary
             * touches here but examples I've seen say that the
             * ACTION_[POINTER_][UP|DOWN] report all touches and they can
             * change between the last MOVE and the UP|DOWN event.
             */
            if (id == pointer_id) {
               nativeOnTouch(id, action, x, y, primary);
            } else {
               nativeOnTouch(id, Const.A5O_EVENT_TOUCH_MOVE, x, y,
                  primary);
            }
         }
      } else {
         nativeOnTouch(pointer_id, action, event.getX(), event.getY(),
            primary);
      }

      return true;
   }
}

/* vim: set sts=3 sw=3 et: */
