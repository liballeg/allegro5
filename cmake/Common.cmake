function(add_our_library target)
    add_library(${target} ${ARGN})
    set_target_properties(${target}
        PROPERTIES
        DEBUG_POSTFIX -debug
        PROFILE_POSTFIX -profile
        )
    install(TARGETS ${target}
	    DESTINATION "lib${LIB_SUFFIX}"
	    )
endfunction(add_our_library)

function(add_our_executable nm)
    add_executable(${nm} ${ARGN})
    target_link_libraries(${nm} alleg)
endfunction()

# Oh my. CMake really is bad for this - but I couldn't find a better
# way.
function(sanitize_cmake_link_flags return)
   SET(acc_libs)
   foreach(lib ${ARGN})
      # Watch out for -framework options (OS X)
      IF (NOT lib MATCHES "-framework.*|.*framework")
         # Remove absolute path.
         string(REGEX REPLACE "/.*/(.*)" "\\1" lib ${lib})

         # Remove .a/.so/.dylib.
         string(REGEX REPLACE "lib(.*)\\.(a|so|dylib)" "\\1" lib ${lib})

         # Remove -l prefix if it's there already.
         string(REGEX REPLACE "-l(.*)" "\\1" lib ${lib})

        set(acc_libs "${acc_libs} -l${lib}")
      ENDIF()
   endforeach(lib)
   set(${return} ${acc_libs} PARENT_SCOPE)
endfunction(sanitize_cmake_link_flags)

function(copy_files target)
    if("${CMAKE_CURRENT_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
        return()
    endif()
    foreach(file ${ARGN})
        add_custom_command(
            OUTPUT  "${file}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
                    "${file}"
            )
    endforeach()
    add_custom_target(${target} ALL DEPENDS ${ARGN})
endfunction()

# vim: set sts=4 sw=4 et:
