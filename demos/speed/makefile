# makefile for building Shawn's Speed Hack entry

OBJ = badguys bullets explode hiscore main message player sound title view

.PHONY: clean zip run8 run16 run32 runlo runhi


ifdef USE_WATCOM


#  Watcom version
#  --------------

OBJS = $(addsuffix .obj,$(OBJ))

speed.exe: $(OBJS)
	echo option quiet > _tmpfile.arg
	echo option stack=128k >> _tmpfile.arg
	echo system dos4g >> _tmpfile.arg
	echo name speed.exe >> _tmpfile.arg
	echo library alleg.lib >> _tmpfile.arg
	echo $(addprefix \nfile ,$(OBJS)) >> _tmpfile.arg
	wlink @_tmpfile.arg
	rm _tmpfile.arg

%.obj: %.c speed.h
	wcl386 -bt=dos4g -5s -s -zq -fo=$@ -c $<


else
ifdef USE_MSVC


#  MSVC version
#  ------------

OBJS = $(addsuffix .obj,$(OBJ))

speed.exe: $(OBJS)
	echo $(OBJS) > _tmpfile.arg
	link -nologo -out:speed.exe @_tmpfile.arg alleg.lib
	rm _tmpfile.arg

%.obj: %.c speed.h
	cl -nologo -Gd -Ox -GB -Fo$@ -c $<


else


#  gcc (Linux and djgpp) version
#  -----------------------------

ifdef DEBUGMODE

CFLAGS = -Wall -m486 -g
LFLAGS =
ALLEG = -lalld

else

CFLAGS = -Wall -m486 -O3 -ffast-math
LFLAGS = -s
ALLEG = -lalleg

endif

OBJS = $(addsuffix .o,$(OBJ))

speed: $(OBJS)
	gcc $(LFLAGS) -o speed $(OBJS) $(ALLEG)

%.o: %.c speed.h
	gcc $(CFLAGS) -o $@ -c $<


endif
endif


#  generic stuff
#  -------------


clean:
	rm -rvf speed speed.exe speed.zip speed.rec *.o *.obj core

zip: clean
	cd ..; zip -9 -r speed.zip speed/; mv speed.zip speed

run8: speed
	speed 640 480 8

run16: speed
	speed 640 480 16

run32: speed
	speed 640 480 32

runlo: speed
	speed 320 200 8

runhi: speed
	speed 1024 768 16

