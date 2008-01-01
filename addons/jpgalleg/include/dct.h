/*
 *         __   _____    ______   ______   ___    ___
 *        /\ \ /\  _ `\ /\  ___\ /\  _  \ /\_ \  /\_ \
 *        \ \ \\ \ \L\ \\ \ \__/ \ \ \L\ \\//\ \ \//\ \      __     __
 *      __ \ \ \\ \  __| \ \ \  __\ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\
 *     /\ \_\/ / \ \ \/   \ \ \L\ \\ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \
 *     \ \____//  \ \_\    \ \____/ \ \_\ \_\/\____\/\____\ \____\ \____ \
 *      \/____/    \/_/     \/___/   \/_/\/_/\/____/\/____/\/____/\/___L\ \
 *                                                                  /\____/
 *                                                                  \_/__/
 *
 *      Version 2.6, by Angelo Mottola, 2000-2006
 *
 *      Fast Discrete Cosine Transform coefficients.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */


#ifndef _JPGALLEG_DCT_H_
#define _JPGALLEG_DCT_H_

#ifndef M_PI
#define M_PI			3.1415926535897932384626
#endif
#define SQRT_2			1.4142135623730950488016

#define SCALE_FACTOR(i)		((i) == 0 ? 1.0 : (cos((i) * M_PI / 16.0) * SQRT_2))
#define AAN_FACTOR(i)		(SCALE_FACTOR(i / 8) * SCALE_FACTOR(i % 8))


#define FIX_0_298631336		2446
#define FIX_0_390180644		3196
#define FIX_0_541196100		4433
#define FIX_0_765366865		6270
#define FIX_0_899976223		7373
#define FIX_1_175875602		9633
#define FIX_1_501321110		12299
#define FIX_1_847759065		15137
#define FIX_1_961570560		16069
#define FIX_2_053119869		16819
#define FIX_2_562915447		20995
#define FIX_3_072711026		25172


#define IFIX_1_082392200	277
#define IFIX_1_414213562	362
#define IFIX_1_847759065	473
#define IFIX_2_613125930	669


#endif
