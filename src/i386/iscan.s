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
 *      Polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *      Uses MMX instructions when possible, but these will be #ifdef'ed
 *      out if your assembler doesn't support them.
 *
 *      By Calin Andrian.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"


.text



/* all these functions share the same parameters */

#define ADDR      ARG1
#define W         ARG2
#define INFO      ARG3

#define MMX_REGS(src, dst)      .byte 0xc0 + 8 * dst + src
#define MMX_REGM(src, dst)                                            \
   .byte 5 + 8 * dst                                                ; \
   .long src

/* pand is broken in GAS 2.8.1 */
#define PAND_R(src, dst)                                              \
   .byte 0x0f, 0xdb                                                 ; \
   MMX_REGS(src, dst)

#define PAND_M(src, dst)                                              \
   .byte 0x0f, 0xdb                                                 ; \
   MMX_REGM(src, dst)

/* Macros for 3Dnow! opcodes */
#define OP3D(src, dst, suf)                                           \
   .byte 0x0f, 0x0f                                                 ; \
   MMX_REGS(src, dst)                                               ; \
   .byte suf

#define FEMMS_R()                   .byte 0x0f, 0x0e
#define PAVGUSB_R(src, dst)         OP3D(src, dst, 0xbf)
#define PF2ID_R(src, dst)           OP3D(src, dst, 0x1d)
#define PFACC_R(src, dst)           OP3D(src, dst, 0xae)
#define PFADD_R(src, dst)           OP3D(src, dst, 0x9e)
#define PFCMPEQ_R(src, dst)         OP3D(src, dst, 0xb0)
#define PFCMPGE_R(src, dst)         OP3D(src, dst, 0x90)
#define PFCMPGT_R(src, dst)         OP3D(src, dst, 0xa0)
#define PFMAX_R(src, dst)           OP3D(src, dst, 0xa4)
#define PFMIN_R(src, dst)           OP3D(src, dst, 0x94)
#define PFMUL_R(src, dst)           OP3D(src, dst, 0xb4)
#define PFRCP_R(src, dst)           OP3D(src, dst, 0x96)
#define PFRCPIT1_R(src, dst)        OP3D(src, dst, 0xa6)
#define PFRCPIT2_R(src, dst)        OP3D(src, dst, 0xb6)
#define PFRSQIT1_R(src, dst)        OP3D(src, dst, 0xa7)
#define PFRSQRT_R(src, dst)         OP3D(src, dst, 0x97)
#define PFSUB_R(src, dst)           OP3D(src, dst, 0x9a)
#define PFSUBR_R(src, dst)          OP3D(src, dst, 0xaa)
#define PI2FD_R(src, dst)           OP3D(src, dst, 0x0d)
#define PMULHRW_R(src, dst)         OP3D(src, dst, 0xb7)



#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_gcol8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_gcol8)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   movl INFO, %edi               /* load registers */
   movl POLYSEG_C(%edi), %edx
   movl POLYSEG_DC(%edi), %esi
   movl W, %ecx
   movl ADDR, %edi

   _align_
gcol_odd_byte_loop:
   testl $3, %edi                /* deal with any odd pixels */
   jz gcol_no_odd_bytes

   movl %edx, %eax 
   shrl $16, %eax
   movb %al, FSEG(%edi)
   addl %esi, %edx
   incl %edi
   decl %ecx
   jg gcol_odd_byte_loop

   _align_
gcol_no_odd_bytes:
   movl %ecx, W                  /* inner loop expanded 4 times */
   shrl $2, %ecx 
   jz gcol_cleanup_leftovers

   _align_
gcol_loop:
   movl %edx, %eax               /* pixel 1 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bl

   movl %edx, %eax               /* pixel 2 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bh

   shll $16, %ebx

   movl %edx, %eax               /* pixel 3 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bl

   movl %edx, %eax               /* pixel 4 */
   shrl $16, %eax
   addl %esi, %edx
   movb %al, %bh

   roll $16, %ebx
   movl %ebx, FSEG(%edi)         /* write four pixels at a time */
   addl $4, %edi
   decl %ecx
   jg gcol_loop

gcol_cleanup_leftovers:
   movl W, %ecx                  /* deal with any leftover pixels */
   andl $3, %ecx
   jz gcol_done

   _align_
gcol_leftover_byte_loop:
   movl %edx, %eax 
   shrl $16, %eax
   movb %al, FSEG(%edi)
   addl %esi, %edx
   incl %edi
   decl %ecx
   jg gcol_leftover_byte_loop

gcol_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_gcol8() */




/* void _poly_scanline_grgb8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb8)
   pushl %ebp
   movl %esp, %ebp
   subl $12, %esp                /* three local variables: */

#define DB     -4(%ebp)
#define DG     -8(%ebp)
#define DR     -12(%ebp)

   pushl %ebx
   pushl %esi
   pushl %edi
grgb_entry:
   movl INFO, %esi               /* load registers */

   movl POLYSEG_DR(%esi), %eax
   movl POLYSEG_DG(%esi), %ebx
   movl POLYSEG_DB(%esi), %ecx

   movl %eax, DR
   movl %ebx, DG
   movl %ecx, DB

   movl POLYSEG_R(%esi), %ebx
   movl POLYSEG_G(%esi), %ecx
   movl POLYSEG_B(%esi), %edx

   movl ADDR, %edi
   decl %edi

   _align_
grgb_loop:
   movl %ecx, %eax               /* green */
   movl %ebx, %esi               /* red */
   shrl $19, %eax                /* green */
   shrl $19, %esi                /* red */
   shll $5, %eax                 /* green */
   shll $10, %esi                /* red */
   addl DR, %ebx
   orl %eax, %esi                /* green */
   movl %edx, %eax               /* blue */
   addl DG, %ecx
   shrl $19, %eax                /* blue */
   addl DB, %edx
   orl %eax, %esi                /* blue */

   movl GLOBL(rgb_map), %eax     /* table lookup */
   movb (%eax, %esi), %al
   incl %edi
   movb %al, FSEG(%edi)          /* write the pixel */
   decl W
   jg grgb_loop

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_grgb8() */

#undef DR
#undef DG
#undef DB

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_grgb8x)
   pushl %ebp
   movl %esp, %ebp
   subl $8, %esp

#define TMP4   -4(%ebp)
#define TMP    -8(%ebp)

   pushl %ebx
   pushl %esi
   pushl %edi

   cmpl $4, W
   jl grgb_entry

   movl INFO, %esi               /* load registers */

   movl POLYSEG_R(%esi), %eax
   movl %eax, TMP
   movl POLYSEG_DR(%esi), %edi
   addl %edi, %eax
   movl %eax, TMP4
   shll $1, %edi
   movq TMP, %mm0
   movd %edi, %mm1
   punpckldq %mm1, %mm1

   movl POLYSEG_G(%esi), %eax
   movl %eax, TMP
   movl POLYSEG_DG(%esi), %edi
   addl %edi, %eax
   movl %eax, TMP4
   shll $1, %edi
   movq TMP, %mm2
   movd %edi, %mm3
   punpckldq %mm3, %mm3

   movl POLYSEG_B(%esi), %eax
   movl %eax, TMP
   movl POLYSEG_DB(%esi), %edi
   addl %edi, %eax
   movl %eax, TMP4
   shll $1, %edi
   movq TMP, %mm4
   movd %edi, %mm5
   punpckldq %mm5, %mm5

   movl GLOBL(rgb_map), %esi
   movl ADDR, %edi
   movl W, %ecx

   _align_
grgb_loop_x:
   movq %mm0, %mm7              /* red */
   movq %mm2, %mm6              /* green */
   psrld $19, %mm7
   psrld $19, %mm6
   pslld $10, %mm7
   pslld $5, %mm6
   por %mm6, %mm7
   movq %mm4, %mm6              /* blue */
   psrld $19, %mm6
   por %mm6, %mm7

   movd %mm7, %edx
   movb (%esi, %edx), %al        /* table lookup */

   subl $2, %ecx
   jl grgb_last_x

   psrlq $32, %mm7
   movd %mm7, %edx
   movb (%esi, %edx), %ah        /* table lookup */

   paddd %mm1, %mm0
   paddd %mm3, %mm2
   paddd %mm5, %mm4

   movw %ax, FSEG(%edi)          /* write two pixels */
   leal 2(%edi), %edi
   jg grgb_loop_x

grgb_done_x:
   emms
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_grgb8x() */
   _align_
grgb_last_x:
   movb %al, FSEG(%edi)          /* write last pixel */
   jmp grgb_done_x

#undef TMP
#undef TMP4
#endif /* MMX */
#endif /* COLOR8 */



#define DU        -4(%ebp)
#define DV        -8(%ebp)
#define UMASK     -12(%ebp)
#define VMASK     -16(%ebp)
#define VSHIFT    -20(%ebp)
#define TMP       -24(%ebp)
#define COUNT     -28(%ebp)
#define C         -32(%ebp)
#define DC        -36(%ebp)
#define TEXTURE   -40(%ebp)

/* helper for setting up an affine texture mapping operation */
#define INIT_ATEX(extra...)                                                  \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $40, %esp                /* ten local variables */                 ; \
									   ; \
   pushl %ebx                                                              ; \
   pushl %esi                                                              ; \
   pushl %edi                                                              ; \
									   ; \
   movl INFO, %esi               /* load registers */                      ; \
									   ; \
   extra                                                                   ; \
									   ; \
   movl POLYSEG_DU(%esi), %eax                                             ; \
   movl POLYSEG_DV(%esi), %ebx                                             ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                         ; \
   movl POLYSEG_UMASK(%esi), %edx                                          ; \
   movl POLYSEG_VMASK(%esi), %edi                                          ; \
									   ; \
   shll %cl, %edi                /* adjust v mask and shift value */       ; \
   negl %ecx                                                               ; \
   addl $16, %ecx                                                          ; \
									   ; \
   movl %eax, DU                                                           ; \
   movl %ebx, DV                                                           ; \
   movl %ecx, VSHIFT                                                       ; \
   movl %edx, UMASK                                                        ; \
   movl %edi, VMASK                                                        ; \
									   ; \
   movl POLYSEG_U(%esi), %ebx                                              ; \
   movl POLYSEG_V(%esi), %edx                                              ; \
									   ; \
   movl ADDR, %edi                                                         ; \
   movl POLYSEG_TEXTURE(%esi), %esi




#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_atex8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex8)
   INIT_ATEX()

   _align_
atex_odd_byte_loop:
   testl $3, %edi                /* deal with any odd pixels */
   jz atex_no_odd_bytes

   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %al        /* read texel */
   movb %al, FSEG(%edi)          /* write the pixel */
   incl %edi
   decl W
   jg atex_odd_byte_loop

   _align_
atex_no_odd_bytes:
   movl W, %ecx                  /* inner loop expanded 4 times */
   movl %ecx, COUNT
   shrl $2, %ecx 
   jz atex_cleanup_leftovers
   movl %ecx, W

   _align_
atex_loop:
   /* pixel 1 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, TMP

   /* pixel 2 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, 1+TMP

   /* pixel 3 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, 2+TMP

   /* pixel 4 */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, 3+TMP

   movl TMP, %eax                /* write four pixels */
   movl %eax, FSEG(%edi) 
   addl $4, %edi
   decl W
   jg atex_loop

atex_cleanup_leftovers:
   movl COUNT, %ecx              /* deal with any leftover pixels */
   andl $3, %ecx
   jz atex_done
   movl %ecx, COUNT

   _align_
atex_leftover_byte_loop:
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %al        /* read texel */
   movb %al, FSEG(%edi)          /* write the pixel */
   incl %edi
   decl COUNT
   jg atex_leftover_byte_loop

atex_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex8() */




/* void _poly_scanline_atex_mask8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask8)
   INIT_ATEX()

   _align_
atex_mask_loop:
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %al        /* read texel */
   orb %al, %al
   jz atex_mask_skip

   movb %al, FSEG(%edi)          /* write solid pixels */
atex_mask_skip:
   addl DU, %ebx
   addl DV, %edx
   incl %edi
   decl W
   jg atex_mask_loop

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex_mask8() */




/* void _poly_scanline_atex_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit8)
   INIT_ATEX(
      movl POLYSEG_C(%esi), %eax
      movl POLYSEG_DC(%esi), %ebx
      sarl $8, %eax
      sarl $8, %ebx
      movl %eax, C
      movl %ebx, DC
   )

   movl %esi, TEXTURE

   _align_
atex_lit_odd_byte_loop:
   testl $3, %edi                /* deal with any odd pixels */
   jz atex_lit_no_odd_bytes

   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movl TEXTURE, %esi            /* read texel */
   movzbl (%esi, %eax), %eax 

   movb 1+C, %ah                 /* lookup in lighting table */
   movl GLOBL(color_map), %esi
   movl DC, %ecx
   movb (%eax, %esi), %al
   addl %ecx, C

   movb %al, FSEG(%edi)          /* write the pixel */
   incl %edi
   decl W
   jg atex_lit_odd_byte_loop

   _align_
atex_lit_no_odd_bytes:
   movl W, %ecx                  /* inner loop expanded 4 times */
   movl %ecx, COUNT
   shrl $2, %ecx 
   jz atex_lit_cleanup_leftovers
   movl %ecx, W

   _align_
atex_lit_loop:
   movl TEXTURE, %esi            /* read four texels */

   /* pixel 1 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, TMP

   /* pixel 2 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, 1+TMP

   /* pixel 3 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, 2+TMP

   /* pixel 4 texture */
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movb (%esi, %eax), %cl        /* read texel */
   movb %cl, 3+TMP

   movl GLOBL(color_map), %esi   /* light four pixels */
   xorl %ecx, %ecx
   pushl %ebx
   pushl %edx
   movl DC, %ebx
   movl C, %edx

   movb %dh, %ch                 /* light pixel 1 */
   movb TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %al

   movb %dh, %ch                 /* light pixel 2 */
   movb 1+TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %ah

   roll $16, %eax

   movb %dh, %ch                 /* light pixel 3 */
   movb 2+TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %al

   movb %dh, %ch                 /* light pixel 4 */
   movb 3+TMP, %cl
   addl %ebx, %edx
   movb (%ecx, %esi), %ah

   movl %edx, C
   roll $16, %eax
   popl %edx
   popl %ebx
   movl %eax, FSEG(%edi)         /* write four pixels at a time */
   addl $4, %edi
   decl W
   jg atex_lit_loop

atex_lit_cleanup_leftovers:
   movl COUNT, %ecx              /* deal with any leftover pixels */
   andl $3, %ecx
   jz atex_lit_done
   movl %ecx, COUNT

   _align_
atex_lit_leftover_byte_loop:
   movl %edx, %eax               /* get v */ 
   movb VSHIFT, %cl
   sarl %cl, %eax
   addl DV, %edx
   andl VMASK, %eax

   movl %ebx, %ecx               /* get u */
   sarl $16, %ecx
   addl DU, %ebx
   andl UMASK, %ecx
   addl %ecx, %eax

   movl TEXTURE, %esi            /* read texel */
   movzbl (%esi, %eax), %eax 

   movb 1+C, %ah                 /* lookup in lighting table */
   movl GLOBL(color_map), %esi
   movl DC, %ecx
   movb (%eax, %esi), %al
   addl %ecx, C

   movb %al, FSEG(%edi)          /* write the pixel */
   incl %edi
   decl COUNT
   jg atex_lit_leftover_byte_loop

atex_lit_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex_lit8() */
#endif /* COLOR8 */



#undef DU
#undef DV
#undef UMASK
#undef VMASK
#undef VSHIFT
#undef TMP
#undef COUNT
#undef C
#undef DC
#undef TEXTURE
#undef INIT_ATEX




#define TMP       -4(%ebp)
#define UMASK     -8(%ebp)
#define VMASK     -12(%ebp)
#define VSHIFT    -16(%ebp)
#define DU1       -20(%ebp)
#define DV1       -24(%ebp)
#define DZ1       -28(%ebp)
#define DU4       -32(%ebp)
#define DV4       -36(%ebp)
#define DZ4       -40(%ebp)
#define UDIFF     -44(%ebp)
#define VDIFF     -48(%ebp)
#define PREVU     -52(%ebp)
#define PREVV     -56(%ebp)
#define COUNT     -60(%ebp)
#define C         -64(%ebp)
#define DC        -68(%ebp)
#define TEXTURE   -72(%ebp)




/* helper for starting an fpu 1/z division */
#define START_FP_DIV()                                                       \
   fld1                                                                    ; \
   fdiv %st(3), %st(0)



/* helper for ending an fpu division, returning corrected u and v values */
#define END_FP_DIV(ureg, vreg)                                               \
   fld %st(0)                    /* duplicate the 1/z value */             ; \
									   ; \
   fmul %st(3), %st(0)           /* divide u by z */                       ; \
   fxch %st(1)                                                             ; \
									   ; \
   fmul %st(2), %st(0)           /* divide v by z */                       ; \
									   ; \
   fistpl TMP                    /* store v */                             ; \
   movl TMP, vreg                                                          ; \
									   ; \
   fistpl TMP                    /* store u */                             ; \
   movl TMP, ureg



/* helper for updating the u, v, and z values */
#define UPDATE_FP_POS(n)                                                     \
   fadds DV##n                   /* update v coord */                      ; \
   fxch %st(1)                   /* swap vuz stack to uvz */               ; \
									   ; \
   fadds DU##n                   /* update u coord */                      ; \
   fxch %st(2)                   /* swap uvz stack to zvu */               ; \
									   ; \
   fadds DZ##n                   /* update z value */                      ; \
   fxch %st(2)                   /* swap zvu stack to uvz */               ; \
   fxch %st(1)                   /* swap uvz stack back to vuz */




/* main body of the perspective-correct texture mapping routine */
#define DO_PTEX(name)                                                        \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $72, %esp                /* eighteen local variables */            ; \
									   ; \
   pushl %ebx                                                              ; \
   pushl %esi                                                              ; \
   pushl %edi                                                              ; \
									   ; \
   movl INFO, %esi               /* load registers */                      ; \
									   ; \
   flds POLYSEG_Z(%esi)          /* z at bottom of fpu stack */            ; \
   flds POLYSEG_FU(%esi)         /* followed by u */                       ; \
   flds POLYSEG_FV(%esi)         /* followed by v */                       ; \
									   ; \
   flds POLYSEG_DFU(%esi)        /* multiply diffs by four */              ; \
   flds POLYSEG_DFV(%esi)                                                  ; \
   flds POLYSEG_DZ(%esi)                                                   ; \
   fxch %st(2)                   /* u v z */                               ; \
   fadd %st(0), %st(0)           /* 2u v z */                              ; \
   fxch %st(1)                   /* v 2u z */                              ; \
   fadd %st(0), %st(0)           /* 2v 2u z */                             ; \
   fxch %st(2)                   /* z 2u 2v */                             ; \
   fadd %st(0), %st(0)           /* 2z 2u 2v */                            ; \
   fxch %st(1)                   /* 2u 2z 2v */                            ; \
   fadd %st(0), %st(0)           /* 4u 2z 2v */                            ; \
   fxch %st(2)                   /* 2v 2z 4u */                            ; \
   fadd %st(0), %st(0)           /* 4v 2z 4u */                            ; \
   fxch %st(1)                   /* 2z 4v 4u */                            ; \
   fadd %st(0), %st(0)           /* 4z 4v 4u */                            ; \
   fxch %st(2)                   /* 4u 4v 4z */                            ; \
   fstps DU4                                                               ; \
   fstps DV4                                                               ; \
   fstps DZ4                                                               ; \
									   ; \
   START_FP_DIV()                /* prime the 1/z calculation */           ; \
									   ; \
   movl POLYSEG_DFU(%esi), %eax  /* copy diff values onto the stack */     ; \
   movl POLYSEG_DFV(%esi), %ebx                                            ; \
   movl POLYSEG_DZ(%esi), %ecx                                             ; \
   movl %eax, DU1                                                          ; \
   movl %ebx, DV1                                                          ; \
   movl %ecx, DZ1                                                          ; \
									   ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                         ; \
   movl POLYSEG_VMASK(%esi), %edx                                          ; \
   movl POLYSEG_UMASK(%esi), %eax                                          ; \
									   ; \
   shll %cl, %edx                /* adjust v mask and shift value */       ; \
   negl %ecx                                                               ; \
   movl %eax, UMASK                                                        ; \
   addl $16, %ecx                                                          ; \
   movl %edx, VMASK                                                        ; \
   movl %ecx, VSHIFT                                                       ; \
									   ; \
   INIT()                                                                  ; \
									   ; \
   movl ADDR, %edi                                                         ; \
									   ; \
   _align_                                                                 ; \
name##_odd_byte_loop:                                                      ; \
   testl $3, %edi                /* deal with any odd pixels */            ; \
   jz name##_no_odd_bytes                                                  ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movb VSHIFT, %cl              /* shift and mask v coordinate */         ; \
   sarl %cl, %edx                                                          ; \
   andl VMASK, %edx                                                        ; \
									   ; \
   sarl $16, %ebx                /* shift and mask u coordinate */         ; \
   andl UMASK, %ebx                                                        ; \
   addl %ebx, %edx                                                         ; \
									   ; \
   SINGLE_PIXEL(1)               /* process the pixel */                   ; \
									   ; \
   decl W                                                                  ; \
   jg name##_odd_byte_loop                                                 ; \
									   ; \
   _align_                                                                 ; \
name##_no_odd_bytes:                                                       ; \
   movl W, %ecx                  /* inner loop expanded 4 times */         ; \
   movl %ecx, COUNT                                                        ; \
   shrl $2, %ecx                                                           ; \
   jg name##_prime_x4                                                      ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl %ebx, PREVU              /* store initial u/v pos */               ; \
   movl %edx, PREVV                                                        ; \
									   ; \
   jmp name##_cleanup_leftovers                                            ; \
									   ; \
   _align_                                                                 ; \
name##_prime_x4:                                                           ; \
   movl %ecx, W                                                            ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(4)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl %ebx, PREVU              /* store initial u/v pos */               ; \
   movl %edx, PREVV                                                        ; \
									   ; \
   jmp name##_start_loop                                                   ; \
									   ; \
   _align_                                                                 ; \
name##_last_time:                                                          ; \
   END_FP_DIV(%eax, %ecx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   jmp name##_loop_contents                                                ; \
									   ; \
   _align_                                                                 ; \
name##_loop:                                                               ; \
   END_FP_DIV(%eax, %ecx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(4)              /* update u/v/z values */                 ; \
									   ; \
name##_loop_contents:                                                      ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl PREVU, %ebx              /* read the previous u/v pos */           ; \
   movl PREVV, %edx                                                        ; \
									   ; \
   movl %eax, PREVU              /* store u/v for the next cycle */        ; \
   movl %ecx, PREVV                                                        ; \
									   ; \
   subl %ebx, %eax               /* get u/v diffs for the four pixels */   ; \
   subl %edx, %ecx                                                         ; \
   sarl $2, %eax                                                           ; \
   sarl $2, %ecx                                                           ; \
   movl %eax, UDIFF                                                        ; \
   movl %ecx, VDIFF                                                        ; \
									   ; \
   PREPARE_FOUR_PIXELS()                                                   ; \
									   ; \
   /* pixel 1 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(0)                /* process pixel 1 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 2 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(1)                /* process pixel 2 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 3 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(2)                /* process pixel 3 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 4 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(3)                /* process pixel 4 */                     ; \
									   ; \
   WRITE_FOUR_PIXELS()           /* write four pixels at a time */         ; \
									   ; \
name##_start_loop:                                                         ; \
   decl W                                                                  ; \
   jg name##_loop                                                          ; \
   jz name##_last_time                                                     ; \
									   ; \
name##_cleanup_leftovers:                                                  ; \
   movl COUNT, %ecx              /* deal with any leftover pixels */       ; \
   andl $3, %ecx                                                           ; \
   jz name##_done                                                          ; \
									   ; \
   movl %ecx, COUNT                                                        ; \
   movl PREVU, %ebx                                                        ; \
   movl PREVV, %edx                                                        ; \
									   ; \
   _align_                                                                 ; \
name##_cleanup_loop:                                                       ; \
   movb VSHIFT, %cl              /* shift and mask v coordinate */         ; \
   sarl %cl, %edx                                                          ; \
   andl VMASK, %edx                                                        ; \
									   ; \
   sarl $16, %ebx                /* shift and mask u coordinate */         ; \
   andl UMASK, %ebx                                                        ; \
   addl %ebx, %edx                                                         ; \
									   ; \
   SINGLE_PIXEL(2)               /* process the pixel */                   ; \
									   ; \
   decl COUNT                                                              ; \
   jz name##_done                                                          ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
   jmp name##_cleanup_loop                                                 ; \
									   ; \
name##_done:                                                               ; \
   fstp %st(0)                   /* pop fpu stack */                       ; \
   fstp %st(0)                                                             ; \
   fstp %st(0)                                                             ; \
   fstp %st(0)                                                             ; \
									   ; \
   popl %edi                                                               ; \
   popl %esi                                                               ; \
   popl %ebx                                                               ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp




#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_ptex8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex8)

   #define INIT()                                                            \
      movl POLYSEG_TEXTURE(%esi), %esi


   #define SINGLE_PIXEL(n)                                                   \
      movb (%esi, %edx), %al     /* read texel */                          ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      /* noop */


   #define FOUR_PIXELS(n)                                                    \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      movb %al, n+TMP            /* store the pixel */


   #define WRITE_FOUR_PIXELS()                                               \
      movl TMP, %eax             /* write four pixels at a time */         ; \
      movl %eax, FSEG(%edi)                                                ; \
      addl $4, %edi


   DO_PTEX(ptex)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELS

   ret                           /* end of _poly_scanline_ptex8() */




/* void _poly_scanline_ptex_mask8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask8)

   #define INIT()                                                            \
      movl POLYSEG_TEXTURE(%esi), %esi


   #define SINGLE_PIXEL(n)                                                   \
      movb (%esi, %edx), %al     /* read texel */                          ; \
      orb %al, %al                                                         ; \
      jz ptex_mask_skip_left_##n                                           ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
   ptex_mask_skip_left_##n:                                                ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      /* noop */


   #define FOUR_PIXELS(n)                                                    \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      orb %al, %al                                                         ; \
      jz ptex_mask_skip_##n                                                ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
   ptex_mask_skip_##n:                                                     ; \
      incl %edi


   #define WRITE_FOUR_PIXELS()                                               \
      /* noop */


   DO_PTEX(ptex_mask)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELS

   ret                           /* end of _poly_scanline_ptex_mask8() */



/* void _poly_scanline_ptex_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit8)

   #define INIT()                                                            \
      movl POLYSEG_C(%esi), %eax                                           ; \
      movl POLYSEG_DC(%esi), %ebx                                          ; \
      movl POLYSEG_TEXTURE(%esi), %ecx                                     ; \
      sarl $8, %eax                                                        ; \
      sarl $8, %ebx                                                        ; \
      movl %eax, C                                                         ; \
      movl %ebx, DC                                                        ; \
      movl %ecx, TEXTURE


   #define SINGLE_PIXEL(n)                                                   \
      movl TEXTURE, %esi         /* read texel */                          ; \
      movzbl (%esi, %edx), %eax                                            ; \
									   ; \
      movb 1+C, %ah              /* lookup in lighting table */            ; \
      movl GLOBL(color_map), %esi                                          ; \
      movl DC, %ecx                                                        ; \
      movb (%eax, %esi), %al                                               ; \
      addl %ecx, C                                                         ; \
									   ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      movl TEXTURE, %esi


   #define FOUR_PIXELS(n)                                                    \
      movl TEXTURE, %esi                                                   ; \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      movb %al, n+TMP            /* store the pixel */


   #define WRITE_FOUR_PIXELS()                                               \
      movl GLOBL(color_map), %esi   /* light four pixels */                ; \
      xorl %ecx, %ecx                                                      ; \
      movl DC, %ebx                                                        ; \
      movl C, %edx                                                         ; \
									   ; \
      movb %dh, %ch              /* light pixel 1 */                       ; \
      movb TMP, %cl                                                        ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %al                                               ; \
									   ; \
      movb %dh, %ch              /* light pixel 2 */                       ; \
      movb 1+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %ah                                               ; \
									   ; \
      roll $16, %eax                                                       ; \
									   ; \
      movb %dh, %ch              /* light pixel 3 */                       ; \
      movb 2+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %al                                               ; \
									   ; \
      movb %dh, %ch              /* light pixel 4 */                       ; \
      movb 3+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %ah                                               ; \
									   ; \
      movl %edx, C                                                         ; \
      roll $16, %eax                                                       ; \
      movl %eax, FSEG(%edi)      /* write four pixels at a time */         ; \
      addl $4, %edi


   DO_PTEX(ptex_lit)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELSS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELSS

   ret                           /* end of _poly_scanline_ptex_lit8() */

#endif /* COLOR8 */
#undef TMP
#undef UMASK
#undef VMASK
#undef VSHIFT
#undef DU1
#undef DV1
#undef DZ1
#undef DU4
#undef DV4
#undef DZ4
#undef UDIFF
#undef VDIFF
#undef PREVU
#undef PREVV
#undef COUNT
#undef C
#undef DC
#undef TEXTURE

#undef START_FP_DIV
#undef END_FP_DIV
#undef UPDATE_FP_POS
#undef DO_PTEX



#define DB      -4(%ebp)
#define DG      -8(%ebp)
#define DR     -12(%ebp)

#define TMP4    -4(%ebp)
#define TMP     -8(%ebp)
#define SB     -16(%ebp)
#define SG     -24(%ebp)
#define SR     -32(%ebp)

/* first part of a grgb routine */
#define INIT_GRGB(depth, r_sft, g_sft, b_sft)                         \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $12, %esp                                                   ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
grgb_entry_##depth:                                                 ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   movl POLYSEG_DR(%esi), %ebx   /* move delta's on stack */        ; \
   movl POLYSEG_DG(%esi), %edi                                      ; \
   movl POLYSEG_DB(%esi), %edx                                      ; \
   movl %ebx, DR                                                    ; \
   movl %edi, DG                                                    ; \
   movl %edx, DB                                                    ; \
								    ; \
   movl POLYSEG_R(%esi), %ebx                                       ; \
   movl POLYSEG_G(%esi), %edi                                       ; \
   movl POLYSEG_B(%esi), %edx                                       ; \
								    ; \
   _align_                                                          ; \
grgb_loop_##depth:                                                  ; \
   movl %ebx, %eax              /* red */                           ; \
   movb GLOBL(_rgb_r_shift_##depth), %cl                            ; \
   shrl $r_sft, %eax                                                ; \
   addl DR, %ebx                                                    ; \
   shll %cl, %eax                                                   ; \
   movl %edi, %esi              /* green */                         ; \
   movb GLOBL(_rgb_g_shift_##depth), %cl                            ; \
   shrl $g_sft, %esi                                                ; \
   addl DG, %edi                                                    ; \
   shll %cl, %esi                                                   ; \
   orl %esi, %eax                                                   ; \
   movl %edx, %esi              /* blue */                          ; \
   movb GLOBL(_rgb_b_shift_##depth), %cl                            ; \
   shrl $b_sft, %esi                                                ; \
   addl DB, %edx                                                    ; \
   shll %cl, %esi                                                   ; \
   orl %esi, %eax                                                   ; \
   movl ADDR, %esi

/* end of grgb routine */
#define END_GRGB(depth)                                               \
   movl %esi, ADDR                                                  ; \
   decl W                                                           ; \
   jg grgb_loop_##depth                                             ; \
								    ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp

/* first part of MMX grgb routine - two for the price of one */
#define INIT_GRGB_MMX(depth, r_sft, g_sft, b_sft)                     \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $32, %esp                                                   ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   cmpl $4, W                                                       ; \
   jl grgb_entry_##depth                                            ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   movl POLYSEG_R(%esi), %eax                                       ; \
   movl %eax, TMP                                                   ; \
   movl POLYSEG_DR(%esi), %edi                                      ; \
   addl %edi, %eax                                                  ; \
   movl %eax, TMP4                                                  ; \
   shll $1, %edi                                                    ; \
   movq TMP, %mm0                                                   ; \
   movd %edi, %mm1                                                  ; \
   movd GLOBL(_rgb_r_shift_##depth), %mm7                           ; \
   punpckldq %mm1, %mm1                                             ; \
   movq %mm7, SR                                                    ; \
								    ; \
   movl POLYSEG_G(%esi), %eax                                       ; \
   movl %eax, TMP                                                   ; \
   movl POLYSEG_DG(%esi), %edi                                      ; \
   addl %edi, %eax                                                  ; \
   movl %eax, TMP4                                                  ; \
   shll $1, %edi                                                    ; \
   movq TMP, %mm2                                                   ; \
   movd %edi, %mm3                                                  ; \
   movd GLOBL(_rgb_g_shift_##depth), %mm7                           ; \
   punpckldq %mm3, %mm3                                             ; \
   movq %mm7, SG                                                    ; \
								    ; \
   movl POLYSEG_B(%esi), %eax                                       ; \
   movl %eax, TMP                                                   ; \
   movl POLYSEG_DB(%esi), %edi                                      ; \
   addl %edi, %eax                                                  ; \
   movl %eax, TMP4                                                  ; \
   shll $1, %edi                                                    ; \
   movq TMP, %mm4                                                   ; \
   movd %edi, %mm5                                                  ; \
   movd GLOBL(_rgb_b_shift_##depth), %mm7                           ; \
   punpckldq %mm5, %mm5                                             ; \
   movq %mm7, SB                                                    ; \
								    ; \
   movl ADDR, %edi                                                  ; \
   movl W, %ecx                                                     ; \
								    ; \
   _align_                                                          ; \
grgb_loop_x_##depth:                                                ; \
   movq %mm0, %mm7              /* red */                           ; \
   movq %mm2, %mm6              /* green */                         ; \
   psrld $r_sft, %mm7                                               ; \
   psrld $g_sft, %mm6                                               ; \
   pslld SR, %mm7                                                   ; \
   pslld SG, %mm6                                                   ; \
   por %mm6, %mm7                                                   ; \
   movq %mm4, %mm6              /* blue */                          ; \
   psrld $b_sft, %mm6                                               ; \
   pslld SB, %mm6                                                   ; \
   por %mm6, %mm7

/* end of grgb MMX routine */
#define END_GRGB_MMX(depth)                                           \
   paddd %mm1, %mm0                                                 ; \
   paddd %mm3, %mm2                                                 ; \
   paddd %mm5, %mm4                                                 ; \
   jg grgb_loop_x_##depth       /* watch out for flags */           ; \
99:                                                                 ; \
   emms                                                             ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_grgb15(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb15)
   INIT_GRGB(15, 19, 19, 19)
   movw %ax, FSEG(%esi)          /* write the pixel */
   addl $2, %esi
   END_GRGB(15)
   ret                           /* end of _poly_scanline_grgb15() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_grgb15x)
   INIT_GRGB_MMX(15, 19, 19, 19)
   subl $2, %ecx
   jl 7f                        /* from here on, flags don't change */
   packssdw %mm7, %mm7
   movd %mm7, FSEG(%edi)        /* write two pixels */
   leal 4(%edi), %edi
   END_GRGB_MMX(15)
   ret                           /* end of _poly_scanline_grgb15x() */
   _align_
7:
   movd %mm7, %eax
   movw %ax, FSEG(%edi)         /* write the pixel */
   jmp 99b
#endif /* MMX */



/* void _poly_scanline_grgb16(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb16)
   INIT_GRGB(16, 19, 18, 19)
   movw %ax, FSEG(%esi)          /* write the pixel */
   addl $2, %esi
   END_GRGB(16)
   ret                           /* end of _poly_scanline_grgb16() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_grgb16x)
   INIT_GRGB_MMX(16, 19, 18, 19)
   subl $2, %ecx
   jl 7f
   movd %mm7, %edx
   psrlq $16, %mm7
   movd %mm7, %eax
   movw %dx, %ax
   movl %eax, FSEG(%edi)        /* write two pixels */
   leal 4(%edi), %edi
   END_GRGB_MMX(16)
   ret                           /* end of _poly_scanline_grgb16x() */
   _align_
7:
   movd %mm7, %eax
   movw %ax, FSEG(%edi)         /* write the pixel */
   jmp 99b
#endif /* MMX */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_grgb32(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb32)
   INIT_GRGB(32, 16, 16, 16)
   movl %eax, FSEG(%esi)         /* write the pixel */
   addl $4, %esi
   END_GRGB(32)
   ret                           /* end of _poly_scanline_grgb32() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_grgb32x)
   INIT_GRGB_MMX(32, 16, 16, 16)
   subl $2, %ecx
   jl 7f
   movq %mm7, FSEG(%edi)         /* write two pixels */
   leal 8(%edi), %edi
   END_GRGB_MMX(32)
   ret                           /* end of _poly_scanline_grgb32x() */
   _align_
7:
   movd %mm7, FSEG(%edi)         /* write the pixel */
   jmp 99b
#endif /* MMX */
#endif /* COLOR32 */

#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_grgb24(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb24)
   INIT_GRGB(24, 16, 16, 16)
   movw %ax, FSEG(%esi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%esi)
   addl $3, %esi
   END_GRGB(24)
   ret                           /* end of _poly_scanline_grgb24() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_grgb24x)
   INIT_GRGB_MMX(24, 16, 16, 16)
   movd %mm7, %eax
   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   subl $2, %ecx
   jl 99f
   psrlq $32, %mm7
   movd %mm7, %eax
   movw %ax, FSEG 3(%edi)        /* write second pixel */
   shrl $16, %eax
   movb %al, FSEG 5(%edi)
   leal 6(%edi), %edi
   cmpl $0, %ecx
   END_GRGB_MMX(24)
   ret                           /* end of _poly_scanline_grgb24x() */
#endif /* MMX */
#endif /* COLOR24 */

#undef DR
#undef DG
#undef DB
#undef TMP
#undef TMP2
#undef TMP4
#undef TMP6
#undef SR
#undef SG
#undef SB




#define VMASK     -8(%ebp)
#define VSHIFT   -16(%ebp)
#define DV       -20(%ebp)
#define DU       -24(%ebp)
#define ALPHA4   -28(%ebp)
#define ALPHA2   -30(%ebp)
#define ALPHA    -32(%ebp)
#define DALPHA4  -36(%ebp)
#define DALPHA   -40(%ebp)
#define UMASK    -44(%ebp)

/* first part of an affine texture mapping operation */
#define INIT_ATEX(extra...)                                           \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $44, %esp                    /* local variables */          ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
   extra                                                            ; \
								    ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl POLYSEG_VMASK(%esi), %eax                                   ; \
   shll %cl, %eax           /* adjust v mask and shift value */     ; \
   negl %ecx                                                        ; \
   addl $16, %ecx                                                   ; \
   movl %ecx, VSHIFT                                                ; \
   movl %eax, VMASK                                                 ; \
								    ; \
   movl POLYSEG_UMASK(%esi), %eax                                   ; \
   movl POLYSEG_DU(%esi), %ebx                                      ; \
   movl POLYSEG_DV(%esi), %edx                                      ; \
   movl %eax, UMASK                                                 ; \
   movl %ebx, DU                                                    ; \
   movl %edx, DV                                                    ; \
								    ; \
   movl POLYSEG_U(%esi), %ebx                                       ; \
   movl POLYSEG_V(%esi), %edx                                       ; \
   movl ADDR, %edi                                                  ; \
   movl POLYSEG_TEXTURE(%esi), %esi                                 ; \
								    ; \
   _align_                                                          ; \
1:                                                                  ; \
   movl %edx, %eax               /* get v */                        ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx               /* get u */                        ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax

/* end of atex routine */
#define END_ATEX()                                                  ; \
   addl DU, %ebx                                                    ; \
   addl DV, %edx                                                    ; \
   decl W                                                           ; \
   jg 1b                                                            ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_atex16(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex16)
   INIT_ATEX()
   movw (%esi, %eax, 2), %ax     /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   addl $2, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex16() */

/* void _poly_scanline_atex_mask15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask15)
   INIT_ATEX()
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask15() */

/* void _poly_scanline_atex_mask16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask16)
   INIT_ATEX()
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask16() */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_atex32(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex32)
   INIT_ATEX()
   movl (%esi, %eax, 4), %eax    /* read texel */
   movl %eax, FSEG(%edi)         /* write the pixel */
   addl $4, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex32() */

/* void _poly_scanline_atex_mask32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask32)
   INIT_ATEX()
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   movl %eax, FSEG(%edi)         /* write solid pixels */
7: addl $4, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask32() */
#endif /* COLOR32 */

#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_atex24(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex24)
   INIT_ATEX()
   leal (%eax, %eax, 2), %ecx
   movw (%esi, %ecx), %ax        /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   movb 2(%esi, %ecx), %al       /* read texel */
   movb %al, FSEG 2(%edi)        /* write the pixel */
   addl $3, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex24() */

/* void _poly_scanline_atex_mask24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask24)
   INIT_ATEX()
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
7: addl $3, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask24() */
#endif /* COLOR24 */



/* helper for setting up ALPHA and DALPHA before MMX lit routines */
#define INIT_MMX_ALPHA()                                              \
   movd POLYSEG_C(%esi), %mm0                                       ; \
   movd POLYSEG_DC(%esi), %mm1                                      ; \
   psrad $9, %mm0             /* alpha precision = 15 bits */       ; \
   psrad $9, %mm1                                                   ; \
   punpcklwd %mm0, %mm0       /* double */                          ; \
   punpcklwd %mm1, %mm1                                             ; \
   punpckldq %mm0, %mm0       /* quad */                            ; \
   punpckldq %mm1, %mm1



/* void _poly_scanline_atex_mask_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit8)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 7f
   movb ALPHA2, %ah
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
7:
   movl DALPHA, %eax
   incl %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit8() */



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_atex_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit15)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax    /* read texel */
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)         /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit15() */

/* void _poly_scanline_atex_mask_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit15)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movw (%esi, %eax, 2), %ax    /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit15() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_atex_lit15x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     movq GLOBL(_mask_mmx_15), %mm7
     movl GLOBL(_blender_col_15), %eax
     movw %ax, ALPHA
     movw %ax, ALPHA2
     movb %ah, ALPHA4
     movq ALPHA, %mm6
     PAND_R(7, 6)
   )
   movw (%esi, %eax, 2), %ax
   movw %ax, ALPHA
   movw %ax, ALPHA2
   movb %ah, ALPHA4
   movq ALPHA, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   psubw %mm6, %mm2
   paddw %mm2, %mm2             /* this trick solves everything */
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   movq %mm2, ALPHA
   movd %mm2, %eax
   orw ALPHA2, %ax
   orb ALPHA4, %ah              /* %ax = RGB */

   paddw %mm1, %mm0
   movw %ax, FSEG(%edi)
   addl $2, %edi
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_lit15x() */

FUNC(_poly_scanline_atex_mask_lit15x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     movq GLOBL(_mask_mmx_15), %mm7
     movl GLOBL(_blender_col_15), %eax
     movw %ax, ALPHA
     movw %ax, ALPHA2
     movb %ah, ALPHA4
     movq ALPHA, %mm6
     PAND_R(7, 6)
   )
   movw (%esi, %eax, 2), %ax
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   movw %ax, ALPHA
   movw %ax, ALPHA2
   movb %ah, ALPHA4
   movq ALPHA, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   psubw %mm6, %mm2
   paddw %mm2, %mm2
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   movq %mm2, ALPHA
   movd %mm2, %eax
   orw ALPHA2, %ax
   orb ALPHA4, %ah              /* %ax = RGB */
   movw %ax, FSEG(%edi)
7:
   addl $2, %edi
   paddw %mm1, %mm0
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_mask_lit15x() */
#endif /* MMX */



/* void _poly_scanline_atex_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit16)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit16() */

/* void _poly_scanline_atex_mask_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit16)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit16() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_atex_lit16x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     movq GLOBL(_mask_mmx_16), %mm7
     movl GLOBL(_blender_col_16), %eax
     movw %ax, ALPHA
     movw %ax, ALPHA2
     movb %ah, ALPHA4
     movq ALPHA, %mm6
     PAND_R(7, 6)
   )
   movw (%esi, %eax, 2), %ax
   movw %ax, ALPHA
   movw %ax, ALPHA2
   movb %ah, ALPHA4
   movq ALPHA, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   psubw %mm6, %mm2
   paddw %mm2, %mm2
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   movq %mm2, ALPHA
   movd %mm2, %eax
   orw ALPHA2, %ax
   orb ALPHA4, %ah              /* %ax = RGB */

   paddw %mm1, %mm0
   movw %ax, FSEG(%edi)
   addl $2, %edi
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_lit16x() */

FUNC(_poly_scanline_atex_mask_lit16x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     movq GLOBL(_mask_mmx_16), %mm7
     movl GLOBL(_blender_col_16), %eax
     movw %ax, ALPHA
     movw %ax, ALPHA2
     movb %ah, ALPHA4
     movq ALPHA, %mm6
     PAND_R(7, 6)
   )
   movw (%esi, %eax, 2), %ax
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   movw %ax, ALPHA
   movw %ax, ALPHA2
   movb %ah, ALPHA4
   movq ALPHA, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   psubw %mm6, %mm2
   paddw %mm2, %mm2
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */

   movq %mm2, ALPHA
   movd %mm2, %eax
   orw ALPHA2, %ax
   orb ALPHA4, %ah              /* %ax = RGB */
   movw %ax, FSEG(%edi)
7:
   addl $2, %edi
   paddw %mm1, %mm0
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_mask_lit16x() */
#endif /* MMX */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_atex_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit32)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movl (%esi, %eax, 4), %eax    /* read texel */
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit32() */

/* void _poly_scanline_atex_mask_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit32)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit32() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_atex_lit32x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     pxor %mm7, %mm7
     movd GLOBL(_blender_col_32), %mm6
     punpcklbw %mm7, %mm6           /* extend RGB to words */
   )
   movd (%esi, %eax, 4), %mm2
   punpcklbw %mm7, %mm2             /* extend RGB to words */

   psubw %mm6, %mm2
   paddw %mm2, %mm2
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2

   packuswb %mm2, %mm2
   paddw %mm1, %mm0
   movd %mm2, FSEG(%edi)
   addl $4, %edi
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_lit32x() */

FUNC(_poly_scanline_atex_mask_lit32x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     pxor %mm7, %mm7
     movd GLOBL(_blender_col_32), %mm6
     punpcklbw %mm7, %mm6           /* extend RGB to words */
   )
   movl (%esi, %eax, 4), %eax
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   movd %eax, %mm2
   punpcklbw %mm7, %mm2             /* extend RGB to words */

   psubw %mm6, %mm2
   paddw %mm2, %mm2
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2

   packuswb %mm2, %mm2
   movd %mm2, FSEG(%edi)
7:
   addl $4, %edi
   paddw %mm1, %mm0
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_mask_lit32x() */
#endif /* MMX */
#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_atex_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit24)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   leal (%eax, %eax, 2), %ecx
   movb 2(%esi, %ecx), %al        /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit24() */

/* void _poly_scanline_atex_mask_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit24)
   INIT_ATEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
7:
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit24() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_atex_lit24x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     pxor %mm7, %mm7
     movd GLOBL(_blender_col_24), %mm6
     punpcklbw %mm7, %mm6
   )
   leal (%eax, %eax, 2), %ecx
   movb 2(%esi, %ecx), %al
   shll $16, %eax
   movw (%esi, %ecx), %ax
   movd %eax, %mm2
   punpcklbw %mm7, %mm2

   psubw %mm6, %mm2
   paddw %mm2, %mm2
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2

   packuswb %mm2, %mm2
   paddw %mm1, %mm0

   movd %mm2, %eax
   movw %ax, FSEG(%edi)
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   addl $3, %edi
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_lit24x() */

FUNC(_poly_scanline_atex_mask_lit24x)
   INIT_ATEX(
     INIT_MMX_ALPHA()
     pxor %mm7, %mm7
     movd GLOBL(_blender_col_24), %mm6
     punpcklbw %mm7, %mm6
   )
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   movd %eax, %mm2
   punpcklbw %mm7, %mm2

   psubw %mm6, %mm2
   paddw %mm2, %mm2
   pmulhw %mm0, %mm2
   paddw %mm6, %mm2

   packuswb %mm2, %mm2
   movd %mm2, %eax
   movw %ax, FSEG(%edi)
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
7:
   addl $3, %edi
   paddw %mm1, %mm0
   END_ATEX()
   emms
   ret                           /* end of _poly_scanline_atex_mask_lit24x() */
#endif /* MMX */
#endif /* COLOR24 */



#undef VMASK
#undef VSHIFT
#undef UMASK
#undef DU
#undef DV
#undef ALPHA
#undef ALPHA2
#undef ALPHA4
#undef DALPHA
#undef DALPHA4



#define VMASK     -4(%ebp)
#define VSHIFT    -8(%ebp)
#define ALPHA4   -12(%ebp)
#define ALPHA    -16(%ebp)
#define DALPHA4  -20(%ebp)
#define DALPHA   -24(%ebp)
#define TMP4     -28(%ebp)
#define TMP2     -30(%ebp)
#define TMP      -32(%ebp)
#define BLEND4   -36(%ebp)
#define BLEND2   -38(%ebp)
#define BLEND    -40(%ebp)
#define U1       -44(%ebp)
#define V1       -48(%ebp)
#define DU       -52(%ebp)
#define DV       -56(%ebp)
#define DU4      -60(%ebp)
#define DV4      -64(%ebp)
#define DZ4      -68(%ebp)
#define UMASK    -72(%ebp)
#define COUNT    -76(%ebp)
#define SM       -80(%ebp)
#define SV       -84(%ebp)
#define SU       -88(%ebp)
#define SZ       -92(%ebp)

/* helper for starting an fpu 1/z division */
#define START_FP_DIV()                                                \
   fld1                                                             ; \
   fdiv %st(3), %st(0)

/* helper for ending an fpu division, returning corrected u and v values */
#define END_FP_DIV()                                                  \
   fld %st(0)                    /* duplicate the 1/z value */      ; \
   fmul %st(3), %st(0)           /* divide u by z */                ; \
   fxch %st(1)                                                      ; \
   fmul %st(2), %st(0)           /* divide v by z */

#define UPDATE_FP_POS_4()                                             \
   fadds DV4                     /* update v coord */               ; \
   fxch %st(1)                   /* swap vuz stack to uvz */        ; \
   fadds DU4                     /* update u coord */               ; \
   fxch %st(2)                   /* swap uvz stack to zvu */        ; \
   fadds DZ4                     /* update z value */               ; \
   fxch %st(2)                   /* swap zvu stack to uvz */        ; \
   fxch %st(1)                   /* swap uvz stack back to vuz */

/* main body of the perspective-correct texture mapping routine */
#define INIT_PTEX(extra...)                                           \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $100, %esp               /* local variables */              ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   flds POLYSEG_Z(%esi)          /* z at bottom of fpu stack */     ; \
   flds POLYSEG_FU(%esi)         /* followed by u */                ; \
   flds POLYSEG_FV(%esi)         /* followed by v */                ; \
								    ; \
   flds POLYSEG_DFU(%esi)        /* multiply diffs by four */       ; \
   flds POLYSEG_DFV(%esi)                                           ; \
   flds POLYSEG_DZ(%esi)                                            ; \
   fxch %st(2)                   /* u v z */                        ; \
   fadd %st(0), %st(0)           /* 2u v z */                       ; \
   fxch %st(1)                   /* v 2u z */                       ; \
   fadd %st(0), %st(0)           /* 2v 2u z */                      ; \
   fxch %st(2)                   /* z 2u 2v */                      ; \
   fadd %st(0), %st(0)           /* 2z 2u 2v */                     ; \
   fxch %st(1)                   /* 2u 2z 2v */                     ; \
   fadd %st(0), %st(0)           /* 4u 2z 2v */                     ; \
   fxch %st(2)                   /* 2v 2z 4u */                     ; \
   fadd %st(0), %st(0)           /* 4v 2z 4u */                     ; \
   fxch %st(1)                   /* 2z 4v 4u */                     ; \
   fadd %st(0), %st(0)           /* 4z 4v 4u */                     ; \
   fxch %st(2)                   /* 4u 4v 4z */                     ; \
   fstps DU4                                                        ; \
   fstps DV4                                                        ; \
   fstps DZ4                                                        ; \
								    ; \
   START_FP_DIV()                                                   ; \
								    ; \
   extra                                                            ; \
								    ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl POLYSEG_VMASK(%esi), %eax                                   ; \
   movl POLYSEG_UMASK(%esi), %edx                                   ; \
   shll %cl, %eax           /* adjust v mask and shift value */     ; \
   negl %ecx                                                        ; \
   addl $16, %ecx                                                   ; \
   movl %ecx, VSHIFT                                                ; \
   movl %eax, VMASK                                                 ; \
   movl %edx, UMASK                                                 ; \
								    ; \
   END_FP_DIV()                                                     ; \
   fistpl V1                     /* store v */                      ; \
   fistpl U1                     /* store u */                      ; \
   UPDATE_FP_POS_4()                                                ; \
   START_FP_DIV()                                                   ; \
								    ; \
   movl ADDR, %edi                                                  ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
   movl POLYSEG_TEXTURE(%esi), %esi                                 ; \
   movl $0, COUNT                /* COUNT ranges from 3 to -1 */    ; \
   jmp 3f                                                           ; \
								    ; \
   _align_                                                          ; \
1:                  /* step 2: compute DU, DV for next 4 steps */   ; \
   movl $3, COUNT                                                   ; \
								    ; \
   END_FP_DIV()                 /* finish the divide */             ; \
   fistpl V1                                                        ; \
   fistpl U1                                                        ; \
   UPDATE_FP_POS_4()            /* 4 pixels away */                 ; \
   START_FP_DIV()                                                   ; \
								    ; \
   movl V1, %eax                                                    ; \
   movl U1, %ecx                                                    ; \
   subl %edx, %eax                                                  ; \
   subl %ebx, %ecx                                                  ; \
   sarl $2, %eax                                                    ; \
   sarl $2, %ecx                                                    ; \
   movl %eax, DV                                                    ; \
   movl %ecx, DU                                                    ; \
2:                              /* step 3 & 4: next U, V */         ; \
   addl DV, %edx                                                    ; \
   addl DU, %ebx                                                    ; \
3:                              /* step 1: just use U and V */      ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax

#define END_PTEX()                                                    \
   decl W                                                           ; \
   jle 6f                                                           ; \
   decl COUNT                                                       ; \
   jg 2b                                                            ; \
   nop                                                              ; \
   jl 1b                                                            ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
   jmp 3b                                                           ; \
   _align_                                                          ; \
6:                                                                  ; \
   fstp %st(0)                   /* pop fpu stack */                ; \
   fstp %st(0)                                                      ; \
   fstp %st(0)                                                      ; \
   fstp %st(0)                                                      ; \
								    ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



/* main body of the perspective-correct texture mapping routine,
   using 3D enhancement CPUs */
#define INIT_PTEX_3D(extra...)                                        \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $100, %esp               /* local variables */              ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   FEMMS_R()                                                        ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   extra                                                            ; \
								    ; \
   movd POLYSEG_DZ(%esi), %mm1                                      ; \
   movl POLYSEG_DFV(%esi), %eax                                     ; \
   movl POLYSEG_DFU(%esi), %edx                                     ; \
   movl %eax, DV4                                                   ; \
   movl %edx, DU4                                                   ; \
   movq DV4, %mm3                                                   ; \
   PFADD_R(1, 1)                /* pfadd %mm1, %mm1 */              ; \
   PFADD_R(3, 3)                /* pfadd %mm3, %mm3 */              ; \
   PFADD_R(1, 1)                /* pfadd %mm1, %mm1 */              ; \
   PFADD_R(3, 3)                /* pfadd %mm3, %mm3 */              ; \
								    ; \
   movd POLYSEG_Z(%esi), %mm0                                       ; \
   PFRCP_R(0, 4)                /* pfrcp %mm0, %mm4 */              ; \
   movl POLYSEG_FV(%esi), %eax                                      ; \
   movl POLYSEG_FU(%esi), %edx                                      ; \
   movl %eax, V1                                                    ; \
   movl %edx, U1                                                    ; \
   movq V1, %mm2                                                    ; \
   PFMUL_R(2, 4)                /* pfmul %mm2, %mm4 */              ; \
   PF2ID_R(4, 4)                /* pf2id %mm4, %mm4 */              ; \
   movq %mm4, V1                                                    ; \
								    ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl POLYSEG_VMASK(%esi), %eax                                   ; \
   movl POLYSEG_UMASK(%esi), %edx                                   ; \
   shll %cl, %eax           /* adjust v mask and shift value */     ; \
   negl %ecx                                                        ; \
   addl $16, %ecx                                                   ; \
   movl %ecx, VSHIFT                                                ; \
   movl %eax, VMASK                                                 ; \
   movl %edx, UMASK                                                 ; \
								    ; \
   movl ADDR, %edi                                                  ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
   movl POLYSEG_TEXTURE(%esi), %esi                                 ; \
   movl $0, COUNT                /* COUNT ranges from 3 to -1 */    ; \
   jmp 3f                                                           ; \
								    ; \
   _align_                                                          ; \
1:                  /* step 2: compute DU, DV for next 4 steps */   ; \
   movl $3, COUNT                                                   ; \
								    ; \
   PFADD_R(1, 0)                /* pfadd %mm1, %mm0 */              ; \
   PFADD_R(3, 2)                /* pfadd %mm3, %mm2 */              ; \
   PFRCP_R(0, 4)                /* pfrcp %mm0, %mm4 */              ; \
   PFMUL_R(2, 4)                /* pfmul %mm2, %mm4 */              ; \
   PF2ID_R(4, 4)                /* pf2id %mm4, %mm4 */              ; \
								    ; \
   movq V1, %mm5                                                    ; \
   movq %mm4, V1                                                    ; \
   psubd %mm5, %mm4                                                 ; \
   psrad $2, %mm4                                                   ; \
   movq %mm4, DV                                                    ; \
2:                              /* step 3 & 4: next U, V */         ; \
   addl DV, %edx                                                    ; \
   addl DU, %ebx                                                    ; \
3:                              /* step 1: just use U and V */      ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax

#define END_PTEX_3D()                                                 \
   decl W                                                           ; \
   jle 6f                                                           ; \
   decl COUNT                                                       ; \
   jg 2b                                                            ; \
   nop                                                              ; \
   jl 1b                                                            ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
   jmp 3b                                                           ; \
   _align_                                                          ; \
6:                                                                  ; \
   FEMMS_R()                                                        ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_ptex16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex16)
   INIT_PTEX()
   movw (%esi, %eax, 2), %ax     /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   addl $2, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex16() */

/* void _poly_scanline_ptex_mask15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask15)
   INIT_PTEX()
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask15() */

/* void _poly_scanline_ptex_mask16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask16)
   INIT_PTEX()
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask16() */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_ptex32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex32)
   INIT_PTEX()
   movl (%esi, %eax, 4), %eax    /* read texel */
   movl %eax, FSEG(%edi)         /* write the pixel */
   addl $4, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex32() */

/* void _poly_scanline_ptex_mask32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask32)
   INIT_PTEX()
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   movl %eax, FSEG(%edi)         /* write solid pixels */
7: addl $4, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask32() */
#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_ptex24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex24)
   INIT_PTEX()
   leal (%eax, %eax, 2), %ecx
   movw (%esi, %ecx), %ax        /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   movb 2(%esi, %ecx), %al       /* read texel */
   movb %al, FSEG 2(%edi)        /* write the pixel */
   addl $3, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex24() */

/* void _poly_scanline_ptex_mask24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask24)
   INIT_PTEX()
   leal (%eax, %eax, 2), %ecx
   xorl %eax, %eax
   movb 2(%esi, %ecx), %al       /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
7: addl $3, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask24() */
#endif /* COLOR24 */



/* main body of the perspective-correct texture mapping routine
   special version for PTEX LIT with MMX */
#define DO_PTEX_LIT()                                                 \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $100, %esp               /* local variables */              ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   INIT()                                                           ; \
								    ; \
   flds POLYSEG_Z(%esi)          /* z at bottom of fpu stack */     ; \
   flds POLYSEG_FU(%esi)         /* followed by u */                ; \
   flds POLYSEG_FV(%esi)         /* followed by v */                ; \
								    ; \
   flds POLYSEG_DFU(%esi)        /* multiply diffs by four */       ; \
   flds POLYSEG_DFV(%esi)                                           ; \
   flds POLYSEG_DZ(%esi)                                            ; \
   fxch %st(2)                   /* u v z */                        ; \
   fadd %st(0), %st(0)           /* 2u v z */                       ; \
   fxch %st(1)                   /* v 2u z */                       ; \
   fadd %st(0), %st(0)           /* 2v 2u z */                      ; \
   fxch %st(2)                   /* z 2u 2v */                      ; \
   fadd %st(0), %st(0)           /* 2z 2u 2v */                     ; \
   fxch %st(1)                   /* 2u 2z 2v */                     ; \
   fadd %st(0), %st(0)           /* 4u 2z 2v */                     ; \
   fxch %st(2)                   /* 2v 2z 4u */                     ; \
   fadd %st(0), %st(0)           /* 4v 2z 4u */                     ; \
   fxch %st(1)                   /* 2z 4v 4u */                     ; \
   fadd %st(0), %st(0)           /* 4z 4v 4u */                     ; \
   fxch %st(2)                   /* 4u 4v 4z */                     ; \
   fstps DU4                                                        ; \
   fstps DV4                                                        ; \
   fstps DZ4                                                        ; \
								    ; \
   START_FP_DIV()                                                   ; \
								    ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl POLYSEG_VMASK(%esi), %eax                                   ; \
   movl POLYSEG_UMASK(%esi), %edx                                   ; \
   shll %cl, %eax           /* adjust v mask and shift value */     ; \
   negl %ecx                                                        ; \
   addl $16, %ecx                                                   ; \
   movl %ecx, VSHIFT                                                ; \
   movl %eax, VMASK                                                 ; \
   movl %edx, UMASK                                                 ; \
								    ; \
   END_FP_DIV()                                                     ; \
   fistpl V1                     /* store v */                      ; \
   fistpl U1                     /* store u */                      ; \
   UPDATE_FP_POS_4()                                                ; \
   START_FP_DIV()                                                   ; \
								    ; \
   movl ADDR, %edi                                                  ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
   movl POLYSEG_TEXTURE(%esi), %esi                                 ; \
								    ; \
   fstps SM                                                         ; \
   fstps SV                                                         ; \
   fstps SU                                                         ; \
   fstps SZ                                                         ; \
								    ; \
   LOAD()                                                           ; \
								    ; \
   _align_                                                          ; \
1:                              /* step 1: just use U and V */      ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax                                                  ; \
								    ; \
   PIXEL()                                                          ; \
   decl W                                                           ; \
   jle 99f                                                          ; \
   SAVE()                                                           ; \
   emms                                                             ; \
		    /* step 2: compute DU, DV for next 4 steps */   ; \
   flds SZ                                                          ; \
   flds SU                                                          ; \
   flds SV                                                          ; \
   flds SM                                                          ; \
								    ; \
   END_FP_DIV()                 /* finish the divide */             ; \
   fistpl V1                                                        ; \
   fistpl U1                                                        ; \
   UPDATE_FP_POS_4()            /* 4 pixels away */                 ; \
   START_FP_DIV()                                                   ; \
								    ; \
   movl V1, %eax                                                    ; \
   movl U1, %ecx                                                    ; \
   subl %edx, %eax                                                  ; \
   subl %ebx, %ecx                                                  ; \
   sarl $2, %eax                                                    ; \
   sarl $2, %ecx                                                    ; \
   movl %eax, DV                                                    ; \
   movl %ecx, DU                                                    ; \
								    ; \
   addl %eax, %edx                                                  ; \
   addl %ecx, %ebx                                                  ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax                                                  ; \
								    ; \
   fstps SM                                                         ; \
   fstps SV                                                         ; \
   fstps SU                                                         ; \
   fstps SZ                                                         ; \
								    ; \
   LOAD()                                                           ; \
   PIXEL()                                                          ; \
   decl W                                                           ; \
   jle 99f                                                          ; \
				/* step 3 : next U, V */            ; \
   addl DV, %edx                                                    ; \
   addl DU, %ebx                                                    ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax                                                  ; \
								    ; \
   PIXEL()                                                          ; \
   decl W                                                           ; \
   jle 99f                                                          ; \
				/* step 4: next U, V */             ; \
   addl DV, %edx                                                    ; \
   addl DU, %ebx                                                    ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax                                                  ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
								    ; \
   PIXEL()                                                          ; \
   decl W                                                           ; \
   jg 1b                                                            ; \
99:                                                                 ; \
   emms                                                             ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



/* void _poly_scanline_ptex_mask_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit8)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 7f
   movb 2+ALPHA, %ah
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
7:
   movl DALPHA, %eax
   incl %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit8() */



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_ptex_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit15)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit15() */

/* void _poly_scanline_ptex_mask_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit15)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit15() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_ptex_lit15x)

#define INIT()                                                          \
   INIT_MMX_ALPHA()                                                   ; \
   movq %mm0, ALPHA                                                   ; \
   movq %mm1, DALPHA                                                  ; \
   emms                                                               ; \
   movl GLOBL(_blender_col_15), %eax                                  ; \
   movl %eax, %ecx                                                    ; \
   movl %eax, %edx                                                    ; \
   andl $0x001f, %eax                                                 ; \
   andl $0x03e0, %ecx                                                 ; \
   andl $0x7c00, %edx                                                 ; \
   movw %ax, BLEND                                                    ; \
   shrl $8, %edx                                                      ; \
   movw %cx, BLEND2                                                   ; \
   movl %edx, BLEND4

#define LOAD()                                                          \
   movq ALPHA, %mm0                                                   ; \
   movq DALPHA, %mm1                                                  ; \
   movq BLEND, %mm6                                                   ; \
   movq GLOBL(_mask_mmx_15), %mm7

#define PIXEL()                                                         \
   movw (%esi, %eax, 2), %ax                                          ; \
   movw %ax, TMP                                                      ; \
   movw %ax, TMP2                                                     ; \
   movb %ah, TMP4                                                     ; \
   movq TMP, %mm2                                                     ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   movq %mm2, TMP                                                     ; \
   movd %mm2, %eax                                                    ; \
   orw TMP2, %ax                                                      ; \
   orb TMP4, %ah                                                      ; \
   movw %ax, FSEG(%edi)                                               ; \
								      ; \
   addl $2, %edi                                                      ; \
   paddw %mm1, %mm0

#define SAVE()                                                          \
   movq %mm0, ALPHA

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_lit15x() */

#undef PIXEL

FUNC(_poly_scanline_ptex_mask_lit15x)

#define PIXEL()                                                         \
   movw (%esi, %eax, 2), %ax                                          ; \
   cmpw $MASK_COLOR_15, %ax                                           ; \
   jz 7f                                                              ; \
   movw %ax, TMP                                                      ; \
   movw %ax, TMP2                                                     ; \
   movb %ah, TMP4                                                     ; \
   movq TMP, %mm2                                                     ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   movq %mm2, TMP                                                     ; \
   movd %mm2, %eax                                                    ; \
   orw TMP2, %ax                                                      ; \
   orb TMP4, %ah                                                      ; \
   movw %ax, FSEG(%edi)                                               ; \
7:                                                                    ; \
   addl $2, %edi                                                      ; \
   paddw %mm1, %mm0

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_mask_lit15x() */

#undef INIT
#undef LOAD
#undef SAVE
#undef PIXEL

FUNC(_poly_scanline_ptex_lit15d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      movl GLOBL(_blender_col_15), %eax
      movw %ax, BLEND
      movw %ax, BLEND2
      movb %ah, BLEND4
      movq BLEND, %mm6
      PAND_M(GLOBL(_mask_mmx_15), 6)        /* pand mem, %mm6 */
   )
   movw (%esi, %eax, 2), %ax
   movw %ax, TMP
   movw %ax, TMP2
   movb %ah, TMP4
   movq TMP, %mm4
   PAND_M(GLOBL(_mask_mmx_15), 4)      /* pand mem, %mm4 */

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4
   PAND_M(GLOBL(_mask_mmx_15), 4)      /* pand mem, %mm4 */

   movq %mm4, TMP
   movd %mm4, %eax
   orw TMP2, %ax
   orb TMP4, %ah                 /* %ax = RGB */
   movw %ax, FSEG(%edi)

   addl $2, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_lit15d() */

FUNC(_poly_scanline_ptex_mask_lit15d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      movl GLOBL(_blender_col_15), %eax
      movw %ax, BLEND
      movw %ax, BLEND2
      movb %ah, BLEND4
      movq BLEND, %mm6
      PAND_M(GLOBL(_mask_mmx_15), 6)   /* pand mem, %mm6 */
   )
   movw (%esi, %eax, 2), %ax
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   movw %ax, TMP
   movw %ax, TMP2
   movb %ah, TMP4
   movq TMP, %mm4
   PAND_M(GLOBL(_mask_mmx_15), 4)      /* pand mem, %mm4 */

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4
   PAND_M(GLOBL(_mask_mmx_15), 4)      /* pand mem, %mm4 */

   movq %mm4, TMP
   movd %mm4, %eax
   orw TMP2, %ax
   orb TMP4, %ah              /* %ax = RGB */
   movw %ax, FSEG(%edi)
7:
   addl $2, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_mask_lit15d() */

#endif /* MMX */



/* void _poly_scanline_ptex_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit16)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit16() */

/* void _poly_scanline_ptex_mask_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit16)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit16() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_ptex_lit16x)

#define INIT()                                                          \
   INIT_MMX_ALPHA()                                                   ; \
   movq %mm0, ALPHA                                                   ; \
   movq %mm1, DALPHA                                                  ; \
   emms                                                               ; \
   movl GLOBL(_blender_col_16), %eax                                  ; \
   movl %eax, %ecx                                                    ; \
   movl %eax, %edx                                                    ; \
   andl $0x001f, %eax                                                 ; \
   andl $0x07e0, %ecx                                                 ; \
   andl $0xf800, %edx                                                 ; \
   movw %ax, BLEND                                                    ; \
   shrl $8, %edx                                                      ; \
   movw %cx, BLEND2                                                   ; \
   movl %edx, BLEND4

#define LOAD()                                                          \
   movq ALPHA, %mm0                                                   ; \
   movq DALPHA, %mm1                                                  ; \
   movq BLEND, %mm6                                                   ; \
   movq GLOBL(_mask_mmx_16), %mm7

#define PIXEL()                                                         \
   movw (%esi, %eax, 2), %ax                                          ; \
   movw %ax, TMP                                                      ; \
   movw %ax, TMP2                                                     ; \
   movb %ah, TMP4                                                     ; \
   movq TMP, %mm2                                                     ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   movq %mm2, TMP                                                     ; \
   movd %mm2, %eax                                                    ; \
   orw TMP2, %ax                                                      ; \
   orb TMP4, %ah                                                      ; \
   movw %ax, FSEG(%edi)                                               ; \
								      ; \
   addl $2, %edi                                                      ; \
   paddw %mm1, %mm0

#define SAVE()                                                          \
   movq %mm0, ALPHA

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_lit16x() */

#undef PIXEL

FUNC(_poly_scanline_ptex_mask_lit16x)

#define PIXEL()                                                         \
   movw (%esi, %eax, 2), %ax                                          ; \
   cmpw $MASK_COLOR_16, %ax                                           ; \
   jz 7f                                                              ; \
   movw %ax, TMP                                                      ; \
   movw %ax, TMP2                                                     ; \
   movb %ah, TMP4                                                     ; \
   movq TMP, %mm2                                                     ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
   PAND_R(7, 2)                 /* pand %mm7, %mm2 */                 ; \
								      ; \
   movq %mm2, TMP                                                     ; \
   movd %mm2, %eax                                                    ; \
   orw TMP2, %ax                                                      ; \
   orb TMP4, %ah                                                      ; \
   movw %ax, FSEG(%edi)                                               ; \
7:                                                                    ; \
   addl $2, %edi                                                      ; \
   paddw %mm1, %mm0

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_mask_lit16x() */

#undef INIT
#undef LOAD
#undef SAVE
#undef PIXEL

FUNC(_poly_scanline_ptex_lit16d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      movl GLOBL(_blender_col_16), %eax
      movw %ax, BLEND
      movw %ax, BLEND2
      movb %ah, BLEND4
      movq BLEND, %mm6
      PAND_M(GLOBL(_mask_mmx_16), 6)            /* pand mem, %mm6 */
   )
   movw (%esi, %eax, 2), %ax
   movw %ax, TMP
   movw %ax, TMP2
   movb %ah, TMP4
   movq TMP, %mm4
   PAND_M(GLOBL(_mask_mmx_16), 4)            /* pand mem, %mm4 */

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4
   PAND_M(GLOBL(_mask_mmx_16), 4)            /* pand mem, %mm4 */

   movq %mm4, TMP
   movd %mm4, %eax
   orw TMP2, %ax
   orb TMP4, %ah              /* %ax = RGB */
   movw %ax, FSEG(%edi)

   addl $2, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_lit16d() */

FUNC(_poly_scanline_ptex_mask_lit16d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      movl GLOBL(_blender_col_16), %eax
      movw %ax, BLEND
      movw %ax, BLEND2
      movb %ah, BLEND4
      movq BLEND, %mm6
      PAND_M(GLOBL(_mask_mmx_16), 6)            /* pand mem, %mm6 */
   )
   movw (%esi, %eax, 2), %ax
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   movw %ax, TMP
   movw %ax, TMP2
   movb %ah, TMP4
   movq TMP, %mm4
   PAND_M(GLOBL(_mask_mmx_16), 4)               /* pand mem, %mm4 */

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4
   PAND_M(GLOBL(_mask_mmx_16), 4)               /* pand mem, %mm4 */

   movq %mm4, TMP
   movd %mm4, %eax
   orw TMP2, %ax
   orb TMP4, %ah              /* %ax = RGB */
   movw %ax, FSEG(%edi)
7:
   addl $2, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_mask_lit16d() */

#endif /* MMX */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_ptex_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit32)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movl (%esi, %eax, 4), %eax    /* read texel */
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit32() */

/* void _poly_scanline_ptex_mask_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit32)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit32() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_ptex_lit32x)

#define INIT()                                                          \
   INIT_MMX_ALPHA()                                                   ; \
   movq %mm0, ALPHA                                                   ; \
   movq %mm1, DALPHA                                                  ; \
   emms

#define LOAD()                                                          \
   movq ALPHA, %mm0                                                   ; \
   movq DALPHA, %mm1                                                  ; \
   pxor %mm7, %mm7                                                    ; \
   movd GLOBL(_blender_col_32), %mm6                                  ; \
   punpcklbw %mm7, %mm6

#define PIXEL()                                                         \
   movd (%esi, %eax, 4), %mm2                                         ; \
   punpcklbw %mm7, %mm2                                               ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
								      ; \
   packuswb %mm2, %mm2                                                ; \
   movd %mm2, FSEG(%edi)                                              ; \
								      ; \
   addl $4, %edi                                                      ; \
   paddw %mm1, %mm0

#define SAVE()                                                          \
   movq %mm0, ALPHA

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_lit32x() */

#undef PIXEL

FUNC(_poly_scanline_ptex_mask_lit32x)

#define PIXEL()                                                         \
   movl (%esi, %eax, 4), %eax                                         ; \
   cmpl $MASK_COLOR_32, %eax                                          ; \
   jz 7f                                                              ; \
   movd %eax, %mm2                                                    ; \
   punpcklbw %mm7, %mm2                                               ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
								      ; \
   packuswb %mm2, %mm2                                                ; \
   movd %mm2, FSEG(%edi)                                              ; \
7:                                                                    ; \
   addl $4, %edi                                                      ; \
   paddw %mm1, %mm0

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_mask_lit32x() */

#undef INIT
#undef LOAD
#undef SAVE
#undef PIXEL

FUNC(_poly_scanline_ptex_lit32d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      pxor %mm5, %mm5
      movd GLOBL(_blender_col_32), %mm6
      punpcklbw %mm5, %mm6
   )
   movd (%esi, %eax, 4), %mm4
   pxor %mm5, %mm5
   punpcklbw %mm5, %mm4

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4

   packuswb %mm4, %mm4
   movd %mm4, FSEG(%edi)

   addl $4, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_lit32d() */

FUNC(_poly_scanline_ptex_mask_lit32d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      pxor %mm5, %mm5
      movd GLOBL(_blender_col_32), %mm6
      punpcklbw %mm5, %mm6
   )
   movl (%esi, %eax, 4), %eax
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   movd %eax, %mm4
   pxor %mm5, %mm5
   punpcklbw %mm5, %mm4

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4

   packuswb %mm4, %mm4
   movd %mm4, FSEG(%edi)
7:
   addl $4, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_mask_lit32d() */

#endif /* MMX */
#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_ptex_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit24)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   leal (%eax, %eax, 2), %ecx
   movb 2(%esi, %ecx), %al       /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit24() */

/* void _poly_scanline_ptex_mask_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit24)
   INIT_PTEX(
     movl POLYSEG_C(%esi), %eax
     movl POLYSEG_DC(%esi), %edx
     movl %eax, ALPHA
     movl %edx, DALPHA
   )
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
7:
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit24() */

#ifdef ALLEGRO_MMX
FUNC(_poly_scanline_ptex_lit24x)

#define INIT()                                                          \
   INIT_MMX_ALPHA()                                                   ; \
   movq %mm0, ALPHA                                                   ; \
   movq %mm1, DALPHA                                                  ; \
   emms

#define LOAD()                                                          \
   movq ALPHA, %mm0                                                   ; \
   movq DALPHA, %mm1                                                  ; \
   pxor %mm7, %mm7                                                    ; \
   movd GLOBL(_blender_col_24), %mm6                                  ; \
   punpcklbw %mm7, %mm6

#define PIXEL()                                                         \
   leal (%eax, %eax, 2), %ecx                                         ; \
   movb 2(%esi, %ecx), %al                                            ; \
   shll $16, %eax                                                     ; \
   movw (%esi, %ecx), %ax                                             ; \
   movd %eax, %mm2                                                    ; \
   punpcklbw %mm7, %mm2                                               ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
								      ; \
   packuswb %mm2, %mm2                                                ; \
   movd %mm2, %eax                                                    ; \
   movw %ax, FSEG(%edi)                                               ; \
   shrl $16, %eax                                                     ; \
   movb %al, FSEG 2(%edi)                                             ; \
								      ; \
   addl $3, %edi                                                      ; \
   paddw %mm1, %mm0

#define SAVE()                                                          \
   movq %mm0, ALPHA

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_lit24x() */

#undef PIXEL

FUNC(_poly_scanline_ptex_mask_lit24x)

#define PIXEL()                                                         \
   leal (%eax, %eax, 2), %ecx                                         ; \
   movzbl 2(%esi, %ecx), %eax                                         ; \
   shll $16, %eax                                                     ; \
   movw (%esi, %ecx), %ax                                             ; \
   cmpl $MASK_COLOR_24, %eax                                          ; \
   jz 7f                                                              ; \
   movd %eax, %mm2                                                    ; \
   punpcklbw %mm7, %mm2                                               ; \
								      ; \
   psubw %mm6, %mm2                                                   ; \
   paddw %mm2, %mm2                                                   ; \
   pmulhw %mm0, %mm2                                                  ; \
   paddw %mm6, %mm2                                                   ; \
								      ; \
   packuswb %mm2, %mm2                                                ; \
   movd %mm2, %eax                                                    ; \
   movw %ax, FSEG(%edi)                                               ; \
   shrl $16, %eax                                                     ; \
   movb %al, FSEG 2(%edi)                                             ; \
7:                                                                    ; \
   addl $3, %edi                                                      ; \
   paddw %mm1, %mm0

   DO_PTEX_LIT()
   ret                           /* end of _poly_scanline_ptex_mask_lit24x() */

#undef INIT
#undef LOAD
#undef SAVE
#undef PIXEL

FUNC(_poly_scanline_ptex_lit24d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      pxor %mm5, %mm5
      movd GLOBL(_blender_col_24), %mm6
      punpcklbw %mm5, %mm6
   )
   leal (%eax, %eax, 2), %ecx
   movb 2(%esi, %ecx), %al
   shll $16, %eax
   movw (%esi, %ecx), %ax
   movd %eax, %mm4
   pxor %mm5, %mm5
   punpcklbw %mm5, %mm4

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4

   packuswb %mm4, %mm4
   movd %mm4, %eax
   movw %ax, FSEG(%edi)
   shrl $16, %eax
   movb %al, FSEG 2(%edi)

   addl $3, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_lit24d() */

FUNC(_poly_scanline_ptex_mask_lit24d)
   INIT_PTEX_3D(
      INIT_MMX_ALPHA()
      movq %mm0, %mm7
      movq %mm1, DALPHA
      pxor %mm5, %mm5
      movd GLOBL(_blender_col_24), %mm6
      punpcklbw %mm5, %mm6
   )
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   movd %eax, %mm4
   pxor %mm5, %mm5
   punpcklbw %mm5, %mm4

   psubw %mm6, %mm4
   paddw %mm4, %mm4
   pmulhw %mm7, %mm4
   paddw %mm6, %mm4

   packuswb %mm4, %mm4
   movd %mm4, %eax
   movw %ax, FSEG(%edi)
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
7:
   addl $3, %edi
   paddw DALPHA, %mm7
   END_PTEX_3D()
   ret                           /* end of _poly_scanline_ptex_mask_lit24d() */

#endif /* MMX */
#endif /* COLOR24 */


