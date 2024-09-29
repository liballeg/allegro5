# FOO_LINK_WITH and SUPPORT_FOO need to be propagated up to the parent scope so
# examples know if they can use the FOO addon, and what to link against.
# FOO_LINK_WITH should be the name of a single target.  The target should
# automatically pull in dependencies so they don't need to be listed.

include(FindPackageHandleStandardArgs)

set(ADDON_PKG_CONFIG_FILES)

set(ADDONS_CONFIGURATION_SUMMARY "")

function(addons_summary msg yesno)
    if(${yesno})
        set(yesno "yes")
    else()
        # No's are more important, so shout them.
        set(yesno "NO")
    endif()
    set(ADDONS_CONFIGURATION_SUMMARY "${ADDONS_CONFIGURATION_SUMMARY}${msg}: ${yesno}\n" PARENT_SCOPE)
endfunction()

if(WANT_PRIMITIVES)
    add_subdirectory(primitives)
    set(SUPPORT_PRIMITIVES 1)
    set(SUPPORT_PRIMITIVES 1 PARENT_SCOPE)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_primitives)
endif(WANT_PRIMITIVES)

addons_summary("- Primitives addon" SUPPORT_PRIMITIVES)

if(WANT_IMAGE)
    add_subdirectory(image)
    set(SUPPORT_IMAGE 1)
    set(SUPPORT_IMAGE 1 PARENT_SCOPE)
    set(IMAGE_LINK_WITH ${IMAGE_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_image)
endif(WANT_IMAGE)

addons_summary("- Image addon" SUPPORT_IMAGE)
set(ADDONS_CONFIGURATION_SUMMARY "${ADDONS_CONFIGURATION_SUMMARY}${IMAGE_CONFIGURATION_SUMMARY}")

if(WANT_FONT AND SUPPORT_IMAGE)
    add_subdirectory(font)
    set(SUPPORT_FONT 1)
    set(SUPPORT_FONT 1 PARENT_SCOPE)
    set(FONT_LINK_WITH ${FONT_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_font)
endif(WANT_FONT AND SUPPORT_IMAGE)

addons_summary("- Font addon" SUPPORT_FONT)

if(WANT_AUDIO)
    add_subdirectory(audio)
    if(SUPPORT_AUDIO)
        set(SUPPORT_AUDIO 1 PARENT_SCOPE)
        set(AUDIO_LINK_WITH ${AUDIO_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_audio)
    endif(SUPPORT_AUDIO)
endif(WANT_AUDIO)

addons_summary("- Audio addon" SUPPORT_AUDIO)
set(ADDONS_CONFIGURATION_SUMMARY "${ADDONS_CONFIGURATION_SUMMARY}${AUDIO_CONFIGURATION_SUMMARY}")

if(SUPPORT_AUDIO)
    add_subdirectory(acodec)
    if(SUPPORT_ACODEC)
        set(SUPPORT_ACODEC 1 PARENT_SCOPE)
        set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_acodec)
    endif(SUPPORT_ACODEC)
endif(SUPPORT_AUDIO)

addons_summary("- Acodec addon" SUPPORT_ACODEC)
set(ADDONS_CONFIGURATION_SUMMARY "${ADDONS_CONFIGURATION_SUMMARY}${ACODEC_CONFIGURATION_SUMMARY}")

if(SUPPORT_FONT AND WANT_TTF)
    find_package(Freetype)

    option(FREETYPE_ZLIB "Enable if FreeType static library needs zlib linked in." off)
    option(FREETYPE_BZIP2 "Enable if FreeType static library needs bzip2 linked in." off)
    option(FREETYPE_PNG "Enable if FreeType static library needs png linked in." off)
    option(FREETYPE_HARFBUZZ "Enable if FreeType static library needs harfbuzz linked in." off)
	
    if(FREETYPE_FOUND)
        set(FREETYPE_TEST_SOURCE "
            #include <ft2build.h>
            #include FT_FREETYPE_H
            int main(void) {
                FT_Library ft;
                FT_Open_Args args;
                FT_Face face;
                FT_Int32 ft_load_flags;
                FT_Error e;
                int ft_index;
                FT_Init_FreeType(&ft);
                FT_Open_Face(ft, &args, 0, &face);
                return FT_Load_Glyph(face, ft_index, ft_load_flags);
            }")

        set(TTF_INCLUDE_DIRECTORIES ${FREETYPE_INCLUDE_DIRS})

        set(CMAKE_REQUIRED_LIBRARIES ${FREETYPE_LIBRARIES})
        set(CMAKE_REQUIRED_INCLUDES ${FREETYPE_INCLUDE_DIRS})
        run_c_compile_test("${FREETYPE_TEST_SOURCE}" TTF_COMPILES)
        if(TTF_COMPILES)
            set(TTF_LIBRARIES ${FREETYPE_LIBRARIES})
        else()
            if(FREETYPE_ZLIB)
                find_package(ZLIB)
            endif()

            if(FREETYPE_BZIP2)
                find_package(BZip2)
            endif()

            if(FREETYPE_PNG)
                find_package(PNG)
            endif()
			
            if (FREETYPE_HARFBUZZ)
                find_package(HarfBuzz)
            endif()

            # Try compiling with the extra dependencies.
            set(FREETYPE_STATIC_LIBRARIES ${FREETYPE_LIBRARIES})
            if(FREETYPE_ZLIB AND ZLIB_FOUND)
                list(APPEND FREETYPE_STATIC_LIBRARIES "${ZLIB_LIBRARIES}")
            endif()

            if(FREETYPE_BZIP2 AND BZIP2_FOUND)
                list(APPEND FREETYPE_STATIC_LIBRARIES "${BZIP2_LIBRARIES}")
            endif()

            if(FREETYPE_PNG AND PNG_FOUND)
                list(APPEND FREETYPE_STATIC_LIBRARIES "${PNG_LIBRARIES}")
            endif()
			
            if (FREETYPE_HARFBUZZ AND HARFBUZZ_FOUND)
                # there's a circular dependency between Harfbuzz and Freetype, so one of them has to be
                # repeated to ensure proper static linking order
                list(APPEND FREETYPE_STATIC_LIBRARIES "${HARFBUZZ_LIBRARIES}" "${FREETYPE_LIBRARIES}")
            endif()

            set(CMAKE_REQUIRED_LIBRARIES ${FREETYPE_STATIC_LIBRARIES})
            set(CMAKE_REQUIRED_INCLUDES ${FREETYPE_INCLUDE_DIRS})
            run_c_compile_test("${FREETYPE_TEST_SOURCE}" TTF_COMPILES_WITH_EXTRA_DEPS)
            if(TTF_COMPILES_WITH_EXTRA_DEPS)
                set(TTF_COMPILES 1)
                set(TTF_LIBRARIES ${FREETYPE_STATIC_LIBRARIES})
            endif()
        endif()
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)

        if(TTF_COMPILES)
            add_subdirectory(ttf)
            set(SUPPORT_TTF 1 PARENT_SCOPE)
            set(TTF_LINK_WITH ${TTF_LINK_WITH} PARENT_SCOPE)
            list(APPEND ADDON_PKG_CONFIG_FILES allegro_ttf)
        else()
            message(WARNING "FreeType doesn't compile. Disabling support.")
        endif()
    else(FREETYPE_FOUND)
        message("WARNING: FreeType not found, disabling support.")
    endif(FREETYPE_FOUND)
endif(SUPPORT_FONT AND WANT_TTF)

addons_summary("- TTF addon" SUPPORT_TTF)

if(WANT_COLOR)
    add_subdirectory(color)
    set(SUPPORT_COLOR 1 PARENT_SCOPE)
    set(COLOR_LINK_WITH ${COLOR_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_color)
endif(WANT_COLOR)

addons_summary("- Color addon" SUPPORT_COLOR)

if(WANT_MEMFILE)
    add_subdirectory(memfile)
    set(SUPPORT_MEMFILE 1 PARENT_SCOPE)
    set(MEMFILE_LINK_WITH ${MEMFILE_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_memfile)
endif(WANT_MEMFILE)

addons_summary("- Memfile addon" SUPPORT_MEMFILE)

if(WANT_PHYSFS)
    find_package(PhysFS)
    # FindPhysFS.cmake in cmake-2.6.2 doesn't call this but it should.
    find_package_handle_standard_args(PHYSFS DEFAULT_MSG
        PHYSFS_LIBRARY PHYSFS_INCLUDE_DIR)
    mark_as_advanced(PHYSFS_INCLUDE_DIR)
    mark_as_advanced(PHYSFS_LIBRARY)

    find_package(ZLIB)

    # Does this copy of PhysicsFS require zlib to be linked separately?
    # FindPhysFS should really figure this out for us, but it doesn't.
    if(PHYSFS_FOUND)
        set(CMAKE_REQUIRED_INCLUDES ${PHYSFS_INCLUDE_DIR})
        set(CMAKE_REQUIRED_LIBRARIES ${PHYSFS_LIBRARY})
        run_c_compile_test("
            #include <physfs.h>

            int main(int argc, char **argv)
            {
                (void)argc;
                PHYSFS_init(argv[0]);
                PHYSFS_deinit();
                return 0;
            }"
            PHYSFS_IMPLICIT_ZLIB)
        set(CMAKE_REQUIRED_INCLUDES)
        set(CMAKE_REQUIRED_LIBRARIES)

        if(PHYSFS_IMPLICIT_ZLIB)
            set(PHYSFS_LIBRARIES ${PHYSFS_LIBRARY})
            set(SUPPORT_PHYSFS 1)
        elseif(ZLIB_FOUND)
            set(PHYSFS_LIBRARIES ${PHYSFS_LIBRARY} ${ZLIB_LIBRARY})
            set(SUPPORT_PHYSFS 1)
        else()
            message("WARNING: PhysicsFS needs zlib, zlib not found, disabling PhysFS support.")
        endif()
    endif(PHYSFS_FOUND)

    if(SUPPORT_PHYSFS)
        add_subdirectory(physfs)
        set(SUPPORT_PHYSFS 1 PARENT_SCOPE)
        set(PHYSFS_LINK_WITH ${PHYSFS_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_physfs)
    endif(SUPPORT_PHYSFS)
endif(WANT_PHYSFS)

addons_summary("- PhysFS addon" SUPPORT_PHYSFS)

if(WANT_NATIVE_DIALOG)
    add_subdirectory(native_dialog)
    if(SUPPORT_NATIVE_DIALOG)
        set(SUPPORT_NATIVE_DIALOG 1 PARENT_SCOPE)
        set(NATIVE_DIALOG_LINK_WITH ${NATIVE_DIALOG_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_dialog)
    endif()
endif(WANT_NATIVE_DIALOG)

addons_summary("- Native Dialog addon" SUPPORT_NATIVE_DIALOG)

if(WANT_VIDEO)
    add_subdirectory(video)
    if(SUPPORT_VIDEO)
        set(SUPPORT_VIDEO 1 PARENT_SCOPE)
        set(VIDEO_LINK_WITH ${VIDEO_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_video)
    endif(SUPPORT_VIDEO)
endif(WANT_VIDEO)

addons_summary("- Video addon" SUPPORT_VIDEO)

add_subdirectory(main)
# SUPPORT_MAIN and MAIN_LINK_WITH are spelt as such in this scope,
# but are prefixed with ALLEGRO in the parent scope.
set(SUPPORT_A5O_MAIN ${SUPPORT_MAIN} PARENT_SCOPE)
set(A5O_MAIN_LINK_WITH ${MAIN_LINK_WITH} PARENT_SCOPE)
list(APPEND ADDON_PKG_CONFIG_FILES allegro_main)

set(MONOLITH_SOURCES ${MONOLITH_SOURCES} PARENT_SCOPE)
set(MONOLITH_INCLUDE_DIRECTORIES ${MONOLITH_INCLUDE_DIRECTORIES} PARENT_SCOPE)
set(MONOLITH_LINK_DIRECTORIES ${MONOLITH_LINK_DIRECTORIES} PARENT_SCOPE)
set(MONOLITH_LIBRARIES ${MONOLITH_LIBRARIES} PARENT_SCOPE)
set(MONOLITH_HEADERS ${MONOLITH_HEADERS} PARENT_SCOPE)
set(MONOLITH_DEFINES ${MONOLITH_DEFINES} PARENT_SCOPE)

set(ADDON_PKG_CONFIG_FILES ${ADDON_PKG_CONFIG_FILES} PARENT_SCOPE)

set(CONFIGURATION_SUMMARY "${CONFIGURATION_SUMMARY}${ADDONS_CONFIGURATION_SUMMARY}" PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
