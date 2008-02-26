#ifndef INCLUDE_ALLEGRO_GL_HEADER_DETECT_H_GUARD
#define INCLUDE_ALLEGRO_GL_HEADER_DETECT_H_GUARD


/* Some GL headers are broken - they don't define ARB_imaging when they should
 */
#if defined GL_CONVOLUTION_FORMAT && !defined GL_ARB_imaging
#define GL_ARB_imaging 1
#endif


/* NV header detection */
#ifdef GL_FORCE_SOFTWARE_NV
#define AGL_HEADER_NV 1
#endif



#endif
