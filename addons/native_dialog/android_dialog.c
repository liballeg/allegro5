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
 *      Native dialog addon for Android.
 *
 *      By Alexandre Martins.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_native_dialog.h"
#include "allegro5/internal/aintern_android.h"
#include "allegro5/internal/aintern_native_dialog.h"

ALLEGRO_DEBUG_CHANNEL("android")

static bool open_file_chooser(int flags, const char *patterns, const char *initial_path, ALLEGRO_PATH ***out_uri_strings, size_t *out_uri_count);
static char *really_open_file_chooser(int flags, const char *patterns, const char *initial_path);
static int show_message_box(const char *title, const char *message, const char *buttons, int flags);
static void append_to_textlog(const char *tag, const char *message);




bool _al_init_native_dialog_addon(void)
{
    return true;
}

void _al_shutdown_native_dialog_addon(void)
{
}

bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display, ALLEGRO_NATIVE_DIALOG *fd)
{
    /* al_show_native_file_dialog() is specified as blocking in the API. Since
       there is a need to handle ALLEGRO_EVENT_DISPLAY_HALT_DRAWING before this
       function returns, users must call it from a separate thread. */

    const char *patterns = al_cstr(fd->fc_patterns);
    const char *initial_path = NULL;
    (void)display;

    /* initial path (optional) */
    if (fd->fc_initial_path != NULL)
        initial_path = al_path_cstr(fd->fc_initial_path, '/');

    /* release any pre-existing paths */
    if (fd->fc_paths != NULL) {
        for (size_t i = 0; i < fd->fc_path_count; i++)
            al_destroy_path(fd->fc_paths[i]);
        al_free(fd->fc_paths);
    }

    /* open the file chooser */
    return open_file_chooser(fd->flags, patterns, initial_path, &fd->fc_paths, &fd->fc_path_count);
}

int _al_show_native_message_box(ALLEGRO_DISPLAY *display, ALLEGRO_NATIVE_DIALOG *nd)
{
    const char *heading = al_cstr(nd->mb_heading);
    const char *text = al_cstr(nd->mb_text);
    const char *buttons = nd->mb_buttons != NULL ? al_cstr(nd->mb_buttons) : NULL;
    (void)display;

    return show_message_box(heading, text, buttons, nd->flags);
}

bool _al_open_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    textlog->is_active = true;
    return true;
}

void _al_close_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    textlog->is_active = false;
}

void _al_append_native_text_log(ALLEGRO_NATIVE_DIALOG *textlog)
{
    if (textlog->is_active) {
        const char *title = al_cstr(textlog->title);
        const char *text = al_cstr(textlog->tl_pending_text);

        append_to_textlog(title, text);

        al_ustr_truncate(textlog->tl_pending_text, 0);
    }
}

bool _al_init_menu(ALLEGRO_MENU *menu)
{
    (void)menu;
    return false;
}

bool _al_init_popup_menu(ALLEGRO_MENU *menu)
{
    (void)menu;
    return false;
}

bool _al_insert_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
    (void)item;
    (void)i;
    return false;
}

bool _al_destroy_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
    (void)item;
    (void)i;
    return false;
}

bool _al_update_menu_item_at(ALLEGRO_MENU_ITEM *item, int i)
{
    (void)item;
    (void)i;
    return false;
}

bool _al_show_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
    (void)display;
    (void)menu;
    return false;
}

bool _al_hide_display_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
    (void)display;
    (void)menu;
    return false;
}

bool _al_show_popup_menu(ALLEGRO_DISPLAY *display, ALLEGRO_MENU *menu)
{
    (void)display;
    (void)menu;
    return false;
}

int _al_get_menu_display_height(void)
{
   return 0;
}




bool open_file_chooser(int flags, const char *patterns, const char *initial_path, ALLEGRO_PATH ***out_uri_strings, size_t *out_uri_count)
{
    const char URI_DELIMITER = '\n';
    char *result = NULL;

    /* initialize the results */
    *out_uri_count = 0;
    *out_uri_strings = NULL;

    /* open the file chooser */
    result = really_open_file_chooser(flags, patterns, initial_path);

    /* error? */
    if (result == NULL)
        return false;

    /* split the returned string. If the file chooser was cancelled, result is an empty string */
    for (char *next_uri = result, *p = result; *p; p++) {
        if (*p == URI_DELIMITER) {
            int last = (*out_uri_count)++;
            *out_uri_strings = al_realloc(*out_uri_strings, (*out_uri_count) * sizeof(ALLEGRO_PATH**));

            /* ALLEGRO_PATHs don't explicitly support URIs at this time, but this works nonetheless.
               See parse_path_string() in src/path.c */
            *p = '\0';
            (*out_uri_strings)[last] = al_create_path(next_uri);
            next_uri = p+1;
        }
    }

    /* success! */
    free(result);
    return true;
}

char *really_open_file_chooser(int flags, const char *patterns, const char *initial_path)
{
    char *result = NULL;

    JNIEnv *env = _al_android_get_jnienv();
    jobject activity = _al_android_activity_object();
    jobject dialog = _jni_callObjectMethod(env, activity, "getNativeDialogAddon", "()L" ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/AllegroDialog;");

    jstring jpatterns = _jni_call(env, jstring, NewStringUTF, patterns != NULL ? patterns : "");
    jstring jinitial_path = _jni_call(env, jstring, NewStringUTF, initial_path != NULL ? initial_path : "");

    jstring jresult = (jstring)_jni_callObjectMethodV(env, dialog, "openFileChooser", "(ILjava/lang/String;Ljava/lang/String;)Ljava/lang/String;", (jint)flags, jpatterns, jinitial_path);
    jboolean is_null_result = _jni_call(env, jboolean, IsSameObject, jresult, NULL);
    if (!is_null_result) {
        const char *tmp = _jni_call(env, const char*, GetStringUTFChars, jresult, NULL);
        result = strdup(tmp);
        _jni_callv(env, ReleaseStringUTFChars, jresult, tmp);
    }
    _jni_callv(env, DeleteLocalRef, jresult);

    _jni_callv(env, DeleteLocalRef, jinitial_path);
    _jni_callv(env, DeleteLocalRef, jpatterns);

    _jni_callv(env, DeleteLocalRef, dialog);

    return result;
}

int show_message_box(const char *title, const char *message, const char *buttons, int flags)
{
    JNIEnv *env = _al_android_get_jnienv();
    jobject activity = _al_android_activity_object();
    jobject dialog = _jni_callObjectMethod(env, activity, "getNativeDialogAddon", "()L" ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/AllegroDialog;");

    jstring jtitle = _jni_call(env, jstring, NewStringUTF, title != NULL ? title : "");
    jstring jmessage = _jni_call(env, jstring, NewStringUTF, message != NULL ? message : "");
    jstring jbuttons = _jni_call(env, jstring, NewStringUTF, buttons != NULL ? buttons : "");

    int result = _jni_callIntMethodV(env, dialog, "showMessageBox", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;I)I", jtitle, jmessage, jbuttons, (jint)flags);

    _jni_callv(env, DeleteLocalRef, jbuttons);
    _jni_callv(env, DeleteLocalRef, jmessage);
    _jni_callv(env, DeleteLocalRef, jtitle);

    _jni_callv(env, DeleteLocalRef, dialog);

    return result;
}

void append_to_textlog(const char *tag, const char *message)
{
    JNIEnv *env = _al_android_get_jnienv();
    jobject activity = _al_android_activity_object();
    jobject dialog = _jni_callObjectMethod(env, activity, "getNativeDialogAddon", "()L" ALLEGRO_ANDROID_PACKAGE_NAME_SLASH "/AllegroDialog;");

    jstring jtag = _jni_call(env, jstring, NewStringUTF, tag != NULL ? tag : "");
    jstring jmessage = _jni_call(env, jstring, NewStringUTF, message != NULL ? message : "");

    _jni_callVoidMethodV(env, dialog, "appendToTextLog", "(Ljava/lang/String;Ljava/lang/String;)V", jtag, jmessage);

    _jni_callv(env, DeleteLocalRef, jmessage);
    _jni_callv(env, DeleteLocalRef, jtag);

    _jni_callv(env, DeleteLocalRef, dialog);
}

/* vim: set sts=4 sw=4 et: */
