#
#  Rules for building the Allegro library with BeOS. This file is included
#  by the primary makefile, and should not be used directly.
#
#  The "depend" target uses sed.
#
#  See makefile.all for a list of the available targets.



# -------- define some variables that the primary makefile will use --------

PLATFORM = BeOS
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
WFLAGS = -Wall -W -Werror -Wno-unused -Wno-multichar -Wno-ctor-dtor-privacy
else
WFLAGS = -Wall -Wno-unused -Wno-multichar -Wno-ctor-dtor-privacy
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
   TARGET_OPTS = -O6 -funroll-loops -ffast-math
endif

OFLAGS =  $(TARGET_ARCH) $(TARGET_OPTS)

ifdef DEBUGMODE



# -------- debugging build --------
CFLAGS = -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS) -g -O0
SFLAGS = -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS)
LFLAGS = -g


else
ifdef PROFILEMODE

# -------- profiling build --------
CFLAGS = $(WFLAGS) $(OFLAGS) -pg
SFLAGS = $(WFLAGS)
LFLAGS = -pg

else

# -------- optimised build --------
CFLAGS = $(WFLAGS) $(OFLAGS) -fomit-frame-pointer
SFLAGS = $(WFLAGS)

ifdef SYMBOLMODE
LFLAGS = -s
else
LFLAGS = 
endif

endif
endif


# -------- list which platform specific objects to include --------

VPATH = src/beos src/i386 src/misc tools/beos

LIBRARIES = -lbe -lgame -ldevice -lmidi -lmedia

OBJECT_LIST = $(COMMON_OBJECTS) $(I386_OBJECTS) \
	      $(basename $(notdir $(ALLEGRO_SRC_BEOS_FILES)))

PROGRAMS = bfixicon

bfixicon: tools/beos/bfixicon

DISTCLEAN_FILES += tools/beos/bfixicon



# -------- rules for installing and removing the library files --------

INC_DIR = /boot/develop/headers
LIB_DIR = /boot/develop/lib/x86
SHARED_LIB_DIR = /boot/home/config/lib

ifdef STATICLINK

LFLAGS += $(LIBRARIES)
$(LIB_DIR)/lib$(VERSION).a: $(LIB_NAME)
	cp $< $@
	
else	
		
$(SHARED_LIB_DIR)/lib$(VERSION)-$(shared_version).so: $(LIB_NAME)
	cp $< $@	
	
endif

$(INC_DIR)/%: include/%
	cp $< $@

$(INC_DIR)/allegro:
	mkdir $(INC_DIR)/allegro

$(INC_DIR)/allegro/%.h: include/allegro/%.h $(INC_DIR)/allegro
	cp $< $@

$(INC_DIR)/allegro/internal:
	mkdir $(INC_DIR)/allegro/internal

$(INC_DIR)/allegro/internal/%.h: include/allegro/internal/%.h $(INC_DIR)/allegro/internal
	cp $< $@

$(INC_DIR)/allegro/inline:
	mkdir $(INC_DIR)/allegro/inline

$(INC_DIR)/allegro/inline/%.inl: include/allegro/inline/%.inl $(INC_DIR)/allegro/inline
	cp $< $@

$(INC_DIR)/allegro/platform:
	mkdir $(INC_DIR)/allegro/platform

$(INC_DIR)/allegro/platform/%.h: include/allegro/platform/%.h $(INC_DIR)/allegro/platform
	cp $< $@

HEADERS = $(subst /include,,$(addprefix $(INC_DIR)/,$(wildcard include/allegro/*.h)))          \
          $(subst /include,,$(addprefix $(INC_DIR)/,$(wildcard include/allegro/internal/*.h))) \
          $(subst /include,,$(addprefix $(INC_DIR)/,$(wildcard include/allegro/inline/*.inl)))

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

INSTALL_FILES = $(INC_DIR)/allegro.h                   \
		$(INC_DIR)/allegro                     \
		$(INC_DIR)/allegro/internal            \
		$(INC_DIR)/allegro/inline              \
		$(INC_DIR)/allegro/platform            \
		$(INC_DIR)/bealleg.h                   \
		$(INC_DIR)/allegro/platform/aintbeos.h \
		$(INC_DIR)/allegro/platform/al386gcc.h \
		$(INC_DIR)/allegro/platform/albecfg.h  \
		$(INC_DIR)/allegro/platform/alplatf.h  \
		$(INC_DIR)/allegro/platform/albeos.h   \
		$(HEADERS)                             \
		/bin/allegro-config
		
ifdef STATICLINK
	INSTALL_FILES += $(LIB_DIR)/lib$(VERSION).a 
else
	INSTALL_FILES += $(SHARED_LIB_DIR)/lib$(VERSION)-$(shared_version).so
endif		

install: $(INSTALL_FILES)
	@echo The $(DESCRIPTION) $(PLATFORM) library has been installed.

UNINSTALL_FILES = $(LIB_DIR)/liballeg.a                           \
		  $(LIB_DIR)/liballd.a                            \
		  $(LIB_DIR)/liballp.a                            \
		  $(SHARED_LIB_DIR)/liballeg-$(shared_version).so \
		  $(SHARED_LIB_DIR)/liballd-$(shared_version).so  \
		  $(SHARED_LIB_DIR)/liballp-$(shared_version).so  \
		  $(INC_DIR)/allegro.h                            \
		  $(INC_DIR)/bealleg.h                            \
		  $(INC_DIR)/allegro/*.h                          \
		  $(INC_DIR)/allegro/internal/*.h                 \
		  $(INC_DIR)/allegro/inline/*.inl                 \
		  $(INC_DIR)/allegro/platform/*.h                 \
		  /bin/allegro-config

uninstall:
	-rm -fv $(UNINSTALL_FILES)
	-rmdir $(INC_DIR)/allegro/platform
	-rmdir $(INC_DIR)/allegro/inline
	-rmdir $(INC_DIR)/allegro/internal
	-rmdir $(INC_DIR)/allegro
	@echo All gone!	


# test assembler capabilities.

include makefile.tst


# -------- finally, we get to the fun part... --------

ifdef STATICLINK

# -------- link as a static library --------
define MAKE_LIB
ar rs $(LIB_NAME) $(OBJECTS)
endef

else

# -------- link as a shared library --------

define MAKE_LIB
gcc -nostart $(PFLAGS) -o $(LIB_NAME) $(OBJECTS) $(LIBRARIES)
endef

endif # STATICLINK

COMPILE_FLAGS = $(subst src/,-DALLEGRO_SRC ,$(findstring src/, $<))$(CFLAGS)

$(OBJ_DIR)/%.o: %.c
	gcc $(COMPILE_FLAGS) -I. -I./include -o $@ -c $<

$(OBJ_DIR)/%.o: %.cpp
	gcc $(COMPILE_FLAGS) -I. -I./include -o $@ -c $<

$(OBJ_DIR)/%.o: %.s
	gcc $(SFLAGS) -I. -I./include -x assembler-with-cpp -o $@ -c $<

*/%: $(OBJ_DIR)/%.o $(LIB_NAME)
	gcc $(LFLAGS) -o $@ $< $(LIB_NAME)

# link without Allegro, because we have no shared library yet
docs/makedoc: $(OBJ_DIR)/makedoc$(OBJ)
	gcc -o $@ $<

obj/beos/asmdef.inc: obj/beos/asmdef
	obj/beos/asmdef obj/beos/asmdef.inc

obj/beos/asmdef: src/i386/asmdef.c include/*.h include/allegro/*.h obj/beos/asmcapa.h
	gcc -O $(WFLAGS) -I. -I./include -o obj/beos/asmdef src/i386/asmdef.c

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
gcc $(LFLAGS) -o $@ $< $(strip $(PLUGIN_LIB) $(addprefix @,$(PLUGIN_SCRIPTS)) $(LIB_NAME))
endef

tools/beos/%: $(OBJ_DIR)/%.o $(LIB_NAME)
	gcc $(LFLAGS) -o $@ $< $(LIB_NAME) $(LIBRARIES)



# -------- demo program iconification --------

.PHONY: fixdemo

fixdemo: demo/demo demo/demo.dat tools/beos/bfixicon
	tools/beos/bfixicon demo/demo -d demo/demo.dat SHIP3 GAME_PAL



# -------- generate automatic dependencies --------

DEPEND_PARAMS = -MM -MG -I. -I./include -DSCAN_DEPEND -DALLEGRO_BEOS

depend:
	gcc $(DEPEND_PARAMS) src/*.c src/beos/*.c src/beos/*.cpp src/i386/*.c src/misc/*.c demo/*.c examples/*.c setup/*.c tests/*.c tools/*.c tools/beos/*.cpp tools/plugins/*.c > _depend.tmp
	gcc $(DEPEND_PARAMS) -x assembler-with-cpp src/i386/*.s src/misc/*.s >> _depend.tmp
	sed -e "s/^[a-zA-Z0-9_\/]*\///" _depend.tmp > _depend2.tmp
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/alleg\/\1/" _depend2.tmp > obj/beos/alleg/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/alld\/\1/" _depend2.tmp > obj/beos/alld/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/allp\/\1/" _depend2.tmp > obj/beos/allp/makefile.dep
	rm _depend.tmp _depend2.tmp

