/* include/allegro/internal/platform/alunixac.h.  Generated automatically by configure.  */
/* include/allegro/internal/platform/alunixac.hin.  Generated automatically from configure.in by autoheader.  */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define if you don't have vprintf but do have _doprnt.  */
/* #undef HAVE_DOPRNT */

/* Define if you have a working `mmap' system call.  */
#define HAVE_MMAP 1

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF 1

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define if your processor stores words with the most significant
   byte first (like Motorola and SPARC, unlike Intel and VAX).  */
/* #undef WORDS_BIGENDIAN */

/* Define if you want support for 8 bpp modes.  */
#define ALLEGRO_COLOR8 1

/* Define if you want support for 16 bpp modes.  */
#define ALLEGRO_COLOR16 1

/* Define if you want support for 24 bpp modes.  */
#define ALLEGRO_COLOR24 1

/* Define if you want support for 32 bpp modes.  */
#define ALLEGRO_COLOR32 1

/* Define if compiler prepends underscore to symbols.  */
/* #undef ALLEGRO_ASM_PREFIX */

/* Define if assembler supports MMX.  */
#define ALLEGRO_MMX 1

/* Define for Unix platforms, to use C convention for bank switching.  */
/* #undef ALLEGRO_NO_ASM */

/* Define if target platform is linux.  */
#define ALLEGRO_LINUX 1

/* Define to enable Linux console VGA driver */
#define ALLEGRO_LINUX_VGA 1

/* Define to enable Linux console fbcon driver */
#define ALLEGRO_LINUX_FBCON 1

/* Define to enable Linux console VBE/AF driver */
#define ALLEGRO_LINUX_VBEAF 1

/* Define to enable Linux console SVGAlib driver */
#define ALLEGRO_LINUX_SVGALIB 1

/* Define if SVGAlib driver can check vga_version */
/* #undef ALLEGRO_LINUX_SVGALIB_HAVE_VGA_VERSION */

/* Define if target machine is little endian.  */
#define ALLEGRO_LITTLE_ENDIAN 1

/* Define if target machine is big endian.  */
/* #undef ALLEGRO_BIG_ENDIAN */

/* Define if dynamically loaded modules are supported.  */
#define ALLEGRO_WITH_MODULES 1

/* Define if you need support for X-Windows.  */
#define ALLEGRO_WITH_XWINDOWS 1

/* Define if MIT-SHM extension is supported.  */
#define ALLEGRO_XWINDOWS_WITH_SHM 1

/* Define if XF86VidMode extension is supported.  */
#define ALLEGRO_XWINDOWS_WITH_XF86VIDMODE 1

/* Define if XF86DGA extension is supported.  */
#define ALLEGRO_XWINDOWS_WITH_XF86DGA 1

/* Define if DGA version 2.0 or newer is supported */
/* #undef ALLEGRO_XWINDOWS_WITH_XF86DGA2 */

/* Define if OSS DIGI driver is supported.  */
#define ALLEGRO_WITH_OSSDIGI 1

/* Define if OSS MIDI driver is supported.  */
#define ALLEGRO_WITH_OSSMIDI 1

/* Define if ALSA DIGI driver is supported.  */
/* #undef ALLEGRO_WITH_ALSADIGI */

/* Define if ALSA MIDI driver is supported.  */
/* #undef ALLEGRO_WITH_ALSAMIDI */

/* Define if ESD DIGI driver is supported.  */
#define ALLEGRO_WITH_ESDDIGI 1

/* Define to (void *)-1, if MAP_FAILED is not defined.  */
/* #undef MAP_FAILED */

/* Define if constructor attribute is supported. */
#define ALLEGRO_USE_CONSTRUCTOR 1

/* Define if sched_yield is provided by some library.  */
#define ALLEGRO_USE_SCHED_YIELD 1

/* Define if you have the getpagesize function.  */
#define HAVE_GETPAGESIZE 1

/* Define if you have the memcmp function.  */
#define HAVE_MEMCMP 1

/* Define if you have the mkstemp function.  */
#define HAVE_MKSTEMP 1

/* Define if you have the stricmp function.  */
/* #undef HAVE_STRICMP */

/* Define if you have the strlwr function.  */
/* #undef HAVE_STRLWR */

/* Define if you have the strupr function.  */
/* #undef HAVE_STRUPR */

/* Define if you have the <dirent.h> header file.  */
#define HAVE_DIRENT_H 1

/* Define if you have the <dlfcn.h> header file.  */
#define HAVE_DLFCN_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <linux/joystick.h> header file.  */
#define HAVE_LINUX_JOYSTICK_H 1

/* Define if you have the <linux/soundcard.h> header file.  */
#define HAVE_LINUX_SOUNDCARD_H 1

/* Define if you have the <machine/soundcard.h> header file.  */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define if you have the <ndir.h> header file.  */
/* #undef HAVE_NDIR_H */

/* Define if you have the <soundcard.h> header file.  */
/* #undef HAVE_SOUNDCARD_H */

/* Define if you have the <sys/dir.h> header file.  */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/io.h> header file.  */
#define HAVE_SYS_IO_H 1

/* Define if you have the <sys/ndir.h> header file.  */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/soundcard.h> header file.  */
#define HAVE_SYS_SOUNDCARD_H 1

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/utsname.h> header file.  */
#define HAVE_SYS_UTSNAME_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the pthread library (-lpthread).  */
#define HAVE_LIBPTHREAD 1
