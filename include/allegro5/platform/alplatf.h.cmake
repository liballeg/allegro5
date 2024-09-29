/* alplatf.h is generated from alplatf.h.cmake */
#cmakedefine A5O_MINGW32
#cmakedefine A5O_UNIX
#cmakedefine A5O_MSVC
#cmakedefine A5O_MACOSX
#cmakedefine A5O_BCC32
#cmakedefine A5O_IPHONE
#cmakedefine A5O_ANDROID
#cmakedefine A5O_RASPBERRYPI
#cmakedefine A5O_CFG_NO_FPU
#cmakedefine A5O_CFG_DLL_TLS
#cmakedefine A5O_CFG_PTHREADS_TLS
#cmakedefine A5O_CFG_RELEASE_LOGGING

#cmakedefine A5O_CFG_D3D
#cmakedefine A5O_CFG_D3D9EX
#cmakedefine A5O_CFG_D3DX9
#cmakedefine A5O_CFG_XINPUT
#cmakedefine A5O_CFG_OPENGL
#cmakedefine A5O_CFG_OPENGLES
#cmakedefine A5O_CFG_OPENGLES1
#cmakedefine A5O_CFG_OPENGLES2
#cmakedefine A5O_CFG_OPENGLES3
#cmakedefine A5O_CFG_OPENGL_FIXED_FUNCTION
#cmakedefine A5O_CFG_OPENGL_PROGRAMMABLE_PIPELINE
#cmakedefine A5O_CFG_SHADER_GLSL
#cmakedefine A5O_CFG_SHADER_HLSL

#cmakedefine A5O_CFG_ANDROID_LEGACY

#cmakedefine A5O_CFG_HAVE_XINPUT_CAPABILITIES_EX

/*---------------------------------------------------------------------------*/

/* Define to 1 if you have the corresponding header file. */
#cmakedefine A5O_HAVE_DIRENT_H
#cmakedefine A5O_HAVE_INTTYPES_H
#cmakedefine A5O_HAVE_LINUX_AWE_VOICE_H
#cmakedefine A5O_HAVE_LINUX_INPUT_H
#cmakedefine A5O_HAVE_LINUX_SOUNDCARD_H
#cmakedefine A5O_HAVE_MACHINE_SOUNDCARD_H
#cmakedefine A5O_HAVE_SOUNDCARD_H
#cmakedefine A5O_HAVE_STDBOOL_H
#cmakedefine A5O_HAVE_STDINT_H
#cmakedefine A5O_HAVE_SV_PROCFS_H
#cmakedefine A5O_HAVE_SYS_IO_H
#cmakedefine A5O_HAVE_SYS_SOUNDCARD_H
#cmakedefine A5O_HAVE_SYS_STAT_H
#cmakedefine A5O_HAVE_SYS_TIME_H
#cmakedefine A5O_HAVE_TIME_H
#cmakedefine A5O_HAVE_SYS_UTSNAME_H
#cmakedefine A5O_HAVE_SYS_TYPES_H
#cmakedefine A5O_HAVE_OSATOMIC_H
#cmakedefine A5O_HAVE_SYS_INOTIFY_H
#cmakedefine A5O_HAVE_SAL_H

/* Define to 1 if the corresponding functions are available. */
#cmakedefine A5O_HAVE_GETEXECNAME
#cmakedefine A5O_HAVE_MKSTEMP
#cmakedefine A5O_HAVE_MMAP
#cmakedefine A5O_HAVE_MPROTECT
#cmakedefine A5O_HAVE_SCHED_YIELD
#cmakedefine A5O_HAVE_SYSCONF
#cmakedefine A5O_HAVE_SYSCTL

#cmakedefine A5O_HAVE_FSEEKO
#cmakedefine A5O_HAVE_FTELLO
#cmakedefine A5O_HAVE_STRERROR_R
#cmakedefine A5O_HAVE_STRERROR_S
#cmakedefine A5O_HAVE_VA_COPY
#cmakedefine A5O_HAVE_FTELLI64
#cmakedefine A5O_HAVE_FSEEKI64

/* Define to 1 if procfs reveals argc and argv */
#cmakedefine A5O_HAVE_PROCFS_ARGCV

/*---------------------------------------------------------------------------*/

/* Define if target machine is little endian. */
#cmakedefine A5O_LITTLE_ENDIAN

/* Define if target machine is big endian. */
#cmakedefine A5O_BIG_ENDIAN

/* Define if target platform is Darwin. */
#cmakedefine A5O_DARWIN

/*---------------------------------------------------------------------------*/

/* Define if you need support for X-Windows. */
#cmakedefine A5O_WITH_XWINDOWS

/* Define if XCursor ARGB extension is available. */
#cmakedefine A5O_XWINDOWS_WITH_XCURSOR

/* Define if XF86VidMode extension is supported. */
#cmakedefine A5O_XWINDOWS_WITH_XF86VIDMODE

/* Define if Xinerama extension is supported. */
#cmakedefine A5O_XWINDOWS_WITH_XINERAMA

/* Define if XRandR extension is supported. */
#cmakedefine A5O_XWINDOWS_WITH_XRANDR

/* Define if XScreenSaver extension is supported. */
#cmakedefine A5O_XWINDOWS_WITH_XSCREENSAVER

/* Define if XIM extension is supported. */
#cmakedefine A5O_XWINDOWS_WITH_XIM

/* Define if XInput 2.2 X11 extension is supported. */
#cmakedefine A5O_XWINDOWS_WITH_XINPUT2


/*---------------------------------------------------------------------------*/

/* Define if target platform is linux. */
#cmakedefine A5O_LINUX

/* Define if we are building with SDL backend. */
#cmakedefine A5O_SDL

/* Define if sleep should be used instead of threads (only useful for emscripten without web workers) */
#cmakedefine A5O_WAIT_EVENT_SLEEP

/*---------------------------------------------------------------------------*/
/* vi: set ft=c ts=3 sts=3 sw=3 et: */
