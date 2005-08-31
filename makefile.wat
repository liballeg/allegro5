#
#  Rules for building the Allegro library with Watcom. This file is
#  included by the primary makefile, and should not be used directly.
#
#  This build uses djgpp for building the assembler sources and calculating
#  source dependencies, so you'll need to have that installed as well.
#
#  The "depend" target and assembler operations use sed.
#
#  See makefile.all for a list of the available targets.



# -------- define some variables that the primary makefile will use --------

PLATFORM = Watcom
OBJ_DIR = obj/watcom/$(VERSION)
LIB_NAME = lib/watcom/$(VERSION).lib
RUNNER = obj/watcom/runner.exe
GCC = gcc
EXE = .exe
OBJ = .obj
HTML = htm

PLATFORM_DIR = obj/watcom

ifneq (,$(findstring bash,$(SHELL)))
   UNIX_TOOLS = 1
endif



# -------- check that the WATCOM environment variable is set --------

.PHONY: badwatcom

ifndef WATCOM
badwatcom:
	@echo Your WATCOM environment variable is not set!
endif

WATDIR_U = $(subst \,/,$(WATCOM))
WATDIR_D = $(subst /,\,$(WATCOM))



# -------- try to figure out the compiler version --------

ifndef WATCOM_VERSION
ifeq ($(wildcard $(WATDIR_U)/binw/wdisasm.exe),)
WATCOM_VERSION=11
else
WATCOM_VERSION=10.6
endif
endif



# -------- give a sensible default target for make without any args --------

.PHONY: _default

_default: default



# -------- decide what compiler options to use --------

ifdef WARNMODE
WFLAGS = -w3 -we
else
WFLAGS = -w1
endif

ifdef DEBUGMODE

# -------- debugging build --------

CFLAGS = -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS) -bt=dos4g -5s -s -d2
SFLAGS = -DDEBUGMODE=$(DEBUGMODE) -Wall
LFLAGS = 'option quiet' 'option stack=128k' 'system dos4g' 'debug all'

else
ifdef PROFILEMODE

# -------- profiling build --------
CFLAGS = $(WFLAGS) -bt=dos4g -5s -ox -et
SFLAGS = -Wall
LFLAGS = 'option quiet' 'option stack=128k' 'system dos4g'

else

# -------- optimised build --------
CFLAGS = $(WFLAGS) -bt=dos4g -5s -ox
SFLAGS = -Wall
LFLAGS = 'option quiet' 'option stack=128k' 'system dos4g'

endif
endif



# -------- list which platform specific objects to include --------

VPATH = src/dos src/i386 src/misc

OBJECT_LIST = $(COMMON_OBJECTS) $(I386_OBJECTS) \
		  $(basename $(notdir $(ALLEGRO_SRC_DOS_FILES))) \
		  wat



# -------- rules for installing and removing the library files --------

INSTALLDIR = $(WATDIR_U)
LIBDIR = lib386
INCDIR = h


ifdef UNIX_TOOLS

$(WATDIR_U)/lib386/$(VERSION).lib: $(LIB_NAME)
	cp lib/watcom/$(VERSION).lib $(WATDIR_U)/lib386

else

$(WATDIR_U)/lib386/$(VERSION).lib: $(LIB_NAME)
	copy lib\watcom\$(VERSION).lib $(WATDIR_D)\lib386

endif # UNIX_TOOLS


HEADERS = $(WATDIR_U)/h/allegro/platform/aintdos.h  \
          $(WATDIR_U)/h/allegro/platform/al386wat.h \
          $(WATDIR_U)/h/allegro/platform/alwatcom.h \
          $(WATDIR_U)/h/allegro/platform/alplatf.h  \
          $(WATDIR_U)/h/allegro/platform/astdint.h  \
          $(WATDIR_U)/h/allegro/platform/aldos.h

INSTALL_FILES = $(WATDIR_U)/lib386/$(VERSION).lib \
                $(HEADERS)               

install: generic-install
	@echo The $(DESCRIPTION) $(PLATFORM) library has been installed.

UNINSTALL_FILES = $(WATDIR_U)/lib386/alleg.lib \
                  $(WATDIR_U)/lib386/alld.lib  \
                  $(WATDIR_U)/lib386/allp.lib  \
                  $(HEADERS)

uninstall: generic-uninstall
	@echo All gone!



# -------- test capabilities --------

TEST_CPP = wpp386 src\misc\test.cpp -zq -fr=nul -fo=obj\watcom\test.o

include makefile.tst



# -------- finally, we get to the fun part... --------

define MAKE_LIB
$(RUNNER) wlib \\ @ -q -b -n $(LIB_NAME) $(addprefix +,$(OBJECTS))
endef

GCC2WATCOM = -D__SW_3S -D__SW_S

COMPILE_FLAGS = $(subst src/,-dALLEGRO_SRC ,$(findstring src/, $<))$(CFLAGS)

$(OBJ_DIR)/%.obj: %.c $(RUNNER)
	$(RUNNER) wcc386 \\ $< @ $(COMPILE_FLAGS) -zq -fr=nul -I. -I./include -fo=$@

$(OBJ_DIR)/%.obj: %.cpp $(RUNNER)
	$(RUNNER) wpp386 \\ $< @ $(COMPILE_FLAGS) -zq -fr=nul -I. -I./include -fo=$@

ifeq ($(WATCOM_VERSION),11)

# -------- Watcom 11.0 supports COFF --------
$(OBJ_DIR)/%.obj: %.s
	$(RUNNER) wcc386 \\ $< -p -I. -I./include -fo=$(OBJ_DIR)/$*.S
	as -o $@ $(OBJ_DIR)/$*.S
ifdef UNIX_TOOLS
	rm $(OBJ_DIR)/$*.S
else
	del $(subst /,\,$(OBJ_DIR)/$*.S)
endif

else

# -------- black magic to build asm sources with Watcom 10.6 --------
$(OBJ_DIR)/%.obj: %.s $(RUNNER)
	$(RUNNER) wcc386 \\ $< -p -I. -I./include -fo=$(OBJ_DIR)/$*.S
	as -o $(OBJ_DIR)/$*.o $(OBJ_DIR)/$*.S
ifdef UNIX_TOOLS
	rm $(OBJ_DIR)/$*.S
else
	del $(subst /,\,$(OBJ_DIR)/$*.S)
endif
	$(RUNNER) wdisasm \\ -a $(OBJ_DIR)/$*.o -l=$(OBJ_DIR)/$*.lst
	sed -e "s/\.text/_TEXT/; s/\.data/_DATA/; s/\.bss/_BSS/; s/\.386/\.586/; s/lar *ecx,cx/lar ecx,ecx/; s/ORG     [0-9]*H/ORG     00000000H/" $(OBJ_DIR)/$*.lst > $(OBJ_DIR)/$*.asm
	$(RUNNER) wasm \\ $(WFLAGS) -zq -fr=nul -fp3 -fo=$@ $(OBJ_DIR)/$*.asm

$(OBJ_DIR)/iscanmmx.obj: iscanmmx.s $(RUNNER)
	$(RUNNER) echo \\ END > $(OBJ_DIR)/iscanmmx.asm
	$(RUNNER) wasm \\ $(WFLAGS) -zq -fr=nul -fp3 -fo=$(OBJ_DIR)/iscanmmx.obj $(OBJ_DIR)/iscanmmx.asm

endif

demo/demo.exe: $(OBJ_DIR)/demo.obj
	$(RUNNER) wlink \\ @ $(LFLAGS) 'name $@' $(patsubst %,'file %',$(OBJECTS_DEMO)) 'library $(LIB_NAME)'

*/%.exe: $(OBJ_DIR)/%.obj $(LIB_NAME) $(RUNNER)
	$(RUNNER) wlink \\ @ $(LFLAGS) 'name $@' 'file $<' 'library $(LIB_NAME)'

obj/watcom/asmdef.inc: obj/watcom/asmdef.exe
	obj/watcom/asmdef.exe obj/watcom/asmdef.inc

obj/watcom/asmdef.exe: src/i386/asmdef.c include/*.h include/allegro/*.h $(RUNNER)
	wcl386 $(WFLAGS) -zq -fr=nul -bt=dos4g -5s -s -I. -I.\\include -fo=obj\\watcom\\asmdef.obj -fe=obj\\watcom\\asmdef.exe src\\i386\\asmdef.c

obj/watcom/runner.exe: src/misc/runner.c
	$(GCC) -O -Wall -Werror -o obj/watcom/runner.exe src/misc/runner.c

define LINK_WITHOUT_LIB
   $(RUNNER) wlink \\ @ $(LFLAGS) 'name $@' '$(addprefix file ,$^)'
endef

PLUGIN_LIB = lib/watcom/$(VERY_SHORT_VERSION)dat.lib
PLUGINS_H = obj/watcom/plugins.h
PLUGIN_DEPS = $(LIB_NAME) $(PLUGIN_LIB) $(RUNNER)
PLUGIN_SCR = scw

ifdef UNIX_TOOLS
   define GENERATE_PLUGINS_H
      cat tools/plugins/*.inc > obj/watcom/plugins.h
   endef
else
   define GENERATE_PLUGINS_H
      copy /B tools\plugins\*.inc obj\watcom\plugins.h
   endef
endif

define MAKE_PLUGIN_LIB
$(RUNNER) wlib \\ @ -q -b -n $(PLUGIN_LIB) $(addprefix +,$(PLUGIN_OBJS))
endef

define LINK_WITH_PLUGINS
$(RUNNER) wlink \\ @ $(LFLAGS) 'name $@' 'file $<' $(strip 'library $(PLUGIN_LIB)' $(addprefix @,$(PLUGIN_SCRIPTS)) 'library $(LIB_NAME)')
endef



# -------- generate automatic dependencies --------

DEPEND_PARAMS = -MM -MG -I. -I./include -DSCAN_DEPEND -DALLEGRO_WATCOM

depend:
	$(GCC) $(DEPEND_PARAMS) src/*.c src/dos/*.c src/i386/*.c src/misc/*.c demo/*.c > _depend.tmp
	$(GCC) $(DEPEND_PARAMS) docs/src/makedoc/*.c examples/*.c setup/*.c tests/*.c tools/*.c tools/plugins/*.c >> _depend.tmp
	$(GCC) $(DEPEND_PARAMS) -x c tests/*.cpp >> _depend.tmp
	$(GCC) $(DEPEND_PARAMS) -x assembler-with-cpp src/i386/*.s src/dos/*.s src/misc/*.s >> _depend.tmp
	sed -e "s/^[a-zA-Z0-9_\/]*\///" _depend.tmp > _depend2.tmp
ifdef UNIX_TOOLS
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/watcom\/alleg\/\1\.obj:/" _depend2.tmp > obj/watcom/alleg/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/watcom\/alld\/\1\.obj:/" _depend2.tmp > obj/watcom/alld/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/watcom\/allp\/\1\.obj:/" _depend2.tmp > obj/watcom/allp/makefile.dep
	rm _depend.tmp _depend2.tmp
else
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/watcom\/alleg\/\1\.obj:/" _depend2.tmp > obj\watcom\alleg\makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/watcom\/alld\/\1\.obj:/" _depend2.tmp > obj\watcom\alld\makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\)\.o:/obj\/watcom\/allp\/\1\.obj:/" _depend2.tmp > obj\watcom\allp\makefile.dep
	del _depend.tmp
	del _depend2.tmp
endif
