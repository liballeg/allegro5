# Check if constructors are supported.
#  CHECK_CTORS(VARIABLE)
#  VARIABLE - variable to store the result to
#

macro(CHECK_CTORS VAR)
    if("${VAR}" MATCHES "^${VAR}$")
        file(WRITE ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/src.c
            "
                static int notsupported = 1;
                void test_ctor (void) __attribute__ ((constructor));
                void test_ctor (void) { notsupported = 0; }
                int main (void) { return (notsupported); }
            ")
        if(CMAKE_CROSSCOMPILING)
            # Just assume constructors are not supported.  Actually, the mingw
            # port unconditionally sets ALLEGRO_USE_CONSTRUCTOR, so the outcome
            # of this test is immaterial for cross-compiling with mingw.
            set(${VAR} 0 CACHE INTERNAL "Constructors supported")
        else(CMAKE_CROSSCOMPILING)
            try_run(_ctor_runresult _ctor_compileresult
                ${CMAKE_BINARY_DIR}
                ${CMAKE_BINARY_DIR}/CMakeFiles/CMakeTmp/src.c)
            if(_ctor_runresult EQUAL 0)
                set(${VAR} 1 CACHE INTERNAL "Constructors supported")
                message(STATUS "Check if constructors are supported - yes")
            else(_ctor_runresult EQUAL 0)
                set(${VAR} 0 CACHE INTERNAL "Constructors supported")
                message(STATUS "Check if constructors are supported - no")
            endif(_ctor_runresult EQUAL 0)
        endif(CMAKE_CROSSCOMPILING)
    endif("${VAR}" MATCHES "^${VAR}$")
endmacro(CHECK_CTORS)
