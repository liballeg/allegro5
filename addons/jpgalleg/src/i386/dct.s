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
 *      (Inverse) Discrete Cosine Transform MMX optimized routine.
 *
 *      See the readme.txt file for instructions on using this package in your
 *      own programs.
 */


#include <mmx.h>
#include <dct.h>

#ifdef JPGALLEG_MMX


#define FUNC(name)            .globl _##name ; .balign 8, 0x90 ; _##name:
#define GLOBL(name)           _##name

#define ARG1       8(%ebp)
#define ARG2      12(%ebp)
#define ARG3      16(%ebp)
#define ARG4      20(%ebp)


.data
.balign 16, 0x90

const_0:	.short	0, 0, IFIX_1_414213562, 0
const_1:	.short	1, 1, IFIX_1_847759065, IFIX_1_847759065
const_2:	.short	IFIX_1_414213562, -IFIX_1_414213562, 0, 0
const_3:	.short	-IFIX_2_613125930, 0, IFIX_1_082392200, 0
const_4:	.short	1, 0, -1, 0
const_5:	.short	0, IFIX_1_414213562, 0, 1
const_6:	.short	1, 1, -1, -1
const_7:	.short	1, 1, -IFIX_1_414213562, IFIX_1_414213562
const_8:	.short	0, 0, IFIX_1_847759065, IFIX_1_847759065
const_9:	.short	-1, 0, 1, 0
const_10:	.short	1, -1, 1, 1
const_11:	.short	1, 1, -1, -1
const_12:	.short	128, 128, 128, 128
const_13:	.short	IFIX_1_082392200, 0, -IFIX_2_613125930, 0
const_14:	.short	0, 0, 0xFFFF, 0



.text



/*
 *	void
 *	_jpeg_mmx_idct(short *data, short *output, short *dequant, short *workspace)
 */
FUNC(_jpeg_mmx_idct)
	pushl %ebp;
	movl %esp, %ebp;
	pushl %edi;
	pushl %esi;
	pushl %ebx;
	
	movl ARG1, %esi;
	movl ARG4, %edi;
	movl ARG3, %ebx;
	movl $8, %ecx;

.balign 16, 0x90
pass_1:
	movw 16(%esi), %ax;
	orw 32(%esi), %ax;
	orw 48(%esi), %ax;
	orw 64(%esi), %ax;
	orw 80(%esi), %ax;
	orw 96(%esi), %ax;
	orw 112(%esi), %ax;
	jnz pass_1_start;
	movw (%esi), %ax;
	imulw (%ebx);
	movw %ax, (%edi);
	movw %ax, 16(%edi);
	movw %ax, 32(%edi);
	movw %ax, 48(%edi);
	movw %ax, 64(%edi);
	movw %ax, 80(%edi);
	movw %ax, 96(%edi);
	movw %ax, 112(%edi);
	addl $2, %esi;
	addl $2, %ebx;
	addl $2, %edi;
	decl %ecx;
	jnz pass_1;
	jmp pass_1_end;
	
pass_1_start:
	movw (%esi), %ax;
	shll $16, %eax;
	movw 64(%esi), %ax;
	movd %eax, %mm0;	/* mm0 = |       |       |  i[0] | i[32] | */
	negw %ax;
	movd %eax, %mm1;	/* mm1 = |       |       |  i[0] | -i[32]| */
	punpckldq %mm1, %mm0;	/* mm0 = |  i[0] | -i[32]|  i[0] | i[32] | */
	movw (%ebx), %ax;
	shll $16, %eax;
	movw 64(%ebx), %ax;
	movd %eax, %mm1;
	punpckldq %mm1, %mm1;	/* mm1 = |  q[0] | q[32] |  q[0] | q[32] | */
	pmaddwd %mm1, %mm0;	/* mm0 = |         tmp11 |         tmp10 | */
	movw 32(%esi), %ax;
	shll $16, %eax;
	movw 96(%esi), %ax;
	movd %eax, %mm1;	/* mm1 = |               | i[16] | i[48] | */
	negw %ax;
	movd %eax, %mm2;
	punpckldq %mm2, %mm1;	/* mm1 = | i[16] | -i[48]| i[16] | i[48] | */
	movw 32(%ebx), %ax;
	shll $16, %eax;
	movw 96(%ebx), %ax;
	movd %eax, %mm2;
	punpckldq %mm2, %mm2;	/* mm2 = | q[16] | q[48] | q[16] | q[48] | */
	pmaddwd %mm2, %mm1;	/* mm1 = | (tmp1 - tmp3) |         tmp13 | */
	movq %mm1, %mm2;
	pmaddwd (const_0), %mm2;
	psrad $8, %mm2;
	psrlq $32, %mm2;
	psubw %mm1, %mm2;	/* mm2 = |             ? |         tmp12 | */
	punpckldq %mm2, %mm1;	/* mm1 = |         tmp12 |         tmp13 | */
	movq %mm0, %mm3;
	paddw %mm1, %mm0;	/* mm0 = |          tmp1 |          tmp0 | */
	psubw %mm1, %mm3;	/* mm3 = |          tmp2 |          tmp3 | */
	
	movw 16(%esi), %ax;
	shll $16, %eax;
	movw 112(%esi), %ax;
	movd %eax, %mm1;
	negw %ax;
	movd %eax, %mm2;
	punpckldq %mm2, %mm1	/* mm1 = |  i[8] | -i[56]|  i[8] | i[56] | */
	movw 16(%ebx), %ax;
	shll $16, %eax;
	movw 112(%ebx), %ax;
	movd %eax, %mm2;
	punpckldq %mm2, %mm2;	/* mm2 = |  q[8] | q[56] |  q[8] | q[56] | */
	pmaddwd %mm2, %mm1;	/* mm1 = |           z12 |           z11 | */
	movw 80(%esi), %ax;
	shll $16, %eax;
	movw 48(%esi), %ax;
	movd %eax, %mm2;
	negw %ax;
	movd %eax, %mm4;
	punpckldq %mm4, %mm2;	/* mm2 = | i[40] | -i[24]| i[40] | i[24] | */
	movw 80(%ebx), %ax;
	shll $16, %eax;
	movw 48(%ebx), %ax;
	movd %eax, %mm4;
	punpckldq %mm4, %mm4;	/* mm4 = | q[40] | q[24] | q[40] | q[24] | */
	pmaddwd %mm4, %mm2;	/* mm2 = |           z10 |           z13 | */
	movq %mm1, %mm5;
	movq %mm1, %mm6;
	punpcklwd %mm2, %mm5;	/* mm5 = |               |   z13 |   z11 | */
	punpckhwd %mm2, %mm6;	/* mm6 = |               |   z10 |   z12 | */
	punpckldq %mm6, %mm5;
	movq %mm5, %mm4;	/* mm4 = |   z10 |   z12 |   z13 |   z11 | */
	pmaddwd (const_1), %mm5;
	movq %mm5, %mm6;	/* mm5 = |             ? |          tmp7 | */
	psrad $8, %mm6;		/* mm6 = |            z5 |             ? | */
	pmaddwd (const_2), %mm4;
	packssdw %mm1, %mm2;	/* mm2 = |   z12 |   z11 |   z10 |   z13 | */
	psrlq $16, %mm2;	/* mm2 = |       |   z12 |     ? |   z10 | */
	punpckhdq %mm6, %mm6;
	pmaddwd (const_3), %mm2;
	pmullw (const_4), %mm6;	/* mm6 = |           -z5 |            z5 | */
	psrad $8, %mm2;
	psrad $8, %mm4;		/* mm4 = |               |         tmp11 | */
	paddw %mm6, %mm2;	/* mm2 = |         tmp10 |         tmp12 | */
	movq %mm2, %mm1;
	psubw %mm5, %mm1;	/* mm1 = |             ? |          tmp6 | */
	punpckldq %mm1, %mm5;	/* mm5 = |          tmp6 |          tmp7 | */
	psubw %mm1, %mm4;	/* mm4 = |             ? |          tmp5 | */
	psrlq $32, %mm2;
	punpckldq %mm4, %mm4;
	paddw %mm2, %mm4;	/* mm4 = |          tmp5 |          tmp4 | */
	
	movq %mm0, %mm1;
	movq %mm3, %mm2;
	paddw %mm5, %mm1;	/* mm1 = |          w[8] |          w[0] | */
	paddw %mm4, %mm2;	/* mm2 = |         w[16] |         w[32] | */
	psubw %mm5, %mm0;	/* mm0 = |         w[48] |         w[56] | */
	psubw %mm4, %mm3;	/* mm3 = |         w[40] |         w[24] | */
	movd %mm1, %eax;
	movd %mm2, %edx;
	movw %ax, (%edi);
	movw %dx, 64(%edi);
	movd %mm0, %eax;
	movd %mm3, %edx;
	movw %ax, 112(%edi);
	movw %dx, 48(%edi);
	psrlq $32, %mm1;
	psrlq $32, %mm2;
	psrlq $32, %mm0;
	psrlq $32, %mm3;
	movd %mm1, %eax;
	movd %mm2, %edx;
	movw %ax, 16(%edi);
	movw %dx, 32(%edi);
	movd %mm0, %eax;
	movd %mm3, %edx;
	movw %ax, 96(%edi);
	movw %dx, 80(%edi);
	addl $2, %esi;
	addl $2, %ebx;
	addl $2, %edi;
	decl %ecx;
	jnz pass_1;

pass_1_end:
	movl ARG4, %esi;
	movl ARG2, %edi;
	movl $8, %ecx;
pass_2:
	movd (%esi), %mm0;
	punpcklwd %mm0, %mm0;	/* mm0 = |               |  w[0] |  w[0] | */
	movw 8(%esi), %ax;
	movd %eax, %mm1;
	negw %ax;
	movd %eax, %mm2;
	punpcklwd %mm2, %mm1;	/* mm1 = |               | -w[4] |  w[4] | */
	movd 4(%esi), %mm3;
	punpcklwd %mm3, %mm3;	/* mm3 = |               |  w[2] |  w[2] | */
	movw 12(%esi), %ax;
	movd %eax, %mm4;
	negw %ax;
	movd %eax, %mm2;
	punpcklwd %mm4, %mm2;	/* mm2 = |               |  w[6] | -w[6] | */
	punpckldq %mm3, %mm0;	/* mm0 = |  w[2] |  w[2] |  w[0] |  w[0] | */
	punpckldq %mm2, %mm1;	/* mm1 = |  w[6] | -w[6] | -w[4] |  w[4] | */
	paddw %mm1, %mm0;	/* mm0 = | tmp13 |~tmp12 | tmp11 | tmp10 | */
	punpckhwd %mm0, %mm4;	/* mm4 = | tmp13 |     ? |~tmp12 |     ? | */
	movq %mm4, %mm2;
	pmaddwd (const_5), %mm4;
	psrlq $48, %mm2;	/* mm2 = |                       | tmp13 | */
	psrad $8, %mm4;
	psubw %mm2, %mm4;	/* mm4 = |                       | tmp12 | */
	punpcklwd %mm4, %mm2;	/* mm2 = |               | tmp12 | tmp13 | */
	punpckldq %mm0, %mm0;	/* mm0 = | tmp11 | tmp10 | tmp11 | tmp10 | */
	punpckldq %mm2, %mm2;
	pmullw (const_6), %mm2;	/* mm2 = |-tmp12 |-tmp13 | tmp12 | tmp13 | */
	paddw %mm2, %mm0;	/* mm0 = |  tmp2 |  tmp3 |  tmp1 |  tmp0 | */
	
	movd 10(%esi), %mm1;
	movd 2(%esi), %mm4;
	movq %mm1, %mm2;
	punpcklwd %mm4, %mm1;	/* mm1 = |               |  w[1] |  w[5] | */
	punpcklwd %mm2, %mm4;	/* mm4 = |               |  w[5] |  w[1] | */
	punpckldq %mm4, %mm1;	/* mm1 = |  w[5] |  w[1] |  w[1] |  w[5] | */
	movd 6(%esi), %mm2;
	movd 14(%esi), %mm3;
	movq %mm2, %mm4;
	punpcklwd %mm3, %mm2;	/* mm2 = |               |  w[7] |  w[3] | */
	punpcklwd %mm4, %mm3;	/* mm3 = |               |  w[3] |  w[7] | */
	punpckldq %mm3, %mm2;	/* mm2 = |  w[3] |  w[7] |  w[7] |  w[3] | */
	pmullw (const_11), %mm2;/* mm2 = | -w[3] | -w[7] |  w[7] |  w[3] | */
	paddw %mm2, %mm1;	/* mm1 = |   z10 |   z12 |   z11 |   z13 | */
	movq %mm1, %mm3;
	movq %mm1, %mm4;
	punpckldq %mm1, %mm1;
	pmaddwd (const_7), %mm1;/* mm1 = |        ~tmp11 |          tmp7 | */
	punpckhwd %mm3, %mm3;	/* mm3 = |   z10 |   z10 |   z12 |   z12 | */
	pmaddwd (const_8), %mm4;
	psrad $8, %mm4;		/* mm4 = |            z5 |               | */
	punpckhdq %mm4, %mm4;
	pmullw (const_9), %mm4;	/* mm4 = |            z5 |           -z5 | */
	pmaddwd (const_13), %mm3;
	psrad $8, %mm3;
	paddw %mm3, %mm4;	/* mm4 = |         tmp12 |         tmp10 | */
	movq %mm1, %mm5;
	movq %mm4, %mm6;
	movq %mm4, %mm7;
	psrad $8, %mm5;		/* mm5 = |         tmp11 |             ? | */
	psrlq $32, %mm4;	/* mm4 = |                       | tmp12 | */
	punpcklwd %mm1, %mm1;
	punpckhwd %mm5, %mm5;	/* mm5 = |               | tmp11 | tmp11 | */
	punpckldq %mm1, %mm1;
	psllq $32, %mm6;	/* mm6 = |       | tmp10 |               | */
	psllq $32, %mm5;	/* mm5 = | tmp11 | tmp11 |               | */
	punpckhwd %mm7, %mm7;	/* mm7 = |               | tmp12 | tmp12 | */
	pand (const_14), %mm6;
	punpckldq %mm4, %mm7;	/* mm7 = |       | tmp12 | tmp12 | tmp12 | */
	psllq $16, %mm7;
	pmullw (const_10), %mm1;/* mm1 = |  tmp7 |  tmp7 | -tmp7 |  tmp7 | */
	pmullw (const_11), %mm7;/* mm7 = |-tmp12 |-tmp12 | tmp12 |       | */
	paddw %mm6, %mm1;
	paddw %mm5, %mm1;
	paddw %mm7, %mm1;	/* mm1 = |  tmp5 |  tmp4 |  tmp6 |  tmp7 | */

	movq %mm0, %mm2;
	paddw %mm1, %mm0;	/* mm0 = |  o[2] |  o[4] |  o[1] |  o[0] | */
	psubw %mm1, %mm2;	/* mm2 = |  o[5] |  o[3] |  o[6] |  o[7] | */
	psraw $5, %mm0;
	psraw $5, %mm2;
	paddw (const_12), %mm0;
	paddw (const_12), %mm2;
	movq %mm0, %mm1;
	movq %mm0, %mm3;
	psrlq $16, %mm1;	/* mm1 = |       |  o[2] |               | */
	movq %mm2, %mm4;
	punpckhwd %mm2, %mm1;	/* mm1 = |               |  o[3] |  o[2] | */
	movq %mm2, %mm5;
	punpckldq %mm1, %mm0;	/* mm0 = |  o[3] |  o[2] |  o[1] |  o[0] | */
	psrlq $16, %mm4;	/* mm4 = |       |  o[5] |               | */
	punpckhwd %mm4, %mm3;	/* mm3 = |               |  o[5] |  o[4] | */
	psrlq $16, %mm5;	/* mm5 = |                          o[6] | */
	punpcklwd %mm2, %mm5;	/* mm5 = |               |  o[7] |  o[6] | */
	punpckldq %mm5, %mm3;	/* mm3 = |  o[7] |  o[6] |  o[5] |  o[4] | */
	movq %mm0, (%edi);
	movq %mm3, 8(%edi);
	addl $16, %esi;
	addl $16, %edi;
	decl %ecx;
	jnz pass_2;
	
	emms;
	
	popl %ebx;
	popl %esi;
	popl %edi;
	popl %ebp;
	ret;


#endif
