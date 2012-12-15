# FOO_LINK_WITH and SUPPORT_FOO need to be propagated up to the parent scope so
# examples know if they can use the FOO addon, and what to link against.
# FOO_LINK_WITH should be the name of a single target.  The target should
# automatically pull in dependencies so they don't need to be listed.

include(FindPackageHandleStandardArgs)

set(ADDON_PKG_CONFIG_FILES)

if(WANT_PRIMITIVES)
    add_subdirectory(primitives)
    set(SUPPORT_PRIMITIVES 1)
    set(SUPPORT_PRIMITIVES 1 PARENT_SCOPE)
    set(PRIMITIVES_LINK_WITH ${PRIMITIVES_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_primitives)
endif(WANT_PRIMITIVES)

if(WANT_IMAGE)
    add_subdirectory(image)
    set(SUPPORT_IMAGE 1)
    set(SUPPORT_IMAGE 1 PARENT_SCOPE)
    set(IMAGE_LINK_WITH ${IMAGE_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_image)
endif(WANT_IMAGE)

if(WANT_FONT AND SUPPORT_IMAGE)
    add_subdirectory(font)
    set(SUPPORT_FONT 1)
    set(SUPPORT_FONT 1 PARENT_SCOPE)
    set(FONT_LINK_WITH ${FONT_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_font)
endif(WANT_FONT AND SUPPORT_IMAGE)

if(WANT_AUDIO)
    add_subdirectory(audio)
    if(SUPPORT_AUDIO)
        set(SUPPORT_AUDIO 1 PARENT_SCOPE)
        set(AUDIO_LINK_WITH ${AUDIO_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_audio)
    endif(SUPPORT_AUDIO)
endif(WANT_AUDIO)

if(SUPPORT_AUDIO)
    add_subdirectory(acodec)
    if(SUPPORT_ACODEC)
        set(SUPPORT_ACODEC 1 PARENT_SCOPE)
        set(ACODEC_LINK_WITH ${ACODEC_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_acodec)
    endif(SUPPORT_ACODEC)
endif(SUPPORT_AUDIO)

if(SUPPORT_FONT AND WANT_TTF)
    find_package(Freetype)
    if(FREETYPE_FOUND)
        add_subdirectory(ttf)
        set(SUPPORT_TTF 1 PARENT_SCOPE)
        set(TTF_LINK_WITH ${TTF_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_ttf)
    else(FREETYPE_FOUND)
        message("WARNING: FreeType not found, disabling support.")
    endif(FREETYPE_FOUND)
endif(SUPPORT_FONT AND WANT_TTF)

if(WANT_COLOR)
    add_subdirectory(color)
    set(SUPPORT_COLOR 1 PARENT_SCOPE)
    set(COLOR_LINK_WITH ${COLOR_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_color)
endif(WANT_COLOR)

if(WANT_MEMFILE)
    add_subdirectory(memfile)
    set(SUPPORT_MEMFILE 1 PARENT_SCOPE)
    set(MEMFILE_LINK_WITH ${MEMFILE_LINK_WITH} PARENT_SCOPE)
    list(APPEND ADDON_PKG_CONFIG_FILES allegro_memfile)
endif(WANT_MEMFILE)

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
        check_c_source_compiles("
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

if(WANT_NATIVE_DIALOG)
    add_subdirectory(native_dialog)
    if(SUPPORT_NATIVE_DIALOG)
        set(SUPPORT_NATIVE_DIALOG 1 PARENT_SCOPE)
        set(NATIVE_DIALOG_LINK_WITH ${NATIVE_DIALOG_LINK_WITH} PARENT_SCOPE)
        list(APPEND ADDON_PKG_CONFIG_FILES allegro_dialog)
    endif()
endif(WANT_NATIVE_DIALOG)

add_subdirectory(main)
set(SUPPORT_ALLEGRO_MAIN ${SUPPORT_ALLEGRO_MAIN} PARENT_SCOPE)
set(ALLEGRO_MAIN_LINK_WITH ${ALLEGRO_MAIN_LINK_WITH} PARENT_SCOPE)
list(APPEND ADDON_PKG_CONFIG_FILES allegro_main)

set(ADDON_PKG_CONFIG_FILES ${ADDON_PKG_CONFIG_FILES} PARENT_SCOPE)

#-----------------------------------------------------------------------------#
# vi: set ts=8 sts=4 sw=4 et:
