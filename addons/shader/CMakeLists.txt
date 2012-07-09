option(WANT_D3D_SHADERS "Build D3D shader support into the shader addon" on)
option(WANT_CG_SHADERS "Build Cg shader support into the shader addon" on)

set(SHADER_INCLUDE_FILES allegro5/allegro_shader.h)

set_our_header_properties(${SHADER_INCLUDE_FILES})

set(SHADER_SOURCES
    shader.c
    )

set(SHADER_DEFINES "-DALLEGRO_SHADER_SRC")

if(WANT_OPENGL)
    list(APPEND SHADER_SOURCES shader_glsl.c)
    list(APPEND SHADER_INCLUDE_FILES allegro5/allegro_shader_glsl.h)
    set_our_header_properties(${SHADER_INCLUDE_FILES})
    set(SHADER_DEFINES "${SHADER_DEFINES} -DALLEGRO_CFG_WANT_GLSL_SHADERS")
    set(ALLEGRO_CFG_GLSL_SHADERS 1 PARENT_SCOPE)
endif(WANT_OPENGL)

if(WANT_D3D AND WANT_D3D_SHADERS)
    if(WIN32 AND WANT_D3D)
        if(D3DX9_FOUND)
            list(APPEND SHADER_SOURCES shader_hlsl.cpp)
            list(APPEND SHADER_INCLUDE_FILES allegro5/allegro_shader_hlsl.h)
            set_our_header_properties(${SHADER_INCLUDE_FILES})
            set(SHADER_DEFINES "${SHADER_DEFINES} -DALLEGRO_CFG_WANT_HLSL_SHADERS")
            set(SHADER_LIBRARIES "${D3DX9_LIBRARY};${SHADER_LIBRARIES}")
            set(ALLEGRO_CFG_HLSL_SHADERS 1 PARENT_SCOPE)
        endif(D3DX9_FOUND)
    endif(WIN32 AND WANT_D3D)
endif(WANT_D3D AND WANT_D3D_SHADERS)

if(WANT_CG_SHADERS)
    find_package(Cg)
    if(CG_FOUND)
        list(APPEND SHADER_SOURCES shader_cg.cpp)
        set(SHADER_DEFINES "${SHADER_DEFINES} -DALLEGRO_CFG_WANT_CG_SHADERS")
        set(SHADER_LIBRARIES "${CG_LIBRARY};${SHADER_LIBRARIES}")
        if(WANT_OPENGL)
            set(SHADER_LIBRARIES "${CG_GL_LIBRARY};${SHADER_LIBRARIES}")
        endif(WANT_OPENGL)
        if(WANT_D3D)
            set(SHADER_LIBRARIES "${CG_D3D9_LIBRARY};${SHADER_LIBRARIES}")
        endif(WANT_D3D)

        include_directories(SYSTEM ${CG_INCLUDE_PATH})

        # This allows the monolith build to find the files.
        list(APPEND SHADER_INCLUDE_DIRECTORIES ${CG_INCLUDE_PATH})

        foreach(arg ${CG_CFLAGS})
            set(CG_CFLAGS_STRING "${CG_CFLAGS_STRING} ${arg}")
        endforeach(arg)

        set_source_files_properties(
            shader_cg.cpp
            PROPERTIES
            COMPILE_FLAGS "${CG_CFLAGS_STRING}"
            )
        set(ALLEGRO_CFG_CG_SHADERS 1 PARENT_SCOPE)
    endif(CG_FOUND)
endif(WANT_CG_SHADERS)

include_directories(.)

add_our_addon_library(allegro_shader
   "${SHADER_SOURCES};${SHADER_INCLUDE_FILES}"
   "${SHADER_DEFINES}"
   "${ALLEGRO_LINK_WITH};${SHADER_LIBRARIES}"
   )

set_our_framework_properties(allegro_shader AllegroShader-${ALLEGRO_SOVERSION})

install_our_headers(${SHADER_INCLUDE_FILES})

add_addon(shader)

# vim: set sts=4 sw=4 et:
