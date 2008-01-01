#
#	JPGalleg 2.6, by Angelo Mottola, 2000-2006
#
#	BeOS specific makefile rules
#


PLATFORM = BeOS
PLATFORM_PATH = beos
EXE = 
OBJ = .o

UNIX = 1

CC = gcc
OFLAGS = -mpentium -O2 -ffast-math -fomit-frame-pointer
CFLAGS = -c -s -I./include -I../../include $(OFLAGS) -Wall -W

LIB = ar
LFLAGS = rs

ECHO = @echo $(1)
CP = cp
MV = mv
RM = rm -f

ifdef STATICLINK
  ifdef DEBUG
    LIB_NAME = libjpgad_s.a
  else
    LIB_NAME = libjpgal_s.a
  endif
else
  ifdef DEBUG
    LIB_NAME = libjpgad.a
  else
    LIB_NAME = libjpgal.a
  endif
endif

LIBS = -lm `../../allegro-config --libs --addon`
LDFLAGS = -L../../lib/$(PLATFORM_PATH)
INSTALL_LIB_PATH = /boot/develop/lib/x86
INSTALL_HEADER_PATH = /boot/develop/headers


$(INSTALL_LIB_PATH)/$(LIB_NAME): lib/beos/$(LIB_NAME)
	$(CP) lib/beos/$(LIB_NAME) $(INSTALL_LIB_PATH)

$(INSTALL_HEADER_PATH)/jpgalleg.h: include/jpgalleg.h
	$(CP) include/jpgalleg.h $(INSTALL_HEADER_PATH)

INSTALL_FILES = $(INSTALL_LIB_PATH)/$(LIB_NAME) $(INSTALL_HEADER_PATH)/jpgalleg.h

install: $(INSTALL_FILES)
	$(call ECHO,JPGalleg for $(PLATFORM) has been successfully installed.)

uninstall:
	$(RM) $(INSTALL_LIB_PATH)/$(LIB_NAME)
	$(RM) $(INSTALL_HEADER_PATH)/jpgalleg.h
ifdef ALLEGRO
	$(call REMOVE_PLUGIN_UNIX)
else
	$(call ECHO,Cannot remove grabber plugin: ALLEGRO environmental variable not set)
endif
	$(call ECHO,All gone!)


mmxtest:
	- @echo > mmx.h
	- @echo .text > mmxtest.s
	- @echo emms >> mmxtest.s
	- @$(CC) -c mmxtest.s -o obj/beos/mmxtest.o
	- @echo #define JPGALLEG_MMX > mmx.h

include/mmx.h:
	$(call ECHO,Testing for MMX assembler support...)
	@-$(MAKE) mmxtest --quiet
	$(MV) mmx.h include
	$(RM) mmxtest.s

