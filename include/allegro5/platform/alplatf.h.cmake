/* alplatf.h is generated from alplatf.h.cmake */
#cmakedefine ALLEGRO_MINGW32
#cmakedefine ALLEGRO_UNIX
#cmakedefine ALLEGRO_MSVC
#cmakedefine ALLEGRO_CFG_D3D
#cmakedefine ALLEGRO_CFG_D3D9EX
#cmakedefine ALLEGRO_CFG_OPENGL
#cmakedefine ALLEGRO_MACOSX
#cmakedefine ALLEGRO_BCC32
#cmakedefine ALLEGRO_GP2XWIZ
#cmakedefine ALLEGRO_IPHONE

#cmakedefine ALLEGRO_CFG_ALLOW_SSE
#cmakedefine ALLEGRO_NO_ASM
#cmakedefine ALLEGRO_CFG_NO_FPU
#cmakedefine ALLEGRO_CFG_DLL_TLS
#cmakedefine ALLEGRO_CFG_PTHREADS_TLS

#cmakedefine ALLEGRO_CFG_GLSL_SHADERS
#cmakedefine ALLEGRO_CFG_HLSL_SHADERS
#cmakedefine ALLEGRO_CFG_CG_SHADERS

/*---------------------------------------------------------------------------*/

/* TODO: rename this */
#define RETSIGTYPE void

/* This is defined on the command-line in the autotools build. */
#define ALLEGRO_MODULES_PATH ${ALLEGRO_MODULES_PATH}

/*---------------------------------------------------------------------------*/

/* Define to 1 if you have the corresponding header file. */
#cmakedefine ALLEGRO_HAVE_DIRENT_H
#cmakedefine ALLEGRO_HAVE_INTTYPES_H
#cmakedefine ALLEGRO_HAVE_LINUX_AWE_VOICE_H
#cmakedefine ALLEGRO_HAVE_LINUX_INPUT_H
#cmakedefine ALLEGRO_HAVE_LINUX_JOYSTICK_H
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
#cmakedefine ALLEGRO_HAVE_SYS_TIMERFD_H

/* Define to 1 if the corresponding functions are available. */
#cmakedefine ALLEGRO_HAVE_GETEXECNAME
#cmakedefine ALLEGRO_HAVE_MKSTEMP
#cmakedefine ALLEGRO_HAVE_MMAP
#cmakedefine ALLEGRO_HAVE_MPROTECT
#cmakedefine ALLEGRO_HAVE_SCHED_YIELD
#cmakedefine ALLEGRO_HAVE_SYSCONF
#cmakedefine ALLEGRO_HAVE_FSEEKO
#cmakedefine ALLEGRO_HAVE_FTELLO
#cmakedefine ALLEGRO_HAVE_VA_COPY

/* Define to 1 if procfs reveals argc and argv */
#cmakedefine ALLEGRO_HAVE_PROCFS_ARGCV

/*---------------------------------------------------------------------------*/

/* Define if target machine is little endian. */
#cmakedefine ALLEGRO_LITTLE_ENDIAN

/* Define if target machine is big endian. */
#cmakedefine ALLEGRO_BIG_ENDIAN

/* Define for Unix platforms, to use C convention for bank switching. */
#cmakedefine ALLEGRO_NO_ASM

/* Define if compiler prepends underscore to symbols. */
#cmakedefine ALLEGRO_ASM_PREFIX

/* Define if assembler supports MMX. */
#cmakedefine ALLEGRO_MMX

/* Define if assembler supports SSE. */
#cmakedefine ALLEGRO_SSE

/* Define if target platform is Darwin. */
#cmakedefine ALLEGRO_DARWIN

/* Define if you have the pthread library. */
#cmakedefine ALLEGRO_HAVE_LIBPTHREAD

/* Define if constructor attribute is supported. */
#cmakedefine ALLEGRO_USE_CONSTRUCTOR

/* Define if dynamically loaded modules are supported. */
#cmakedefine ALLEGRO_WITH_MODULES

/*---------------------------------------------------------------------------*/

/* Define if you need support for X-Windows. */
#cmakedefine ALLEGRO_WITH_XWINDOWS

/* Define if MIT-SHM extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_SHM

/* Define if XCursor ARGB extension is available. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XCURSOR

/* Define if DGA version 2.0 or newer is supported */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XF86DGA2

/* Define if XF86VidMode extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XF86VIDMODE

/* Define if Xinerama extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XINERAMA

/* Define if XRandR extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XRANDR

/* Define if XIM extension is supported. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XIM

/* Define if xpm bitmap support is available. */
#cmakedefine ALLEGRO_XWINDOWS_WITH_XPM

/*---------------------------------------------------------------------------*/

/* Define if target platform is linux. */
#cmakedefine ALLEGRO_LINUX

/* Define to enable Linux console fbcon driver. */
#cmakedefine ALLEGRO_LINUX_FBCON

/* Define to enable Linux console SVGAlib driver. */
#cmakedefine ALLEGRO_LINUX_SVGALIB

/* Define if SVGAlib driver can check vga_version. */
#cmakedefine ALLEGRO_LINUX_SVGALIB_HAVE_VGA_VERSION

/* Define to enable Linux console VBE/AF driver. */
#cmakedefine ALLEGRO_LINUX_VBEAF

/* Define to enable Linux console VGA driver. */
#cmakedefine ALLEGRO_LINUX_VGA

/*---------------------------------------------------------------------------*/

/* Define to the installed ALSA version. */
#cmakedefine ALLEGRO_ALSA_VERSION ${ALLEGRO_ALSA_VERSION}

/* Define if ALSA DIGI driver is supported. */
#cmakedefine ALLEGRO_WITH_ALSADIGI

/* Define if ALSA MIDI driver is supported. */
#cmakedefine ALLEGRO_WITH_ALSAMIDI

/* Define if aRts DIGI driver is supported. */
#cmakedefine ALLEGRO_WITH_ARTSDIGI

/* Define if ESD DIGI driver is supported. */
#cmakedefine ALLEGRO_WITH_ESDDIGI

/* Define if JACK DIGI driver is supported. */
#cmakedefine ALLEGRO_WITH_JACKDIGI

/* Define if OSS DIGI driver is supported. */
#cmakedefine ALLEGRO_WITH_OSSDIGI

/* Define if OSS MIDI driver is supported. */
#cmakedefine ALLEGRO_WITH_OSSMIDI

/* Define if SGI AL DIGI driver is supported. */
#cmakedefine ALLEGRO_WITH_SGIALDIGI

/*---------------------------------------------------------------------------*/

/* TODO: Define to (void *)-1, if MAP_FAILED is not defined. */
/* TODO: rename this */
/* # cmakedefine MAP_FAILED */

/*---------------------------------------------------------------------------*/
/* vi: set ft=c ts=3 sts=3 sw=3 et: */
