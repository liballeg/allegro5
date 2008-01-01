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
 *      MMX optimized YCbCr <--> RGB color conversion routines.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */


#include <mmx.h>

#ifdef JPGALLEG_MMX


#define FUNC(name)            .globl _##name ; .balign 8, 0x90 ; _##name:
#define GLOBL(name)           _##name

#define ARG1       8(%ebp)
#define ARG2      12(%ebp)
#define ARG3      16(%ebp)
#define ARG4      20(%ebp)
#define ARG5      24(%ebp)
#define ARG6      28(%ebp)
#define ARG7      32(%ebp)
#define ARG8      36(%ebp)
#define ARG9      40(%ebp)
#define ARG10     44(%ebp)
#define ARG11     48(%ebp)
#define ARG12     52(%ebp)
#define ARG13     56(%ebp)


.data
.balign 16, 0x90

/* 10.6 factors */

f_const_0:		.short -27, -5, -11, -21
f_const_1:		.short 19, 38, 7, 0
f_const_2:		.short -21, -11, -5, -27
f_const_3:		.short 7, 38, 19, 0

i_const_0:		.short 64, 113, 64, 90
i_const_1:		.short -22, -46, -22, -46
i_const_2:		.short 64, 90, 64, 113
i_const_3:		.short -46, -22, -46, -22

level_shift:		.short 128, 128, 128, 128


.text



/*
 *	void
 *	_jpeg_mmx_ycbcr2bgr(int addr, int y1, int cb1, int cr1, int y2, int cb2, int cr2, int y3, int cb3, int cr3, int y4, int cb4, int cr4)
 */
FUNC(_jpeg_mmx_ycbcr2bgr)
	pushl %ebp;
	movl %esp, %ebp;
	pushl %edi;
	
	movl ARG1, %edi;
	movd ARG2, %mm0;
	movd ARG3, %mm1;
	movd ARG4, %mm2;
	movd ARG5, %mm3;
	movd ARG6, %mm4;
	movd ARG7, %mm5;
	punpcklwd %mm2, %mm1;	/* mm1 = |     |     | cr1 | cb1 | */
	punpcklwd %mm5, %mm4;	/* mm4 = |     |     | cr2 | cb2 | */
	punpcklwd %mm0, %mm0;	/* mm0 = |     |     |  y1 |  y1 | */
	punpckldq %mm4, %mm1;	/* mm1 = | cr2 | cb2 | cr1 | cb1 | */
	movq %mm0, %mm5;
	punpcklwd %mm3, %mm3;	/* mm3 = |     |     |  y2 |  y2 | */
	psubw (level_shift), %mm1;
	punpckldq %mm3, %mm5;	/* mm5 = |  y2 |  y2 |  y1 |  y1 | */
	punpcklwd %mm1, %mm0;	/* mm0 = | cr1 |  y1 | cb1 |  y1 | */
	movq %mm5, %mm6;
	pmaddwd (i_const_0), %mm0;
	punpckhwd %mm1, %mm6;	/* mm6 = | cr2 |  y2 | cb2 |  y2 | */
	psrad $6, %mm0;		/* mm0 = |        r1 |        b1 | */
	pmaddwd (i_const_0), %mm6;
	psllw $6, %mm5;
	pmaddwd (i_const_1), %mm1;
	psrad $6, %mm6;		/* mm6 = |        r2 |        b2 | */
	paddw %mm5, %mm1;
	psraw $6, %mm1;		/* mm1 = |        g2 |        g1 | */
	
	movq %mm0, %mm7;
	psrlq $32, %mm0;
	punpcklwd %mm1, %mm7;	/* mm7 = |           |  g1 |  b1 | */
	punpcklwd %mm6, %mm0;	/* mm0 = |           |  b2 |  r1 | */
	punpckldq %mm0, %mm7;	/* mm7 = |  b2 |  r1 |  g1 |  b1 | */
	punpckhwd %mm6, %mm1;	/* mm1 = |           |  r2 |  g2 | */
	packuswb %mm1, %mm7;	/* mm7 = |  |  |r2|g2|b2|r1|g1|b1| */

	movd ARG8, %mm0;
	movd ARG9, %mm1;
	movd ARG10, %mm2;
	movd ARG11, %mm3;
	movd ARG12, %mm4;
	movd ARG13, %mm5;
	punpcklwd %mm2, %mm1;	/* mm1 = |     |     | cr3 | cb3 | */
	punpcklwd %mm5, %mm4;	/* mm4 = |     |     | cr4 | cb4 | */
	punpcklwd %mm0, %mm0;	/* mm0 = |     |     |  y3 |  y3 | */
	punpckldq %mm4, %mm1;	/* mm1 = | cr4 | cb4 | cr3 | cb3 | */
	movq %mm0, %mm5;
	punpcklwd %mm3, %mm3;	/* mm3 = |     |     |  y4 |  y4 | */
	psubw (level_shift), %mm1;
	punpckldq %mm3, %mm5;	/* mm5 = |  y4 |  y4 |  y3 |  y3 | */
	punpcklwd %mm1, %mm0;	/* mm0 = | cr3 |  y3 | cb3 |  y3 | */
	movq %mm5, %mm6;
	pmaddwd (i_const_0), %mm0;
	punpckhwd %mm1, %mm6;	/* mm6 = | cr4 |  y4 | cb4 |  y4 | */
	psrad $6, %mm0;		/* mm0 = |        r3 |        b3 | */
	pmaddwd (i_const_0), %mm6;
	psllw $6, %mm5;
	pmaddwd (i_const_1), %mm1;
	psrad $6, %mm6;		/* mm6 = |        r4 |        b4 | */
	paddw %mm5, %mm1;
	psraw $6, %mm1;		/* mm1 = |        g4 |        g3 | */

	movq %mm0, %mm2;
	punpcklwd %mm1, %mm0;	/* mm0 = |           |  g3 |  b3 | */
	psrlq $32, %mm2;
	punpcklwd %mm6, %mm2;	/* mm2 = |           |  b4 |  r3 | */
	punpckhwd %mm6, %mm1;	/* mm1 = |           |  r4 |  g4 | */
	punpckldq %mm1, %mm2;	/* mm2 = |  r4 |  g4 |  b4 |  r3 | */
	movq %mm7, %mm3;
	packuswb %mm0, %mm2;	/* mm2 = |  |  |g3|b3|r4|g4|b4|r3| */
	punpckhwd %mm2, %mm3;	/* mm3 = |           |g3|b3|r2|g2| */
	punpckldq %mm3, %mm7;	/* mm7 = |g3|b3|r2|g2|b2|r1|g1|b1| */
	
	movd %mm2, 8(%edi);
	movq %mm7, (%edi);
	
	emms;
	
	popl %edi;
	popl %ebp;
	ret;



/*
 *	void
 *	_jpeg_mmx_ycbcr2rgb(int addr, int y1, int cb1, int cr1, int y2, int cb2, int cr2, int y3, int cb3, int cr3, int y4, int cb4, int cr4)
 */
FUNC(_jpeg_mmx_ycbcr2rgb)
	pushl %ebp;
	movl %esp, %ebp;
	pushl %edi;
	
	movl ARG1, %edi;
	movd ARG2, %mm0;
	movd ARG3, %mm1;
	movd ARG4, %mm2;
	movd ARG5, %mm3;
	movd ARG6, %mm4;
	movd ARG7, %mm5;
	punpcklwd %mm1, %mm2;	/* mm2 = |     |     | cb1 | cr1 | */
	punpcklwd %mm4, %mm5;	/* mm5 = |     |     | cb2 | cr2 | */
	punpcklwd %mm0, %mm0;	/* mm0 = |     |     |  y1 |  y1 | */
	punpckldq %mm5, %mm2;	/* mm2 = | cb2 | cr2 | cb1 | cr1 | */
	movq %mm0, %mm5;
	punpcklwd %mm3, %mm3;
	psubw (level_shift), %mm2;
	punpckldq %mm3, %mm3;	/* mm3 = |  y2 |  y2 |  y2 |  y2 | */
	punpckldq %mm3, %mm5;	/* mm5 = |  y2 |  y2 |  y1 |  y1 | */
	punpcklwd %mm2, %mm0;	/* mm0 = | cb1 |  y1 | cr1 |  y1 | */
	punpckhwd %mm2, %mm3;	/* mm3 = | cb2 |  y2 | cr2 |  y2 | */
	psllw $6, %mm5;
	pmaddwd (i_const_3), %mm2;
	pmaddwd (i_const_2), %mm0;
	pmaddwd (i_const_2), %mm3;
	paddw %mm5, %mm2;
	psrad $6, %mm0;		/* mm0 = |        b1 |        r1 | */
	psrad $6, %mm3;		/* mm3 = |        b2 |        r2 | */
	psraw $6, %mm2;		/* mm2 = |        g2 |        g1 | */
	
	movq %mm0, %mm7;
	psrlq $32, %mm0;
	punpcklwd %mm2, %mm7;	/* mm7 = |           |  g1 |  r1 | */
	punpcklwd %mm3, %mm0;	/* mm0 = |           |  r2 |  b1 | */
	punpckldq %mm0, %mm7;	/* mm7 = |  r2 |  b1 |  g1 |  r1 | */
	punpckhwd %mm3, %mm2;	/* mm2 = |           |  b2 |  g2 | */
	packuswb %mm2, %mm7;	/* mm7 = |     |b2|g2|r2|b1|g1|r1| */

	movd ARG8, %mm0;
	movd ARG9, %mm1;
	movd ARG10, %mm2;
	movd ARG11, %mm3;
	movd ARG12, %mm4;
	movd ARG13, %mm5;
	punpcklwd %mm1, %mm2;	/* mm2 = |     |     | cb1 | cr1 | */
	punpcklwd %mm4, %mm5;	/* mm5 = |     |     | cb2 | cr2 | */
	punpcklwd %mm0, %mm0;	/* mm0 = |     |     |  y1 |  y1 | */
	punpckldq %mm5, %mm2;	/* mm2 = | cb2 | cr2 | cb1 | cr1 | */
	movq %mm0, %mm5;
	punpcklwd %mm3, %mm3;
	psubw (level_shift), %mm2;
	punpckldq %mm3, %mm3;	/* mm3 = |  y2 |  y2 |  y2 |  y2 | */
	punpckldq %mm3, %mm5;	/* mm5 = |  y2 |  y2 |  y1 |  y1 | */
	punpcklwd %mm2, %mm0;	/* mm0 = | cb1 |  y1 | cr1 |  y1 | */
	punpckhwd %mm2, %mm3;	/* mm3 = | cb2 |  y2 | cr2 |  y2 | */
	psllw $6, %mm5;
	pmaddwd (i_const_3), %mm2;
	pmaddwd (i_const_2), %mm0;
	pmaddwd (i_const_2), %mm3;
	paddw %mm5, %mm2;
	psrad $6, %mm0;		/* mm0 = |        b3 |        r3 | */
	psrad $6, %mm3;		/* mm3 = |        b4 |        r4 | */
	psraw $6, %mm2;		/* mm2 = |        g4 |        g3 | */
	
	movq %mm0, %mm1;
	punpcklwd %mm2, %mm1;	/* mm1 = |           |  g3 |  r3 | */
	punpckhwd %mm3, %mm2;	/* mm2 = |           |  b4 |  g4 | */
	psrlq $32, %mm0;
	punpcklwd %mm3, %mm0;	/* mm0 = |           |  r4 |  b3 | */
	punpckldq %mm2, %mm0;	/* mm0 = |  b4 |  g4 |  r4 |  b3 | */
	movq %mm7, %mm4;
	packuswb %mm1, %mm0;	/* mm0 = |     |g3|r3|b4|g4|r4|b3| */
	punpckhwd %mm0, %mm4;	/* mm4 = |           |g3|r3|b2|g2| */
	punpckldq %mm4, %mm7;	/* mm7 = |g3|r3|b2|g2|r2|b1|g1|r1| */
	
	movd %mm0, 8(%edi);
	movq %mm7, (%edi);
	
	emms;
	
	popl %edi;
	popl %ebp;
	ret;



/*
 *	void
 *	_jpeg_mmx_rgb2ycbcr(int addr, short *y1, short *cb1, short *cr1, short *y2, short *cb2, short *cr2)
 */
FUNC(_jpeg_mmx_rgb2ycbcr)
	pushl %ebp;
	movl %esp, %ebp;
	pushl %edi;
	
	movl ARG1, %edi;
	
	movq (%edi), %mm0;	/* mm0 = |  |b2|g2|r2|  |b1|g1|r1| */
	pxor %mm2, %mm2;
	movq %mm0, %mm1;
	punpcklbw %mm2, %mm0;	/* mm0 = |     |  b1 |  g1 |  r1 | */
	punpckhbw %mm2, %mm1;	/* mm1 = |     |  b2 |  g2 |  r2 | */
	movq %mm0, %mm3;
	movq %mm1, %mm4;
	movq %mm0, %mm5;
	movq %mm1, %mm6;
	psrlq $16, %mm3;
	psrlq $16, %mm4;
	punpckldq %mm0, %mm3;	/* mm3 = |  g1 |  r1 |  b1 |  g1 | */
	punpckldq %mm1, %mm4;	/* mm4 = |  g2 |  r2 |  b2 |  g2 | */
	pmaddwd (f_const_0), %mm3;
	pmaddwd (f_const_0), %mm4;
	pmaddwd (f_const_1), %mm5;
	pmaddwd (f_const_1), %mm6;
	movq %mm5, %mm2;
	psraw $6, %mm3;
	psraw $6, %mm4;
	punpckldq %mm6, %mm5;	/* mm5 = |   r2+g2   |   r1+g1   | */
	punpckhdq %mm6, %mm2;	/* mm2 = |     b2    |     b1    | */
	psrlw $1, %mm0;
	psrlw $1, %mm1;
	paddw %mm5, %mm2;
	paddw %mm0, %mm3;	/* mm3 = |     | cb1 |     | cr1 | */
	psraw $6, %mm2;
	paddw %mm1, %mm4;	/* mm4 = |     | cb2 |     | cr2 | */
	psubw (level_shift), %mm2;/* mm2 = |     |  y2 |     |  y1 | */
	
	movl ARG2, %edi;
	movd %mm2, %eax;
	movw %ax, (%edi);
	movl ARG4, %edi;
	movd %mm3, %eax;
	movw %ax, (%edi);
	movl ARG7, %edi;
	movd %mm4, %eax;
	movw %ax, (%edi);
	psrlq $32, %mm2;
	movl ARG5, %edi;
	movd %mm2, %eax;
	movw %ax, (%edi);
	psrlq $32, %mm3;
	movl ARG3, %edi;
	movd %mm3, %eax;
	movw %ax, (%edi);
	psrlq $32, %mm4;
	movl ARG6, %edi;
	movd %mm4, %eax;
	movw %ax, (%edi);
	
	emms;
	
	popl %edi;
	popl %ebp;
	ret;



/*
 *	void
 *	_jpeg_mmx_bgr2ycbcr(int addr, short *y1, short *cb1, short *cr1, short *y2, short *cb2, short *cr2)
 */
FUNC(_jpeg_mmx_bgr2ycbcr)
	pushl %ebp;
	movl %esp, %ebp;
	pushl %edi;
	
	movl ARG1, %edi;
	movq (%edi), %mm0;	/* mm0 = |  |r2|g2|b2|  |r1|g1|b1| */
	pxor %mm2, %mm2;
	movq %mm0, %mm1;
	punpcklbw %mm2, %mm0;	/* mm0 = |     |  r1 |  g1 |  b1 | */
	punpckhbw %mm2, %mm1;	/* mm1 = |     |  r2 |  g2 |  b2 | */
	movq %mm0, %mm3;
	movq %mm1, %mm4;
	movq %mm0, %mm5;
	movq %mm1, %mm6;
	psrlq $16, %mm3;
	psrlq $16, %mm4;
	punpckldq %mm0, %mm3;	/* mm3 = |  g1 |  b1 |  r1 |  g1 | */
	punpckldq %mm1, %mm4;	/* mm4 = |  g2 |  b2 |  r2 |  g2 | */
	pmaddwd (f_const_2), %mm3;
	pmaddwd (f_const_2), %mm4;
	pmaddwd (f_const_3), %mm5;
	pmaddwd (f_const_3), %mm6;
	movq %mm5, %mm2;
	psraw $6, %mm3;
	psraw $6, %mm4;
	punpckldq %mm6, %mm5;	/* mm5 = |   g2+b2   |   g1+b1   | */
	punpckhdq %mm6, %mm2;	/* mm2 = |     r2    |     r1    | */
	psrlw $1, %mm0;
	psrlw $1, %mm1;
	paddw %mm5, %mm2;
	paddw %mm0, %mm3;	/* mm3 = |     | cr1 |     | cb1 | */
	psraw $6, %mm2;
	paddw %mm1, %mm4;	/* mm4 = |     | cr2 |     | cb2 | */
	psubw (level_shift), %mm2;/* mm2 = |     |  y2 |     |  y1 | */
	
	movl ARG2, %edi;
	movd %mm2, %eax;
	movw %ax, (%edi);
	movl ARG3, %edi;
	movd %mm3, %eax;
	movw %ax, (%edi);
	movl ARG6, %edi;
	movd %mm4, %eax;
	movw %ax, (%edi);
	psrlq $32, %mm2;
	movl ARG5, %edi;
	movd %mm2, %eax;
	movw %ax, (%edi);
	psrlq $32, %mm3;
	movl ARG4, %edi;
	movd %mm3, %eax;
	movw %ax, (%edi);
	psrlq $32, %mm4;
	movl ARG7, %edi;
	movd %mm4, %eax;
	movw %ax, (%edi);
	
	emms;
	
	popl %edi;
	popl %ebp;
	ret;


#endif
