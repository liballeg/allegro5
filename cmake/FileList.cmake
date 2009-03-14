set(ALLEGRO_SRC_FILES
    src/allegro.c
    src/bitmap_new.c
    src/blenders.c
    src/config.c
    src/convert.c
    src/display_new.c
    src/display_settings.c
    src/dtor.c
    src/events.c
    src/evtsrc.c
    src/fshook.c
    src/fshook_stdio.c
    src/graphics.c
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
    src/rotate.c
    src/system_new.c
    src/threads.c
    src/timernu.c
    src/tls.c
    src/unicode.c
    src/utf8.c
    src/misc/bstrlib.c
    src/misc/vector.c
    )

set(ALLEGRO_SRC_C_FILES
    )

set(ALLEGRO_SRC_I386_FILES
    )

set(ALLEGRO_SRC_DOS_FILES
    src/dos/adlib.c
    src/dos/awedata.c
    src/dos/dfile.c
    src/dos/dgfxdrv.c
    src/dos/djoydrv.c
    src/dos/dkeybd.c
    src/dos/dma.c
    src/dos/dmouse.c
    src/dos/dpmi.c
    src/dos/dsnddrv.c
    src/dos/dsystem.c
    src/dos/dtimer.c
    src/dos/emu8k.c
    src/dos/emu8kmid.c
    src/dos/essaudio.c
    src/dos/gpro.c
    src/dos/grip.c
    src/dos/gripjoy.c
    src/dos/gripfnc.s
    src/dos/ifsega.c
    src/dos/ifsega2f.c
    src/dos/ifsega2p.c
    src/dos/joystd.c
    src/dos/mpu.c
    src/dos/multijoy.c
    src/dos/n64pad.c
    src/dos/pic.c
    src/dos/psxpad.c
    src/dos/sb.c
    src/dos/sndscape.c
    src/dos/snespad.c
    src/dos/sw.c
    src/dos/swpp.c
    src/dos/swpps.s
    src/dos/vesa.c
    src/dos/vesas.s
    src/dos/wss.c
    src/dos/ww.c
    src/misc/modex.c
    src/misc/modexgfx.s
    src/misc/modexsms.c
    src/misc/pckeys.c
    src/misc/vbeaf.c
    src/misc/vbeafs.s
    src/misc/vbeafex.c
    src/misc/vga.c
    src/misc/vgaregs.c
    )

set(ALLEGRO_SRC_WIN_FILES
    src/win/winput.c
    src/win/wjoydrv.c
    src/win/wjoydxnu.c
    src/win/wkeybdnu.c
    src/win/wmcursor.c
    src/win/wmouse.c
    src/win/wnewsys.c
    src/win/wnewwin.c
    src/win/wthread.c
    src/win/wtime.c
    src/win/wxthread.c
    )

set(ALLEGRO_SRC_D3D_FILES
    src/win/d3d_bmp.cpp
    src/win/d3d_disp.cpp
    )

set(ALLEGRO_SRC_OPENGL_FILES
    src/opengl/extensions.c
    src/opengl/ogl_bitmap.c
    src/opengl/ogl_draw.c
    src/opengl/ogl_display.c
    )

set(ALLEGRO_SRC_WGL_FILES
    src/win/wgl_disp.c
    )

set(ALLEGRO_SRC_BEOS_FILES
    src/beos/baccel.cpp
    src/beos/bdispsw.cpp
    src/beos/bdwindow.cpp
    src/beos/bgfx.c
    src/beos/bgfxapi.cpp
    src/beos/bgfxdrv.c
    src/beos/bjoy.c
    src/beos/bjoydrv.c
    src/beos/bjoyapi.cpp
    src/beos/bkey.c
    src/beos/bkeyapi.cpp
    src/beos/bkeydrv.c
    src/beos/bmidi.c
    src/beos/bmidiapi.cpp
    src/beos/bmididrv.c
    src/beos/bmousapi.cpp
    src/beos/bmousdrv.c
    src/beos/bmouse.c
    src/beos/boverlay.c
    src/beos/bsnd.c
    src/beos/bsndapi.cpp
    src/beos/bsnddrv.c
    src/beos/bswitch.s
    src/beos/bsysapi.cpp
    src/beos/bsysdrv.c
    src/beos/bsystem.c
    src/beos/btimeapi.cpp
    src/beos/btimedrv.c
    src/beos/btimer.c
    src/beos/bwindow.c
    src/beos/bwscreen.cpp
    src/unix/ufile.c
    src/misc/colconv.c
    src/misc/pckeys.c
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
    src/unix/udjgpp.c
    src/unix/udrvlist.c
    src/unix/udummy.c
    src/unix/ufdwatch.c
    src/unix/ugfxdrv.c
    src/unix/ujoydrv.c
    src/unix/ukeybd.c
    src/unix/umain.c
    src/unix/umodules.c
    src/unix/umouse.c
    src/unix/usystem.c
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
    src/x/xsystem.c
    src/linux/ljoynu.c
    )

set(ALLEGRO_SRC_QNX_FILES
    src/qnx/qdrivers.c
    src/qnx/qkeydrv.c
    src/qnx/qmouse.c
    src/qnx/qphaccel.c
    src/qnx/qphbmp.c
    src/qnx/qphfull.c
    src/qnx/qphoton.c
    src/qnx/qphwin.c
    src/qnx/qswitch.s
    src/qnx/qsystem.c
    src/unix/udjgpp.c
    src/unix/ufile.c
    src/unix/umain.c
    src/unix/usystem.c
    src/unix/utimer.c
    src/misc/colconv.c
    src/misc/pckeys.c
    )

set(ALLEGRO_SRC_MACOSX_FILES
    src/macosx/hidjoy.m
    src/macosx/hidman.m
    src/macosx/keybd.m
    src/macosx/main.m
    src/macosx/qzmouse.m
    src/macosx/system.m
    src/macosx/osxgl.m
    src/unix/utime.c
    src/unix/uxthread.c
    )

set(ALLEGRO_MODULE_VGA_FILES
    src/linux/lvga.c
    src/misc/modex.c
    src/misc/modexgfx.s
    src/misc/vga.c
    )

set(ALLEGRO_MODULE_SVGALIB_FILES
    src/linux/svgalib.c
    src/linux/svgalibs.s
    )

set(ALLEGRO_MODULE_FBCON_FILES
    src/linux/fbcon.c
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
    include/allegro5/alinline.h
    include/allegro5/altime.h
    include/allegro5/base.h
    include/allegro5/bitmap_new.h
    include/allegro5/color.h
    include/allegro5/color_new.h
    include/allegro5/config.h
    include/allegro5/debug.h
    include/allegro5/display_new.h
    include/allegro5/error.h
    include/allegro5/events.h
    include/allegro5/fixed.h
    include/allegro5/fmaths.h
    include/allegro5/fshook.h
    include/allegro5/joystick.h
    include/allegro5/keyboard.h
    include/allegro5/keycodes.h
    include/allegro5/memory.h
    include/allegro5/mouse.h
    include/allegro5/path.h
    include/allegro5/a5_opengl.h
    include/allegro5/a5_direct3d.h
    include/allegro5/system_new.h
    include/allegro5/threads.h
    include/allegro5/tls.h
    include/allegro5/timer.h
    include/allegro5/unicode.h
    include/allegro5/utf8.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_INLINE_FILES
    include/allegro5/inline/fmaths.inl
    )

set(ALLEGRO_INCLUDE_ALLEGRO_INTERNAL_FILES
    include/allegro5/internal/aintern.h
    include/allegro5/internal/aintern_atomicops.h
    include/allegro5/internal/aintern_bitmap.h
    include/allegro5/internal/aintern_display.h
    include/allegro5/internal/aintern_dtor.h
    include/allegro5/internal/aintern_events.h
    include/allegro5/internal/aintern_float.h
    include/allegro5/internal/aintern_fshook.h
    include/allegro5/internal/aintern_joystick.h
    include/allegro5/internal/aintern_keyboard.h
    include/allegro5/internal/aintern_mouse.h
    include/allegro5/internal/aintern_opengl.h
    include/allegro5/internal/aintern_pixels.h
    include/allegro5/internal/aintern_system.h
    include/allegro5/internal/aintern_thread.h
    include/allegro5/internal/aintern_vector.h
    include/allegro5/internal/aintvga.h
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
    include/allegro5/platform/aintbeos.h
    include/allegro5/platform/aintdos.h
    include/allegro5/platform/aintlnx.h
    include/allegro5/platform/aintosx.h
    include/allegro5/platform/aintqnx.h
    include/allegro5/platform/aintunix.h
    include/allegro5/platform/aintuthr.h
    include/allegro5/platform/aintwin.h
    include/allegro5/platform/aintwthr.h
    include/allegro5/platform/al386gcc.h
    include/allegro5/platform/al386vc.h
    include/allegro5/platform/al386wat.h
    include/allegro5/platform/albcc32.h
    include/allegro5/platform/albecfg.h
    include/allegro5/platform/albeos.h
    include/allegro5/platform/aldjgpp.h
    include/allegro5/platform/aldos.h
    include/allegro5/platform/almngw32.h
    include/allegro5/platform/almsvc.h
    include/allegro5/platform/alosx.h
    include/allegro5/platform/alosxcfg.h
    include/allegro5/platform/alqnx.h
    include/allegro5/platform/alqnxcfg.h
    include/allegro5/platform/alucfg.h
    include/allegro5/platform/alunix.h
    include/allegro5/platform/alwatcom.h
    include/allegro5/platform/alwin.h
    include/allegro5/platform/astdbool.h
    include/allegro5/platform/astdint.h
    )

set(ALLEGRO_INCLUDE_ALLEGRO_PLATFORM_FILES_GENERATED
    include/allegro5/platform/alplatf.h
    )
