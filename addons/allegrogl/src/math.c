/* This code is (C) AllegroGL contributors, and double licensed under
 * the GPL and zlib licenses. See gpl.txt or zlib.txt for details.
 */
/** \file math.c
 *  Converting mathematical structures from Allegro to OpenGL.
 *
 *  This file provides routines to make Allegro matrices from
 *  GL-style matrices, and vice versa.
 *  This also provides a QUAT to glRotate converter.
 *  
 *  Note that Allegro matrices can only store affine 
 *  transformations.
 */

#include <math.h>
#include <allegro.h>
#include "alleggl.h"
#include "allglint.h"


#ifndef M_PI
   #define M_PI   3.14159265358979323846
#endif


#define ALGL_NOCONV(x) x

#define TRANSLATE_AL_TO_GL(al_type, gl_type, convertor) \
	void allegro_gl_##al_type##_to_##gl_type (al_type *m, gl_type gl[16]) \
	{ \
		int col, row; \
		for (col = 0; col < 3; col++) \
			for (row = 0; row < 3; row++) \
				gl[col*4+row] = convertor (m->v[col][row]); \
		for (row = 0; row < 3; row++) \
			gl[12+row] = convertor (m->t[row]); \
		for (col = 0; col < 3; col++) \
			gl[4*col + 3] = 0; \
		gl[15] = 1; \
	}



/** \ingroup math
 *  Converts an Allegro fixed-point matrix to an array of
 *  floats suitable for OpenGL's matrix operations.
 *
 *  \b Example:
 *  <pre>
 *    MATRIX m = identity_matrix;
 *    GLfloat gl_m[16];
 *    get_vector_rotation_matrix(&m, itofix(1), 0, itofix(1), ftofix(43.83));
 *    allegro_gl_MATRIX_to_GLfloat(&m, &gl_m);
 *    glLoadMatrixf(&gl_m);
 *  </pre>
 *    
 *
 * \see allegro_gl_MATRIX_to_GLdouble()
 * \see allegro_gl_MATRIX_f_to_GLfloat()
 * \see allegro_gl_MATRIX_f_to_GLdouble()
 * \see allegro_gl_GLdouble_to_MATRIX()
 * \see allegro_gl_GLfloat_to_MATRIX_f()
 * \see allegro_gl_GLdouble_to_MATRIX_f()
 * \see allegro_gl_GLfloat_to_MATRIX()
 */
TRANSLATE_AL_TO_GL(MATRIX, GLfloat, fixtof)


	
/** \ingroup math
 *  Converts an Allegro fixed-point matrix to an array of
 *  doubles suitable for OpenGL's matrix operations.
 *
 * \see allegro_gl_MATRIX_to_GLfloat()
 * \see allegro_gl_MATRIX_f_to_GLfloat()
 * \see allegro_gl_MATRIX_f_to_GLdouble()
 * \see allegro_gl_GLdouble_to_MATRIX()
 * \see allegro_gl_GLfloat_to_MATRIX_f()
 * \see allegro_gl_GLdouble_to_MATRIX_f()
 * \see allegro_gl_GLfloat_to_MATRIX()
 */
TRANSLATE_AL_TO_GL(MATRIX, GLdouble, fixtof)

	

/** \ingroup math
 *  Converts an Allegro floating-point matrix to an array of
 *  floats suitable for OpenGL's matrix operations.
 *
 * \see allegro_gl_MATRIX_to_GLfloat()
 * \see allegro_gl_MATRIX_to_GLdouble()
 * \see allegro_gl_MATRIX_f_to_GLdouble()
 * \see allegro_gl_GLdouble_to_MATRIX()
 * \see allegro_gl_GLfloat_to_MATRIX_f()
 * \see allegro_gl_GLdouble_to_MATRIX_f()
 * \see allegro_gl_GLfloat_to_MATRIX()
 */
TRANSLATE_AL_TO_GL(MATRIX_f, GLfloat, ALGL_NOCONV)



/** \ingroup math
 *  Converts an Allegro floating-point matrix to an array of
 *  doubles suitable for OpenGL's matrix operations.
 *
 * \see allegro_gl_MATRIX_to_GLfloat()
 * \see allegro_gl_MATRIX_to_GLdouble()
 * \see allegro_gl_MATRIX_f_to_GLfloat()
 * \see allegro_gl_GLdouble_to_MATRIX()
 * \see allegro_gl_GLfloat_to_MATRIX_f()
 * \see allegro_gl_GLdouble_to_MATRIX_f()
 * \see allegro_gl_GLfloat_to_MATRIX()
 */
TRANSLATE_AL_TO_GL(MATRIX_f, GLdouble, ALGL_NOCONV)


	
#define TRANSLATE_GL_TO_AL(gl_type, al_type, convertor) \
	void allegro_gl_##gl_type##_to_##al_type (gl_type gl[16], al_type *m) \
	{ \
		int col, row; \
		for (col = 0; col < 3; col++) \
			for (row = 0; row < 3; row++) \
				m->v[col][row] = convertor (gl[col*4+row]); \
		for (row = 0; row < 3; row++) \
			m->t[row] = convertor (gl[12+row]); \
	}



/** \ingroup math
 *  Converts an OpenGL floating-point matrix issued from the
 *  matrix stack to an Allegro fixed-point matrix.
 *
 * \see allegro_gl_MATRIX_to_GLfloat()
 * \see allegro_gl_MATRIX_to_GLdouble()
 * \see allegro_gl_MATRIX_f_to_GLfloat()
 * \see allegro_gl_MATRIX_f_to_GLdouble()
 * \see allegro_gl_GLdouble_to_MATRIX()
 * \see allegro_gl_GLfloat_to_MATRIX_f()
 * \see allegro_gl_GLdouble_to_MATRIX_f()
 */
TRANSLATE_GL_TO_AL(GLfloat, MATRIX, ftofix)



/** \ingroup math
 *  Converts an OpenGL double precision floating-point matrix
 *  issued from the matrix stack to an Allegro fixed-point matrix.
 *
 * \see allegro_gl_MATRIX_to_GLfloat()
 * \see allegro_gl_MATRIX_to_GLdouble()
 * \see allegro_gl_MATRIX_f_to_GLfloat()
 * \see allegro_gl_MATRIX_f_to_GLdouble()
 * \see allegro_gl_GLfloat_to_MATRIX_f()
 * \see allegro_gl_GLdouble_to_MATRIX_f()
 * \see allegro_gl_GLfloat_to_MATRIX()
 */
TRANSLATE_GL_TO_AL(GLdouble, MATRIX, ftofix)



/** \ingroup math
 *  Converts an OpenGL floating-point matrix issued from the
 *  matrix stack to an Allegro floating-point matrix.
 *
 * \see allegro_gl_MATRIX_to_GLfloat()
 * \see allegro_gl_MATRIX_to_GLdouble()
 * \see allegro_gl_MATRIX_f_to_GLfloat()
 * \see allegro_gl_MATRIX_f_to_GLdouble()
 * \see allegro_gl_GLdouble_to_MATRIX()
 * \see allegro_gl_GLdouble_to_MATRIX_f()
 * \see allegro_gl_GLfloat_to_MATRIX()
 */
TRANSLATE_GL_TO_AL(GLfloat, MATRIX_f, ALGL_NOCONV)

	

/** \ingroup math
 *  Converts an OpenGL double precision floating-point matrix
 *  issued from the matrix stack to an Allegro single-precision
 *  floating-point matrix.
 *
 * \see allegro_gl_MATRIX_to_GLfloat()
 * \see allegro_gl_MATRIX_to_GLdouble()
 * \see allegro_gl_MATRIX_f_to_GLfloat()
 * \see allegro_gl_MATRIX_f_to_GLdouble()
 * \see allegro_gl_GLdouble_to_MATRIX()
 * \see allegro_gl_GLfloat_to_MATRIX_f()
 * \see allegro_gl_GLfloat_to_MATRIX()
 */
TRANSLATE_GL_TO_AL(GLdouble, MATRIX_f, ALGL_NOCONV)


#undef ALGL_NOCONV


#ifndef RAD_2_DEG
	#define RAD_2_DEG(a) ((a) * 180 / M_PI)
#endif

	

/* void allegro_gl_apply_quat(QUAT *q) */
/** \ingroup math
 *  Multiplies the Quaternion to the current transformation
 *  matrix, by converting it to a call to glRotatef()
 *
 *  \b Example:
 *  <pre>
 *    QUAT q = identity_quat;
 *    get_vector_rotation_quat(&q, itofix(1), 0, itofix(1), ftofix(43.83));
 *    glLoadIdentity();
 *    allegro_gl_apply_quat(&q);
 *  </pre>
 * 
 *
 * \sa allegro_gl_quat_to_glrotatef(), allegro_gl_quat_to_glrotated()
 *
 * \param q The Quaternion to apply.
 */
void allegro_gl_apply_quat(QUAT *q) {

	float theta;
	ASSERT(q);
	ASSERT(__allegro_gl_valid_context);

	theta = RAD_2_DEG(2 * acos(q->w));
	if (q->w < 1.0f && q->w > -1.0f) 
		glRotatef(theta, q->x, q->y, q->z);	

	return;
}



/* void allegro_gl_quat_to_glrotatef(QUAT *q, float *angle, float *x, float *y, float *z) */
/** \ingroup math
 *  Converts a quaternion to a vector/angle, which can be used with
 *  glRotate*(). Values are returned in the parameters.
 *
 *  \b Example:
 *  <pre>
 *    QUAT q = identity_quat;
 *    float x, y, z, angle;
 *    allegro_gl_quat_to_glrotatef(&q, &angle, &x, &y, &z);
 *    glRotatef(angle, x, y, z);
 *  </pre>
 *
 * \sa allegro_gl_quat_to_glrotated(), allegro_gl_apply_quat()
 *
 * \param q     The Quaternion to convert.
 * \param angle The angle of rotation, in degrees.
 * \param x     The rotation vector's x-axis component.
 * \param y     The rotation vector's y-axis component.
 * \param z     The rotation vector's z-axis component.
 */
void allegro_gl_quat_to_glrotatef(QUAT *q, float *angle, float *x, float *y, float *z) {

	ASSERT(q);
	ASSERT(angle);
	ASSERT(x);
	ASSERT(y);
	ASSERT(z);

	*angle = RAD_2_DEG(2 * acos(q->w));
	*x = q->x;
	*y = q->y;
	*z = q->z;

	return;
}



/* void allegro_gl_quat_to_glrotated(QUAT *q, double *angle, double *x, double *y, double *z) */
/** \ingroup math
 *  Converts a quaternion to a vector/angle, which can be used with
 *  glRotate*(). Values are returned in the parameters.
 *  See allegro_gl_quat_to_rotatef() for an example.
 *
 * \sa allegro_gl_quat_to_glrotatef(), allegro_gl_apply_quat()
 *
 * \param q     The Quaternion to convert.
 * \param angle The angle of rotation, in degrees.
 * \param x     The rotation vector's x-axis component.
 * \param y     The rotation vector's y-axis component.
 * \param z     The rotation vector's z-axis component.
 */
void allegro_gl_quat_to_glrotated(QUAT *q, double *angle, double *x, double *y, double *z) {

	ASSERT(q);
	ASSERT(angle);
	ASSERT(x);
	ASSERT(y);
	ASSERT(z);

	*angle = RAD_2_DEG(2 * acos(q->w));
	*x = q->x;
	*y = q->y;
	*z = q->z;

	return;
}

