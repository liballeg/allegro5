#include "d3d.h"

static const int _16BIT_DS = 2; /* # 16 bit depth stencil formats */
static const int _32BIT_DS = 4; /* # 32 bit depth stencil formats */

/*
 * This is a list of all supported display modes. Some other information will be
 * filled in later like multisample type and samples. This is for display scoring.
 */
// +2 for friendly mode and d3d depth/stencil format
static int d3d_fmt_desc[][ALLEGRO_DISPLAY_OPTIONS_COUNT+2] =
{
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, 0 },
   //{ 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D24X4S4 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, 0 },
   //{ 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 11, D3DFMT_D24X4S4 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, 0 },
   //{ 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D24X4S4 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, 0 },
   //{ 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 11, D3DFMT_D24X4S4 },
   //
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, 0 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D24X4S4 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, 0 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 11, D3DFMT_D24X4S4 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, 0 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D24X4S4 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32,  0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, 0 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 32, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D32 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 8, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D24S8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D24X8 },
   { 8, 8, 8, 0, 16, 8, 0,  0, 0, 0, 0, 0, 0, 0, 32, 24, 4, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 11, D3DFMT_D24X4S4 },


   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D24X4S4 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 14, D3DFMT_D24X4S4 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D24X4S4 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 1, 0, 0, 0, 0, 1, 0, 1, 1, 0, 14, D3DFMT_D24X4S4 },
   //
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D24X4S4 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1, 14, D3DFMT_D24X4S4 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D24X4S4 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16,  0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, 0 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 15, 1, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D15S1 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 16, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D16 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 32, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D32 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 8, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D24S8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D24X8 },
   { 5, 6, 5, 0, 11, 5, 0,  0, 0, 0, 0, 0, 0, 0, 16, 24, 4, 1, 0, 0, 0, 0, 1, 0, 1, 1, 1, 14, D3DFMT_D24X4S4 }
};

static const int D3D_DISPLAY_COMBINATIONS = sizeof(d3d_fmt_desc) / sizeof(*d3d_fmt_desc);


static ALLEGRO_EXTRA_DISPLAY_SETTINGS **eds_list = NULL;
static int eds_list_count = 0;

void _al_d3d_destroy_display_format_list(void)
{
   if (!eds_list)
      return;

   /* Free the display format list */
   for (int j = 0; j < eds_list_count; j++) {
      al_free(eds_list[j]);
   }
   al_free(eds_list);
   eds_list = 0;
   eds_list_count = 0;
}

void _al_d3d_generate_display_format_list(void)
{
   static bool fullscreen = !(al_get_new_display_flags() & ALLEGRO_FULLSCREEN); /* stop warning */
   static int adapter = ~al_get_new_display_adapter(); /* stop warning */
   int i;

   if ((eds_list != NULL) && (fullscreen == (bool)(al_get_new_display_flags() & ALLEGRO_FULLSCREEN))
         && (adapter == al_get_new_display_adapter())) {
      return;
   }
   else if (eds_list != NULL) {
     _al_d3d_destroy_display_format_list();
   }

   // Create display format list
   int n = _al_d3d_num_display_formats();
   DWORD quality_levels[n];
   eds_list_count = D3D_DISPLAY_COMBINATIONS;
   int count = 0;

   fullscreen = (al_get_new_display_flags() & ALLEGRO_FULLSCREEN) != 0;
   adapter = al_get_new_display_adapter();
   if (adapter < 0)
      adapter = 0;

   for (i = 0; i < n; i++) {
      quality_levels[i] = 0;
      int allegro_format_i;
      D3DFORMAT d3d_format_i;
      _al_d3d_get_nth_format(i, &allegro_format_i, &d3d_format_i);
      if (allegro_format_i < 0)
         break;
      if (_al_pixel_format_is_real(allegro_format_i) && !_al_format_has_alpha(allegro_format_i)) {
         if (_al_d3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, d3d_format_i,
            !fullscreen, D3DMULTISAMPLE_NONMASKABLE, &quality_levels[count]) != D3D_OK) {
            _al_d3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_REF, d3d_format_i,
               !fullscreen, D3DMULTISAMPLE_NONMASKABLE, &quality_levels[count]);
         }
         if (quality_levels[count] > 0) {
            if (al_get_pixel_size(allegro_format_i) == 4) {
               eds_list_count += (quality_levels[count]-1) * (_32BIT_DS+1) * 4; /* +1 for no DepthStencil */
            }
            else {
               eds_list_count += (quality_levels[count]-1) * (_16BIT_DS+_32BIT_DS+1) * 4;
            }
         }
         count++;
      }
   }

   eds_list = (ALLEGRO_EXTRA_DISPLAY_SETTINGS **)al_malloc(
      eds_list_count * sizeof(*eds_list)
   );
   memset(eds_list, 0, eds_list_count * sizeof(*eds_list));
   for (i = 0; i < eds_list_count; i++) {
      eds_list[i] = (ALLEGRO_EXTRA_DISPLAY_SETTINGS *)al_malloc(sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
      memset(eds_list[i], 0, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS));
   }

   count = 0;

   int fmt_num = 0;
   int curr_fmt = d3d_fmt_desc[0][ALLEGRO_DISPLAY_OPTIONS_COUNT];

   for (i = 0; i < D3D_DISPLAY_COMBINATIONS; i++) {
      if (d3d_fmt_desc[i][ALLEGRO_SAMPLE_BUFFERS]) {
         for (int k = 0; k < (int)quality_levels[fmt_num]; k++) {
            memcpy(eds_list[count]->settings, &d3d_fmt_desc[i], sizeof(int)*ALLEGRO_DISPLAY_OPTIONS_COUNT);
            eds_list[count]->settings[ALLEGRO_SAMPLES] = k;
            count++;
         }
      }
      else {
         memcpy(eds_list[count]->settings, &d3d_fmt_desc[i], sizeof(int)*ALLEGRO_DISPLAY_OPTIONS_COUNT);
         count++;
      }
      if (d3d_fmt_desc[i][ALLEGRO_DISPLAY_OPTIONS_COUNT] != curr_fmt) {
         curr_fmt = d3d_fmt_desc[i][ALLEGRO_DISPLAY_OPTIONS_COUNT];
         fmt_num++;
      }
   }
}

void _al_d3d_score_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref)
{
   for (int i = 0; i < eds_list_count; i++) {
      eds_list[i]->score = _al_score_display_settings(eds_list[i], ref);
      eds_list[i]->index = i;
   }

   qsort(eds_list, eds_list_count, sizeof(void*), _al_display_settings_sorter);
}

/* Helper function for sorting pixel formats by index */
static int d3d_display_list_resorter(const void *p0, const void *p1)
{
   const ALLEGRO_EXTRA_DISPLAY_SETTINGS *f0 = *((ALLEGRO_EXTRA_DISPLAY_SETTINGS **)p0);
   const ALLEGRO_EXTRA_DISPLAY_SETTINGS *f1 = *((ALLEGRO_EXTRA_DISPLAY_SETTINGS **)p1);

   if (!f0)
      return 1;
   if (!f1)
      return -1;
   if (f0->index == f1->index) {
      return 0;
   }
   else if (f0->index < f1->index) {
      return -1;
   }
   else {
      return 1;
   }
}

void _al_d3d_resort_display_settings(void)
{
   qsort(eds_list, eds_list_count, sizeof(void*), d3d_display_list_resorter);
}

ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_d3d_get_display_settings(int i)
{
   if (i < eds_list_count)
      return eds_list[i];
   return NULL;
}
