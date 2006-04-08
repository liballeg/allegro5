#
#  Rules for building the Allegro library with BeOS. This file is included
#  by the primary makefile, and should not be used directly.
#
#  The "depend" target uses sed.
#
#  See makefile.all for a list of the available targets.



# -------- define some variables that the primary makefile will use --------

PLATFORM = BeOS
CC = gcc
EXE = 
OBJ = .o
HTML = html

PLATFORM_DIR = obj/beos

UNIX_TOOLS = 1

ifdef STATICLINK

# -------- link as a static library --------
OBJ_DIR = obj/beos/$(VERSION)
LIB_NAME = lib/beos/lib$(VERSION).a

else

# -------- link as a DLL --------
OBJ_DIR = obj/beos/$(VERSION)
LIB_NAME = lib/beos/lib$(VERSION)-$(shared_version).so

endif # STATICLINK

# -------- give a sensible default target for make without any args --------

.PHONY: _default

_default: default


# -------- decide what compiler options to use --------

ifdef WARNMODE
WFLAGS = -Wall -W -Wstrict-prototypes -Wno-unused -Wno-multichar -Wno-ctor-dtor-privacy -Werror
else
WFLAGS = -Wall -Wno-unused -Wno-multichar -Wno-ctor-dtor-privacy
endif

ifdef TARGET_ARCH_COMPAT
   TARGET_ARCH = $(GCC_MTUNE)=$(TARGET_ARCH_COMPAT)
else
   ifdef TARGET_ARCH_EXCL
      TARGET_ARCH = -march=$(TARGET_ARCH_EXCL)
   else
      TARGET_ARCH = $(GCC_MTUNE)=i586
   endif
endif

ifndef TARGET_OPTS
   TARGET_OPTS = -O6 -funroll-loops -ffast-math
endif

OFLAGS =  $(TARGET_ARCH) $(TARGET_OPTS)

CFLAGS = -DALLEGRO_LIB_BUILD



ifdef DEBUGMODE

# -------- debugging build --------
CFLAGS += -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS) -g -O0
SFLAGS = -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS)
LFLAGS = -g

else
ifdef PROFILEMODE

# -------- profiling build --------
CFLAGS += $(WFLAGS) $(OFLAGS) -pg
SFLAGS = $(WFLAGS)
LFLAGS = -pg

else

# -------- optimised build --------
CFLAGS += $(WFLAGS) $(OFLAGS) -fomit-frame-pointer
SFLAGS = $(WFLAGS)

ifndef SYMBOLMODE
LFLAGS = -s
else
LFLAGS = 
endif

endif
endif


# -------- list which platform specific objects to include --------

VPATH = src/beos src/misc src/unix tools/beos

ifdef ALLEGRO_USE_C

# ------ build a C-only version ------

VPATH += src/c
MY_OBJECTS = $(C_OBJECTS) cmiscs
CFLAGS += -DALLEGRO_NO_ASM

else

# ------ build the normal asm version ------

VPATH += src/i386
MY_OBJECTS = $(I386_OBJECTS)

endif # ALLEGRO_USE_C

OBJECT_LIST = $(COMMON_OBJECTS) $(MY_OBJECTS) $(basename $(notdir $(ALLEGRO_SRC_BEOS_FILES)))

LIBRARIES = -lbe -lgame -ldevice -lmidi -lmedia -lnet

PROGRAMS = bfixicon

bfixicon: tools/beos/bfixicon

DISTCLEAN_FILES += tools/beos/bfixicon



# -------- rules for installing and removing the library files --------

INSTALLDIR = /boot/develop
LIBDIR = lib/x86
INCDIR = headers

SHARED_LIBDIR = /boot/home/config/lib


ifdef STATICLINK

$(INSTALLDIR)/$(LIBDIR)/lib$(VERSION).a: $(LIB_NAME)
	cp $< $@
	
else	
		
$(SHARED_LIBDIR)/lib$(VERSION)-$(shared_version).so: $(LIB_NAME)
	cp $< $@	
	
endif


/bin/allegro-config:
ifdef STATICLINK
	sed -e "s/@LINK_WITH_STATIC_LIBS@/yes/" misc/allegro-config.in >temp
else
	sed -e "s/@LINK_WITH_STATIC_LIBS@/no/" misc/allegro-config.in >temp
endif
	sed -e "s/@prefix@/\/boot\/develop/" temp > temp2
	sed -e "s/@LIB_TO_LINK@/$(VERSION)/" temp2 > temp
	sed -e "s/@LDFLAGS@//" temp > temp2
	sed -e "s/@LIBS@/$(LIBRARIES)/" temp2 > temp
	sed -e "s/include/headers/" temp >temp2
	sed -e "s/ -l\$${lib_type}_unsharable//" temp2 >temp
	sed -e "s/libdirs=-L\$${exec_prefix}\/lib/libdirs=\"-L\$${exec_prefix}\/lib\/x86 -L\/boot\/home\/config\/lib\"/" temp >/bin/allegro-config
	rm -f temp temp2
	chmod a+x /bin/allegro-config


HEADERS = $(INSTALLDIR)/$(INCDIR)/bealleg.h                   \
          $(INSTALLDIR)/$(INCDIR)/allegro/platform/aintbeos.h \
          $(INSTALLDIR)/$(INCDIR)/allegro/platform/al386gcc.h \
          $(INSTALLDIR)/$(INCDIR)/allegro/platform/albecfg.h  \
          $(INSTALLDIR)/$(INCDIR)/allegro/platform/alplatf.h  \
          $(INSTALLDIR)/$(INCDIR)/allegro/platform/astdint.h  \
          $(INSTALLDIR)/$(INCDIR)/allegro/platform/albeos.h

ifdef STATICLINK
   INSTALL_FILES = $(INSTALLDIR)/$(LIBDIR)/lib$(VERSION).a 
else
   INSTALL_FILES = $(SHARED_LIBDIR)/lib$(VERSION)-$(shared_version).so
endif

INSTALL_FILES += $(HEADERS) /bin/allegro-config


install: generic-install
	@echo The $(DESCRIPTION) $(PLATFORM) library has been installed.

UNINSTALL_FILES = $(INSTALLDIR)/$(LIBDIR)/liballeg.a             \
                  $(INSTALLDIR)/$(LIBDIR)/liballd.a              \
                  $(INSTALLDIR)/$(LIBDIR)/liballp.a              \
                  $(SHARED_LIBDIR)/liballeg-$(shared_version).so \
                  $(SHARED_LIBDIR)/liballd-$(shared_version).so  \
                  $(SHARED_LIBDIR)/liballp-$(shared_version).so  \
                  $(HEADERS)                                     \
                  /bin/allegro-config

uninstall: generic-uninstall
	@echo All gone!



# -------- test capabilities --------

TEST_CPP = @echo ...system compiler

include makefile.tst



# -------- finally, we get to the fun part... --------

ifdef PROFILEMODE
OTHER_OBJECTS = /boot/develop/lib/x86/i386-mcount.o
endif

ifdef STATICLINK

# -------- link as a static library --------
define MAKE_LIB
ar rs $(LIB_NAME) $(OBJECTS) $(OTHER_OBJECTS)
endef

else

# -------- link as a shared library --------

define MAKE_LIB
$(CC) -nostart $(PFLAGS) -o $(LIB_NAME) $(OBJECTS) $(OTHER_OBJECTS) $(LIBRARIES)
endef

endif # STATICLINK

COMPILE_FLAGS = $(subst src/,-DALLEGRO_SRC ,$(findstring src/, $<))$(CFLAGS)

$(OBJ_DIR)/%.o: %.c
	$(CC) $(COMPILE_FLAGS) -I. -I./include -o $@ -c $<

$(OBJ_DIR)/%.o: %.cpp
	$(CC) $(COMPILE_FLAGS) -I. -I./include -o $@ -c $<

$(OBJ_DIR)/%.o: %.s
	$(CC) $(SFLAGS) -I. -I./include -x assembler-with-cpp -o $@ -c $<

demo/demo: $(OBJECTS_DEMO) $(LIB_NAME)
	$(CC) $(LFLAGS) -o $@ $(OBJECTS_DEMO) $(LIB_NAME) $(LIBRARIES)

*/%: $(OBJ_DIR)/%.o $(LIB_NAME)
	$(CC) $(LFLAGS) -o $@ $< $(LIB_NAME) $(LIBRARIES)

obj/beos/asmdef.inc: obj/beos/asmdef
	obj/beos/asmdef obj/beos/asmdef.inc

obj/beos/asmdef: src/i386/asmdef.c include/*.h include/allegro/*.h obj/beos/asmcapa.h
	$(CC) -O $(WFLAGS) -I. -I./include -o obj/beos/asmdef src/i386/asmdef.c

define LINK_WITHOUT_LIB
   $(CC) $(LFLAGS) -o $@ $^ $(OTHER_OBJECTS)
endef

PLUGIN_LIB = lib/beos/lib$(VERY_SHORT_VERSION)dat.a
PLUGINS_H = obj/beos/plugins.h
PLUGIN_DEPS = $(LIB_NAME) $(PLUGIN_LIB)
PLUGIN_SCR = scr

define GENERATE_PLUGINS_H
cat tools/plugins/*.inc > obj/beos/plugins.h
endef

define MAKE_PLUGIN_LIB
ar rs $(PLUGIN_LIB) $(PLUGIN_OBJS)
endef

define LINK_WITH_PLUGINS
$(CC) $(LFLAGS) -o $@ $< $(strip $(PLUGIN_LIB) $(addprefix @,$(PLUGIN_SCRIPTS)) $(LIB_NAME))
endef

tools/beos/%: $(OBJ_DIR)/%.o $(LIB_NAME)
	$(CC) $(LFLAGS) -o $@ $< $(LIB_NAME) $(LIBRARIES)



# -------- demo program iconification --------

.PHONY: fixdemo

fixdemo: demo/demo demo/demo.dat tools/beos/bfixicon
	tools/beos/bfixicon demo/demo -d demo/demo.dat SHIP3 GAME_PAL



# -------- generate automatic dependencies --------

DEPEND_PARAMS = -MM -MG -I. -I./include -DSCAN_DEPEND -DALLEGRO_BEOS

depend:
	$(CC) $(DEPEND_PARAMS) src/*.c src/beos/*.c src/i386/*.c src/misc/*.c src/unix/*.c demo/*.c > _depend.tmp
	$(CC) $(DEPEND_PARAMS) docs/src/makedoc/*.c examples/*.c setup/*.c tests/*.c tools/*.c tools/plugins/*.c >> _depend.tmp
	$(CC) $(DEPEND_PARAMS) -x c src/beos/*.cpp tests/*.cpp tools/beos/*.cpp >> _depend.tmp
	$(CC) $(DEPEND_PARAMS) -x assembler-with-cpp src/i386/*.s src/misc/*.s >> _depend.tmp
	sed -e "s/^[a-zA-Z0-9_\/]*\///" _depend.tmp > _depend2.tmp
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/alleg\/\1/" _depend2.tmp > obj/beos/alleg/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/alld\/\1/" _depend2.tmp > obj/beos/alld/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/allp\/\1/" _depend2.tmp > obj/beos/allp/makefile.dep
	rm _depend.tmp _depend2.tmp
