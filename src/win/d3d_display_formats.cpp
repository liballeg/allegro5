#include "d3d.h"

ALLEGRO_DEBUG_CHANNEL("d3d")

struct DEPTH_STENCIL_DESC {
   int d;
   int s;
   D3DFORMAT format;
};

static DEPTH_STENCIL_DESC depth_stencil_formats[] = {
   {  0, 0, (D3DFORMAT)0 },
   { 32, 0, D3DFMT_D32 },
   { 15, 1, D3DFMT_D15S1 },
   { 24, 8, D3DFMT_D24S8 },
   { 24, 0, D3DFMT_D24X8 },
   { 24, 4, D3DFMT_D24X4S4 },
   { 16, 0, D3DFMT_D16 },
};

static BOOL IsDepthFormatExisting(D3DFORMAT DepthFormat, D3DFORMAT AdapterFormat) 
{
   HRESULT hr = _al_d3d->CheckDeviceFormat(D3DADAPTER_DEFAULT,
      D3DDEVTYPE_HAL,
      AdapterFormat,
      D3DUSAGE_DEPTHSTENCIL,
      D3DRTYPE_SURFACE,
      DepthFormat);

   return SUCCEEDED(hr);
}

static const int D3D_DEPTH_FORMATS = sizeof(depth_stencil_formats) / sizeof(*depth_stencil_formats);

static _AL_VECTOR eds_list;

void _al_d3d_destroy_display_format_list(void)
{
   /* Free the display format list */
   for (int j = 0; j < (int)_al_vector_size(&eds_list); j++) {
      void **eds = (void **)_al_vector_ref(&eds_list, j);
      al_free(*eds);
   }
   _al_vector_free(&eds_list);
}

void _al_d3d_generate_display_format_list(void)
{
   static bool fullscreen = !(al_get_new_display_flags() & ALLEGRO_FULLSCREEN); /* stop warning */
   static int adapter = ~al_get_new_display_adapter(); /* stop warning */
   int i;

   if (!_al_vector_is_empty(&eds_list) && (fullscreen == (bool)(al_get_new_display_flags() & ALLEGRO_FULLSCREEN))
         && (adapter == al_get_new_display_adapter())) {
      return;
   }
   else if (!_al_vector_is_empty(&eds_list)) {
     _al_d3d_destroy_display_format_list();
   }

   fullscreen = (al_get_new_display_flags() & ALLEGRO_FULLSCREEN) != 0;
   adapter = al_get_new_display_adapter();
   if (adapter < 0)
      adapter = 0;

   _al_vector_init(&eds_list, sizeof(ALLEGRO_EXTRA_DISPLAY_SETTINGS *));

   /* Loop through each bit combination of:
    * bit 0: 16/32 bit
    * bit 1: single-buffer
    * bit 2: vsync
    */
   for (i = 0; i < 8; i++) {
      int format_num = !!(i & 1);
      int single_buffer = !!(i & 2);
      int vsync = !!(i & 4);
      int allegro_format = ALLEGRO_PIXEL_FORMAT_XRGB_8888;
      if (format_num == 1) allegro_format = ALLEGRO_PIXEL_FORMAT_RGB_565;
      D3DFORMAT d3d_format = (D3DFORMAT)_al_pixel_format_to_d3d(allegro_format);

     /* Count available multisample quality levels. */
      DWORD quality_levels = 0;

      if (_al_d3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_HAL, d3d_format,
         !fullscreen, D3DMULTISAMPLE_NONMASKABLE, &quality_levels) != D3D_OK) {
         _al_d3d->CheckDeviceMultiSampleType(adapter, D3DDEVTYPE_REF, d3d_format,
            !fullscreen, D3DMULTISAMPLE_NONMASKABLE, &quality_levels);
      }

      /* Loop through available depth/stencil formats. */
      for (int j = 0; j < D3D_DEPTH_FORMATS; j++) {
         if (j == 0 || IsDepthFormatExisting(
               depth_stencil_formats[j].format, d3d_format)) {
            DEPTH_STENCIL_DESC *ds = depth_stencil_formats + j;
            for (int k = 0; k < (int)quality_levels + 1; k++) {
               ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, **peds;
               peds = (ALLEGRO_EXTRA_DISPLAY_SETTINGS **)_al_vector_alloc_back(&eds_list);
               eds = *peds = (ALLEGRO_EXTRA_DISPLAY_SETTINGS *)al_malloc(sizeof *eds);               
               memset(eds->settings, 0, sizeof(int) * ALLEGRO_DISPLAY_OPTIONS_COUNT);

               eds->settings[ALLEGRO_COMPATIBLE_DISPLAY] = 1;

               if (format_num == 0) {
                  eds->settings[ALLEGRO_RED_SIZE] = 8;
                  eds->settings[ALLEGRO_GREEN_SIZE] = 8;
                  eds->settings[ALLEGRO_BLUE_SIZE] = 8;
                  eds->settings[ALLEGRO_RED_SHIFT] = 16;
                  eds->settings[ALLEGRO_GREEN_SHIFT] = 8;
                  eds->settings[ALLEGRO_BLUE_SHIFT] = 0;
                  eds->settings[ALLEGRO_COLOR_SIZE] = 32;
               }
               else if (format_num == 1) {
                  eds->settings[ALLEGRO_RED_SIZE] = 5;
                  eds->settings[ALLEGRO_GREEN_SIZE] = 6;
                  eds->settings[ALLEGRO_BLUE_SIZE] = 5;
                  eds->settings[ALLEGRO_RED_SHIFT] = 11;
                  eds->settings[ALLEGRO_GREEN_SHIFT] = 5;
                  eds->settings[ALLEGRO_BLUE_SHIFT] = 0;
                  eds->settings[ALLEGRO_COLOR_SIZE] = 16;
               }

               if (single_buffer) {
                  eds->settings[ALLEGRO_SINGLE_BUFFER] = 1;
                  eds->settings[ALLEGRO_UPDATE_DISPLAY_REGION] = 1;
               }

               if (vsync) {
                  eds->settings[ALLEGRO_VSYNC] = 1;
               }

               eds->settings[ALLEGRO_DEPTH_SIZE] = ds->d;
               eds->settings[ALLEGRO_STENCIL_SIZE] = ds->s;
               
               if (k > 1) {
                  eds->settings[ALLEGRO_SAMPLE_BUFFERS] = 1;
                  // TODO: Is it ok to use the quality level here?
                  eds->settings[ALLEGRO_SAMPLES] = k;
               }
            }
         }
      }
      
   }
   ALLEGRO_INFO("found %d format combinations\n", _al_vector_size(&eds_list));
}

void _al_d3d_score_display_settings(ALLEGRO_EXTRA_DISPLAY_SETTINGS *ref)
{
   for (int i = 0; i < (int)_al_vector_size(&eds_list); i++) {
      ALLEGRO_EXTRA_DISPLAY_SETTINGS *eds, **peds;
      peds = (ALLEGRO_EXTRA_DISPLAY_SETTINGS **)_al_vector_ref(&eds_list, i);
      eds = *peds;
      eds->score = _al_score_display_settings(eds, ref);
      eds->index = i;
   }

   qsort(eds_list._items, eds_list._size, eds_list._itemsize, _al_display_settings_sorter);
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
   qsort(eds_list._items, eds_list._size, eds_list._itemsize, d3d_display_list_resorter);
}

ALLEGRO_EXTRA_DISPLAY_SETTINGS *_al_d3d_get_display_settings(int i)
{
   if (i < (int)_al_vector_size(&eds_list))
      return *(ALLEGRO_EXTRA_DISPLAY_SETTINGS **)_al_vector_ref(&eds_list, i);
   return NULL;
}
