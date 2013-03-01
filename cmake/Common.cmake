function(set_our_header_properties)
    foreach(file ${ARGN})
        # Infer which subdirectory this header file should be installed.
        set(loc ${file})
        string(REPLACE "${CMAKE_CURRENT_BINARY_DIR}/" "" loc ${loc})
        string(REGEX REPLACE "^include/" "" loc ${loc})
        string(REGEX REPLACE "/[-A-Za-z0-9_]+[.](h|inl)$" "" loc ${loc})

        # If we have inferred correctly then it should be under allegro5.
        string(REGEX MATCH "^allegro5" matched ${loc})
        if(matched STREQUAL "allegro5")
            # MACOSX_PACKAGE_LOCATION is also used in install_our_headers.
            set_source_files_properties(${file}
                PROPERTIES
                MACOSX_PACKAGE_LOCATION Headers/${loc}
                )
        else()
            message(FATAL_ERROR "Could not infer where to install ${file}")
        endif()
    endforeach(file)
endfunction(set_our_header_properties)

function(append_lib_type_suffix var)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_TOLOWER)
    if(CMAKE_BUILD_TYPE_TOLOWER STREQUAL "debug")
        set(${var} "${${var}}-debug" PARENT_SCOPE)
    endif(CMAKE_BUILD_TYPE_TOLOWER STREQUAL "debug")
    if(CMAKE_BUILD_TYPE_TOLOWER MATCHES "profile")
        set(${var} "${${var}}-profile" PARENT_SCOPE)
    endif(CMAKE_BUILD_TYPE_TOLOWER MATCHES "profile")
endfunction(append_lib_type_suffix)

function(append_lib_linkage_suffix var)
    if(NOT BUILD_SHARED_LIBS)
        set(${var} "${${var}}-static" PARENT_SCOPE)
    endif(NOT BUILD_SHARED_LIBS)
endfunction(append_lib_linkage_suffix)

# Oh my. CMake really is bad for this - but I couldn't find a better
# way.
function(sanitize_cmake_link_flags ...)
   SET(return)
   foreach(lib ${ARGV})
      # Watch out for -framework options (OS X)
      IF (NOT lib MATCHES "-framework.*" AND NOT lib MATCHES ".*framework")
         # Remove absolute path.
         string(REGEX REPLACE "/.*/(.*)" "\\1" lib ${lib})
   
         # Remove .a/.so/.dylib.
         string(REGEX REPLACE "lib(.*)\\.a" "\\1" lib ${lib})
         string(REGEX REPLACE "lib(.*)\\.so" "\\1" lib ${lib})
         string(REGEX REPLACE "lib(.*)\\.dylib" "\\1" lib ${lib})

         # Remove -l prefix if it's there already.
         string(REGEX REPLACE "-l(.*)" "\\1" lib ${lib})
         
         # Make sure we don't include our own libraries.
         # FIXME: Use a global list instead of a very unstable regexp.
         IF(NOT lib MATCHES "allegro_.*" AND NOT lib STREQUAL "allegro" AND NOT lib STREQUAL "allegro_audio")
            set(return "${return} -l${lib}")
         ENDIF()
      ENDIF(NOT lib MATCHES "-framework.*" AND NOT lib MATCHES ".*framework")  
   endforeach(lib)
   set(return ${return} PARENT_SCOPE)
endfunction(sanitize_cmake_link_flags)

function(add_our_library target sources extra_flags link_with)
    # BUILD_SHARED_LIBS controls whether this is a shared or static library.
    add_library(${target} ${sources})

    if(NOT BUILD_SHARED_LIBS)
        set(static_flag "-DALLEGRO_STATICLINK")
    endif(NOT BUILD_SHARED_LIBS)
    set_target_properties(${target}
        PROPERTIES
        COMPILE_FLAGS "${extra_flags} ${static_flag} -DALLEGRO_LIB_BUILD"
        VERSION ${ALLEGRO_VERSION}
        SOVERSION ${ALLEGRO_SOVERSION}
        )

    # Construct the output name.
    set(output_name ${target})
    append_lib_type_suffix(output_name)
    append_lib_linkage_suffix(output_name)
    set_target_properties(${target}
        PROPERTIES
        OUTPUT_NAME ${output_name}
        )

    # Put version numbers on DLLs but not on import libraries nor static
    # archives.  Make MinGW not add a lib prefix to DLLs, to match MSVC.
    if(WIN32 AND SHARED)
        set_target_properties(${target}
            PROPERTIES
            PREFIX ""
            SUFFIX -${ALLEGRO_SOVERSION}.dll
            IMPORT_SUFFIX ${CMAKE_IMPORT_LIBRARY_SUFFIX}
            )
    endif()

    # Suppress errors about _mangled_main_address being undefined on Mac OS X.
    if(MACOSX)
        set_target_properties(${target}
            PROPERTIES
            LINK_FLAGS "-flat_namespace -undefined suppress"
            )
    endif(MACOSX)

    # Specify a list of libraries to be linked into the specified target.
    # Library dependencies are transitive by default.  Any target which links
    # with this target will therefore pull in these dependencies automatically.
    target_link_libraries(${target} ${link_with})

    # Set list of dependencies that the user would need to explicitly link with
    # if static linking.
    sanitize_cmake_link_flags(${link_with})
    set_target_properties(${target}
        PROPERTIES
        static_link_with "${return}"
        )
endfunction(add_our_library)

function(set_our_framework_properties target nm)
    if(WANT_FRAMEWORKS)
        if(WANT_EMBED)
            set(install_name_dir "@executable_path/../Frameworks")
        else()
            set(install_name_dir "${FRAMEWORK_INSTALL_PREFIX}")
        endif(WANT_EMBED)
        set_target_properties(${target}
            PROPERTIES
            FRAMEWORK on
            OUTPUT_NAME ${nm}
            INSTALL_NAME_DIR "${install_name_dir}"
            )
    endif(WANT_FRAMEWORKS)
endfunction(set_our_framework_properties)

function(install_our_library target)
    install(TARGETS ${target}
            LIBRARY DESTINATION "lib${LIB_SUFFIX}"
            ARCHIVE DESTINATION "lib${LIB_SUFFIX}"
            FRAMEWORK DESTINATION "${FRAMEWORK_INSTALL_PREFIX}"
            RUNTIME DESTINATION "bin"
            # Doesn't work, see below.
            # PUBLIC_HEADER DESTINATION "include"
            )
endfunction(install_our_library)

# Unfortunately, CMake's PUBLIC_HEADER support doesn't install into nested
# directories well, otherwise we could rely on install(TARGETS) to install
# header files associated with the target.  Instead we use the install(FILES)
# to install headers.  We reuse the MACOSX_PACKAGE_LOCATION property,
# substituting the "Headers" prefix with "include".
function(install_our_headers)
    if(NOT WANT_FRAMEWORKS)
        foreach(hdr ${ARGN})
            get_source_file_property(LOC ${hdr} MACOSX_PACKAGE_LOCATION)
            string(REGEX REPLACE "^Headers" "include" LOC ${LOC})
            install(FILES ${hdr} DESTINATION ${LOC})
        endforeach()
    endif()
endfunction(install_our_headers)

function(fix_executable nm)
    if(IPHONE)
        string(REPLACE "_" "" bundle_nm ${nm})
        set_target_properties(${nm} PROPERTIES MACOSX_BUNDLE_GUI_IDENTIFIER "org.liballeg.${bundle_nm}")

        # FIXME:We want those as project attributes, not target attributes, but I don't know how
        set_target_properties(${nm} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
        
        # We have to add an icon to every executable on IPhone else
        # cmake won't create a resource copy build phase for us.
        # And re-creating those by hand would be a major pain.
        set_target_properties(${nm} PROPERTIES MACOSX_BUNDLE_ICON_FILE icon.png)
        
        set_source_files_properties("${CMAKE_SOURCE_DIR}/misc/icon.png" PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources"
        )

    endif(IPHONE)
endfunction(fix_executable)

# Arguments after nm should be source files, libraries, or defines (-D).
# Source files must end with .c or .cpp.  If no source file was explicitly
# specified, we assume an implied C source file.
# 
# Free variable: EXECUTABLE_TYPE
#
function(add_our_executable nm)
    set(srcs)
    set(libs)
    set(defines)
    set(regex "[.](c|cpp)$")
    set(regexd "^-D")
    foreach(arg ${ARGN})
        if("${arg}" MATCHES "${regex}")
            list(APPEND srcs ${arg})
        else("${arg}" MATCHES "${regex}")
            if ("${arg}" MATCHES "${regexd}")
                string(REGEX REPLACE "${regexd}" "" arg "${arg}")
                list(APPEND defines ${arg})                
            else("${arg}" MATCHES "${regexd}")
                list(APPEND libs ${arg})
            endif("${arg}" MATCHES "${regexd}")
        endif("${arg}" MATCHES "${regex}")
    endforeach(arg ${ARGN})

    if(NOT srcs)
        set(srcs "${nm}.c")
    endif(NOT srcs)
    
    if(IPHONE)
    set(EXECUTABLE_TYPE MACOSX_BUNDLE)
    set(srcs ${srcs} "${CMAKE_SOURCE_DIR}/misc/icon.png")
    endif(IPHONE)

    add_executable(${nm} ${EXECUTABLE_TYPE} ${srcs})
    target_link_libraries(${nm} ${libs})
    if(WANT_POPUP_EXAMPLES AND SUPPORT_NATIVE_DIALOG)
        list(APPEND defines ALLEGRO_POPUP_EXAMPLES)                        
    endif()
    if(NOT BUILD_SHARED_LIBS)
        list(APPEND defines ALLEGRO_STATICLINK)                        
    endif(NOT BUILD_SHARED_LIBS)
    
    foreach(d ${defines})
        set_property(TARGET ${nm} APPEND PROPERTY COMPILE_DEFINITIONS ${d})
    endforeach(d ${defines})

    if(MINGW)
        if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
            set_target_properties(${nm} PROPERTIES LINK_FLAGS "-Wl,-subsystem,windows")
        endif(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
   endif(MINGW)
   
   fix_executable(${nm})
endfunction(add_our_executable)

# Recreate data directory for out-of-source builds.
# Note: a symlink is unsafe as make clean will delete the contents
# of the pointed-to directory.
#
# Files are only copied if they don't are inside a .svn folder so we
# won't end up with read-only .svn folders in the build folder.
function(copy_data_dir_to_build target name destination)
    if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
        return()
    endif()

    file(GLOB_RECURSE allfiles RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/${name}/*)

    set(files)
    # Filter out files inside .svn folders.
    foreach(file ${allfiles})
        string(REGEX MATCH .*\\.svn.* is_svn ${file})
        if("${is_svn}" STREQUAL "")
            list(APPEND files ${file})
        endif()
    endforeach(file)
    
    add_custom_target(${target} ALL DEPENDS ${files})

    foreach(file ${files})
        add_custom_command(
            OUTPUT "${destination}/${file}"
            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${file}"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${CMAKE_CURRENT_SOURCE_DIR}/${file}" ${file}
            )
    endforeach(file)
endfunction(copy_data_dir_to_build)

#-----------------------------------------------------------------------------#
# vim: set ft=cmake sts=4 sw=4 et:
