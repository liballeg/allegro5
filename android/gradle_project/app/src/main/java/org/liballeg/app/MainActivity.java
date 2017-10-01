package org.liballeg.app;
import org.liballeg.android.AllegroActivity;
import android.util.Log;

public class MainActivity extends AllegroActivity {

    static void loadLibrary(String name) {
        try {
            // try loading the debug library first.
            Log.d("loadLibrary", name + "-debug");
            System.loadLibrary(name + "-debug");
        } catch (UnsatisfiedLinkError e) {
            try {
                // If it fails load the release library.
                Log.d("loadLibrary", name);
                System.loadLibrary(name);
            }
            catch (UnsatisfiedLinkError e2) {
                // We still continue as failing to load an addon may
                // not be a fatal error - for example if the TTF was
                // not built we can still run an example which does not
                // need that addon.
                Log.d("loadLibrary", name + " FAILED");
            }
        }
    }
        
    static {
        loadLibrary("allegro");
        loadLibrary("allegro_primitives");
        loadLibrary("allegro_image");
        loadLibrary("allegro_font");
        loadLibrary("allegro_ttf");
        loadLibrary("allegro_audio");
        loadLibrary("allegro_acodec");
        loadLibrary("allegro_color");
        loadLibrary("allegro_memfile");
        loadLibrary("allegro_physfs");
        loadLibrary("allegro_video");
    }
    public MainActivity() {
        super("libnative-lib.so");
    }
}
