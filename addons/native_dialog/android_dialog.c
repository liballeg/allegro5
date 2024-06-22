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

static ALLEGRO_DISPLAY *get_active_display(void);
static void wait_for_display_events(ALLEGRO_DISPLAY *dpy);
static bool open_file_chooser(int flags, const char *patterns, const char *initial_path, ALLEGRO_PATH ***out_uri_strings, size_t *out_uri_count);
static char *really_open_file_chooser(int flags, const char *patterns, const char *initial_path);
static int show_message_box(const char *title, const char *message, const char *buttons, int flags);
static void append_to_textlog(const char *tag, const char *message);

static ALLEGRO_EVENT_QUEUE *queue = NULL;
static ALLEGRO_MUTEX *mutex = NULL;




bool _al_init_native_dialog_addon(void)
{
    if (NULL == (mutex = al_create_mutex()))
        return false;

    if (NULL == (queue = al_create_event_queue())) {
        al_destroy_mutex(mutex);
        return false;
    }

    return true;
}

void _al_shutdown_native_dialog_addon(void)
{
    al_destroy_mutex(mutex);
    mutex = NULL;

    /*al_destroy_event_queue(queue);*/ /* already released by Allegro's destructors */
    queue = NULL;
}

bool _al_show_native_file_dialog(ALLEGRO_DISPLAY *display, ALLEGRO_NATIVE_DIALOG *fd)
{
    /* al_show_native_file_dialog() has a blocking interface. Since there is a
       need to handle drawing halt and drawing resume events before this
       function returns, users must call it from a separate thread. */
    (void)display; /* possibly NULL */

    /* fail if the native dialog addon is not initialized */
    if (!al_is_native_dialog_addon_initialized()) {
        ALLEGRO_DEBUG("the native dialog addon is not initialized");
        return false;
    }

    /* only one file chooser may be opened at a time */
    al_lock_mutex(mutex);

    /* initial path (optional) */
    const char *initial_path = NULL;
    if (fd->fc_initial_path != NULL)
        initial_path = al_path_cstr(fd->fc_initial_path, '/');

    /* release any pre-existing paths */
    if (fd->fc_paths != NULL) {
        for (size_t i = 0; i < fd->fc_path_count; i++)
            al_destroy_path(fd->fc_paths[i]);
        al_free(fd->fc_paths);
    }

    /* register the event source */
    ALLEGRO_DISPLAY *dpy = get_active_display();
    if (dpy != NULL)
        al_register_event_source(queue, &dpy->es);

    /* open the file chooser */
    ALLEGRO_DEBUG("waiting for the file chooser");
    ALLEGRO_USTR *mime_patterns = al_ustr_new("");
    bool first = true;
    bool any_catchalls = false;
    for (size_t i = 0; i < _al_vector_size(&fd->fc_patterns); i++) {
       _AL_PATTERNS_AND_DESC *patterns_and_desc = _al_vector_ref(&fd->fc_patterns, (int)i);
       for (size_t j = 0; j < _al_vector_size(&patterns_and_desc->patterns_vec); j++) {
           _AL_PATTERN *pattern = _al_vector_ref(&patterns_and_desc->patterns_vec, (int)j);
           if (pattern->is_catchall) {
               any_catchalls = true;
               break;
           }
           if (pattern->is_mime)
           {
               if (!first)
                   al_ustr_append_chr(mime_patterns, ';');
               first = false;
               al_ustr_append(mime_patterns, al_ref_info(&pattern->info));
           }
       }
    }
    if (any_catchalls)
       al_ustr_truncate(mime_patterns, 0);
    bool ret = open_file_chooser(fd->flags, al_cstr(mime_patterns), initial_path, &fd->fc_paths, &fd->fc_path_count);
    al_ustr_free(mime_patterns);
    ALLEGRO_DEBUG("done waiting for the file chooser");

    /* ensure predictable behavior */
    if (dpy != NULL) {

        /* We expect al_show_native_file_dialog() to be called before a drawing
           halt. We expect it to return after a drawing resume. */
        wait_for_display_events(dpy);

        /* unregister the event source */
        al_unregister_event_source(queue, &dpy->es);

    }

    /* done! */
    ALLEGRO_DEBUG("done");

    al_unlock_mutex(mutex);
    return ret;
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




ALLEGRO_DISPLAY *get_active_display(void)
{
    ALLEGRO_SYSTEM *sys = al_get_system_driver();
    ASSERT(sys);

    if (_al_vector_size(&sys->displays) == 0)
        return NULL;

    ALLEGRO_DISPLAY **dptr = (ALLEGRO_DISPLAY **)_al_vector_ref(&sys->displays, 0);
    return *dptr;
}

void wait_for_display_events(ALLEGRO_DISPLAY *dpy)
{
    ALLEGRO_DISPLAY_ANDROID *d = (ALLEGRO_DISPLAY_ANDROID *)dpy;
    ALLEGRO_TIMEOUT timeout;
    ALLEGRO_EVENT event;
    bool expected_state = false;

    memset(&event, 0, sizeof(event));

    /* We expect a drawing halt event to be on the queue. If that is not true,
       then the drawing halt did not take place for some unusual reason */
    ALLEGRO_DEBUG("looking for a ALLEGRO_EVENT_DISPLAY_HALT_DRAWING");
    while (al_get_next_event(queue, &event)) {
        if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING) {
            expected_state = true;
            break;
        }
    }

    /* skip if we're in an unexpected state */
    if (!expected_state) {
        ALLEGRO_DEBUG("ALLEGRO_EVENT_DISPLAY_HALT_DRAWING not found");
        return;
    }

    wait_for_drawing_resume:

    /* wait for ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING */
    ALLEGRO_DEBUG("waiting for ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING");
    while (event.type != ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING)
        al_wait_for_event(queue, &event);
    ALLEGRO_DEBUG("done waiting for ALLEGRO_EVENT_DISPLAY_RESUME_DRAWING");

    /* wait for al_acknowledge_drawing_resume() */
    ALLEGRO_DEBUG("waiting for al_acknowledge_drawing_resume");
    al_lock_mutex(d->mutex);
    while (!d->resumed)
        al_wait_cond(d->cond, d->mutex);
    al_unlock_mutex(d->mutex);
    ALLEGRO_DEBUG("done waiting for al_acknowledge_drawing_resume");

    /* A resize event takes place here, as can be seen in the implementation of
       AllegroSurface.nativeOnChange() at src/android_display.c (at the time of
       this writing). However, the Allegro documentation does not specify that
       a resize event must follow a drawing resume.

       We expect that the user will call al_acknowledge_resize() immediately.
       We don't wait for the acknowledgement of the resize event, in case the
       implementation changes someday. We just wait a little bit. */
    ;

    /* check if a new ALLEGRO_EVENT_DISPLAY_HALT_DRAWING is emitted */
    ALLEGRO_DEBUG("waiting for another ALLEGRO_EVENT_DISPLAY_HALT_DRAWING");
    al_init_timeout(&timeout, 0.5);
    while (al_wait_for_event_until(queue, &event, &timeout)) {
        if (event.type == ALLEGRO_EVENT_DISPLAY_HALT_DRAWING)
            goto wait_for_drawing_resume;
        else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE)
            al_init_timeout(&timeout, 0.5);
    }
    ALLEGRO_DEBUG("done waiting for another ALLEGRO_EVENT_DISPLAY_HALT_DRAWING");
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

    /* split the returned string. If the file chooser was cancelled, variable result is an empty string */
    for (char *next_uri = result, *p = result; *p; p++) {
        if (*p == URI_DELIMITER) {
            int last = (*out_uri_count)++;
            *out_uri_strings = al_realloc(*out_uri_strings, (*out_uri_count) * sizeof(ALLEGRO_PATH**));

            /* ALLEGRO_PATHs don't explicitly support URIs at this time, but this works nonetheless.
               See parse_path_string() at src/path.c */
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
