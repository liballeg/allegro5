# testing capabilities.

PLATFORM_DIR_U = $(subst \,/,$(PLATFORM_DIR))
PLATFORM_DIR_D = $(subst /,\,$(PLATFORM_DIR))

.PHONY: init-asmtests test-mmx test-sse test-cpp

init-asmtests:
# we can't use C-style comments here because they may be expanded by echo and
# we need to write something in the file in order to make sure it is created.
ifdef UNIX_TOOLS
	echo "#define ALLEGRO_GENERATED_BY_MAKEFILE_TST" > $(PLATFORM_DIR_U)/asmcapa.h
else
	echo #define ALLEGRO_GENERATED_BY_MAKEFILE_TST > $(PLATFORM_DIR_D)\asmcapa.h
endif

test-mmx:
	as --defsym ASMCAPA_MMX_TEST=1 -o $(PLATFORM_DIR_U)/asmcapa$(OBJ) src/misc/asmcapa.s
ifdef UNIX_TOOLS
	echo "#define ALLEGRO_MMX" >> $(PLATFORM_DIR_U)/asmcapa.h
else
	echo #define ALLEGRO_MMX >> $(PLATFORM_DIR_D)\asmcapa.h
endif

test-sse:
	as --defsym ASMCAPA_SSE_TEST=1 -o $(PLATFORM_DIR_U)/asmcapa$(OBJ) src/misc/asmcapa.s
ifdef UNIX_TOOLS
	echo "#define ALLEGRO_SSE" >> $(PLATFORM_DIR_U)/asmcapa.h
else
	echo #define ALLEGRO_SSE >> $(PLATFORM_DIR_D)\asmcapa.h
endif

$(PLATFORM_DIR)/asmcapa.h:
	@echo Testing assembler capabilities...
	-$(MAKE) init-asmtests
	-$(MAKE) test-mmx
	-$(MAKE) test-sse

test-cpp:
	$(TEST_CPP)
ifdef UNIX_TOOLS
	echo "." > $(PLATFORM_DIR_U)/cpp-yes
else
	echo . > $(PLATFORM_DIR_D)\cpp-yes
endif

$(PLATFORM_DIR)/cpp-tested:
	@echo Testing for the presence of a C++ compiler...
	-$(MAKE) test-cpp
ifdef UNIX_TOOLS
	echo "." > $(PLATFORM_DIR_U)/cpp-tested
else
	echo . > $(PLATFORM_DIR_D)\cpp-tested
endif
