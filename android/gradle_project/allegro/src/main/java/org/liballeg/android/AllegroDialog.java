package org.liballeg.android;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.net.Uri;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.webkit.MimeTypeMap;

import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;


/**
 * Native Dialog Addon
 */
public class AllegroDialog
{
    private final AllegroActivity activity;
    private final AllegroMessageBox messageBox = new AllegroMessageBox();
    private final AllegroFileChooser fileChooser = new AllegroFileChooser();
    private final AllegroTextLog textLog = new AllegroTextLog();

    public AllegroDialog(AllegroActivity activity)
    {
        this.activity = activity;
    }

    public int showMessageBox(String title, String message, String buttons, int flags)
    {
        return messageBox.show(activity, title, message, buttons, flags);
    }

    public void dismissMessageBox()
    {
        messageBox.dismiss();
    }

    public void appendToTextLog(String tag, String message)
    {
        textLog.append(tag, message);
    }

    public String openFileChooser(int flags, String patterns, String initialPath)
    {
        return fileChooser.open(activity, flags, patterns, initialPath);
    }

    public void onActivityResult(int requestCode, int resultCode, Intent resultData)
    {
        if (requestCode == AllegroFileChooser.REQUEST_FILE_CHOOSER)
            fileChooser.onActivityResult(requestCode, resultCode, resultData);
    }
}


/**
 * A Message Box implementation
 */
class AllegroMessageBox
{
    private static final String TAG = "AllegroMessageBox";
    private static final int NO_BUTTON = 0;
    private static final int POSITIVE_BUTTON = 1;
    private static final int NEGATIVE_BUTTON = 2;
    private static final int NEUTRAL_BUTTON = 3;
    private AlertDialog currentDialog = null;

    public int show(final Activity activity, final String title, final String message, final String buttons, final int flags)
    {
        final MutableInteger result = new MutableInteger(NO_BUTTON); // use a boxed integer
        final Looper looper = myLooper();

        // create and show an alert dialog
        activity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // create an alert dialog
                AlertDialog newDialog = createAlertDialog(activity, title, message, buttons, flags, result, looper);

                // only one alert dialog can be active at any given time
                if (currentDialog != null)
                    currentDialog.dismiss();

                // show the alert dialog
                currentDialog = newDialog;
                showAlertDialog(currentDialog, activity);
            }
        });

        // make the dialog a modal
        Log.d(TAG, "Waiting for user input");
        try { Looper.loop(); } catch (LooperInterrupterException e) { ; }
        Log.d(TAG, "Result = " + result.get());

        // done!
        return result.get();
    }

    public void dismiss()
    {
        if (currentDialog != null) {
            Log.d(TAG, "Dismissed by Allegro");
            currentDialog.dismiss();
        }
    }

    private AlertDialog createAlertDialog(Activity activity, String title, String message, String buttons, int flags, MutableInteger result, Looper looper)
    {
        final LooperInterrupter interrupter = new LooperInterrupter(looper);
        OnClickListenerGenerator resultSetter = new OnClickListenerGenerator(result);
        AlertDialog.Builder builder = new AlertDialog.Builder(activity);

        // configure the alert dialog
        builder.setTitle(title);
        builder.setMessage(message);
        builder.setCancelable(true);

        if (0 != (flags & AllegroDialogConst.ALLEGRO_MESSAGEBOX_WARN))
            builder.setIcon(android.R.drawable.ic_dialog_alert);
        else if (0 != (flags & AllegroDialogConst.ALLEGRO_MESSAGEBOX_ERROR))
            builder.setIcon(android.R.drawable.ic_dialog_alert); // ic_delete
        else if (0 != (flags & AllegroDialogConst.ALLEGRO_MESSAGEBOX_QUESTION))
            builder.setIcon(android.R.drawable.ic_dialog_info);

        builder.setOnCancelListener(new DialogInterface.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                dialog.dismiss();
            }
        });

        builder.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                if (dialog == currentDialog)
                    currentDialog = null;

                interrupter.interrupt();
            }
        });

        // configure the buttons
        boolean wantCustomButtons = !buttons.equals("");
        String[] buttonText = wantCustomButtons ? buttons.split("\\|") : null;

        if (!wantCustomButtons) {
            builder.setPositiveButton(android.R.string.ok, resultSetter.generate(POSITIVE_BUTTON));

            if (0 != (flags & (AllegroDialogConst.ALLEGRO_MESSAGEBOX_OK_CANCEL | AllegroDialogConst.ALLEGRO_MESSAGEBOX_YES_NO))) {
                // unfortunately, android.R.string.yes and android.R.string.no
                // are deprecated and resolve to android.R.string.ok and
                // android.R.string.cancel, respectively.
                builder.setNegativeButton(android.R.string.cancel, resultSetter.generate(NEGATIVE_BUTTON));
            }
        }
        else if (buttonText.length == 1) {
            builder.setPositiveButton(buttonText[0], resultSetter.generate(POSITIVE_BUTTON));
        }
        else if (buttonText.length == 2) {
            builder.setPositiveButton(buttonText[0], resultSetter.generate(POSITIVE_BUTTON));
            builder.setNegativeButton(buttonText[1], resultSetter.generate(NEGATIVE_BUTTON));
        }
        else if (buttonText.length >= 3) { // we support up to 3 buttons
            builder.setPositiveButton(buttonText[0], resultSetter.generate(POSITIVE_BUTTON));
            builder.setNegativeButton(buttonText[1], resultSetter.generate(NEGATIVE_BUTTON));
            builder.setNeutralButton(buttonText[2], resultSetter.generate(NEUTRAL_BUTTON));
        }

        // create the alert dialog
        return builder.create();
    }

    private void showAlertDialog(AlertDialog dialog, Activity activity)
    {
        ImmersiveDialogWrapper dlg = new ImmersiveDialogWrapper(dialog);
        dlg.show(activity);
    }

    private Looper myLooper()
    {
        try { Looper.prepare(); } catch (Exception e) { ; }
        return Looper.myLooper();
    }

    private class LooperInterrupter extends Handler
    {
        public LooperInterrupter(Looper looper)
        {
            super(looper);
        }

        public void interrupt()
        {
            sendMessage(obtainMessage());
        }

        @Override
        public void handleMessage(Message msg)
        {
            throw new LooperInterrupterException();
        }
    }

    private class LooperInterrupterException extends RuntimeException
    {
    }

    private class MutableInteger
    {
        private int value;

        public MutableInteger(int value)
        {
            this.value = value;
        }

        public void set(int value)
        {
            this.value = value;
        }

        public int get()
        {
            return value;
        }
    }

    private class OnClickListenerGenerator
    {
        private final MutableInteger result;

        public OnClickListenerGenerator(MutableInteger result)
        {
            this.result = result;
        }

        public DialogInterface.OnClickListener generate(final int value)
        {
            return new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    result.set(value);
                    dialog.dismiss();
                }
            };
        }
    }

    private class ImmersiveDialogWrapper
    {
        /*

        This class wraps a Dialog object with code that handles the immersive
        mode in a special way.

        Message boxes have a blocking interface. When the app is in immersive
        mode (i.e., the ALLEGRO_FRAMELESS display flag is on), a deadlock may
        occur as soon as a message box is invoked. That will happen if the
        thread that calls al_acknowledge_resize() is the same thread that
        invokes the message box.

        Without a special handling of the immersive mode, the Android system
        will momentarily show the navigation bar and trigger a surface change
        as soon as the message box is called forth. This, in turn, triggers a
        display resize event. In summary, here is what happens:

        - Allegro will block the UI thread until ALLEGRO_EVENT_DISPLAY_RESIZE
          is acknowledged.

        - The thread that invoked the message box from native code will remain
          blocked until the alert dialog is dismissed.

        - The thread that invoked the message box cannot possibly acknowledge
          the display resize while it is blocked.

        - The alert dialog is displayed from the UI thread. If the UI thread
          is blocked, then the dialog can't be dismissed. If the dialog isn't
          dismissed, then the thread that invoked the message box will remain
          blocked.

        - Unless al_acknowledge_resize() is called by some other thread, then
          both threads will be waiting forever. Deadlock!

        The solution below prevents the deadlock from occurring by preventing
        the triggering of a display resize event in the first place. This is
        achieved with a little trick that makes the dialog non-focusable while
        it's being created.

        */
        private final Dialog dialog;

        public ImmersiveDialogWrapper(Dialog dialog)
        {
            this.dialog = dialog;
        }

        public void show(Activity activity)
        {
            if (!isInImmersiveMode(activity)) {
                dialog.show();
                return;
            }

            // This solution is based on https://stackoverflow.com/a/23207365
            View view = activity.getWindow().getDecorView();
            int immersiveFlags = view.getSystemUiVisibility();
            Window dialogWindow = dialog.getWindow();

            dialogWindow.addFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);

            dialog.show();

            dialogWindow.getDecorView().setSystemUiVisibility(immersiveFlags);
            dialogWindow.clearFlags(WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE);
        }

        private boolean isInImmersiveMode(Activity activity)
        {
            if (Build.VERSION.SDK_INT < 19)
                return false;

            // This is based on AllegroActivity.setAllegroFrameless()
            View view = activity.getWindow().getDecorView();
            int flags = view.getSystemUiVisibility();
            final int IMMERSIVE_FLAGS = View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION;

            return (flags & IMMERSIVE_FLAGS) != 0;
        }
    }
}


/**
 * A file chooser based on the Storage Access Framework (API level 19+)
 */
class AllegroFileChooser
{
    public static final int REQUEST_FILE_CHOOSER = 0xF11E;

    private static final String TAG = "AllegroFileChooser";
    private static final String URI_DELIMITER = "\n"; // the Line Feed character cannot be part of a URI (RFC 3986)
    private CountDownLatch signal = null;
    private Uri[] resultUri = null;
    private boolean canceled = false;

    public String open(Activity activity, int flags, String patterns, String initialPath)
    {
        String result;

        try {
            // only one file chooser can be opened at any given time
            if (signal != null)
                throw new UnsupportedOperationException("Only one file chooser can be opened at any given time");
            signal = new CountDownLatch(1);

            // reset the result
            resultUri = null;
            canceled = false;

            // get an URI from the initial path
            Uri initialUri = getUriFromInitialPath(initialPath);

            // get the mime types from the patterns
            String[] mimeTypes = getMimeTypesFromPatterns(patterns);
            Log.d(TAG, "Patterns: " + patterns);
            Log.d(TAG, "Mime types: " + TextUtils.join(";", mimeTypes));

            // open the file chooser
            // note: onActivityResult() will be called on the UI thread
            reallyOpen(activity, flags, mimeTypes, initialUri);

            /*

            The Allegro API specifies that al_show_native_file_dialog() blocks
            the calling thread until it returns. Since there is a need to
            handle ALLEGRO_EVENT_DISPLAY_HALT_DRAWING before this function
            returns, this must be called from a different thread.

            If al_show_native_file_dialog() is not called from a different
            thread, then the app will freeze. That will happen after the
            Activity is paused and before it is stopped. The freezing will
            take place in two places: at AllegroSurface.nativeOnDestroy() and
            in here.

            1) Here we wait for onActivityResult(), which is called after the
               we have the results of the file chooser.

            2) At AllegroSurface.nativeOnDestroy(), we find a condition variable
               that waits for al_acknowledge_drawing_halt().

            Such acknowledgement cannot happen because the file chooser is
            blocking. This fact, in turn, freezes the normal life cycle of the
            Activity after onPause() and before onStopped(). As a consequence,
            since onActivityResult() would be called by the system only after
            onStopped() and before onRestart(), we never get a result from the
            file chooser and the app freezes.

            Solution: call al_show_native_file_dialog() from another thread.

            */

            // block the calling thread
            Log.d(TAG, "Waiting for user input");
            signal.await();
            signal = null;
            Log.d(TAG, "The file chooser has just been closed!");
        }
        catch (Exception e) {
            Log.e(TAG, e.getMessage(), e);
            resultUri = null;
        }

        // process the result
        // due to the constraints of Scoped Storage, we return content:// URIs instead of file paths
        if (resultUri != null) {
            Log.d(TAG, "Result: success");

            StringBuilder sb = new StringBuilder();
            for (int i = 0; i < resultUri.length; i++) {
                sb.append(resultUri[i].toString());
                sb.append(URI_DELIMITER);
            }
            result = sb.toString();
        }
        else if (canceled) {
            Log.d(TAG, "Result: canceled");
            result = "";
        }
        else {
            Log.d(TAG, "Result: failure");
            result = null;
        }

        // done!
        resultUri = null;
        return result;
    }

    public void onActivityResult(int requestCode, int resultCode, Intent resultData)
    {
        if (requestCode != REQUEST_FILE_CHOOSER)
            return;

        // canceled?
        if (resultCode != Activity.RESULT_OK || resultData == null) {
            canceled = (resultCode != Activity.RESULT_OK);
            close(null);
            return;
        }

        // check for multiple results
        if (Build.VERSION.SDK_INT >= 16) {
            android.content.ClipData clipData = resultData.getClipData();
            if (clipData != null) {
                // multiple results
                int n = clipData.getItemCount();
                Uri[] uri = new Uri[n];
                for (int i = 0; i < n; i++)
                    uri[i] = clipData.getItemAt(i).getUri();
                close(uri);
                return;
            }
        }

        // single result
        Uri uri = resultData.getData();
        if (uri != null) {
            close(new Uri[] { uri });
            return;
        }

        // shouldn't happen
        close(null);
    }

    private void close(Uri[] result)
    {
        // store the result, which is null or an array of content:// URIs
        resultUri = result;

        // unblock the calling thread
        if (signal != null)
            signal.countDown();
    }

    private void reallyOpen(Activity activity, int flags, String[] mimeTypes, Uri initialUri) throws RuntimeException
    {
        if (Build.VERSION.SDK_INT < 19)
            throw new UnsupportedOperationException("Unsupported operation in API level " + Build.VERSION.SDK_INT);

        String action = selectAction(flags);
        Intent intent = new Intent(action);

        if (0 != (flags & AllegroDialogConst.ALLEGRO_FILECHOOSER_SAVE)) {
            // "Save file": must indicate a mime type
            intent.setType("application/octet-stream"); // binary
            //intent.setType("text/plain"); // plain text
            if (mimeTypes.length > 0 && !mimeTypes[0].equals("*/*"))
                intent.setType(mimeTypes[0]);
        }
        else if (0 == (flags & AllegroDialogConst.ALLEGRO_FILECHOOSER_FOLDER)) {
            // "Load file"
            if (mimeTypes.length != 1) {
                intent.setType("*/*");
                if (mimeTypes.length > 0)
                    intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
                else if (0 != (flags & AllegroDialogConst.ALLEGRO_FILECHOOSER_PICTURES))
                    intent.setType("image/*");
            }
            else
                intent.setType(mimeTypes[0]);
        }

        if (0 == (flags & AllegroDialogConst.ALLEGRO_FILECHOOSER_FOLDER))
            intent.addCategory(Intent.CATEGORY_OPENABLE);

        if (0 != (flags & AllegroDialogConst.ALLEGRO_FILECHOOSER_MULTIPLE))
            intent.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, true);

        if (Build.VERSION.SDK_INT >= 26) {
            if (initialUri != null)
                intent.putExtra(android.provider.DocumentsContract.EXTRA_INITIAL_URI, initialUri);
        }

        // invoke the intent
        Log.d(TAG, "invoking intent " + action);
        activity.startActivityForResult(intent, REQUEST_FILE_CHOOSER); // throws ActivityNotFoundException

        /*

        "To support media file access on devices that run Android 9 (API level
        28) or lower, declare the READ_EXTERNAL_STORAGE permission and set the
        maxSdkVersion to 28."

        https://developer.android.com/training/data-storage/shared/documents-files

        */
    }

    private String selectAction(int flags) throws UnsupportedOperationException
    {
        if (0 != (flags & AllegroDialogConst.ALLEGRO_FILECHOOSER_FOLDER)) {
            if (Build.VERSION.SDK_INT < 21)
                throw new UnsupportedOperationException("Unsupported operation in API level " + Build.VERSION.SDK_INT);

            return Intent.ACTION_OPEN_DOCUMENT_TREE;
        }

        if (0 != (flags & AllegroDialogConst.ALLEGRO_FILECHOOSER_SAVE)) {
            /*

            "ACTION_CREATE_DOCUMENT cannot overwrite an existing file. If your
            app tries to save a file with the same name, the system appends a
            number in parentheses at the end of the file name.

            For example, if your app tries to save a file called confirmation.pdf
            in a directory that already has a file with that name, the system
            saves the new file with the name confirmation(1).pdf."

            https://developer.android.com/training/data-storage/shared/documents-files#create-file

            */

            return Intent.ACTION_CREATE_DOCUMENT;
        }

        return Intent.ACTION_OPEN_DOCUMENT;
    }

    String[] getMimeTypesFromPatterns(String patterns)
    {
        MimeTypeMap mimeTypeMap = MimeTypeMap.getSingleton();
        String[] pattern = patterns.toLowerCase().split(";");
        ArrayList<String> mimeType = new ArrayList<String>();

        for (int i = 0; i < pattern.length; i++) {
            String mime = getMimeTypeFromPattern(pattern[i], mimeTypeMap);
            if (mime != null) // we discard unknown mime types
                mimeType.add(mime);
        }

        return mimeType.toArray(new String[0]);
    }

    private String getMimeTypeFromPattern(String pattern, MimeTypeMap mimeTypeMap)
    {
        // a pattern may be a mime type or an extension
        if (pattern.contains("/"))
            return pattern; // assume it's a mime type
        else if (pattern.equals("") || pattern.equals(".") || pattern.equals("*."))
            return null; // ignore empty
        else if (pattern.equals("*.*") || pattern.equals("*") || pattern.equals(".*"))
            return "*/*"; // return any
        else if (pattern.startsWith("."))
            return mimeTypeMap.getMimeTypeFromExtension(pattern.substring(1)); // remove leading '.'
        else if (pattern.startsWith("*."))
            return mimeTypeMap.getMimeTypeFromExtension(pattern.substring(2)); // possibly null
        else
            return mimeTypeMap.getMimeTypeFromExtension(pattern); // possibly null
    }

    private Uri getUriFromInitialPath(String initialPath)
    {
        // "Location should specify a document URI or a tree URI with document ID."
        // https://developer.android.com/reference/android/provider/DocumentsContract.html#EXTRA_INITIAL_URI
        if (initialPath.startsWith("content://"))
            return Uri.parse(initialPath);

        return null;
    }
}


/**
 * An implementation of the text log
 */
class AllegroTextLog
{
    public void append(String tag, String message)
    {
        Log.i(tag, message);
    }
}


/**
 * Constants of the Native Dialog Addon
 */
class AllegroDialogConst
{
    // allegro_native_dialog.h
    public static final int ALLEGRO_FILECHOOSER_FILE_MUST_EXIST = 1;
    public static final int ALLEGRO_FILECHOOSER_SAVE = 2;
    public static final int ALLEGRO_FILECHOOSER_FOLDER = 4;
    public static final int ALLEGRO_FILECHOOSER_PICTURES = 8;
    public static final int ALLEGRO_FILECHOOSER_SHOW_HIDDEN = 16;
    public static final int ALLEGRO_FILECHOOSER_MULTIPLE = 32;

    public static final int ALLEGRO_MESSAGEBOX_WARN = 1;
    public static final int ALLEGRO_MESSAGEBOX_ERROR = 2;
    public static final int ALLEGRO_MESSAGEBOX_OK_CANCEL = 4;
    public static final int ALLEGRO_MESSAGEBOX_YES_NO = 8;
    public static final int ALLEGRO_MESSAGEBOX_QUESTION = 16;
}


/* vim: set sts=4 sw=4 et: */
