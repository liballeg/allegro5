/* alplatf.h is generated from alplatf.h.cmake */
#cmakedefine ALLEGRO_MINGW32
#cmakedefine ALLEGRO_UNIX
#cmakedefine ALLEGRO_MSVC
#cmakedefine ALLEGRO_MACOSX
#cmakedefine ALLEGRO_BCC32
#cmakedefine ALLEGRO_IPHONE
#cmakedefine ALLEGRO_ANDROID
#cmakedefine ALLEGRO_RASPBERRYPI
#cmakedefine ALLEGRO_CFG_NO_FPU
#cmakedefine ALLEGRO_CFG_DLL_TLS
#cmakedefine ALLEGRO_CFG_PTHREADS_TLS
#cmakedefine ALLEGRO_CFG_RELEASE_LOGGING

#cmakedefine ALLEGRO_CFG_D3D
#cmakedefine ALLEGRO_CFG_D3D9EX
#cmakedefine ALLEGRO_CFG_D3DX9
#cmakedefine ALLEGRO_CFG_XINPUT
#cmakedefine ALLEGRO_CFG_OPENGL
#cmakedefine ALLEGRO_CFG_OPENGLES
#cmakedefine ALLEGRO_CFG_OPENGLES1
#cmakedefine ALLEGRO_CFG_OPENGLES2
#cmakedefine ALLEGRO_CFG_OPENGLES3
#cmakedefine ALLEGRO_CFG_OPENGL_FIXED_FUNCTION
#cmakedefine ALLEGRO_CFG_OPENGL_PROGRAMMABLE_PIPELINE
#cmakedefine ALLEGRO_CFG_SHADER_GLSL
#cmakedefine ALLEGRO_CFG_SHADER_HLSL

#cmakedefine ALLEGRO_CFG_ANDROID_LEGACY

#cmakedefine ALLEGRO_CFG_HAVE_XINPUT_CAPABILITIES_EX

/*---------------------------------------------------------------------------*/

/* Define to 1 if you have the corresponding header file. */
#cmakedefine ALLEGRO_HAVE_DIRENT_H
#cmakedefine ALLEGRO_HAVE_INTTYPES_H
#cmakedefine ALLEGRO_HAVE_LINUX_AWE_VOICE_H
#cmakedefine ALLEGRO_HAVE_LINUX_INPUT_H
#cmakedefine ALLEGRO_HAVE_LINUX_SOUNDCARD_H
#cmakedefine ALLEGRO_HAVE_MACHINE_SOUNDCARD_H
#cmakedefine ALLEGRO_HAVE_SOUNDCARD_H
#cmakedefine ALLEGRO_HAVE_STDBOOL_H
#cmakedefine ALLEGRO_HAVE_STDINT_H
#cmakedefine ALLEGRO_HAVE_SV_PROCFS_H
#cmakedefine ALLEGRO_HAVE_SYS_IO_H
#cmakedefine ALLEGRO_HAVE_SYS_SOUNDCARD_H
#cmakedefine ALLEGRO_HAVE_SYS_STAT_H
#cmakedefine ALLEGRO_HAVE_SYS_TIME_H
#cmakedefine ALLEGRO_HAVE_TIME_H
#cmakedefine ALLEGRO_HAVE_SYS_UTSNAME_H
#cmakedefine ALLEGRO_HAVE_SYS_TYPES_H
#cmakedefine ALLEGRO_HAVE_OSATOMIC_H
#cmakedefine ALLEGRO_HAVE_SYS_INOTIFY_H
#cmakedefine ALLEGRO_HAVE_SAL_H

/* Define to 1 if the corresponding functions are available. */
#cmakedefine ALLEGRO_HAVE_GETEXECNAME
#cmakedefine ALLEGRO_HAVE_MKSTEMP
#cmakedefine ALLEGRO_HAVE_MMAP
#cmakedefine ALLEGRO_HAVE_MPROTECT
#cmakedefine ALLEGRO_HAVE_SCHED_YIELD
#cmakedefine ALLEGRO_HAVE_SYSCONF
#cmakedefine ALLEGRO_HAVE_SYSCTL

#cmakedefine ALLEGRO_HAVE_FSEEKO
#cmakedefine ALLEGRO_HAVE_FTELLO
#cmakedefine ALLEGRO_HAVE_STRERROR_R
#cmakedefine ALLEGRO_HAVE_STRERROR_S
#cmakedefine ALLEGRO_HAVE_VA_COPY
#cmakedefine ALLEGRO_HAVE_FTELLI64
#cmakedefine ALLEGRO_HAVE_FSEEKI64

/* Define to 1 if procfs reveals argc and argv */
#cmakedefine ALLEGRO_HAVE_PROCFS_ARGCV

/*---------------------------------------------------------------------------*/

/* Define if target machine is little endian. */
#cmakedefine ALLEGRO_LITTLE_ENDIAN

/* Define if target machine is big endian. */
#cmakedefine ALLEGRO_BIG_ENDIAN

/* Define if target platform is Darwin. */
#cmakedefine ALLEGRO_DARWIN

/*---------------------------------------------------------------------------*/

/* Define if you need support for X-Windows. */
#cmakedefine ALLEGRO_WITH_XWINDOWS

/* Define if XCursor ARGB extension is available. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XCURSOR

/* Define if XF86VidMode extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XF86VIDMODE

/* Define if Xinerama extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XINERAMA

/* Define if XRandR extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XRANDR

/* Define if XScreenSaver extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XSCREENSAVER

/* Define if XIM extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XIM

/* Define if XInput 2.2 X11 extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XINPUT2


/*---------------------------------------------------------------------------*/

/* Define if target platform is linux. */
#cmakedefine ALLEGRO_LINUX

/* Define if we are building with SDL backend. */
#cmakedefine ALLEGRO_SDL

/* Define if sleep should be used instead of threads (only useful for emscripten without web workers) */
#cmakedefine ALLEGRO_WAIT_EVENT_SLEEP

/*---------------------------------------------------------------------------*/
/* vi: set ft=c ts=3 sts=3 sw=3 et: */
