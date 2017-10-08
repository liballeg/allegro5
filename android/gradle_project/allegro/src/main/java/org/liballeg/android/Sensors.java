package org.liballeg.android;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import java.util.List;

public class Sensors implements SensorEventListener
{
   private static SensorManager sensorManager;
   private List<Sensor> sensors;

   public native void nativeOnAccel(int id, float x, float y, float z);

   Sensors(Context context)
   {
      /* Only check for Accelerometers for now, not sure how we should utilize
       * other types.
       */
      sensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);
      sensors = sensorManager.getSensorList(Sensor.TYPE_ACCELEROMETER);
   }

   void listen()
   {
      for (int i = 0; i < sensors.size(); i++) {
         sensorManager.registerListener(this, sensors.get(i),
            SensorManager.SENSOR_DELAY_GAME);
      }
   }

   void unlisten()
   {
      for (int i = 0; i < sensors.size(); i++) {
         sensorManager.unregisterListener(this, sensors.get(i));
      }
   }

   @Override
   public void onAccuracyChanged(Sensor sensor, int accuracy)
   {
      /* what to do? */
   }

   @Override
   public void onSensorChanged(SensorEvent event)
   {
      int i = sensors.indexOf(event.sensor);
      if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
         nativeOnAccel(i, event.values[0], event.values[1], event.values[2]);
      }
   }
}

/* vim: set sts=3 sw=3 et: */
