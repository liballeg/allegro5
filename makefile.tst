# testing assembler capabilities.

ASMCAPA_DIR_U = $(subst \,/,$(PLATFORM_DIR))
ASMCAPA_DIR_D = $(subst /,\,$(PLATFORM_DIR))

.PHONY: mmxtest ssetest cleantest

cleantest:
# we can't use C-style comments here because they may be expanded by echo and
# we need to write something in the file in order to make sure it is created.
ifdef UNIX_TOOLS
	echo "#define ALLEGRO_GENERATED_BY_MAKEFILE_TST" > $(ASMCAPA_DIR_U)/asmcapa.h
else
	echo #define ALLEGRO_GENERATED_BY_MAKEFILE_TST > $(ASMCAPA_DIR_D)\asmcapa.h
endif

mmxtest:
	gcc -DASMCAPA_MMX_TEST -assembler-with-cpp -c src/misc/asmcapa.s -o $(ASMCAPA_DIR_U)/asmcapa$(OBJ)
ifdef UNIX_TOOLS
	echo "#define ALLEGRO_MMX" >> $(ASMCAPA_DIR_U)/asmcapa.h
else
	echo #define ALLEGRO_MMX >> $(ASMCAPA_DIR_D)\asmcapa.h
endif

ssetest:
	gcc -DASMCAPA_SSE_TEST -assembler-with-cpp -c src/misc/asmcapa.s -o $(ASMCAPA_DIR_U)/asmcapa$(OBJ)
ifdef UNIX_TOOLS
	echo "#define ALLEGRO_SSE" >> $(ASMCAPA_DIR_U)/asmcapa.h
else
	echo #define ALLEGRO_SSE >> $(ASMCAPA_DIR_D)\asmcapa.h
endif

$(ASMCAPA_DIR_U)/asmcapa.h:
	@echo Testing assembler capabilities...
	-$(MAKE) cleantest
	-$(MAKE) mmxtest
	-$(MAKE) ssetest
