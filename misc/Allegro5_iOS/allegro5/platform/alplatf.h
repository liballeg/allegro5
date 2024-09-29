/* alplatf.h is generated from alplatf.h.cmake */
/* #undef A5O_MINGW32 */
/* #undef A5O_UNIX */
/* #undef A5O_MSVC */
/* #undef A5O_MACOSX */
/* #undef A5O_BCC32 */
#define A5O_IPHONE
/* #undef A5O_ANDROID */
/* #undef A5O_RASPBERRYPI */
/* #undef A5O_CFG_NO_FPU */
/* #undef A5O_CFG_DLL_TLS */
#define A5O_CFG_PTHREADS_TLS
/* #undef A5O_CFG_RELEASE_LOGGING */

/* #undef A5O_CFG_D3D */
/* #undef A5O_CFG_D3D9EX */
/* #undef A5O_CFG_D3DX9 */
/* #undef A5O_CFG_XINPUT */
#define A5O_CFG_OPENGL
#define A5O_CFG_OPENGLES
#define A5O_CFG_OPENGLES2
#define A5O_CFG_OPENGL_PROGRAMMABLE_PIPELINE
#define A5O_CFG_SHADER_GLSL
/* #undef A5O_CFG_SHADER_HLSL */
/* #undef A5O_CFG_OPENGL_S3TC_LOCKING */

/* #undef A5O_CFG_ANDROID_LEGACY */

/*---------------------------------------------------------------------------*/

/* Define to 1 if you have the corresponding header file. */
#define A5O_HAVE_DIRENT_H
#define A5O_HAVE_INTTYPES_H
/* #undef A5O_HAVE_LINUX_AWE_VOICE_H */
/* #undef A5O_HAVE_LINUX_INPUT_H */
/* #undef A5O_HAVE_LINUX_SOUNDCARD_H */
/* #undef A5O_HAVE_MACHINE_SOUNDCARD_H */
/* #undef A5O_HAVE_SOUNDCARD_H */
#define A5O_HAVE_STDBOOL_H
#define A5O_HAVE_STDINT_H
/* #undef A5O_HAVE_SV_PROCFS_H */
/* #undef A5O_HAVE_SYS_IO_H */
/* #undef A5O_HAVE_SYS_SOUNDCARD_H */
#define A5O_HAVE_SYS_STAT_H
#define A5O_HAVE_SYS_TIME_H
#define A5O_HAVE_TIME_H
#define A5O_HAVE_SYS_UTSNAME_H
#define A5O_HAVE_SYS_TYPES_H
#define A5O_HAVE_OSATOMIC_H
/* #undef A5O_HAVE_SYS_INOTIFY_H */
/* #undef A5O_HAVE_SAL_H */

/* Define to 1 if the corresponding functions are available. */
/* #undef A5O_HAVE_GETEXECNAME */
#define A5O_HAVE_MKSTEMP
#define A5O_HAVE_MMAP
#define A5O_HAVE_MPROTECT
#define A5O_HAVE_SCHED_YIELD
#define A5O_HAVE_SYSCONF
#define A5O_HAVE_FSEEKO
#define A5O_HAVE_FTELLO
#define A5O_HAVE_STRERROR_R
/* #undef A5O_HAVE_STRERROR_S */
#define A5O_HAVE_VA_COPY

/* Define to 1 if procfs reveals argc and argv */
/* #undef A5O_HAVE_PROCFS_ARGCV */

/*---------------------------------------------------------------------------*/

/* Define if target machine is little endian. */
#define A5O_LITTLE_ENDIAN

/* Define if target machine is big endian. */
/* #undef A5O_BIG_ENDIAN */

/* Define if target platform is Darwin. */
/* #undef A5O_DARWIN */

/*---------------------------------------------------------------------------*/

/* Define if you need support for X-Windows. */
/* #undef A5O_WITH_XWINDOWS */

/* Define if XCursor ARGB extension is available. */
/* #undef A5O_XWINDOWS_WITH_XCURSOR */

/* Define if XF86VidMode extension is supported. */
/* #undef A5O_XWINDOWS_WITH_XF86VIDMODE */

/* Define if Xinerama extension is supported. */
/* #undef A5O_XWINDOWS_WITH_XINERAMA */

/* Define if XRandR extension is supported. */
/* #undef A5O_XWINDOWS_WITH_XRANDR */

/* Define if XIM extension is supported. */
/* #undef A5O_XWINDOWS_WITH_XIM */

/*---------------------------------------------------------------------------*/

/* Define if target platform is linux. */
/* #undef A5O_LINUX */

/* Define if we are building with SDL backend. */
/* #undef A5O_SDL */

/*---------------------------------------------------------------------------*/
/* vi: set ft=c ts=3 sts=3 sw=3 et: */
