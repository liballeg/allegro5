#
#  Rules for building the Allegro library with RSXNT. This file is included
#  by the primary makefile, and should not be used directly.
#
#  This is not a full makefile: it uses the MSVC DLL, so it only has to
#  build an import library and the test/example programs. You must have
#  compiled the MSVC library before using this makefile.
#
#  See makefile.all for a list of the available targets.



# -------- define some variables that the primary makefile will use --------

PLATFORM = RSXNT
OBJ_DIR = obj/rsxnt/$(VERSION)
LIB_BASENAME = lib$(SHORT_VERSION)_i.a
LIB_NAME = lib/rsxnt/$(LIB_BASENAME)
EXE = .exe
OBJ = .o



# -------- check that the RSXNTDJ environment variable is set --------

ifndef RSXNTDJ
baddjgpp:
	@echo Your RSXNTDJ environment variable is not set correctly!
endif

RSXNTDJ_U = $(subst \,/,$(RSXNTDJ))
RSXNTDJ_D = $(subst /,\,$(RSXNTDJ))



# -------- give a sensible default target for make without any args --------

.PHONY: _default

_default: default



# -------- decide what compiler options to use --------

ifdef WARNMODE
   WFLAGS = -Wall -W -Werror -Wno-unused
else
   WFLAGS = -Wall -Wno-unused
endif

ifdef TARGET_ARCH_COMPAT
   TARGET_ARCH = -mcpu=$(TARGET_ARCH_COMPAT)
else
   ifdef TARGET_ARCH_EXCL
      TARGET_ARCH = -march=$(TARGET_ARCH_EXCL)
   else
      TARGET_ARCH = -mcpu=pentium
   endif
endif

ifndef TARGET_OPTS
   TARGET_OPTS = -O2 -funroll-loops -ffast-math
endif

OFLAGS = $(TARGET_ARCH) $(TARGET_OPTS)

ifdef DEBUGMODE

# -------- debugging build --------
CFLAGS =  -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS) -g
LFLAGS =  -g

else
ifdef PROFILEMODE

# -------- profiling build --------
CFLAGS =  $(WFLAGS) $(OFLAGS) -pg
LFLAGS =  -pg

else

# -------- optimised build --------
CFLAGS =  $(WFLAGS) $(OFLAGS) -fomit-frame-pointer
LFLAGS = 

endif
endif



# -------- this code checks for gccw32 or RSXNT 1.5 --------

ifneq ($(wildcard $(RSXNTDJ_U)/bin/gccw32.exe),)
   RSXGCC = gccw32 
else
   RSXGCC = gcc -Zwin32
endif



# -------- list platform specific programs --------

VPATH = tests/win tools/win

LIBRARIES = -lcomdlg32

PROGRAMS = dibgrab dibhello dibsound scrsave wfixicon

dibgrab: tests/win/dibgrab.exe
dibhello: tests/win/dibhello.exe
dibsound: tests/win/dibsound.exe
scrsave: tests/win/scrsave.scr
wfixicon: tools/win/wfixicon.exe



# -------- rules for installing and removing the library files --------

$(RSXNTDJ_U)/lib/$(LIB_BASENAME): $(LIB_NAME)
	copy lib\rsxnt\$(LIB_BASENAME) $(RSXNTDJ_D)\lib
	@echo You should also run fixdjgpp and then make install that,
	@echo to get the documentation and headers copied across.

install: $(RSXNTDJ_U)/lib/$(LIB_BASENAME)
	@echo The $(DESCRIPTION) RSXNT library has been installed.

uninstall:
	-rm -fv $(RSXNTDJ_U)/lib/liball_i.a $(RSXNTDJ_U)/lib/libald_i.a $(RSXNTDJ_U)/lib/libalp_i.a
	@echo All gone!



# -------- finally, we get to the fun part... --------

$(LIB_NAME): lib/msvc/$(SHORT_VERSION)$(LIBRARY_VERSION).dll

define MAKE_LIB
   makelib lib/msvc/$(SHORT_VERSION)$(LIBRARY_VERSION).dll -o $(LIB_NAME)
endef

$(OBJ_DIR)/%.o: %.c
	$(RSXGCC) $(CFLAGS) -I. -I./include -o $@ -c $<

*/%.exe: $(OBJ_DIR)/%.o $(LIB_NAME)
	$(RSXGCC) $(LFLAGS) -o $@ $< $(LIB_NAME) $(LIBRARIES)

tests/win/dibsound.exe: $(OBJ_DIR)/dibsound.o $(OBJ_DIR)/dibsound.res $(LIB_NAME)
	$(RSXGCC) $(LFLAGS) -o tests/win/dibsound.exe $(OBJ_DIR)/dibsound.o $(LIB_NAME) $(LIBRARIES)
	rsrc $(OBJ_DIR)/dibsound.res tests/win/dibsound.exe

tests/win/%.exe: $(OBJ_DIR)/%.o $(LIB_NAME)
	$(RSXGCC) $(LFLAGS) -o $@ $< $(LIB_NAME) $(LIBRARIES)

tests/win/scrsave.scr: $(OBJ_DIR)/scrsave.o $(OBJ_DIR)/scrsave.res $(LIB_NAME)
	$(RSXGCC) $(LFLAGS) -o tests/win/scrsave.exe $(OBJ_DIR)/scrsave.o $(LIB_NAME) $(LIBRARIES)
	rsrc $(OBJ_DIR)/scrsave.res tests/win/scrsave.exe
	rename tests\win\scrsave.exe scrsave.scr

tools/win/%.exe: $(OBJ_DIR)/%.o $(LIB_NAME)
	$(RSXGCC) $(LFLAGS) -o $@ $< $(LIB_NAME) $(LIBRARIES)

$(OBJ_DIR)/dibsound.res: tests/win/dibsound.rc
	grc -o $(OBJ_DIR)/dibsound.res tests/win/dibsound.rc

$(OBJ_DIR)/scrsave.res: tests/win/scrsave.rc
	grc -o $(OBJ_DIR)/scrsave.res tests/win/scrsave.rc

PLUGIN_LIB = lib/rsxnt/lib$(VERY_SHORT_VERSION)dat.a
PLUGINS_H = obj/rsxnt/plugins.h
PLUGIN_DEPS = $(LIB_NAME) $(PLUGIN_LIB)
PLUGIN_SCR = scx

define GENERATE_PLUGINS_H
   copy tools\plugins\*.inc obj\rsxnt\plugins.h
endef

define MAKE_PLUGIN_LIB
   ar rs $(PLUGIN_LIB) $(PLUGIN_OBJS)
endef

define LINK_WITH_PLUGINS
   $(RSXGCC) $(LFLAGS) -o $@ $< $(strip $(PLUGIN_LIB) $(addprefix @,$(PLUGIN_SCRIPTS)) $(LIB_NAME) $(LIBRARIES))
endef



# -------- generate automatic dependencies --------

DEPEND_PARAMS = -MM -MG -I. -I./include -DSCAN_DEPEND -DALLEGRO_RSXNT

depend:
	gcc $(DEPEND_PARAMS) demo/*.c examples/*.c setup/*.c tests/*.c tests/win/*.c tools/*.c tools/win/*.c tools/plugins/*.c > _depend.tmp
	sed -e "s/^[a-zA-Z0-9_\/]*\///" _depend.tmp > _depend2.tmp
ifdef UNIX_TOOLS
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/rsxnt\/alleg\/\1\.o:/" _depend2.tmp > obj/rsxnt/alleg/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/rsxnt\/alld\/\1\.o:/" _depend2.tmp > obj/rsxnt/alld/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/rsxnt\/allp\/\1\.o:/" _depend2.tmp > obj/rsxnt/allp/makefile.dep
	rm _depend.tmp _depend2.tmp
else
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/rsxnt\/alleg\/\1\.o:/" _depend2.tmp > obj\rsxnt\alleg\makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/rsxnt\/alld\/\1\.o:/" _depend2.tmp > obj\rsxnt\alld\makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/rsxnt\/allp\/\1\.o:/" _depend2.tmp > obj\rsxnt\allp\makefile.dep
	del _depend.tmp
	del _depend2.tmp
endif
