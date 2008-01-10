# Allegro Demo Game makefile

BASENAME = demo

ifdef DEBUGMODE
   CFLAGS = -W -Wall -O -g -DDEBUGMODE
endif

ifdef DONT_USE_EXTENSIONS
   CFLAGS += -DDONT_USE_CD
endif

ifdef PROFILEMODE
   CFLAGS += -pg
endif

#ifndef DEBUGMODE
#   CFLAGS += -Werror
#endif

ifdef WITH_ALLEGRO_GL
   CFLAGS += -DSKATER_USE_ALLEGRO_GL -DSKATER_USE_ALLEGRO_GL_FONTS
   LIBS += -lagl -lGL -lGLU
endif

#For Make to detect MingW automatically, the variable MINGDIR must be set
#Likewise, for DJGPP the variable DJGPP must be set
ifdef MINGDIR
   include makefile.mgw
else
   ifdef DJGPP
        include makefile.dj
   else
        include makefile.nix
   endif
endif

include makefile.lst

OBJS=$(addprefix $(OBJDIR), $(DEMO_SRC_FILES:.c=.o))
DEPRULE = $(foreach src, $(DEMO_SRC_FILES), "$(addprefix $(OBJDIR),$(src:.c=.o)):$(addprefix src/,$(src))\n\t$(CC) $(CFLAGS) $(INCLUDE) -c $(addprefix src/,$(src)) -o $(addprefix $(OBJDIR),$(src:.c=.o))\n\n")

include makefile.all

include makefile.dep
