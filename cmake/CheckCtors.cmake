# Check if constructors are supported.
#  CHECK_CTORS(VARIABLE)
#  VARIABLE - variable to store the result to
#

MACRO(CHECK_CTORS VAR)
  IF("${VAR}" MATCHES "^${VAR}$")
    FILE(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/src.c"
      "
	static int notsupported = 1;
	void test_ctor (void) __attribute__ ((constructor));
	void test_ctor (void) { notsupported = 0; }
	int main (void) { return (notsupported); }
      ")
    TRY_RUN(_ctor_runresult
      _ctor_compileresult
      "${CMAKE_BINARY_DIR}"
      "${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/src.c"
      )
    IF(_ctor_runresult EQUAL 0)
      SET(${VAR} 1 CACHE INTERNAL "Constructors supported")
      MESSAGE(STATUS "Check if constructors are supported - yes")
    ELSE(_ctor_runresult EQUAL 0)
      SET(${VAR} 0 CACHE INTERNAL "Constructors supported")
      MESSAGE("Check if constructors are supported - no")
    ENDIF(_ctor_runresult EQUAL 0)
  ENDIF("${VAR}" MATCHES "^${VAR}$")
ENDMACRO(CHECK_CTORS)
