#
#  Rules for building the Allegro library with BeOS. This file is included
#  by the primary makefile, and should not be used directly.
#
#  The "depend" target uses sed.
#
#  See makefile.all for a list of the available targets.



# -------- define some variables that the primary makefile will use --------

PLATFORM = BeOS
OBJ_DIR = obj/beos/$(VERSION)
LIB_NAME = lib/beos/lib$(VERSION).a
EXE = 
OBJ = .o
HTML = html


# -------- give a sensible default target for make without any args --------

.PHONY: _default

_default: default


# -------- decide what compiler options to use --------

ifdef WARNMODE
WFLAGS = -Wall -W -Werror -Wno-unused -Wno-multichar -Wno-ctor-dtor-privacy
else
WFLAGS = -Wall -Wno-unused -Wno-multichar -Wno-ctor-dtor-privacy
endif

OFLAGS = -mpentium -O6 -ffast-math

ifdef DEBUGMODE



# -------- debugging build --------
CFLAGS = -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS) -g -O0
SFLAGS = -DDEBUGMODE=$(DEBUGMODE) $(WFLAGS)
LFLAGS = -lbe -lgame -ldevice -lmidi -g


else
ifdef PROFILEMODE

# -------- profiling build --------
CFLAGS = $(WFLAGS) $(OFLAGS) -pg
SFLAGS = $(WFLAGS)
LFLAGS = -lbe -lgame -ldevice -lmidi -pg

else

# -------- optimised build --------
CFLAGS = $(WFLAGS) $(OFLAGS) -fomit-frame-pointer
SFLAGS = $(WFLAGS)

ifdef SYMBOLMODE
LFLAGS = -lbe -lgame -ldevice -lmidi -s
else
LFLAGS = -lbe -lgame -ldevice -lmidi
endif

endif
endif


# -------- list which platform specific objects to include --------

VPATH = src/beos src/i386 src/misc

OBJECT_LIST = $(COMMON_OBJECTS) $(I386_OBJECTS) \
	      $(basename $(notdir $(ALLEGRO_SRC_BEOS_FILES)))


# -------- rules for installing and removing the library files --------

INC_DIR = /boot/develop/headers/cpp
LIB_DIR = /boot/develop/lib/x86

$(LIB_DIR)/lib$(VERSION).a: $(LIB_NAME)
	cp $(LIB_NAME) $(LIB_DIR)

$(INC_DIR)/%: include/%
	cp $< $@

$(INC_DIR)/allegro/%.h: include/allegro/%.h $(INC_DIR)/allegro
	cp $< $@

$(INC_DIR)/allegro:
	mkdir $(INC_DIR)/allegro

HEADERS = $(subst /include,,$(addprefix $(INC_DIR)/,$(wildcard include/allegro/*.h)))

/bin/allegro-config:
	cp misc/allegro-config.be /bin/allegro-config
	chmod a+x /bin/allegro-config

INSTALL_FILES = $(LIB_DIR)/lib$(VERSION).a \
		$(INC_DIR)/allegro.h \
		$(INC_DIR)/bealleg.h \
		$(INC_DIR)/allegro \
		$(HEADERS) \
		/bin/allegro-config

install: $(INSTALL_FILES)
	@echo The $(DESCRIPTION) BeOS library has been installed.

UNINSTALL_FILES = $(LIB_DIR)/liballeg.a $(LIB_DIR)/liballd.a $(LIB_DIR)/liballp.a \
		$(INC_DIR)/allegro.h $(INC_DIR)/bealleg.h $(INC_DIR)/allegro/*.h

uninstall:
	rm -f $(UNINSTALL_FILES)
	rm -fr $(INC_DIR)/allegro
	rm -f /bin/allegro-config
	@echo All gone!	

# -------- autodetect whether the assembler supports MMX instructions --------

.PHONY: mmxtest

mmxtest:
	echo // no MMX > obj/beos/mmx.h
	echo .text > obj/beos/mmxtest.s
	echo emms >> obj/beos/mmxtest.s
	gcc -c obj/beos/mmxtest.s -o obj/beos/mmxtest.o
	echo #define ALLEGRO_MMX > obj/beos/mmx.h

obj/beos/mmx.h:
	@echo Testing for MMX assembler support...
	-$(MAKE) mmxtest



# -------- finally, we get to the fun part... --------

define MAKE_LIB
ar rs $(LIB_NAME) $(OBJECTS)
endef

COMPILE_FLAGS = $(subst src/,-DALLEGRO_SRC ,$(findstring src/, $<))$(CFLAGS)

$(OBJ_DIR)/%.o: %.c
	gcc $(COMPILE_FLAGS) -I. -I./include -o $@ -c $<

$(OBJ_DIR)/%.o: %.cpp
	gcc $(COMPILE_FLAGS) -I. -I./include -o $@ -c $<

$(OBJ_DIR)/%.o: %.s
	gcc $(SFLAGS) -I. -I./include -x assembler-with-cpp -o $@ -c $<

*/%: $(OBJ_DIR)/%.o $(LIB_NAME)
	gcc $(LFLAGS) -o $@ $< $(LIB_NAME)

obj/beos/asmdef.inc: obj/beos/asmdef
	obj/beos/asmdef obj/beos/asmdef.inc

obj/beos/asmdef: src/i386/asmdef.c include/*.h include/allegro/*.h obj/beos/mmx.h
	gcc -O $(WFLAGS) -I. -I./include -o obj/beos/asmdef src/i386/asmdef.c

obj/beos/setupdat.s: setup/setup.dat tools/dat2s
	tools/dat2s -o obj/beos/setupdat.s setup/setup.dat

obj/beos/setupdat.o: obj/beos/setupdat.s
	gcc -o obj/beos/setupdat.o -c obj/beos/setupdat.s

setup/setup: $(OBJ_DIR)/setup.o obj/beos/setupdat.o $(LIB_NAME)
	gcc $(LFLAGS) -o setup/setup $(OBJ_DIR)/setup.o obj/beos/setupdat.o $(LIB_NAME)

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



# -------- generate automatic dependencies --------

GCC2BEOS = -D__BEOS__ -UDJGPP -U__unix__

ifdef CROSSCOMPILE
  DEPEND_PARAMS = $(GCC2BEOS) -MM -MG -I. -I./include -DSCAN_DEPEND
else
  DEPEND_PARAMS = -MM -MG -I. -I./include -DSCAN_DEPEND
endif

depend:
	gcc $(DEPEND_PARAMS) src/*.c src/beos/*.c src/beos/*.cpp src/i386/*.c src/misc/*.c demo/*.c examples/*.c setup/*.c tests/*.c tools/*.c tools/plugins/*.c > _depend.tmp
	gcc $(DEPEND_PARAMS) -x assembler-with-cpp src/i386/*.s src/misc/*.s >> _depend.tmp
	sed -e "s/^[a-zA-Z0-9_\/]*\///" _depend.tmp > _depend2.tmp
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/alleg\/\1/" _depend2.tmp > obj/beos/alleg/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/alld\/\1/" _depend2.tmp > obj/beos/alld/makefile.dep
	sed -e "s/^\([a-zA-Z0-9_]*\.o *:\)/obj\/beos\/allp\/\1/" _depend2.tmp > obj/beos/allp/makefile.dep
	rm _depend.tmp _depend2.tmp
