/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Android Java/JNI system driver
 *
 *      By Thomas Fjellstrom.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_android.h"
#include "allegro5/internal/aintern_android.h"

#include <jni.h>

JNI_FUNC(void, Sensors, nativeOnAccel, (JNIEnv *env, jobject obj, jint id,
   jfloat x, jfloat y, jfloat z))
{
   (void)env;
   (void)obj;
   (void)id;
   _al_android_generate_accelerometer_event(x, y, z);
}

/* vim: set sts=3 sw=3 et: */
