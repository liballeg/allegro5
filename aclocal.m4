# Place m4 macros here.

dnl
dnl Test if asm files can be used (which ones).
dnl
dnl Variables:
dnl  allegro_enable_asm=(yes|),
dnl  allegro_cv_support_asm=(i386|no).
dnl
AC_DEFUN(ALLEGRO_ACTEST_SUPPORT_ASM,
[AC_BEFORE([$0], [ALLEGRO_ACTEST_SUPPORT_MMX])
AC_ARG_ENABLE(asm,
[  --enable-asm[=x]        enable the use of assembler files [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_asm=yes,
allegro_enable_asm=yes)
AC_MSG_CHECKING(for asm support)
AC_CACHE_VAL(allegro_cv_support_asm,
[if test "$allegro_enable_asm" = yes; then
  AC_TRY_COMPILE([], [asm (".globl _dummy_function\n"
  "_dummy_function:\n"
  "     pushl %%ebp\n"
  "     movl %%esp, %%ebp\n"
  "     movl %%eax, %%ebx\n"
  "     movl %%ecx, %%edx\n"
  "     movl %%esi, %%edi\n"
  "     leal 10(%%ebx, %%ecx, 4), %%edx\n"
  "     call *%%edx\n"
  "     addl %0, %%eax\n"
  "     popl %%ebp\n"
  "     ret" : : "i" (0))],
  allegro_cv_support_asm=i386,
  allegro_cv_support_asm=no)
  dnl Add more tests for assembler types here.
  dnl if test "$allegro_cv_support_asm" = no; then
  dnl   ...
  dnl fi
else
  allegro_cv_support_asm=no
fi
])
AC_MSG_RESULT($allegro_cv_support_asm)])

dnl
dnl Test if MMX instructions are supported by assembler.
dnl
dnl Variables:
dnl  allegro_enable_mmx=(yes|),
dnl  allegro_cv_support_mmx=(yes|no).
dnl
AC_DEFUN(ALLEGRO_ACTEST_SUPPORT_MMX,
[AC_REQUIRE([ALLEGRO_ACTEST_SUPPORT_ASM])
AC_ARG_ENABLE(mmx,
[  --enable-mmx[=x]        enable the use of MMX instructions [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_mmx=yes,
allegro_enable_mmx=yes)
AC_MSG_CHECKING(for MMX support)
AC_CACHE_VAL(allegro_cv_support_mmx,
[if test "$allegro_cv_support_asm" != no -a "$allegro_enable_mmx" = yes; then
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
dnl Turn off Pentium optimizations by default.
dnl
allegro_pentium_optimizations=no

dnl
dnl Test for modules support (dlopen interface and -export-dynamic linker flag).
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
  AC_CHECK_HEADERS(dlfcn.h,
    [AC_CACHE_CHECK(whether -export-dynamic linker flag is supported,
      allegro_cv_support_export_dynamic,
      [allegro_save_LDFLAGS="$LDFLAGS"
      LDFLAGS="-Wl,-export-dynamic $LDFLAGS"
      AC_TRY_LINK(,,
        allegro_cv_support_export_dynamic=yes,
        allegro_cv_support_export_dynamic=no,
        allegro_cv_support_export_dynamic=yes)
      LDFLAGS="$allegro_save_LDFLAGS"])
    if test $allegro_cv_support_export_dynamic = yes; then
      allegro_support_modules=yes
      LIBS="-ldl $LIBS"
      LDFLAGS="-Wl,-export-dynamic $LDFLAGS"
    fi])
fi
])

dnl
dnl Test for X-Windows support.
dnl
dnl Variables:
dnl  allegro_enable_xwin_shm=(yes|)
dnl  allegro_enable_xwin_xf86vidmode=(yes|)
dnl  allegro_enable_xwin_xf86dga=(yes|)
dnl  allegro_enable_xwin_xf86dga2=(yes|)
dnl  allegro_support_xwindows=(yes|)
dnl
dnl CPPFLAGS, LDFLAGS and LIBS can be modified.
dnl
AC_DEFUN(ALLEGRO_ACTEST_SUPPORT_XWINDOWS,
[AC_ARG_ENABLE(xwin-shm,
[  --enable-xwin-shm[=x]   enable the use of MIT-SHM Extension [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_shm=yes,
allegro_enable_xwin_shm=yes)
AC_ARG_ENABLE(xwin-vidmode,
[  --enable-xwin-vidmode[=x] enable the use of XF86VidMode Extension [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_xf86vidmode=yes,
allegro_enable_xwin_xf86vidmode=yes)
AC_ARG_ENABLE(xwin-dga,
[  --enable-xwin-dga[=x]   enable the use of XF86DGA Extension [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_xf86dga=yes,
allegro_enable_xwin_xf86dga=yes)
AC_ARG_ENABLE(xwin-dga2,
[  --enable-xwin-dga2[=x]  enable the use of DGA 2.0 Extension [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_xwin_xf86dga2=yes,
allegro_enable_xwin_xf86dga2=yes)

dnl Process "--with[out]-x", "--x-includes" and "--x-libraries" options.
AC_PATH_X
if test -z "$no_x"; then
  allegro_support_xwindows=yes
  AC_DEFINE(ALLEGRO_WITH_XWINDOWS)

  if test -n "$x_includes"; then
    CPPFLAGS="-I$x_includes $CPPFLAGS"
  fi
  if test -n "$x_libraries"; then
    LDFLAGS="-L$x_libraries $LDFLAGS"
    ALLEGRO_XWINDOWS_LIBDIR="$x_libraries"
  fi
  LIBS="-lX11 $LIBS"

  dnl Test for Xext library.
  AC_CHECK_LIB(Xext, XMissingExtension,
    [LIBS="-lXext $LIBS"])

  dnl Test for SHM extension.
  if test -n "$allegro_enable_xwin_shm"; then
    AC_CHECK_LIB(Xext, XShmQueryExtension,
      [AC_DEFINE(ALLEGRO_XWINDOWS_WITH_SHM)])
  fi

  dnl Test for XF86VidMode extension.
  if test -n "$allegro_enable_xwin_xf86vidmode"; then
    AC_CHECK_LIB(Xxf86vm, XF86VidModeQueryExtension,
      [LIBS="-lXxf86vm $LIBS"
      AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XF86VIDMODE)])
  fi

  dnl Test for XF86DGA extension.
  if test -n "$allegro_enable_xwin_xf86dga"; then
    AC_CHECK_LIB(Xxf86dga, XF86DGAQueryExtension,
      [LIBS="-lXxf86dga $LIBS"
      AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XF86DGA)])
  fi

  dnl Test for DGA 2.0 extension.
  if test -n "$allegro_enable_xwin_xf86dga2"; then
    AC_CHECK_LIB(Xxf86dga, XDGAQueryExtension,
      [allegro_support_xf86dga2=yes
      if test -z "$allegro_support_modules"; then
        LIBS="-lXxf86dga $LIBS"
      fi
      AC_DEFINE(ALLEGRO_XWINDOWS_WITH_XF86DGA2)])
  fi

fi
])

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
  AC_CHECK_HEADERS(soundcard.h, allegro_support_ossdigi=yes)
  AC_CHECK_HEADERS(sys/soundcard.h, allegro_support_ossdigi=yes)
  AC_CHECK_HEADERS(machine/soundcard.h, allegro_support_ossdigi=yes)
  AC_CHECK_HEADERS(linux/soundcard.h, allegro_support_ossdigi=yes)
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
  AC_CHECK_HEADERS(soundcard.h, allegro_support_ossmidi=yes)
  AC_CHECK_HEADERS(sys/soundcard.h, allegro_support_ossmidi=yes)
  AC_CHECK_HEADERS(machine/soundcard.h, allegro_support_ossmidi=yes)
  AC_CHECK_HEADERS(linux/soundcard.h, allegro_support_ossmidi=yes)
fi
])

dnl
dnl Test for ALSA DIGI driver.
dnl
dnl Variables:
dnl  allegro_enable_alsadigi=(yes|)
dnl  allegro_cv_support_alsadigi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_ALSADIGI,
[AC_ARG_ENABLE(alsadigi,
[  --enable-alsadigi[=x]   enable building ALSA DIGI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_alsadigi=yes,
allegro_enable_alsadigi=yes)
 
if test -n "$allegro_enable_alsadigi"; then
  AC_CACHE_CHECK(for supported ALSA version for digital sound,
  allegro_cv_support_alsadigi,
  AC_TRY_RUN([#include <sys/asoundlib.h>
    int main (void) { return SND_LIB_MAJOR != 0 || SND_LIB_MINOR != 5; }],
  allegro_cv_support_alsadigi=yes,
  allegro_cv_support_alsadigi=no,
  allegro_cv_support_alsadigi=no))
  if test "X$allegro_cv_support_alsadigi" = "Xyes" && 
     test -z "$allegro_support_modules"; then
    LIBS="-lasound $LIBS"
  fi
fi])

dnl
dnl Test for ALSA MIDI driver.
dnl
dnl Variables:
dnl  allegro_enable_alsamidi=(yes|)
dnl  allegro_support_alsamidi=(yes|)
dnl
AC_DEFUN(ALLEGRO_ACTEST_ALSAMIDI,
[AC_ARG_ENABLE(alsamidi,
[  --enable-alsamidi[=x]   enable building ALSA MIDI driver [default=yes]],
test "X$enableval" != "Xno" && allegro_enable_alsamidi=yes,
allegro_enable_alsamidi=yes)

if test -n "$allegro_enable_alsamidi"; then
  AC_CACHE_CHECK(for supported ALSA version for MIDI,
  allegro_cv_support_alsamidi,
  AC_TRY_RUN([#include <sys/asoundlib.h>
    int main (void) { return SND_LIB_MAJOR != 0 || SND_LIB_MINOR != 5; }],
  allegro_cv_support_alsamidi=yes,
  allegro_cv_support_alsamidi=no,
  allegro_cv_support_alsamidi=no))
  if test "X$allegro_cv_support_alsamidi" = "Xyes" &&
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
  AC_CHECK_HEADER(esd.h, allegro_support_esddigi=yes)
  if test -n "$allegro_support_esddigi" && 
     test -z "$allegro_support_modules"; then
    LIBS="-lesd $LIBS"
  fi
fi])

dnl
dnl Test where is sched_yield (SunOS).
dnl
dnl Variables:
dnl  allegro_sched_yield_lib
dnl
dnl LIBS can be modified.
dnl
AC_DEFUN(ALLEGRO_ACTEST_SCHED_YIELD,
[AC_MSG_CHECKING(for sched_yield)
allegro_sched_yield_lib="no"
AC_TRY_LINK([ extern void sched_yield (); ],
  [ sched_yield (); ],
  AC_DEFINE(ALLEGRO_USE_SCHED_YIELD)
  allegro_sched_yield_lib="-lc",
  AC_CHECK_LIB(posix4, sched_yield,
    [ LIBS="-lposix4 $LIBS"
    AC_DEFINE(ALLEGRO_USE_SCHED_YIELD)
    allegro_sched_yield_lib="-lposix4"],
    AC_CHECK_LIB(rt, sched_yield,
      [ LIBS="-lrt $LIBS"
      AC_DEFINE(ALLEGRO_USE_SCHED_YIELD)
      allegro_sched_yield_lib="-lrt"])))
AC_MSG_RESULT($allegro_sched_yield_lib)])

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
  allegro_support_constructor=no)
else
  allegro_support_constructor=no
fi

AC_MSG_RESULT($allegro_support_constructor)])

dnl
dnl Test for buggy IRIX linker.
dnl
dnl Variables:
dnl  allegro_cv_prog_ld_s
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
dnl  allegro_cv_support_fomit_frame_pointer
dnl
AC_DEFUN(ALLEGRO_ACTEST_GCC_VERSION,
[AC_MSG_CHECKING(whether -fomit-frame-pointer is safe)
AC_CACHE_VAL(allegro_cv_support_fomit_frame_pointer,
[if test $GCC = yes && $CC --version | grep -q '3\.0[[012]]$'; then
  allegro_cv_support_fomit_frame_pointer=no
else
  allegro_cv_support_fomit_frame_pointer=yes
fi
])
AC_MSG_RESULT($allegro_cv_support_fomit_frame_pointer)])
