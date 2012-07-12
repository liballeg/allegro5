set(ALLEGRO_SRC_FILES
    src/allegro.c
    src/bitmap_io.c
    src/bitmap.c
    src/blenders.c
    src/config.c
    src/convert.c
    src/display.c
    src/display_settings.c
    src/dtor.c
    src/events.c
    src/evtsrc.c
    src/file.c
    src/file_slice.c
    src/file_stdio.c
    src/fshook.c
    src/fshook_stdio.c
    src/inline.c
    src/joynu.c
    src/keybdnu.c
    src/libc.c
    src/math.c
    src/memblit.c
    src/memdraw.c
    src/memory.c
    src/mousenu.c
    src/path.c
    src/pixels.c
    src/system.c
    src/threads.c
    src/timernu.c
    src/tls.c
    src/touch_input.c
    src/transformations.c
    src/tri_soft.c
    src/utf8.c
    src/misc/aatree.c
    src/misc/bstrlib.c
    src/misc/list.c
    src/misc/vector.c
    )

set(ALLEGRO_SRC_C_FILES
    )

set(ALLEGRO_SRC_I386_FILES
    )

set(ALLEGRO_SRC_WIN_FILES
    src/win/wjoydrv.c
    src/win/wjoydxnu.cpp
    src/win/wkeyboard.c
    src/win/wmcursor.c
    src/win/wmouse.c
    src/win/wsystem.c
    src/win/wwindow.c
    src/win/wthread.c
    src/win/wtime.c
    src/win/wtouch_input.c
    src/win/wxthread.c
    )

set(ALLEGRO_SRC_D3D_FILES
    src/win/d3d_bmp.cpp
    src/win/d3d_disp.cpp
    src/win/d3d_render_state.cpp
    src/win/d3d_display_formats.cpp
    )

set(ALLEGRO_SRC_OPENGL_FILES
    src/opengl/extensions.c
    src/opengl/ogl_bitmap.c
    src/opengl/ogl_display.c
    src/opengl/ogl_draw.c
    src/opengl/ogl_lock.c
    src/opengl/ogl_render_state.c
    )

set(ALLEGRO_SRC_WGL_FILES
    src/win/wgl_disp.c
    )

set(ALLEGRO_SRC_LINUX_FILES
#    src/linux/fbcon.c
#    src/linux/lconsole.c
#    src/linux/lgfxdrv.c
#    src/linux/ljoynu.c
#    src/linux/lkeybdnu.c
#    src/linux/lmemory.c
#    src/linux/lmsedrv.c
#    src/linux/lmseev.c
#    src/linux/lsystem.c
#    src/linux/lvga.c
#    src/linux/lvgahelp.c
#    src/linux/svgalib.c
#    src/linux/svgalibs.s
#    src/linux/vtswitch.c
#    src/misc/vbeaf.c
#    src/misc/vbeafs.s
#    src/misc/vgaregs.c
#    src/misc/vga.c
#    src/misc/modex.c
#    src/misc/modexgfx.s
    )

set(ALLEGRO_SRC_UNIX_FILES
    src/unix/udrvlist.c
    src/unix/ufdwatch.c
    src/unix/ugfxdrv.c
    src/unix/ujoydrv.c
    src/unix/ukeybd.c
    # src/unix/umodules.c   # not used currently
    src/unix/umouse.c
    src/unix/upath.c
    src/unix/utime.c
    src/unix/uxthread.c
    )

set(ALLEGRO_SRC_X_FILES
    src/x/xcursor.c
    src/x/xkeyboard.c
    src/x/xmousenu.c
    src/x/xdisplay.c
    src/x/xfullscreen.c
    src/x/xglx_config.c
    src/x/xrandr.c
    src/x/xsystem.c
    src/linux/ljoynu.c
    )

set(ALLEGRO_SRC_MACOSX_FILES
    src/macosx/hidjoy.m
    src/macosx/hidjoy-10.4.m
    src/macosx/hidman.m
    src/macosx/keybd.m
    src/macosx/qzmouse.m
    src/macosx/system.m
    src/macosx/osxgl.m
    src/macosx/osx_app_delegate.m
    src/unix/utime.c
    src/unix/uxthread.c
    )

set(ALLEGRO_SRC_GP2XWIZ_FILES
    src/gp2xwiz/wiz_display_opengl.c
    src/gp2xwiz/wiz_display_fb.c
    src/gp2xwiz/wiz_system.c
    src/gp2xwiz/wiz_joystick.c
    src/optimized.c
    src/linux/ljoynu.c
    )

set(ALLEGRO_SRC_IPHONE_FILES
    src/iphone/
    src/iphone/allegroAppDelegate.m
    src/iphone/EAGLView.m
    src/iphone/ViewController.m
    src/iphone/iphone_display.m
    src/iphone/iphone_joystick.m
    src/iphone/iphone_keyboard.c
    src/iphone/iphone_main.m
    src/iphone/iphone_mouse.m
    src/iphone/iphone_path.m
    src/iphone/iphone_system.c
    src/iphone/iphone_touch_input.m
    src/unix/utime.c
    src/unix/uxthread.c
    )

set(ALLEGRO_SRC_ANDROID_FILES
   src/unix/utime.c
   src/unix/uxthread.c
   src/android/android_apk_file.c
   src/android/android_display.c
   src/android/android_joystick.c
   src/android/android_mouse.c
   src/android/android_keyboard.c
   src/android/android_system.c
   src/android/android_touch.c
   src/android/jni_helpers.c
)

set(ALLEGRO_MODULE_ALSADIGI_FILES
    )

set(ALLEGRO_MODULE_ALSAMIDI_FILES
    )

set(ALLEGRO_MODULE_ESD_FILES
    )

set(ALLEGRO_MODULE_ARTS_FILES
    )

set(ALLEGRO_MODULE_SGIAL_FILES
    )

set(ALLEGRO_MODULE_JACK_FILES
    )

set(ALLEGRO_INCLUDE_FILES
    # No files directly in the `include' root any more.
    )

set(ALLEGRO_INCLUDE_ALLEGRO_FILES
    include/allegro5/allegro5.h
    include/allegro5/allegro.h
    include/allegro5/alcompat.h
    include/allegro5/altime.h
    include/allegro5/base.h
    include/allegro5/bitmap_io.h
    include/allegro5/bitmap.h
    include/allegro5/color.h
    include/allegro5/config.h
    include/allegro5/debug.h
    include/allegro5/display.h
    include/allegro5/error.h
    include/allegro5/events.h
    include/allegro5/file.h
    include/allegro5/fixed.h
    include/allegro5/fmaths.h
    include/allegro5/fshook.h
    include/allegro5/joystick.h
    include/allegro5/keyboard.h
    include/allegro5/keycodes.h
    include/allegro5/memory.h
    include/allegro5/mouse.h
    include/allegro5/path.h
    include/allegro5/render_state.h
    include/allegro5/system.h
    include/allegro5/threads.h
    include/allegro5/tls.h
    include/allegro5/touch_input.h
    include/allegro5/timer.h
    include/allegro5/transformations.h
    include/allegro5/utf8.h
    include/allegro5/allegro_opengl.h
    include/allegro5/allegro_direct3d.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES
    include/allegro5/inline/fmaths.inl
    )

set(ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES
    include/allegro5/internal/aintern.h
    include/allegro5/internal/aintern_atomicops.h
    include/allegro5/internal/aintern_bitmap.h
    include/allegro5/internal/aintern_blend.h
    include/allegro5/internal/aintern_convert.h
    include/allegro5/internal/aintern_display.h
    include/allegro5/internal/aintern_dtor.h
    include/allegro5/internal/aintern_events.h
    include/allegro5/internal/aintern_float.h
    include/allegro5/internal/aintern_fshook.h
    include/allegro5/internal/aintern_joystick.h
    include/allegro5/internal/aintern_keyboard.h
    include/allegro5/internal/aintern_list.h
    include/allegro5/internal/aintern_mouse.h
    include/allegro5/internal/aintern_opengl.h
    include/allegro5/internal/aintern_pixels.h
    include/allegro5/internal/aintern_system.h
    include/allegro5/internal/aintern_thread.h
    include/allegro5/internal/aintern_timer.h
    include/allegro5/internal/aintern_tls.h
    include/allegro5/internal/aintern_touch_input.h
    include/allegro5/internal/aintern_vector.h
    include/allegro5/internal/alconfig.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_OPENGL_FILES
    include/allegro5/opengl/gl_ext.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_OPENGL_GLEXT_FILES
    include/allegro5/opengl/GLext/gl_ext_alias.h
    include/allegro5/opengl/GLext/gl_ext_defs.h
    include/allegro5/opengl/GLext/glx_ext_alias.h
    include/allegro5/opengl/GLext/glx_ext_defs.h
    include/allegro5/opengl/GLext/wgl_ext_alias.h
    include/allegro5/opengl/GLext/wgl_ext_defs.h
    include/allegro5/opengl/GLext/gl_ext_api.h
    include/allegro5/opengl/GLext/gl_ext_list.h
    include/allegro5/opengl/GLext/glx_ext_api.h
    include/allegro5/opengl/GLext/glx_ext_list.h
    include/allegro5/opengl/GLext/wgl_ext_api.h
    include/allegro5/opengl/GLext/wgl_ext_list.h
   )

set(ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES
    include/allegro5/platform/aintandroid.h
    include/allegro5/platform/aintlnx.h
    include/allegro5/platform/aintosx.h
    include/allegro5/platform/aintunix.h
    include/allegro5/platform/aintuthr.h
    include/allegro5/platform/aintwin.h
    include/allegro5/platform/aintwthr.h
    include/allegro5/platform/al386gcc.h
    include/allegro5/platform/al386vc.h
    include/allegro5/platform/al386wat.h
    include/allegro5/platform/alandroid.h
    include/allegro5/platform/alandroidcfg.h
    include/allegro5/platform/albcc32.h
    include/allegro5/platform/almngw32.h
    include/allegro5/platform/almsvc.h
    include/allegro5/platform/alosx.h
    include/allegro5/platform/alosxcfg.h
    include/allegro5/platform/alucfg.h
    include/allegro5/platform/alunix.h
    include/allegro5/platform/alwatcom.h
    include/allegro5/platform/alwin.h
    include/allegro5/platform/ald3d.h
    include/allegro5/platform/astdbool.h
    include/allegro5/platform/astdint.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_WINDOWS_FILES
    include/allegro5/allegro_windows.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_MACOSX_FILES
    include/allegro5/allegro_osx.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_IPHONE_FILES
    include/allegro5/allegro_iphone.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_ANDROID_FILES
    include/allegro5/allegro_android.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED
    include/allegro5/platform/alplatf.h
    )
