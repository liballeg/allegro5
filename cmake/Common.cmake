function(set_our_header_properties)
    foreach(file ${ARGN})
        # Infer which subdirectory this header file should be installed.
        set(loc ${file})
        string(REPLACE "${CMAKE_CURRENT_BINARY_DIR}/" "" loc ${loc})
        string(REGEX REPLACE "^include/" "" loc ${loc})
        string(REGEX REPLACE "/[-A-Za-z0-9_]+[.](h|inl)$" "" loc ${loc})
        string(REGEX REPLACE "^addons/[^/]+/" "" loc ${loc})

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

# Normally CMake caches the "failure" result of a compile test, preventing it
# from re-running. These helpers delete the cache variable on failure so that
# CMake will try again next time.
function(run_c_compile_test source var)
    check_c_source_compiles("${source}" ${var})
    if (NOT ${var})
        unset(${var} CACHE)
    endif(NOT ${var})
endfunction(run_c_compile_test)

function(run_cxx_compile_test source var)
    check_cxx_source_compiles("${source}" ${var})
    if (NOT ${var})
        unset(${var} CACHE)
    endif(NOT ${var})
endfunction(run_cxx_compile_test)

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
    set(return)
    foreach(lib ${ARGV})
        # Watch out for -framework options (OS X)
        if(NOT lib MATCHES "-framework.*" AND NOT lib MATCHES ".*framework")
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
            if(NOT lib MATCHES "allegro_.*" AND NOT lib STREQUAL "allegro" AND NOT lib STREQUAL "allegro_audio")
                set(return "${return} -l${lib}")
            endif()
        endif(NOT lib MATCHES "-framework.*" AND NOT lib MATCHES ".*framework")
    endforeach(lib)
    set(return ${return} PARENT_SCOPE)
endfunction(sanitize_cmake_link_flags)

function(add_our_library target framework_name sources extra_flags link_with)
    # BUILD_SHARED_LIBS controls whether this is a shared or static library.
    add_library(${target} ${sources})
    target_include_directories(${target} INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    list(APPEND ALLEGRO_TARGETS "${target}")
    list(REMOVE_DUPLICATES ALLEGRO_TARGETS)
    set(ALLEGRO_TARGETS "${ALLEGRO_TARGETS}" CACHE INTERNAL "internal")

    if(MSVC)
        # Compile with multiple processors
        set(extra_flags "${extra_flags} /MP")
        if(WANT_STATIC_RUNTIME)
            if(CMAKE_BUILD_TYPE STREQUAL Debug)
                set(extra_flags "${extra_flags} /MTd")
            else()
                set(extra_flags "${extra_flags} /MT")
            endif()
            if (NOT BUILD_SHARED_LIBS)
                # /Zl instructs MSVC to not mention the CRT used at all,
                # allowing the static libraries to be combined with any CRT version
                # in the final dll/exe.
                set(extra_flags "${extra_flags} /Zl")
            endif()
        endif()
    elseif(MINGW)
        if(WANT_STATIC_RUNTIME)
            # TODO: The -static is a bit of a hack for MSYS2 to force the static linking of pthreads.
            # There has to be a better way.
            set(extra_link_flags "-static-libgcc -static-libstdc++ -static -lpthread -Wl,--exclude-libs=libpthread.a -Wl,--exclude-libs=libgcc_eh.a")
        endif()
    endif()

    if(WIN32)
        set(extra_flags "${extra_flags} -DUNICODE -D_UNICODE")
    endif()

    if(NOT BUILD_SHARED_LIBS)
        set(static_flag "-DALLEGRO_STATICLINK")
    endif(NOT BUILD_SHARED_LIBS)

    if(NOT ANDROID)
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${extra_flags} ${static_flag} -DALLEGRO_LIB_BUILD"
            LINK_FLAGS "${extra_link_flags}"
            VERSION ${ALLEGRO_VERSION}
            SOVERSION ${ALLEGRO_SOVERSION}
            )
    else(NOT ANDROID)
        set_target_properties(${target}
            PROPERTIES
            COMPILE_FLAGS "${extra_flags} ${static_flag} -DALLEGRO_LIB_BUILD"
            )
        set_property(GLOBAL APPEND PROPERTY JNI_LIBS ${target})
    endif(NOT ANDROID)

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
            LINK_FLAGS "-undefined dynamic_lookup"
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
    set_our_framework_properties(${target} ${framework_name})
    install_our_library(${target} ${output_name})
endfunction(add_our_library)

macro(add_our_addon_library target framework_name sources extra_flags link_with)
    if(WANT_MONOLITH)
        set(MONOLITH_DEFINES "${MONOLITH_DEFINES} ${extra_flags}")
    else()
        add_our_library(${target} ${framework_name} "${sources}" "${extra_flags}" "${link_with}")
        target_include_directories(${target} INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
        if(ANDROID)
            record_android_load_libs(${target} "${link_with}")
        endif()
    endif()
endmacro(add_our_addon_library)

# Record in a custom target property 'ANDROID_LOAD_LIBS' the list of shared
# objects that will need to be bundled with the APK and loaded manually if
# linking with this target.
function(record_android_load_libs target libs)
    set(load_libs)
    foreach(lib ${libs})
        if(lib MATCHES "/lib[^/]+[.]so$" AND NOT lib MATCHES "/sysroot/")
            list(APPEND load_libs "${lib}")
        endif()
    endforeach()
    set_target_properties(${target} PROPERTIES ANDROID_LOAD_LIBS "${load_libs}")
endfunction(record_android_load_libs)

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

function(install_our_library target filename)
    install(TARGETS ${target}
            FRAMEWORK DESTINATION "${FRAMEWORK_INSTALL_PREFIX}"
            # Doesn't work, see below.
            # PUBLIC_HEADER DESTINATION "include"
            )
    if(MSVC AND BUILD_SHARED_LIBS)
        install(FILES ${CMAKE_BINARY_DIR}/lib/\${CMAKE_INSTALL_CONFIG_NAME}/${filename}.pdb
            DESTINATION ${CMAKE_INSTALL_LIBDIR}
            CONFIGURATIONS Debug RelWithDebInfo
        )
    endif()
endfunction(install_our_library)

# Unfortunately, CMake's PUBLIC_HEADER support doesn't install into nested
# directories well, otherwise we could rely on install(TARGETS) to install
# header files associated with the target.  Instead we use the install(FILES)
# to install headers.  We reuse the MACOSX_PACKAGE_LOCATION property,
# substituting the "Headers" prefix with "include".

# NOTE: modern design is to use `target_sources(FILE_SET)` which requires CMake 3.23
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

# Ads a target for an executable target `nm`.
#
# Arguments:
#
#    SRCS - Sources. If empty, assumes it to be ${nm}.c
#    LIBS - Libraries to link to.
#    DEFINES - Additional defines.
#
# Free variable: EXECUTABLE_TYPE
function(add_our_executable nm)
    set(flags) # none
    set(single_args) # none
    set(multi_args SRCS LIBS DEFINES)
    cmake_parse_arguments(OPTS "${flags}" "${single_args}" "${multi_args}"
        ${ARGN})

    if(NOT OPTS_SRCS)
        set(OPTS_SRCS "${nm}.c")
    endif()

    if(IPHONE)
        set(EXECUTABLE_TYPE MACOSX_BUNDLE)
        set(OPTS_SRCS ${OPTS_SRCS} "${CMAKE_SOURCE_DIR}/misc/icon.png")
    endif()

    add_executable(${nm} ${EXECUTABLE_TYPE} ${OPTS_SRCS})
    target_link_libraries(${nm} ${OPTS_LIBS})
    if(WANT_POPUP_EXAMPLES AND SUPPORT_NATIVE_DIALOG)
        list(APPEND OPTS_DEFINES ALLEGRO_POPUP_EXAMPLES)
    endif()
    if(NOT BUILD_SHARED_LIBS)
        list(APPEND OPTS_DEFINES ALLEGRO_STATICLINK)
    endif()

    foreach(d ${OPTS_DEFINES})
        set_property(TARGET ${nm} APPEND PROPERTY COMPILE_DEFINITIONS ${d})
    endforeach()

    set(extra_flags "")

    if(MSVC)
        # Compile with multiple processors
        set(extra_flags "/MP")
        if(WANT_STATIC_RUNTIME)
            if(CMAKE_BUILD_TYPE STREQUAL Debug)
                set(extra_flags "${extra_flags} /MTd")
            else()
                set(extra_flags "${extra_flags} /MT")
            endif()
        endif()
    endif()

    if(WIN32)
        set(extra_flags "${extra_flags} -DUNICODE -D_UNICODE")
    endif()

    set_target_properties(${nm} PROPERTIES COMPILE_FLAGS "${extra_flags}")

    if(MINGW)
        if(NOT CMAKE_BUILD_TYPE STREQUAL Debug)
            set_target_properties(${nm} PROPERTIES LINK_FLAGS "-Wl,-subsystem,windows")
        endif()
    endif()

    fix_executable(${nm})
endfunction()

function(add_copy_commands src dest destfilesvar)
    set(destfiles)
    foreach(basename ${ARGN})
        list(APPEND destfiles "${dest}/${basename}")
        add_custom_command(
            OUTPUT  "${dest}/${basename}"
            DEPENDS "${src}/${basename}"
            COMMAND "${CMAKE_COMMAND}" -E copy
                    "${src}/${basename}" "${dest}/${basename}"
            )
    endforeach()
    set(${destfilesvar} "${destfiles}" PARENT_SCOPE)
endfunction()

# Recreate data directory for out-of-source builds.
# Note: a symlink is unsafe as make clean will delete the contents
# of the pointed-to directory.
#
# Files are only copied if they don't are inside a .svn folder so we
# won't end up with read-only .svn folders in the build folder.
function(copy_data_dir_to_build target src dest)
    if(IPHONE)
        return()
    endif(IPHONE)

    if(src STREQUAL dest)
        return()
    endif()

    file(GLOB_RECURSE files RELATIVE "${src}" "${src}/*")
    add_copy_commands("${src}" "${dest}" destfiles "${files}")
    add_custom_target(${target} DEPENDS ${destfiles})
endfunction(copy_data_dir_to_build)

macro(add_monolith_sources var addon sources)
    foreach(s ${${sources}})
        list(APPEND ${var} ${addon}/${s})
    endforeach(s ${sources})
endmacro(add_monolith_sources addon sources)

# This macro is called by each addon. It expects the following variables to
# exist:
#
# ${ADDON}_SOURCES
# ${ADDON}_INCLUDE_FILES
# ${ADDON}_INCLUDE_DIRECTORIES
# ${ADDON}_LINK_DIRECTORIES
# ${ADDON}_DEFINES
# ${ADDON}_LIBRARIES
#
# This is useful so we can build the monolith library without having any other
# special code for it in the addon CMakeLists.txt files.
macro(add_addon addon)
    add_addon2(${addon} allegro_${addon})
endmacro(add_addon)

macro(add_addon2 addon addon_target)
    string(TOUPPER ${addon} ADDON)
    set(SUPPORT_${ADDON} 1 PARENT_SCOPE)
    set(${ADDON}_LINK_WITH ${addon_target} PARENT_SCOPE)
    add_monolith_sources(MONOLITH_SOURCES addons/${addon} ${ADDON}_SOURCES)
    add_monolith_sources(MONOLITH_SOURCES addons/${addon} ${ADDON}_INCLUDE_FILES)
    add_monolith_sources(MONOLITH_HEADERS addons/${addon} ${ADDON}_INCLUDE_FILES)
    set(MONOLITH_SOURCES ${MONOLITH_SOURCES} PARENT_SCOPE)
    list(APPEND MONOLITH_INCLUDE_DIRECTORIES ${${ADDON}_INCLUDE_DIRECTORIES})
    list(APPEND MONOLITH_INCLUDE_DIRECTORIES addons/${addon})
    list(APPEND MONOLITH_LINK_DIRECTORIES ${${ADDON}_LINK_DIRECTORIES})
    list(APPEND MONOLITH_LIBRARIES ${${ADDON}_LIBRARIES})
    set(MONOLITH_DEFINES "${MONOLITH_DEFINES} ${${ADDON}_DEFINES}")
    set(MONOLITH_INCLUDE_DIRECTORIES ${MONOLITH_INCLUDE_DIRECTORIES} PARENT_SCOPE)
    set(MONOLITH_LINK_DIRECTORIES ${MONOLITH_LINK_DIRECTORIES} PARENT_SCOPE)
    set(MONOLITH_LIBRARIES ${MONOLITH_LIBRARIES} PARENT_SCOPE)
    set(MONOLITH_HEADERS ${MONOLITH_HEADERS} PARENT_SCOPE)
    set(MONOLITH_DEFINES ${MONOLITH_DEFINES} PARENT_SCOPE)
endmacro(add_addon2)

#-----------------------------------------------------------------------------#
# vim: set ft=cmake sts=4 sw=4 et:
