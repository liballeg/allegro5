/* WGL_ARB_buffer_region */
AGL_API(HANDLE, CreateBufferRegionARB, (HDC, int, UINT))
AGL_API(VOID, DeleteBufferRegionARB, (HANDLE))
AGL_API(BOOL, SaveBufferRegionARB, (HANDLE, int, int, int, int))
AGL_API(BOOL, RestoreBufferRegionARB, (HANDLE, int, int, int, int, int, int))

/* WGL_ARB_extensions_string */
AGL_API(const char *, GetExtensionsStringARB, (HDC))

/* WGL_ARB_pixel_format */
AGL_API(BOOL, GetPixelFormatAttribivARB, (HDC, int, int, UINT, const int *, int *))
AGL_API(BOOL, GetPixelFormatAttribfvARB, (HDC, int, int, UINT, const int *, FLOAT *))
AGL_API(BOOL, ChoosePixelFormatARB, (HDC, const int *, const FLOAT *, UINT, int *, UINT *))

/* WGL_ARB_make_current_read */
AGL_API(BOOL, MakeContextCurrentARB, (HDC, HDC, HGLRC))
AGL_API(HDC, GetCurrentReadDCARB, (void))

/* WGL_ARB_pbuffer */
AGL_API(HPBUFFERARB, CreatePbufferARB, (HDC, int, int, int, const int *))
AGL_API(HDC, GetPbufferDCARB, (HPBUFFERARB))
AGL_API(int, ReleasePbufferDCARB, (HPBUFFERARB, HDC))
AGL_API(BOOL, DestroyPbufferARB, (HPBUFFERARB))
AGL_API(BOOL, QueryPbufferARB, (HPBUFFERARB, int, int *))

/* WGL_ARB_render_texture */
AGL_API(BOOL, BindTexImageARB, (HPBUFFERARB, int))
AGL_API(BOOL, ReleaseTexImageARB, (HPBUFFERARB, int))
AGL_API(BOOL, SetPbufferAttribARB, (HPBUFFERARB, const int *))

/* WGL_EXT_display_color_table */
AGL_API(GLboolean, CreateDisplayColorTableEXT, (GLushort))
AGL_API(GLboolean, LoadDisplayColorTableEXT, (const GLushort *, GLuint))
AGL_API(GLboolean, BindDisplayColorTableEXT, (GLushort))
AGL_API(VOID, DestroyDisplayColorTableEXT, (GLushort))

/* WGL_EXT_extensions_string */
AGL_API(const char *, GetExtensionsStringEXT, (void))

/* WGL_EXT_make_current_read */
AGL_API(BOOL, MakeContextCurrentEXT, (HDC, HDC, HGLRC))
AGL_API(HDC, GetCurrentReadDCEXT, (void))

/* WGL_EXT_pbuffer */
AGL_API(HPBUFFEREXT, CreatePbufferEXT, (HDC, int, int, int, const int *))
AGL_API(HDC, GetPbufferDCEXT, (HPBUFFEREXT))
AGL_API(int, ReleasePbufferDCEXT, (HPBUFFEREXT, HDC))
AGL_API(BOOL, DestroyPbufferEXT, (HPBUFFEREXT))
AGL_API(BOOL, QueryPbufferEXT, (HPBUFFEREXT, int, int *))

/* WGL_EXT_pixel_format */
AGL_API(BOOL, GetPixelFormatAttribivEXT, (HDC, int, int, UINT, int *, int *))
AGL_API(BOOL, GetPixelFormatAttribfvEXT, (HDC, int, int, UINT, int *, FLOAT *))
AGL_API(BOOL, ChoosePixelFormatEXT, (HDC, const int *, const FLOAT *, UINT, int *, UINT *))

/* WGL_EXT_swap_control */
AGL_API(BOOL, SwapIntervalEXT, (int))
AGL_API(int, GetSwapIntervalEXT, (void))

/* WGL_NV_vertex_array_range */
AGL_API(void*, AllocateMemoryNV, (GLsizei, GLfloat, GLfloat, GLfloat))
AGL_API(void, FreeMemoryNV, (void *))

/* WGL_OML_sync_control */
AGL_API(BOOL, GetSyncValuesOML, (HDC, INT64 *, INT64 *, INT64 *))
AGL_API(BOOL, GetMscRateOML, (HDC, INT32 *, INT32 *))
AGL_API(INT64, SwapBuffersMscOML, (HDC, INT64, INT64, INT64))
AGL_API(INT64, SwapLayerBuffersMscOML, (HDC, int, INT64, INT64, INT64))
AGL_API(BOOL, WaitForMscOML, (HDC, INT64, INT64, INT64, INT64 *, INT64 *, INT64 *))
AGL_API(BOOL, WaitForSbcOML, (HDC, INT64, INT64 *, INT64 *, INT64 *))

/* WGL_I3D_digital_video_control */
AGL_API(BOOL, GetDigitalVideoParametersI3D, (HDC, int, int *))
AGL_API(BOOL, SetDigitalVideoParametersI3D, (HDC, int, const int *))

/* WGL_I3D_gamma */
AGL_API(BOOL, GetGammaTableParametersI3D, (HDC, int, int *))
AGL_API(BOOL, SetGammaTableParametersI3D, (HDC, int, const int *))
AGL_API(BOOL, GetGammaTableI3D, (HDC, int, USHORT *, USHORT *, USHORT *))
AGL_API(BOOL, SetGammaTableI3D, (HDC, int, const USHORT *, const USHORT *, const USHORT *))

/* WGL_I3D_genlock */
AGL_API(BOOL, EnableGenlockI3D, (HDC))
AGL_API(BOOL, DisableGenlockI3D, (HDC))
AGL_API(BOOL, IsEnabledGenlockI3D, (HDC, BOOL *))
AGL_API(BOOL, GenlockSourceI3D, (HDC, UINT))
AGL_API(BOOL, GetGenlockSourceI3D, (HDC, UINT *))
AGL_API(BOOL, GenlockSourceEdgeI3D, (HDC, UINT))
AGL_API(BOOL, GetGenlockSourceEdgeI3D, (HDC, UINT *))
AGL_API(BOOL, GenlockSampleRateI3D, (HDC, UINT))
AGL_API(BOOL, GetGenlockSampleRateI3D, (HDC, UINT *))
AGL_API(BOOL, GenlockSourceDelayI3D, (HDC, UINT))
AGL_API(BOOL, GetGenlockSourceDelayI3D, (HDC, UINT *))
AGL_API(BOOL, QueryGenlockMaxSourceDelayI3D, (HDC, UINT *, UINT *))

/* WGL_I3D_image_buffer */
AGL_API(LPVOID, CreateImageBufferI3D, (HDC, DWORD, UINT))
AGL_API(BOOL, DestroyImageBufferI3D, (HDC, LPVOID))
AGL_API(BOOL, AssociateImageBufferEventsI3D, (HDC, const HANDLE *, const LPVOID *, const DWORD *, UINT))
AGL_API(BOOL, ReleaseImageBufferEventsI3D, (HDC, const LPVOID *, UINT))

/* WGL_I3D_swap_frame_lock */
AGL_API(BOOL, EnableFrameLockI3D, (void))
AGL_API(BOOL, DisableFrameLockI3D, (void))
AGL_API(BOOL, IsEnabledFrameLockI3D, (BOOL *))
AGL_API(BOOL, QueryFrameLockMasterI3D, (BOOL *))

/* WGL_I3D_swap_frame_usage */
AGL_API(BOOL, GetFrameUsageI3D, (float *))
AGL_API(BOOL, BeginFrameTrackingI3D, (void))
AGL_API(BOOL, EndFrameTrackingI3D, (void))
AGL_API(BOOL, QueryFrameTrackingI3D, (DWORD *, DWORD *, float *))

