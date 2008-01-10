.PRECIOUS: %.o

CC = gcc
CFLAGS += -Wall -O2 -g -Wno-deprecated-declarations -Wno-unused
INCLUDE = -Iinclude

ifdef CPU
   CFLAGS += -mcpu=$(CPU)
endif

ifdef STATICLINK
   LIBS += `allegro-config --libs --static`
else
   ifdef SHAREDLINK
      LIBS += `allegro-config --libs --shared` -lm
   else
      LIBS += `allegro-config --libs` -lm
   endif
endif

OBJDIR = obj/unix/
LIBDIR = lib/unix/
EXEDIR = bin/unix/

CFLAGS += -pipe
