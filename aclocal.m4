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
"     save %sp, -120, %sp\n"
"     ld [%fp-20], %f12\n"
"     fitod %f12, %f10\n"
"     faddd %f10, %f8, %f8\n"
"     ret\n"
"     restore" : :)],
allegro_cv_processor_type=sparc,
allegro_cv_processor_type=unknown))])
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
      dnl Use libdl if found, else assume dl* functions in libc.
      AC_CHECK_LIB(dl, dlopen,
        [LIBS="-ldl $LIBS"])
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
[  --enable-xwin-vidmode[=x] enable the use of XF86VidMode Ext. [default=yes]],
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
[  --enable-artsdigi[=x]    enable building ARTS DIGI driver [default=yes]],
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
dnl Test where is sched_yield (SunOS).
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
  allegro_support_constructor=no)
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
[if test $GCC = yes && $CC --version | grep '3\.0\(\.\?[[012]]\)\?$' >/dev/null; then
  allegro_cv_support_fomit_frame_pointer=no
else
  allegro_cv_support_fomit_frame_pointer=yes
fi
])
AC_MSG_RESULT($allegro_cv_support_fomit_frame_pointer)])

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
[if test $GCC = yes; then
   AC_TRY_COMPILE(,int foo(){return 0;}, allegro_cv_support_include_prefix=yes, allegro_cv_support_include_prefix=no)
else
   allegro_cv_support_include_prefix=yes
fi
])
CFLAGS="$allegro_save_CFLAGS"
AC_MSG_RESULT($allegro_cv_support_include_prefix)])

