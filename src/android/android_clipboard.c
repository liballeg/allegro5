/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_//_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Android clipboard handling.
 *
 *      By Beoran.
 *
 *      See readme.txt for copyright information.
 */



#include "allegro5/allegro.h"
#include "allegro5/allegro_android.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_bitmap.h"

#include <jni.h>

A5O_DEBUG_CHANNEL("clipboard")

static bool android_set_clipboard_text(A5O_DISPLAY *display, const char *text)
{
   JNIEnv * env = (JNIEnv *)_al_android_get_jnienv();
   jstring jtext= _jni_call(env, jstring, NewStringUTF, text);
   (void) display;
   return _jni_callBooleanMethodV(env, _al_android_activity_object(),
                                 "setClipboardText", "(Ljava/lang/String;)Z", jtext);
}

static char *android_get_clipboard_text(A5O_DISPLAY *display)
{
   JNIEnv * env = (JNIEnv *)_al_android_get_jnienv();
   jobject jtext = _jni_callObjectMethod(env, _al_android_activity_object(), "getClipboardText", "()Ljava/lang/String;");
   jsize len = _jni_call(env, jsize, GetStringUTFLength, jtext);
   const char *str = _jni_call(env, const char *, GetStringUTFChars, jtext, NULL);
   char * text =  al_malloc(len+1);
   (void) display;

   text = _al_sane_strncpy(text, str, len);
   _jni_callv(env, ReleaseStringUTFChars, jtext, str);
   _jni_callv(env, DeleteLocalRef, jtext);

   return text;
}

static bool android_has_clipboard_text(A5O_DISPLAY *display)
{
   JNIEnv * env = (JNIEnv *)_al_android_get_jnienv();
   (void) display;
   return _jni_callBooleanMethodV(env, _al_android_activity_object(),
                                 "hasClipboardText", "()Z");
}


void _al_android_add_clipboard_functions(A5O_DISPLAY_INTERFACE *vt)
{
   vt->set_clipboard_text = android_set_clipboard_text;
   vt->get_clipboard_text = android_get_clipboard_text;
   vt->has_clipboard_text = android_has_clipboard_text;
}

/* vi: set ts=8 sts=3 sw=3 et: */
