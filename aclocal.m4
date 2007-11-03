# Place m4 macros here.

dnl
dnl Test for processor type.
dnl
dnl Variables:
dnl  allegro_cv_processor_type=(i386|sparc|unknown)
dnl
AC_DEFUN(ALLEGRO_ACTEST_PROCESSOR_TYPE,
[AC_BEFORE([$0], [ALLEGRO_ACTEST_SUPPORT_MMX])
AC_MSG_CHECKING(for processor type)
AC_CACHE_VAL(allegro_cv_processor_type,
[AC_TRY_COMPILE([], [asm (".globl _dummy_function\n"
"_dummy_function:\n"
"     pushl %%ebp\n"
"     movl %%esp, %%ebp\n"
"     leal 10(%%ebx, %%ecx, 4), %%edx\n"
"     call *%%edx\n"
"     addl %%ebx, %%eax\n"
"     popl %%ebp\n"
"     ret" : :)],
allegro_cv_processor_type=i386,
AC_TRY_COMPILE([], [asm (".globl _dummy_function\n"
"_dummy_function:\n"
"     save %%sp, -120, %%sp\n"
"     ld [[%%fp-20]], %%f12\n"
"     fitod %%f12, %%f10\n"
"     faddd %%f10, %%f8, %%f8\n"
"     ret\n"
"     restore" : :)],
allegro_cv_processor_type=sparc,
AC_TRY_COMPILE([], [asm (".globl _dummy_function\n"
"_dummy_function:\n"
"     pushq %%rbp\n"
"     movl %%esp, %%ebp\n"
"     leal 10(%%ebx, %%ecx, 4), %%edx\n"
"     callq *%%rdx\n"
"     addl %%ebx, %%eax\n"
"     popq %%rbp\n"
"     ret" : :)],
allegro_cv_processor_type=amd64,
allegro_cv_processor_type=unknown)))])
AC_MSG_RESULT($allegro_cv_processor_type)])

dnl
dnl Test if MMX instructions are supported by assembler.
dnl
dnl Variables:
dnl  allegro_enable_mmx=(yes|),
dnl  allegro_cv_support_mmx=(yes|no).
dnl
AC_DEFUN(ALLEGRO_ACTEST_SUPPORT_MMX,
[AC_REQUIRE([ALLEGRO_ACTEST_PROCESSOR_TYPE])
AC_BEFORE([$0], [ALLEGRO_ACTEST_SUPPORT_SSE])
AC_ARG_ENABLE(mmx,
[  --enable-mmx[=x]        enable the use of MMX instructions [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_mmx=yes,
allegro_enable_mmx=yes)
AC_MSG_CHECKING(for MMX support)
AC_CACHE_VAL(allegro_cv_support_mmx,
[if test "$allegro_cv_processor_type" = i386 -a "$allegro_enable_mmx" = yes; then
  AC_TRY_COMPILE([], [asm (".globl _dummy_function\n"
  "_dummy_function:\n"
  "     pushl %%ebp\n"
  "     movl %%esp, %%ebp\n"
  "     movq -8(%%ebp), %%mm0\n"
  "     movd %%edi, %%mm1\n"
  "     punpckldq %%mm1, %%mm1\n"
  "     psrld $ 19, %%mm7\n"
  "     pslld $ 10, %%mm6\n"
  "     por %%mm6, %%mm7\n"
  "     paddd %%mm1, %%mm0\n"
  "     emms\n"
  "     popl %%ebp\n"
  "     ret" : : )],
  allegro_cv_support_mmx=yes,
  allegro_cv_support_mmx=no)
else
  allegro_cv_support_mmx=no
fi
])
AC_MSG_RESULT($allegro_cv_support_mmx)])

dnl
dnl Test if SSE instructions are supported by assembler.
dnl
dnl Variables:
dnl  allegro_enable_sse=(yes|),
dnl  allegro_cv_support_sse=(yes|no).
dnl
AC_DEFUN(ALLEGRO_ACTEST_SUPPORT_SSE,
[AC_REQUIRE([ALLEGRO_ACTEST_SUPPORT_MMX])
AC_ARG_ENABLE(sse,
[  --enable-sse[=x]        enable the use of SSE instructions [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_sse=yes,
allegro_enable_sse=yes)
AC_MSG_CHECKING(for SSE support)
AC_CACHE_VAL(allegro_cv_support_sse,
[if test "$allegro_cv_support_mmx" != no -a "$allegro_enable_sse" = yes; then
  AC_TRY_COMPILE([], [asm (".globl _dummy_function\n"
  "_dummy_function:\n"
  "     pushl %%ebp\n"
  "     movl %%esp, %%ebp\n"
  "     movq -8(%%ebp), %%mm0\n"
  "     movd %%edi, %%mm1\n"
  "     punpckldq %%mm1, %%mm1\n"
  "     psrld $ 10, %%mm3\n"
  "     maskmovq %%mm3, %%mm1\n"
  "     paddd %%mm1, %%mm0\n"
  "     emms\n"
  "     popl %%ebp\n"
  "     ret" : : )],
  allegro_cv_support_sse=yes,
  allegro_cv_support_sse=no)
else
  allegro_cv_support_sse=no
fi
])
AC_MSG_RESULT($allegro_cv_support_sse)])

dnl
dnl Test for prefix prepended by compiler to global symbols.
dnl
dnl Variables:
dnl  allegro_cv_asm_prefix=(_|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_ASM_PREFIX,
[AC_MSG_CHECKING(for asm prefix before symbols)
AC_CACHE_VAL(allegro_cv_asm_prefix,
[AC_TRY_LINK([int test_for_underscore(void);
asm (".globl _test_for_underscore\n"
"_test_for_underscore:");], [test_for_underscore ();],
allegro_cv_asm_prefix=_,
allegro_cv_asm_prefix=no)
if test "$allegro_cv_asm_prefix" = no; then
  AC_TRY_LINK([int test_for_underscore(void);
  asm (".globl test_for_undercore\n"
  "test_for_underscore:");], [test_for_underscore ();],
    allegro_cv_asm_prefix=,
    allegro_cv_asm_prefix=no)
fi
dnl Add more tests for asm prefix here.
dnl if test "$allegro_cv_asm_prefix" = no; then
dnl   AC_TRY_LINK([], [],
dnl     allegro_cv_asm_prefix=,
dnl     allegro_cv_asm_prefix=no)
dnl fi
if test "$allegro_cv_asm_prefix" = no; then
  allegro_cv_asm_prefix=
fi
])
AC_MSG_RESULT(\"$allegro_cv_asm_prefix\")])

dnl
dnl Test for modules support (dlopen interface and --export-dynamic linker flag).
dnl
dnl Variables:
dnl  allegro_support_modules=(yes|)
dnl  allegro_cv_support_export_dynamic=(yes|no)
dnl
dnl LDFLAGS and LIBS can be modified.
dnl
AC_DEFUN(ALLEGRO_ACTEST_MODULES,
[AC_ARG_ENABLE(modules,
[  --enable-modules[=x]    enable dynamically loaded modules [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_modules=yes,
allegro_enable_modules=yes)

if test -n "$allegro_enable_modules"; then
  AC_CHECK_HEADER(dlfcn.h,
    [AC_CACHE_CHECK(whether --export-dynamic linker flag is supported,
      allegro_cv_support_export_dynamic,
      [allegro_save_LDFLAGS="$LDFLAGS"
      LDFLAGS="-Wl,--export-dynamic $LDFLAGS"
      AC_TRY_LINK(,,
        allegro_cv_support_export_dynamic=yes,
        allegro_cv_support_export_dynamic=no,
        allegro_cv_support_export_dynamic=yes)
      LDFLAGS="$allegro_save_LDFLAGS"])
    if test $allegro_cv_support_export_dynamic = yes; then
      allegro_support_modules=yes
      dnl Use libdl if found, else assume dl* functions in libc.
      AC_CHECK_LIB(dl, dlopen,
        [LIBS="-ldl $LIBS"])
      LDFLAGS="-Wl,--export-dynamic $LDFLAGS"
    fi])
fi
])

dnl
dnl Test for System V sys/procfs.h.
dnl
dnl Variables:
dnl  allegro_sv_procfs=(yes|no)
dnl
AC_MSG_CHECKING(for System V sys/procfs)
AC_DEFUN(ALLEGRO_ACTEST_SV_PROCFS, [
  AC_CHECK_HEADER(sys/procfs.h, [
    AC_TRY_COMPILE(
      [ #include <sys/procfs.h> 
        #include <sys/ioctl.h>
      ],
      [ struct prpsinfo psinfo; 
        ioctl(0, PIOCPSINFO, &psinfo);
      ],
      [allegro_sv_procfs=yes],
      [allegro_sv_procfs=no]
    ),
    [allegro_sv_procfs=no]
  ])
])
AC_MSG_RESULT($allegro_sv_procfs)

dnl
dnl Test if sys/procfs.h tells us argc/argv.
dnl
dnl Variables:
dnl  allegro_procfs_argcv=(yes|no)
dnl
AC_MSG_CHECKING(if sys/procfs.h tells us argc/argv)
AC_DEFUN(ALLEGRO_ACTEST_PROCFS_ARGCV, [
  AC_TRY_COMPILE(
    [#include  <sys/procfs.h>],
    [struct prpsinfo psinfo; psinfo.pr_argc = 0;],
    [allegro_procfs_argcv=yes],
    [allegro_procfs_argcv=no]
  )
])
AC_MSG_RESULT($allegro_procfs_argcv)

dnl
dnl Test for getexecname() on Solaris
dnl
dnl Variables:
dnl  allegro_sys_getexecname=(yes|no)
dnl
AC_MSG_CHECKING(for getexecname)
AC_DEFUN(ALLEGRO_ACTEST_SYS_GETEXECNAME,
   [AC_CHECK_LIB(c, getexecname,
      [allegro_sys_getexecname=yes],
      [allegro_sys_getexecname=no])]
)
AC_MSG_RESULT($allegro_sys_getexecname)

dnl
dnl Test for X-Windows support.
dnl
dnl Variables:
dnl  allegro_enable_xwin_xcursor=(yes|)
dnl  allegro_enable_xwin_shm=(yes|)
dnl  allegro_enable_xwin_xf86vidmode=(yes|)
dnl  allegro_enable_xwin_xf86dga=(yes|)
dnl  allegro_enable_xwin_xf86dga2=(yes|)
dnl  allegro_support_xwindows=(yes|)
dnl
dnl CPPFLAGS, LDFLAGS and LIBS can be modified.
dnl
AC_DEFUN(ALLEGRO_ACTEST_SUPPORT_XWINDOWS,
[AC_ARG_ENABLE(xwin-xcursor,
[  --enable-xwin-xcursor[=x] enable the use of Xcursor library [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_xcursor=yes,
allegro_enable_xwin_xcursor=yes)
AC_ARG_ENABLE(xwin-shm,
[  --enable-xwin-shm[=x]   enable the use of MIT-SHM Extension [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_shm=yes,
allegro_enable_xwin_shm=yes)
AC_ARG_ENABLE(xwin-vidmode,
[  --enable-xwin-vidmode[=x] enable the use of XF86VidMode Ext. [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_xf86vidmode=yes,
allegro_enable_xwin_xf86vidmode=yes)
AC_ARG_ENABLE(xwin-dga2,
[  --enable-xwin-dga2[=x]  enable the use of DGA 2.0 Extension [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_xf86dga2=yes,
allegro_enable_xwin_xf86dga2=yes)
AC_ARG_ENABLE(xim,
[  --enable-xim[=x]        enable the use of XIM keyboard input [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xim=yes,
allegro_enable_xim=yes)

dnl Process "--with[out]-x", "--x-includes" and "--x-libraries" options.
_x11="X11 support: disabled"
AC_PATH_X
if test -z "$no_x"; then
  allegro_support_xwindows=yes
  AC_DEFINE(ALLEGRO_WITH_XWINDOWS,1,[Define if you need support for X-Windows.])

  if test -n "$x_includes"; then
    CPPFLAGS="-I$x_includes $CPPFLAGS"
  fi
  if test -n "$x_libraries"; then
    LDFLAGS="-L$x_libraries $LDFLAGS"
    ALLEGRO_XWINDOWS_LIBDIR="$x_libraries"
  fi
  LIBS="-lX11 $LIBS"
  _x11="X11 support: enabled"
  _x11ext=""

  dnl Test for Xext library.
  AC_CHECK_LIB(Xext, XMissingExtension,
    [_x11ext="$_x11ext Xext"
    LIBS="-lXext $LIBS"])

  dnl Test for Xpm library.
  AC_CHECK_HEADER(X11/xpm.h, AC_CHECK_LIB(Xpm, XpmCreatePixmapFromData,
    [LIBS="-lXpm $LIBS"
    _x11ext="$_x11ext Xpm"
    AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XPM,1,[Define if xpm bitmap support is available.])
    ])
   )

  dnl Test for Xcursor library.
  if test -n "$allegro_enable_xwin_xcursor"; then
    AC_CHECK_LIB(Xcursor, XcursorImageCreate,
      AC_TRY_COMPILE([#include <X11/Xlib.h>
                      #include <X11/Xcursor/Xcursor.h>], 
                     [XcursorImage *xcursor_image;
                      XcursorImageLoadCursor(0, xcursor_image);
                      XcursorSupportsARGB(0);
                     ],
        [LIBS="-lXcursor $LIBS"
        _x11ext="$_x11ext Xcursor"
        AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XCURSOR,1,[Define if XCursor ARGB extension is available.])
        ])
     )
  fi

  dnl Test for SHM extension.
  if test -n "$allegro_enable_xwin_shm"; then
    AC_CHECK_LIB(Xext, XShmQueryExtension,
      [_x11ext="$_x11ext XShm"
      AC_DEFINE(ALLEGRO_XWINDOWS_WITH_SHM,1,[Define if MIT-SHM extension is supported.])])
  fi

  dnl Test for XF86VidMode extension.
  if test -n "$allegro_enable_xwin_xf86vidmode"; then
    AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryExtension,
      [LIBS="-lXxf86vm $LIBS"
      _x11ext="$_x11ext XF86VidMode"
      AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE,1,[Define if XF86VidMode extension is supported.])])
  fi

  dnl Test for DGA 2.0 extension.
  if test -n "$allegro_enable_xwin_xf86dga2"; then
    AC_CHECK_LIB(Xxf86dga, XDGAQueryExtension,
      [allegro_support_xf86dga2=yes
      if test -z "$allegro_support_modules"; then
        LIBS="-lXxf86dga $LIBS"
      fi
      _x11ext="$_x11ext XDGA"
      AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XF86DGA2,1,[Define if DGA version 2.0 or newer is supported])])
  fi

  dnl Test for XIM support.
  if test -n "$allegro_enable_xim"; then
    AC_CHECK_LIB(X11, XOpenIM,
      [_x11ext="$_x11ext XIM"
      AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XIM,1,[Define if XIM extension is supported.])])
  fi

  if test -n "$_x11ext"; then
    _x11="$_x11 with:$_x11ext"
  fi

fi
])

dnl
dnl Test for debugging with Fortify.
dnl
dnl Variables:
dnl  allegro_debug_with_fortify=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_DEBUG_WITH_FORTIFY,
[AC_ARG_ENABLE(dbg-with-fortify,
[  --enable-dbg-with-fortify[=x]    enable debugging with Fortify [default=no]],
test "X$enableval" != "Xno" && allegro_enable_debug_with_fortify=yes)

if test -n "$allegro_enable_debug_with_fortify"; then
   AC_CHECK_HEADER(fortify/fortify.h, allegro_debug_with_fortify=yes)
fi
])

dnl
dnl Test for debugging with DMalloc.
dnl
dnl Variables:
dnl  allegro_debug_with_dmalloc=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_DEBUG_WITH_DMALLOC,
[AC_ARG_ENABLE(dbg-with-dmalloc,
[  --enable-dbg-with-dmalloc[=x]    enable debugging with DMalloc [default=no]],
test "X$enableval" != "Xno" && allegro_enable_debug_with_dmalloc=yes)

if test -n "$allegro_enable_debug_with_dmalloc"; then
   AC_CHECK_HEADER(dmalloc.h, allegro_debug_with_dmalloc=yes)
fi
])

dnl
dnl Test for Linux Framebuffer Console support.
dnl
dnl Variables:
dnl  allegro_cv_support_fbcon=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_FBCON, [
  AC_CHECK_HEADER(linux/fb.h, [
    AC_TRY_COMPILE(
      [#include <linux/fb.h>],
      [int x = FB_SYNC_ON_GREEN;],
      [allegro_cv_support_fbcon=yes]
    )
  ])
])

dnl
dnl Test for Linux SVGAlib support.
dnl
dnl  Variables:
dnl   allegro_cv_support_svgalib=(yes|)
dnl   allegro_cv_have_vga_version=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_SVGALIB,
[AC_CHECK_HEADER(vga.h,
AC_CHECK_LIB(vga, vga_init,
allegro_cv_support_svgalib=yes
AC_MSG_CHECKING(for vga_version in vga.h)
AC_CACHE_VAL(allegro_cv_have_vga_version,
[AC_TRY_COMPILE([#include <vga.h>], [int x = vga_version; x++;],
allegro_cv_have_vga_version=yes,
allegro_cv_have_vga_version=no)])
AC_MSG_RESULT($allegro_cv_have_vga_version)))])

dnl
dnl Test for OSS DIGI driver.
dnl
dnl Variables:
dnl  allegro_support_ossdigi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_OSSDIGI,
[AC_ARG_ENABLE(ossdigi,
[  --enable-ossdigi[=x]    enable building OSS DIGI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_ossdigi=yes,
allegro_enable_ossdigi=yes)

if test -n "$allegro_enable_ossdigi"; then
  AC_CHECK_HEADER(soundcard.h, [
    AC_DEFINE(ALLEGRO_HAVE_SOUNDCARD_H, 1)
    allegro_support_ossdigi=yes
  ])
  AC_CHECK_HEADER(sys/soundcard.h, [
    AC_DEFINE(ALLEGRO_HAVE_SYS_SOUNDCARD_H, 1)
    allegro_support_ossdigi=yes
  ])
  AC_CHECK_HEADER(machine/soundcard.h, [
    AC_DEFINE(ALLEGRO_HAVE_MACHINE_SOUNDCARD_H, 1)
    allegro_support_ossdigi=yes
  ])
  AC_CHECK_HEADER(linux/soundcard.h, [
    AC_DEFINE(ALLEGRO_HAVE_LINUX_SOUNDCARD_H, 1)
    allegro_support_ossdigi=yes
  ])

  dnl Link with libossaudio if necessary, used by some BSD systems.
  AC_CHECK_LIB(ossaudio, _oss_ioctl)
fi
])

dnl
dnl Test for OSS MIDI driver.
dnl
dnl Variables:
dnl  allegro_support_ossmidi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_OSSMIDI,
[AC_ARG_ENABLE(ossmidi,
[  --enable-ossmidi[=x]    enable building OSS MIDI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_ossmidi=yes,
allegro_enable_ossmidi=yes)

if test -n "$allegro_enable_ossmidi"; then
  AC_CHECK_HEADER(soundcard.h,
    AC_DEFINE(ALLEGRO_HAVE_SOUNDCARD_H, 1))
  AC_CHECK_HEADER(sys/soundcard.h,
    AC_DEFINE(ALLEGRO_HAVE_SYS_SOUNDCARD_H, 1))
  AC_CHECK_HEADER(machine/soundcard.h,
    AC_DEFINE(ALLEGRO_HAVE_MACHINE_SOUNDCARD_H, 1))
  AC_CHECK_HEADER(linux/soundcard.h,
    AC_DEFINE(ALLEGRO_HAVE_LINUX_SOUNDCARD_H, 1))
  AC_CHECK_HEADER(linux/awe_voice.h,
    AC_DEFINE(ALLEGRO_HAVE_LINUX_AWE_VOICE_H, 1))

  dnl Link with libossaudio if necessary, used by some BSD systems.
  AC_CHECK_LIB(ossaudio, _oss_ioctl)

  dnl Sequencer support might not be present if this is an incomplete
  dnl emulation of the OSS API.
  AC_MSG_CHECKING(for OSS sequencer support)
  AC_LINK_IFELSE(
    AC_LANG_PROGRAM([[
      #if ALLEGRO_HAVE_SOUNDCARD_H
       #include <soundcard.h>
      #elif ALLEGRO_HAVE_SYS_SOUNDCARD_H
       #include <sys/soundcard.h>
      #elif ALLEGRO_HAVE_MACHINE_SOUNDCARD_H
       #include <machine/soundcard.h>
      #elif ALLEGRO_HAVE_LINUX_SOUNDCARD_H
       #include <linux/soundcard.h>
      #endif]],
      [return SNDCTL_SEQ_NRSYNTHS;]
    ),
    [allegro_support_ossmidi=yes]
  )
  if test -n "$allegro_support_ossmidi"; then
    AC_MSG_RESULT(yes)
  else
    AC_MSG_RESULT(no)
  fi
fi
])

dnl
dnl Test for ALSA DIGI driver.
dnl
dnl Variables:
dnl  allegro_cv_support_alsadigi=(yes|)
dnl  allegro_cv_alsa_version=(5|9)
dnl
AC_DEFUN(ALLEGRO_ACTEST_ALSADIGI,
[AC_BEFORE([$0], [ALLEGRO_ACTEST_ALSAMIDI])
AC_ARG_ENABLE(alsadigi,
[  --enable-alsadigi[=x]   enable building ALSA DIGI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_alsadigi=yes,
allegro_enable_alsadigi=yes)
 
if test -n "$allegro_enable_alsadigi"; then
  AC_CACHE_CHECK(for supported ALSA version for digital sound,
  allegro_cv_support_alsadigi,
  AC_TRY_COMPILE([#include <sys/asoundlib.h>
                  #if (SND_LIB_MAJOR > 0) || (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9 && SND_LIB_SUBMINOR >= 1)
                  #else
                  #error
                  #endif],,
  [allegro_cv_support_alsadigi=yes
   allegro_cv_alsa_version=9],
  AC_TRY_COMPILE([#include <sys/asoundlib.h>
                  #if SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 5
                  #else
                  #error
                  #endif],,
  [allegro_cv_support_alsadigi=yes
   allegro_cv_alsa_version=5],
  allegro_cv_support_alsadigi=no)))
  if test "X$allegro_cv_support_alsadigi" = "Xyes" && 
     test -z "$allegro_support_modules"; then
    LIBS="-lasound $LIBS"
  fi
fi])

dnl
dnl Test for ALSA MIDI driver.
dnl
dnl Variables:
dnl  allegro_support_alsamidi=(yes|)
dnl  allegro_cv_alsa_version=(5|9)
dnl
AC_DEFUN(ALLEGRO_ACTEST_ALSAMIDI,
[AC_ARG_ENABLE(alsamidi,
[  --enable-alsamidi[=x]   enable building ALSA MIDI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_alsamidi=yes,
allegro_enable_alsamidi=yes)

if test -n "$allegro_enable_alsamidi"; then
  AC_CACHE_CHECK(for supported ALSA version for MIDI,
  allegro_cv_support_alsamidi,
  AC_TRY_COMPILE([#include <sys/asoundlib.h>
                  #if (SND_LIB_MAJOR > 0) || (SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 9)
                  #else
                  #error
                  #endif],,
  [allegro_cv_support_alsamidi=yes
   allegro_cv_alsa_version=9],
  AC_TRY_COMPILE([#include <sys/asoundlib.h>
                  #if SND_LIB_MAJOR == 0 && SND_LIB_MINOR == 5
                  #else
                  #error
                  #endif],,
  [allegro_cv_support_alsamidi=yes
   allegro_cv_alsa_version=5],
  allegro_cv_support_alsamidi=no)))
  if test "X$allegro_cv_support_alsamidi" = "Xyes" &&
     test "X$allegro_cv_support_alsadigi" != "Xyes" &&
     test -z "$allegro_support_modules"; then
    LIBS="-lasound $LIBS"
  fi
fi])

dnl
dnl Test for ESD DIGI driver.
dnl
dnl Variables:
dnl  allegro_support_esddigi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_ESDDIGI,
[AC_ARG_ENABLE(esddigi,
[  --enable-esddigi[=x]    enable building ESD DIGI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_esddigi=yes,
allegro_enable_esddigi=yes)

if test -n "$allegro_enable_esddigi"; then
  AC_PATH_PROG(ESD_CONFIG, esd-config)
  if test -n "$ESD_CONFIG"; then
    ALLEGRO_OLD_LIBS="$LIBS"
    ALLEGRO_OLD_CFLAGS="$CFLAGS"
    LIBS="`$ESD_CONFIG --libs` $LIBS"
    CFLAGS="`$ESD_CONFIG --cflags` $CFLAGS"
    AC_MSG_CHECKING(for esd_open_sound)
    AC_TRY_LINK([#include <esd.h>],
      [esd_open_sound(0);],
      [allegro_support_esddigi=yes
       if test -n "$allegro_support_modules"; then
         LIBS="$ALLEGRO_OLD_LIBS"
       fi],
      [CFLAGS="$ALLEGRO_OLD_CFLAGS"
       LIBS="$ALLEGRO_OLD_LIBS"])
    if test -n "$allegro_support_esddigi"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  fi
fi])

dnl
dnl Test for ARTS DIGI driver.
dnl
dnl Variables:
dnl  allegro_support_artsdigi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_ARTSDIGI,
[AC_ARG_ENABLE(artsdigi,
[  --enable-artsdigi[=x]   enable building ARTS DIGI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_artsdigi=yes,
allegro_enable_artsdigi=yes)

if test -n "$allegro_enable_artsdigi"; then
  AC_PATH_PROG(ARTSC_CONFIG, artsc-config)
  if test -n "$ARTSC_CONFIG"; then
    ALLEGRO_OLD_LIBS="$LIBS"
    ALLEGRO_OLD_CFLAGS="$CFLAGS"
    LIBS="`$ARTSC_CONFIG --libs` $LIBS"
    CFLAGS="`$ARTSC_CONFIG --cflags` $CFLAGS"
    AC_MSG_CHECKING(for arts_init)
    AC_TRY_LINK([#include <artsc.h>],
      [arts_init();],
      [allegro_support_artsdigi=yes
       if test -n "$allegro_support_modules"; then
         LIBS="$ALLEGRO_OLD_LIBS"
       fi],
      [CFLAGS="$ALLEGRO_OLD_CFLAGS"
       LIBS="$ALLEGRO_OLD_LIBS"])
    if test -n "$allegro_support_artsdigi"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  fi
fi])

dnl
dnl Test for SGI AL DIGI driver.
dnl
dnl Variables:
dnl  allegro_enable_sgialdigi=(yes|)
dnl  allegro_cv_support_sgialdigi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_SGIALDIGI,
[AC_ARG_ENABLE(sgialdigi,
[  --enable-sgialdigi[=x]  enable building SGI AL DIGI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_sgialdigi=yes,
allegro_enable_sgialdigi=yes)
 
if test -n "$allegro_enable_sgialdigi"; then
  AC_CHECK_LIB(audio, alOpenPort,
  allegro_cv_support_sgialdigi=yes)
  if test -n "$allegro_cv_support_sgialdigi" && 
     test -z "$allegro_support_modules"; then
    LIBS="-laudio $LIBS"
  fi
fi])

dnl
dnl Test for JACK driver.
dnl
dnl Variables:
dnl  allegro_enable_sgialdigi=(yes|)
dnl  allegro_cv_support_jackdigi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_JACK,
[AC_ARG_ENABLE(jackdigi,
[  --enable-jackdigi[=x]    enable building JACK DIGI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_jackdigi=yes,
allegro_enable_jackdigi=yes)

if test -n "$allegro_enable_jackdigi"; then
  AC_PATH_PROG(PKG_CONFIG, pkg-config)
  if test -n "$PKG_CONFIG"; then
    ALLEGRO_OLD_LIBS="$LIBS"
    ALLEGRO_OLD_CFLAGS="$CFLAGS"
    LIBS="`$PKG_CONFIG --libs jack` $LIBS"
    CFLAGS="`$PKG_CONFIG --cflags jack` $CFLAGS"
    AC_MSG_CHECKING(for jack_client_new)
    AC_TRY_LINK([#include <jack/jack.h>],
      [jack_client_new(0);],
      [allegro_cv_support_jackdigi=yes
       if test -n "$allegro_support_modules"; then
         LIBS="$ALLEGRO_OLD_LIBS"
       fi],
      [CFLAGS="$ALLEGRO_OLD_CFLAGS"
       LIBS="$ALLEGRO_OLD_LIBS"])
    if test -n "$allegro_cv_support_jackdigi"; then
      AC_MSG_RESULT(yes)
    else
      AC_MSG_RESULT(no)
    fi
  fi
fi])

dnl
dnl Test that MAP_FAILED is defined in system headers.
dnl
dnl Variables:
dnl  allegro_cv_have_map_failed = (yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_MAP_FAILED,
[AC_MSG_CHECKING(for MAP_FAILED)
AC_CACHE_VAL(allegro_cv_have_map_failed,
[AC_TRY_COMPILE([#include <unistd.h>
#include <sys/mman.h>],
[int test_mmap_failed (void *addr) { return (addr == MAP_FAILED); }],
allegro_cv_have_map_failed=yes,
allegro_cv_have_map_failed=no)])
AC_MSG_RESULT($allegro_cv_have_map_failed)])

dnl
dnl Test that the POSIX threads library is present.
dnl
dnl Variables:
dnl  allegro_cv_support_pthreads = (yes|)
dnl
dnl LIBS can be modified.
dnl
AC_DEFUN(ALLEGRO_ACTEST_PTHREADS,
[AC_CHECK_HEADER(pthread.h,
AC_CHECK_LIB(pthread, pthread_create,
LIBS="-lpthread $LIBS"
allegro_cv_support_pthreads=yes))])

dnl
dnl Test for sched_yield (SunOS).
dnl
dnl Variables:
dnl  allegro_cv_support_sched_yield=(yes|)
dnl
dnl LIBS can be modified.
dnl
AC_DEFUN(ALLEGRO_ACTEST_SCHED_YIELD,
[AC_CHECK_LIB(c, sched_yield,
allegro_cv_support_sched_yield=yes,
AC_SEARCH_LIBS(sched_yield, posix4 rt,
allegro_cv_support_sched_yield=yes))])

dnl
dnl Test for constructor attribute support.
dnl
dnl Variables:
dnl  allegro_support_constructor=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_CONSTRUCTOR,
[AC_MSG_CHECKING(for constructor attribute)
AC_ARG_ENABLE(constructor,
[  --enable-constructor[=x]enable the use of constructor attr [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_constructor=yes,
allegro_enable_constructor=yes)

if test -n "$allegro_enable_constructor"; then
  AC_TRY_RUN([static int notsupported = 1;
    void test_ctor (void) __attribute__ ((constructor));
    void test_ctor (void) { notsupported = 0; }
    int main (void) { return (notsupported); }],
  allegro_support_constructor=yes,
  allegro_support_constructor=no,
  allegro_support_constructor=yes)
else
  allegro_support_constructor=no
fi

AC_MSG_RESULT($allegro_support_constructor)])

dnl
dnl Test for buggy IRIX linker.
dnl
dnl Variables:
dnl  allegro_cv_prog_ld_s=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_PROG_LD_S,
[AC_MSG_CHECKING(whether linker works with -s option)
allegro_save_LDFLAGS=$LDFLAGS
LDFLAGS="-s $LDFLAGS"
AC_CACHE_VAL(allegro_cv_prog_ld_s, [
AC_TRY_LINK(,{},allegro_cv_prog_ld_s=yes, allegro_cv_prog_ld_s=no)])
LDFLAGS=$allegro_save_LDFLAGS
AC_MSG_RESULT($allegro_cv_prog_ld_s)
])

dnl
dnl Test for buggy gcc version.
dnl
dnl Variables:
dnl  allegro_cv_support_fomit_frame_pointer=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_GCC_VERSION,
[AC_MSG_CHECKING(whether -fomit-frame-pointer is safe)
AC_CACHE_VAL(allegro_cv_support_fomit_frame_pointer,
[if test "$GCC" = yes && $CC --version | grep '3\.0\(\.\?[[012]]\)\?$' >/dev/null; then
  allegro_cv_support_fomit_frame_pointer=no
else
  if test "$GCC" = yes; then
    allegro_cv_support_fomit_frame_pointer=yes
  else
    allegro_cv_support_fomit_frame_pointer=no
  fi
fi
])
AC_MSG_RESULT($allegro_cv_support_fomit_frame_pointer)])

dnl
dnl Test for include path conflict with gcc 3.1 or later.
dnl
dnl Variables:
dnl  allegro_cv_support_include_prefix=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_GCC_INCLUDE_PREFIX,
[AC_MSG_CHECKING(whether an include prefix is needed)
allegro_save_CFLAGS="$CFLAGS"
CFLAGS="-Werror -I$prefix/include $CFLAGS"
AC_CACHE_VAL(allegro_cv_support_include_prefix,
[if test "$GCC" = yes; then
   AC_TRY_COMPILE(,int foo(){return 0;}, allegro_cv_support_include_prefix=yes, allegro_cv_support_include_prefix=no)
else
   allegro_cv_support_include_prefix=yes
fi
])
CFLAGS="$allegro_save_CFLAGS"
AC_MSG_RESULT($allegro_cv_support_include_prefix)])

dnl
dnl Test for the presence of a C++ compiler.
dnl
dnl Variables:
dnl  allegro_cv_support_cplusplus=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_GCC_CXX,
[AC_MSG_CHECKING(whether a C++ compiler is installed)
AC_CACHE_VAL(allegro_cv_support_cplusplus,
[if test "$GCC" = yes; then
   allegro_save_CFLAGS=$CFLAGS
   CFLAGS="-x c++"
   AC_TRY_COMPILE(,class foo {foo() {}};, allegro_cv_support_cplusplus=yes, allegro_cv_support_cplusplus=no)
   CFLAGS=$allegro_save_CFLAGS
else
   allegro_cv_support_cplusplus=no
fi
])
AC_MSG_RESULT($allegro_cv_support_cplusplus)])

dnl
dnl Test for working '-mtune' i386 compile option.
dnl
dnl Variables:
dnl  allegro_cv_support_i386_mtune=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_GCC_I386_MTUNE,
[AC_MSG_CHECKING(whether -mtune is supported)
allegro_save_CFLAGS="$CFLAGS"
CFLAGS="-mtune=i386"
AC_CACHE_VAL(allegro_cv_support_i386_mtune,
[if test "$GCC" = yes; then
   AC_TRY_COMPILE(,int foo(){return 0;}, allegro_cv_support_i386_mtune=yes, allegro_cv_support_i386_mtune=no)
else
   allegro_cv_support_i386_mtune=no
fi
])
CFLAGS="$allegro_save_CFLAGS"
AC_MSG_RESULT($allegro_cv_support_i386_mtune)])

dnl
dnl Test for working '-mtune' amd64 compile option.
dnl
dnl Variables:
dnl  allegro_cv_support_amd64_mtune=(yes|no)
dnl
AC_DEFUN(ALLEGRO_ACTEST_GCC_AMD64_MTUNE,
[AC_MSG_CHECKING(whether -mtune is supported)
allegro_save_CFLAGS="$CFLAGS"
CFLAGS="-mtune=k8"
AC_CACHE_VAL(allegro_cv_support_amd64_mtune,
[if test "$GCC" = yes; then
   AC_TRY_COMPILE(,int foo(){return 0;}, allegro_cv_support_amd64_mtune=yes, allegro_cv_support_amd64_mtune=no)
else
   allegro_cv_support_amd64_mtune=no
fi
])
CFLAGS="$allegro_save_CFLAGS"
AC_MSG_RESULT($allegro_cv_support_amd64_mtune)])

dnl vim: set sts=2 sw=2 et:
