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
 *      D3DX9 DLL dynamic loading.
 *
 *      See LICENSE.txt for copyright information.
 */

#include "allegro5/allegro.h"
#include "allegro5/allegro_direct3d.h"
#include "allegro5/internal/aintern.h"
#include "allegro5/internal/aintern_bitmap.h"
#include "allegro5/internal/aintern_direct3d.h"

#ifdef A5O_CFG_D3DX9

#include <d3dx9.h>
#include <stdio.h>

#include "d3d.h"

A5O_DEBUG_CHANNEL("d3dx9")

// DXSDK redistributable install d3dx9_xx.dll from version
// 24 to 43. It's unlikely that any new version will come,
// since HLSL compiler was moved to D3DCompiler_xx.dll and
// most recent versions of this utility library are bound
// to DirectX 11.
//
// However, if any new version appears anyway, this range
// should be changed.
#define D3DX9_MIN_VERSION   24
#define D3DX9_MAX_VERSION   43

static HMODULE _imp_d3dx9_module = 0;
_A5O_D3DXCREATEEFFECTPROC _al_imp_D3DXCreateEffect = NULL;
_A5O_D3DXLSFLSPROC _al_imp_D3DXLoadSurfaceFromSurface = NULL;

extern "C"
void _al_unload_d3dx9_module()
{
   FreeLibrary(_imp_d3dx9_module);
   _imp_d3dx9_module = NULL;
}

static bool _imp_load_d3dx9_module_version(int version)
{
   char module_name[16];

   // Sanity check
   // Comented out, to not reject choice of the user if any new version
   // appears. See force_d3dx9_version entry in config file.
   /*
   if (version < 24 || version > 43) {
      A5O_ERROR("Error: Requested version (%d) of D3DX9 library is invalid.\n", version);
      return false;
   }
   */

   sprintf(module_name, "d3dx9_%d.dll", version);

   _imp_d3dx9_module = _al_win_safe_load_library(module_name);
   if (NULL == _imp_d3dx9_module)
      return false;

   _al_imp_D3DXCreateEffect = (_A5O_D3DXCREATEEFFECTPROC)GetProcAddress(_imp_d3dx9_module, "D3DXCreateEffect");
   if (NULL == _al_imp_D3DXCreateEffect) {
      FreeLibrary(_imp_d3dx9_module);
      _imp_d3dx9_module = NULL;
      return false;
   }

   _al_imp_D3DXLoadSurfaceFromSurface =
      (_A5O_D3DXLSFLSPROC)GetProcAddress(_imp_d3dx9_module, "D3DXLoadSurfaceFromSurface");
   if (NULL == _al_imp_D3DXLoadSurfaceFromSurface) {
      FreeLibrary(_imp_d3dx9_module);
      _imp_d3dx9_module = NULL;
      return false;
   }

   A5O_INFO("Module \"%s\" loaded.\n", module_name);

   return true;
}

extern "C"
bool _al_load_d3dx9_module()
{
   long version;
   char const *value;

   if (_imp_d3dx9_module) {
      return true;
   }

   value = al_get_config_value(al_get_system_config(),
      "shader", "force_d3dx9_version");
   if (value) {
      errno = 0;
      version = strtol(value, NULL, 10);
      if (errno) {
         A5O_ERROR("Failed to override D3DX9 version. \"%s\" is not valid integer number.\n", value);
         return false;
      }
      else
         return _imp_load_d3dx9_module_version((int)version);
   }

   // Iterate over all valid versions.
   for (version = D3DX9_MAX_VERSION; version >= D3DX9_MIN_VERSION; version--)
      if (_imp_load_d3dx9_module_version((int)version))
         return true;

   A5O_ERROR("Failed to load D3DX9 library. Library is not installed.\n");

   return false;
}

#endif
