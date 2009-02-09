/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Header file for Direct3D specific API.
 *
 *      By Milan Mimica.
 *
 */

#ifndef ALLEGRO_DIRECT3D_H
#define ALLEGRO_DIRECT3D_H

#ifndef SCAN_DEPEND
#include <d3d9.h>
#endif

#ifdef __cplusplus
   extern "C" {
#endif

/*
 *  Public Direct3D-related API
 */

/* Display creation flag. */
#define ALLEGRO_DIRECT3D     8


AL_FUNC(LPDIRECT3DDEVICE9,  al_d3d_get_device,         (ALLEGRO_DISPLAY *));
AL_FUNC(HWND,               al_d3d_get_hwnd,           (ALLEGRO_DISPLAY *));
AL_FUNC(LPDIRECT3DTEXTURE9, al_d3d_get_system_texture, (ALLEGRO_BITMAP *));
AL_FUNC(LPDIRECT3DTEXTURE9, al_d3d_get_video_texture,  (ALLEGRO_BITMAP *));
AL_FUNC(bool,               al_d3d_supports_non_pow2_textures,   (void));
AL_FUNC(bool,               al_d3d_supports_non_square_textures, (void));


#ifdef __cplusplus
   }
#endif

#endif

/* vim: set ts=8 sts=3 sw=3 et: */
