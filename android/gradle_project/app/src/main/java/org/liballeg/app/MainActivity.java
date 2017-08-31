package org.liballeg.app;
import org.liballeg.android.AllegroActivity;
public class MainActivity extends AllegroActivity {
    static {
        System.loadLibrary("allegro-debug");
        System.loadLibrary("allegro_primitives-debug");
        System.loadLibrary("allegro_image-debug");
        System.loadLibrary("allegro_font-debug");
        System.loadLibrary("allegro_ttf-debug");
        System.loadLibrary("allegro_audio-debug");
        System.loadLibrary("allegro_acodec-debug");
        System.loadLibrary("allegro_color-debug");
    }
    public MainActivity() {
        super("libnative-lib.so");
    }
}
