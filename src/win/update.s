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
 *      Asm routines for software color conversion.
 *      Suggestions to make it faster are welcome :)
 *
 *      By Isaac Cruz.
 *
 *      24-bit color support and non MMX routines by Eric Botcazou. 
 *
 *      See readme.txt for copyright information.
 */

#include "src/i386/asmdefs.inc"


.text

#ifdef ALLEGRO_MMX

/* it seems pand is broken in GAS 2.8.1 */
#define PAND(src, dst)   \
   .byte 0x0f, 0xdb    ; \
   .byte 0xc0 + 8*dst + src  /* mod field */

/* local variables */
#define LOCAL1   -4(%esp)
#define LOCAL2   -8(%esp)
#define LOCAL3   -12(%esp)
#define LOCAL4   -16(%esp)


/* helper macro */
#define INIT_CONVERSION_1(mask_red, mask_green, mask_blue) \
      /* init register values */                                                   ; \
                                                                                   ; \
      movl mask_green, %eax                                                        ; \
      movd %eax, %mm3                                                              ; \
      punpckldq %mm3, %mm3                                                         ; \
      movl mask_red, %eax                                                          ; \
      movd %eax, %mm4                                                              ; \
      punpckldq %mm4, %mm4                                                         ; \
      movl mask_blue, %eax                                                         ; \
      movd %eax, %mm5                                                              ; \
      punpckldq %mm5, %mm5                                                         ; \
                                                                                   ; \
      movl ARG1, %eax             /* eax = &src_desc */                            ; \
      movl 8(%eax), %ecx          /* ecx = src_desc.dwHeight */                    ; \
      movl 12(%eax), %edx         /* edx = src_desc.dwWidth */                     ; \
      movl 16(%eax), %esi         /* esi = src_desc.lPitch */                      ; \
      movl 36(%eax), %eax         /* eax = src_desc.lpSurface */                   ; \
      shll $2, %edx               /* edx = SCREEN_W * 4 */                         ; \
      subl %edx, %esi             /* esi = (src_desc.lPitch) - edx */              ; \
                                                                                   ; \
      movl ARG2, %ebx             /* ebx = &dest_desc */                           ; \
      shrl $1, %edx               /* edx = SCREEN_W * 2 */                         ; \
      movl 16(%ebx), %edi         /* edi = dest_desc.lPitch */                     ; \
      movl 36(%ebx), %ebx         /* ebx = dest_desc.lpSurface */                  ; \
      subl %edx, %edi             /* edi = (dest_desc.lPitch) - edx */             ; \
      shrl $2, %edx               /* edx = SCREEN_W / 2 */
      

#define INIT_CONVERSION_2(mask_red, mask_green, mask_blue) \
      /* init register values */                                                   ; \
                                                                                   ; \
      movl mask_green, %eax                                                        ; \
      movd %eax, %mm3                                                              ; \
      punpckldq %mm3, %mm3                                                         ; \
      movl mask_red, %eax                                                          ; \
      movd %eax, %mm4                                                              ; \
      punpckldq %mm4, %mm4                                                         ; \
      movl mask_blue, %eax                                                         ; \
      movd %eax, %mm5                                                              ; \
      punpckldq %mm5, %mm5                                                         ; \
                                                                                   ; \
      movl ARG1, %eax             /* eax = &src_desc */                            ; \
      movl 8(%eax), %ecx          /* ecx = src_desc.dwHeight */                    ; \
      movl 12(%eax), %edx         /* edx = src_desc.dwWidth */                     ; \
      movl 16(%eax), %esi         /* esi = src_desc.lPitch */                      ; \
      movl 36(%eax), %eax         /* eax = src_desc.lpSurface */                   ; \
      addl %edx, %edx             /* edx = SCREEN_W * 2 */                         ; \
      subl %edx, %esi             /* esi = (src_desc.lPitch) - edx */              ; \
                                                                                   ; \
      movl ARG2, %ebx             /* ebx = &dest_desc */                           ; \
      addl %edx, %edx             /* edx = SCREEN_W * 4 */                         ; \
      movl 16(%ebx), %edi         /* edi = dest_desc.lPitch */                     ; \
      movl 36(%ebx), %ebx         /* ebx = dest_desc.lpSurface */                  ; \
      subl %edx, %edi             /* edi = (dest_desc.lPitch) - edx */             ; \
      shrl $3, %edx               /* edx = SCREEN_W / 2 */
      

/* void _update_32_to_16 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_32_to_16)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _update_32_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_1 ($0xf80000, $0x00fc00, $0x0000f8);

   /* 32 bit to 16 bit conversion:
    we have:
    eax = src_desc.lpSurface
    ebx = dest_desc.lpSurface
    ecx = SCREEN_H
    edx = SCREEN_W / 2
    esi = offset from the end of a line to the beginning of the next
    edi = same as esi, but for the dest bitmap
   */
   
   next_line_32_to_16:
      movl $0, %ebp      /* (better than xor ebp, ebp) */
   
   next_block_32_to_16:
      movq (%eax), %mm0
      movq %mm0, %mm1
      nop
      movq %mm0, %mm2
      PAND (5, 0)        /* pand %mm5, %mm0 */
      psrld $3, %mm0
      PAND (3, 1)        /* pand %mm3, %mm1 */
      psrld $5, %mm1
      por %mm1, %mm0
      addl $8, %eax
      PAND (4, 2)        /* pand %mm4, %mm2 */
      psrld $8, %mm2
      nop
      nop
      por %mm2, %mm0
      movq %mm0, %mm6
      psrlq $16, %mm0
      por %mm0, %mm6
      movd %mm6, (%ebx)
      addl $4, %ebx

      incl %ebp
      cmpl %edx, %ebp
      jb next_block_32_to_16
      
      addl %esi, %eax
      addl %edi, %ebx
      decl %ecx
      jnz next_line_32_to_16
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret


/* void _update_32_to_15 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_32_to_15)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _update_32_to_15_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_1 ($0xf80000, $0x00f800, $0x0000f8);

   /* 32 bit to 15 bit conversion:
    we have:
    eax = src_desc.lpSurface
    ebx = dest_desc.lpSurface
    ecx = SCREEN_H
    edx = SCREEN_W / 2
    esi = offset from the end of a line to the beginning of the next
    edi = same as esi, but for the dest bitmap
   */
   
   next_line_32_to_15:
      movl $0, %ebp      /* (better than xor ebp, ebp) */
   
   next_block_32_to_15:
      movq (%eax), %mm0
      movq %mm0, %mm1
      movq %mm0, %mm2
      PAND (5, 0)        /* pand %mm5, %mm0 */
      psrld $3, %mm0
      PAND (3, 1)        /* pand %mm3, %mm1 */
      psrld $6, %mm1
      por %mm1, %mm0
      addl $8, %eax
      PAND (4, 2)        /* pand %mm4, %mm2 */
      psrld $9, %mm2
      por %mm2, %mm0
      movq %mm0, %mm6
      psrlq $16, %mm0
      por %mm0, %mm6
      movd %mm6, (%ebx)
      addl $4, %ebx

      incl %ebp
      cmpl %edx, %ebp
      jb next_block_32_to_15
      
      addl %esi, %eax
      addl %edi, %ebx
      decl %ecx
      jnz next_line_32_to_15
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret




/* void _update_16_to_32 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_16_to_32)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _update_16_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_2 ($0xf800, $0x07e0, $0x001f);

   /* 16 bit to 32 bit conversion:
    we have:
    eax = src_desc.lpSurface
    ebx = dest_desc.lpSurface
    ecx = SCREEN_H
    edx = SCREEN_W / 2
    esi = offset from the end of a line to the beginning of the next
    edi = same as esi, but for the dest bitmap
   */
   
   next_line_16_to_32:
      movl $0, %ebp      /* (better than xor ebp, ebp) */
   
   next_block_16_to_32:
      movd (%eax), %mm0    /* mm0 = 0000 0000  [rgb1][rgb2] */
      punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
      movq %mm0, %mm1
      movq %mm0, %mm2
      PAND (5, 0)        /* pand %mm5, %mm0 */
      pslld $3, %mm0
      PAND (3, 1)        /* pand %mm3, %mm1 */
      pslld $5, %mm1
      por %mm1, %mm0
      addl $4, %eax
      PAND (4, 2)        /* pand %mm4, %mm2 */
      pslld $8, %mm2
      por %mm2, %mm0
      movq %mm0, (%ebx)
      addl $8, %ebx

      incl %ebp
      cmpl %edx, %ebp
      jb next_block_16_to_32
      
      addl %esi, %eax
      addl %edi, %ebx
      decl %ecx
      jnz next_line_16_to_32
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret



/* void _update_8_to_32 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_8_to_32)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _update_8_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax             /* eax = &src_desc */
   movl 8(%eax), %ecx          /* ecx = src_desc.dwHeight */
   movl %ecx, LOCAL1           /* LOCAL1 = SCREEN_H */
   movl 12(%eax), %edi         /* edi = src_desc.dwWidth */
   movl 16(%eax), %esi         /* esi = src_desc.lPitch */
   movl 36(%eax), %eax         /* eax = src_desc.lpSurface */
   subl %edi, %esi
   movl %esi, LOCAL2           /* LOCAL2 = src_desc.lPitch - SCREEN_W */

   movl ARG2, %ebx             /* ebx = &dest_desc */
   shll $2, %edi               /* edi = SCREEN_W * 4 */
   movl 16(%ebx), %edx         /* edx = dest_desc.lPitch */
   movl 36(%ebx), %ebx         /* ebx = dest_desc.lpSurface */
   subl %edi, %edx
   movl %edx, LOCAL3           /* LOCAL3 = (dest_desc.lPitch) - (SCREEN_W * 4) */
   shrl $4, %edi               /* edi = SCREEN_W / 4 */
   movl GLOBL(allegro_palette), %esi  /* esi = allegro_palette */


   /* 8 bit to 32 bit conversion:
    we have:
    eax = src_desc.lpSurface
    ebx = dest_desc.lpSurface
    esi = allegro_palette
    edi = SCREEN_W / 4
    LOCAL1 = SCREEN_H
    LOCAL2 = offset from the end of a line to the beginning of the next
    LOCAL3 = same as LOCAL2, but for the dest bitmap
   */
   
   next_line_8_to_32:
      movl $0, %ebp      /* (better than xor ebp, ebp) */
   
   next_block_8_to_32:
      movl (%eax), %edx          /* edx = [4][3][2][1] */ 
      movzbl %dl, %ecx
      movd (%esi,%ecx,4), %mm0   /* mm0 = xxxxxxxxx [   1   ] */
      shrl $8, %edx
      movzbl %dl, %ecx
      movd (%esi,%ecx,4), %mm1   /* mm1 = xxxxxxxxx [   2   ] */
      punpckldq %mm1, %mm0       /* mm0 = [   2   ] [   1   ] */
      addl $4, %eax
      movq %mm0, (%ebx)
      shrl $8, %edx
      movzbl %dl, %ecx
      movd (%esi,%ecx,4), %mm0   /* mm0 = xxxxxxxxx [   3   ] */
      shrl $8, %edx
      movl %edx, %ecx
      movd (%esi,%ecx,4), %mm1   /* mm1 = xxxxxxxxx [   4   ] */
      punpckldq %mm1, %mm0       /* mm0 = [   4   ] [   3   ] */
      movq %mm0, 8(%ebx)
      addl $16, %ebx

      incl %ebp
      cmpl %edi, %ebp
      jb next_block_8_to_32
      
      movl LOCAL2, %edx
      addl %edx, %eax
      movl LOCAL3, %ecx
      addl %ecx, %ebx
      movl LOCAL1, %edx
      decl %edx
      movl %edx, LOCAL1
      jnz next_line_8_to_32
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret


/* void _update_8_to_16 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
/* void _update_8_to_15 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_8_to_16)
FUNC (_update_8_to_15)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _update_8_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax             /* eax = &src_desc */
   movl 8(%eax), %ecx          /* ecx = src_desc.dwHeight */
   movl %ecx, LOCAL1           /* LOCAL1 = SCREEN_H */
   movl 12(%eax), %edi         /* edi = src_desc.dwWidth */
   movl 16(%eax), %esi         /* esi = src_desc.lPitch */
   movl 36(%eax), %eax         /* eax = src_desc.lpSurface */
   subl %edi, %esi
   movl %esi, LOCAL2           /* LOCAL2 = src_desc.lPitch - SCREEN_W */

   movl ARG2, %ebx             /* ebx = &dest_desc */
   addl %edi, %edi             /* edi = SCREEN_W * 2 */
   movl 16(%ebx), %edx         /* edx = dest_desc.lPitch */
   movl 36(%ebx), %ebx         /* ebx = dest_desc.lpSurface */
   subl %edi, %edx
   movl %edx, LOCAL3           /* LOCAL3 = (dest_desc.lPitch) - (SCREEN_W * 2) */
   shrl $3, %edi               /* edi = SCREEN_W / 4 */
   movl GLOBL(allegro_palette), %esi  /* esi = allegro_palette */


   /* 8 bit to 16 bit conversion:
    we have:
    eax = src_desc.lpSurface
    ebx = dest_desc.lpSurface
    esi = allegro_palette
    edi = SCREEN_W / 4
    LOCAL1 = SCREEN_H
    LOCAL2 = offset from the end of a line to the beginning of the next
    LOCAL3 = same as LOCAL2, but for the dest bitmap
   */
   
   next_line_8_to_16:
      movl $0, %ebp      /* better than xor ebp, ebp */
   
   next_block_8_to_16:
      movl (%eax), %edx         /* edx = [4][3][2][1] */
      movzbl %dl, %ecx
      movd (%esi,%ecx,4), %mm0  /* mm0 = xxxxxxxxxx xxxxx[ 1 ] */
      shrl $8, %edx
      movzbl %dl, %ecx
      movd (%esi,%ecx,4), %mm1  /* mm1 = xxxxxxxxxx xxxxx[ 2 ] */
      punpcklwd %mm1, %mm0      /* mm0 = xxxxxxxxxx [ 2 ][ 1 ] */
      shrl $8, %edx
      movzbl %dl, %ecx
      movd (%esi,%ecx,4), %mm2  /* mm2 = xxxxxxxxxx xxxxx[ 3 ] */
      shrl $8, %edx
      movl %edx, %ecx
      movd (%esi,%ecx,4), %mm3  /* mm3 = xxxxxxxxxx xxxxx[ 4 ] */
      punpcklwd %mm3, %mm2      /* mm2 = xxxxxxxxxx [ 4 ][ 3 ] */
      addl $4, %eax
      punpckldq %mm2, %mm0      /* mm0 = [ 4 ][ 3 ] [ 2 ][ 1 ] */
      movq %mm0, (%ebx)
      addl $8, %ebx

      incl %ebp
      cmpl %edi, %ebp
      jb next_block_8_to_16
      
      movl LOCAL2, %edx
      addl %edx, %eax
      movl LOCAL3, %ecx
      addl %ecx, %ebx
      movl LOCAL1, %edx
      decl %edx
      movl %edx, LOCAL1
      jnz next_line_8_to_16
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret

#endif  /* ALLEGRO_MMX */


/********************************************************************************************/
/* pure 80386 asm routines                                                                  */
/*  optimized for Intel Pentium                                                             */
/********************************************************************************************/

/* create the (pseudo - we need %ebp) stack frame */
#define CREATE_STACK_FRAME  \
   pushl %ebp             ; \
   movl %esp, %ebp        ; \
   pushl %ebx             ; \
   pushl %esi             ; \
   pushl %edi

#define DESTROY_STACK_FRAME \
   popl %edi              ; \
   popl %esi              ; \
   popl %ebx              ; \
   popl %ebp

/* reserve storage for ONE 32-bit push on the stack */ 
#define MYLOCAL1   -8(%esp)
#define MYLOCAL2  -12(%esp)
#define MYLOCAL3  -16(%esp)

/* initialize the registers */
#define SIZE_1
#define SIZE_2 addl %ebx, %ebx
#define SIZE_3 leal (%ebx,%ebx,2), %ebx
#define SIZE_4 shll $2, %ebx
#define LOOP_RATIO_2 1
#define LOOP_RATIO_4 2

#define INIT_REGISTERS_NO_MMX(src_mul_code, dest_mul_code, width_ratio)      \
   movl ARG1, %eax          /* eax    = &src_desc                   */     ; \
   movl 12(%eax), %ebx      /* ebx    = src_desc.dwWidth            */     ; \
   movl 8(%eax), %ecx       /* ecx    = src_desc.dwHeight           */     ; \
   movl 16(%eax), %edx      /* edx    = src_desc.lPitch             */     ; \
   movl %ebx, %edi          /* edi    = width                       */     ; \
   src_mul_code             /* ebx    = width*x                     */     ; \
   movl 36(%eax), %esi      /* esi    = src_desc.lpSurface          */     ; \
   subl %ebx, %edx                                                         ; \
   movl %edi, %ebx                                                         ; \
   shrl $width_ratio, %edi                                                 ; \
   movl ARG2, %eax          /* eax    = &dest_desc                  */     ; \
   movl %edi, MYLOCAL1      /* LOCAL1 = width/y                     */     ; \
   movl %edx, MYLOCAL2      /* LOCAL2 = src_desc.lPitch - width*x   */     ; \
   dest_mul_code            /* ebx    = width*y                     */     ; \
   movl 16(%eax), %edx      /* edx    = dest_desc.lPitch            */     ; \
   subl %ebx, %edx                                                         ; \
   movl 36(%eax), %edi      /* edi    = dest_desc.lpSurface         */     ; \
   movl %edx, MYLOCAL3      /* LOCAL3 = dest_desc.lPitch - width*y  */

  /* registers state after initialization:
    eax: free 
    ebx: free
    ecx: (int) height
    edx: free (for the inner loop counter)
    esi: (char *) source surface pointer
    edi: (char *) destination surface pointer
    ebp: free (for the lookup table base pointer)
    LOCAL1: (const int) width/ratio
    LOCAL2: (const int) offset from the end of a line to the beginning of next
    LOCAL3: (const int) same as LOCAL2, but for the dest bitmap
   */
   

/* void _update_24_to_32 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_24_to_32)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_4, LOOP_RATIO_4)
   movl 4(%esi), %ebx  /* init first line */

   _align_
   next_line_24_to_32_no_mmx:
      movl MYLOCAL1, %edx 
      pushl %ecx     

      _align_
      /* 100% Pentium pairable loop */
      /* 9 cycles = 8 cycles/4 pixels + 1 cycle loop */
      next_block_24_to_32_no_mmx:
         shll $8, %ebx             /* ebx = r8g8b0 pixel2       */
         movl (%esi), %eax         /* eax = pixel1              */
         movl 8(%esi), %ecx        /* ecx = pixel4 | r8 pixel 3 */
         movl %eax, (%edi)         /* write pixel1              */
         movb 3(%esi), %bl         /* ebx = pixel2              */ 
         movl %ecx, %eax           /* eax = r8 pixel3           */
         shll $16, %eax            /* eax = r8g0b0 pixel3       */
         movl %ebx, 4(%edi)        /* write pixel2              */
         shrl $8, %ecx             /* ecx = pixel4              */
         movb 6(%esi), %al         /* eax = r8g0b8 pixel3       */
         movb 7(%esi), %ah         /* eax = r8g8b8 pixel3       */ 
         movl %ecx, 12(%edi)       /* write pixel4              */
         movl 16(%esi), %ebx       /* next: ebx = r8g8 pixel2   */
         addl $12, %esi            /* 4 pixels read             */
         movl %eax, 8(%edi)        /* write pixel3              */
         addl $16, %edi            /* 4 pixels written          */
         decl %edx
         jnz next_block_24_to_32_no_mmx
   
      popl %ecx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %ecx
      movl 4(%esi), %ebx  /* init next line */
      jnz next_line_24_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret


/* void _update_16_to_32 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
#ifdef ALLEGRO_MMX
_align_
_update_16_to_32_no_mmx:
#else
FUNC (_update_16_to_32)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_4, LOOP_RATIO_2)
   movl GLOBL(rgb_scale_5335), %ebp
   movl $0, %eax  /* init first line */  

   _align_
   next_line_16_to_32_no_mmx:
      movl MYLOCAL1, %edx
      pushl %ecx

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */
      next_block_16_to_32_no_mmx:
         movl $0, %ebx
         movb (%esi), %al               /* al = low byte pixel1        */
         movl $0, %ecx
         movb 1(%esi), %bl              /* bl = high byte pixel1       */
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel1 */
         movb 2(%esi), %cl              /* cl = low byte pixel2        */
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel1 */ 
         addl $8, %edi                  /* 2 pixels written            */
         addl %ebx, %eax                /* eax = r8g8b8 pixel1         */
         movl $0, %ebx  
         movl 1024(%ebp,%ecx,4), %ecx   /* lookup: ecx = r0g8b8 pixel2 */
         movb 3(%esi), %bl              /* bl = high byte pixel2       */  
         movl %eax, -8(%edi)            /* write pixel1                */
         movl $0, %eax
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel2 */
         addl $4, %esi                  /* 4 pixels read               */
         addl %ebx, %ecx                /* ecx = r8g8b8 pixel2         */ 
         decl %edx         
         movl %ecx, -4(%edi)            /* write pixel2                */         
         jnz next_block_16_to_32_no_mmx
        
      popl %ecx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %ecx
      jnz next_line_16_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret


/* void _update_8_to_32 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
#ifdef ALLEGRO_MMX
_align_
_update_8_to_32_no_mmx:
#else
FUNC (_update_8_to_32)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_4, LOOP_RATIO_4)
   movl $0, %eax     /* init first line */
   movl GLOBL(allegro_palette), %ebp
   movb (%esi), %al  /* init first line */

   _align_
   next_line_8_to_32_no_mmx:
      movl MYLOCAL1, %edx
      pushl %ecx     

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_32_no_mmx:
         movl $0, %ebx
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel1 */
         movb 1(%esi), %bl          /* bl = pixel2          */
         movl %eax, (%edi)          /* write pixel1         */
         addl $16, %edi             /* 4 pixels written     */
         movl $0, %eax
         movl (%ebp,%ebx,4), %ebx   /* lookup: ebx = pixel2 */
         movb 2(%esi), %al          /* al = pixel3          */
         movl %ebx, -12(%edi)       /* write pixel2         */
         movl $0, %ecx
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel3 */
         movb 3(%esi), %cl          /* cl = pixel4          */
         movl %eax, -8(%edi)        /* write pixel3         */
         movl $0, %eax
         movl (%ebp,%ecx,4), %ecx   /* lookup: ecx = pixel4 */
         movb 4(%esi), %al          /* next: al = pixel1    */ 
         movl %ecx, -4(%edi)        /* write pixel4         */
         addl $4, %esi              /* 4 pixels read        */
         decl %edx
         jnz next_block_8_to_32_no_mmx
        
      popl %ecx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %ecx
      movb (%esi), %al  /* init next line */
      jnz next_line_8_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret


/* void _update_32_to_24 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_32_to_24)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_3, LOOP_RATIO_4)
   movl 4(%esi), %ebx  /* init first line */

   _align_
   next_line_32_to_24_no_mmx:
      movl MYLOCAL1, %edx           
      pushl %ecx
           
      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_32_to_24_no_mmx:
         movl %ebx, %ebp        /* ebp = pixel2                    */
         addl $12, %edi         /* 4 pixels written                */ 
         shll $24, %ebx         /* ebx = b8 pixel2 << 24           */
         movl 12(%esi), %ecx    /* ecx = pixel4                    */
         shll $8, %ecx          /* ecx = pixel4 << 8               */
         movl (%esi), %eax      /* eax = pixel1                    */
         movb 10(%esi), %cl     /* ecx = pixel4 | r8 pixel3        */
         orl  %eax, %ebx        /* ebx = b8 pixel2 | pixel1        */
         movl %ebx, -12(%edi)   /* write pixel1..b8 pixel2         */
         movl %ebp, %eax        /* eax = pixel2                    */ 
         shrl $8, %eax          /* eax = r8g8 pixel2               */
         movl 8(%esi), %ebx     /* ebx = pixel 3                   */
         shll $16, %ebx         /* ebx = g8b8 pixel3 << 16         */
         movl %ecx, -4(%edi)    /* write r8 pixel3..pixel4         */
         orl  %ebx, %eax        /* eax = g8b8 pixel3 | r8g8 pixel2 */
         movl 20(%esi), %ebx    /* next: ebx = pixel 2             */
         movl %eax, -8(%edi)    /* write g8r8 pixel2..b8g8 pixel3  */
         addl $16, %esi         /* 4 pixels read                   */
         decl %edx
         jnz next_block_32_to_24_no_mmx
       
      popl %ecx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %ecx
      movl 4(%esi), %ebx  /* init first line */
      jnz next_line_32_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret
 


/* void _update_16_to_24 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_16_to_24)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_3, LOOP_RATIO_4)
   movl GLOBL(rgb_scale_5335), %ebp

   next_line_16_to_24_no_mmx:
      movl MYLOCAL1, %edx      
      pushl %ecx     

      _align_
      /* 100% Pentium pairable loop */
      /* 22 cycles = 20 cycles/4 pixels + 1 cycle stack + 1 cycle loop */
      next_block_16_to_24_no_mmx:
         movl %edx, -16(%esp)           /* fake pushl %edx                  */
         movl $0, %ebx
         movl $0, %eax  
         movb 7(%esi), %bl              /* bl = high byte pixel4            */
         movl $0, %ecx 
         movb 6(%esi), %al              /* al = low byte pixel4             */         
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel4      */
         movb 4(%esi), %cl              /* cl = low byte pixel3             */         
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel4      */
         movl $0, %edx         
         addl %ebx, %eax                /* eax = r8g8b8 pixel4              */
         movb 5(%esi), %dl              /* dl = high byte pixel3            */          
         shll $8, %eax                  /* eax = r8g8b8 pixel4 << 8         */
         movl 5120(%ebp,%ecx,4), %ecx   /* lookup: ecx = g8b800r0 pixel3    */         
         movl 4096(%ebp,%edx,4), %edx   /* lookup: edx = g8b000r8 pixel3    */
         movl $0, %ebx
         addl %edx, %ecx                /* ecx = g8b800r8 pixel3            */
         movb %dl, %al                  /* eax = pixel4 << 8 | r8 pixel3    */
         movl %eax, 8(%edi)             /* write r8 pixel3..pixel4          */   
         movb 3(%esi), %bl              /* bl = high byte pixel2            */          
         movl $0, %eax                
         movl $0, %edx
         movb 2(%esi), %al              /* al = low byte pixel2             */
         movl 2048(%ebp,%ebx,4), %ebx   /* lookup: ebx = b000r8g8 pixel2    */
         movb 1(%esi), %dl              /* dl = high byte pixel1            */
         addl $12, %edi                 /* 4 pixels written                 */
         movl 3072(%ebp,%eax,4), %eax   /* lookup: eax = b800r0g8 pixel2    */
         addl $8, %esi                  /* 4 pixels read                    */
         addl %ebx, %eax                /* eax = b800r8g8 pixel2            */
         movl (%ebp,%edx,4), %edx       /* lookup: edx = r8g8b0 pixel1      */
         movb %al, %cl                  /* ecx = g8b8 pixel3 | 00g8 pixel2  */
         movl $0, %ebx
         movb %ah, %ch                  /* ecx = g8b8 pixel3 | r8g8 pixel2  */
         movb -8(%esi), %bl             /* bl = low byte pixel1             */
         movl %ecx, -8(%edi)            /* write g8r8 pixel2..b8g8 pixel3   */
         andl $0xff000000, %eax         /* eax = b8 pixel2 << 24            */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel1      */
         /* nop */
         addl %edx, %ebx                /* ebx = r8g8b8 pixel1              */ 
         movl -16(%esp), %edx           /* fake popl %edx                   */
         orl  %ebx, %eax                /* eax = b8 pixel2 << 24 | pixel1   */
         decl %edx
         movl %eax, -12(%edi)           /* write pixel1..b8 pixel2          */                         
         jnz next_block_16_to_24_no_mmx

      popl %ecx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %ecx
      jnz next_line_16_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret


/* void _update_8_to_24 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_8_to_24)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_3, LOOP_RATIO_4)
   movl GLOBL(allegro_palette), %ebp
   movl $0, %eax  /* init first line */

   _align_
   next_line_8_to_24_no_mmx:
      movl MYLOCAL1, %edx      
      pushl %ecx     

      _align_
      /* 100% Pentium pairable loop */
      /* 12 cycles = 11 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_24_no_mmx:
         movl $0, %ebx
         movb 3(%esi), %al               /* al = pixel4                     */
         movb 2(%esi), %bl               /* bl = pixel3                     */
         movl $0, %ecx
         movl 3072(%ebp,%eax,4), %eax    /* lookup: eax = pixel4 << 8       */
         movb 1(%esi), %cl               /* cl = pixel 2                    */
         movl 2048(%ebp,%ebx,4), %ebx    /* lookup: ebx = g8b800r8 pixel3   */ 
         addl $12, %edi                  /* 4 pixels written                */ 
         movl 1024(%ebp,%ecx,4), %ecx    /* lookup: ecx = b800r8g8 pixel2   */
         movb %bl, %al                   /* eax = pixel4 << 8 | r8 pixel3   */ 
         movl %eax, -4(%edi)             /* write r8 pixel3..pixel4         */  
         movl $0, %eax
         movb %cl, %bl                   /* ebx = g8b8 pixel3 | 00g8 pixel2 */
         movb (%esi), %al                /* al = pixel1                     */
         movb %ch, %bh                   /* ebx = g8b8 pixel3 | r8g8 pixel2 */
         andl $0xff000000, %ecx          /* ecx = b8 pixel2 << 24           */
         movl %ebx, -8(%edi)             /* write g8r8 pixel2..b8g8 pixel3  */
         movl (%ebp,%eax,4), %eax        /* lookup: eax = pixel1            */ 
         orl  %eax, %ecx                 /* ecx = b8 pixel2 << 24 | pixel1  */
         movl $0, %eax  
         movl %ecx, -12(%edi)            /* write pixel1..b8 pixel2         */
         addl $4, %esi                   /* 4 pixels read                   */ 
         decl %edx
         jnz next_block_8_to_24_no_mmx
       
      popl %ecx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %ecx
      jnz next_line_8_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret


#define CONV_TRUE_TO_16_NO_MMX(name, pixsize_plus_0, pixsize_plus_1, pixsize_plus_2, pixsize_times_2)  \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %edx                                                        ; \
      pushl %ecx                                                                 ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */                         ; \
      next_block_##name:                                                         ; \
         movb pixsize_plus_0(%esi), %al   /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb pixsize_plus_1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb (%esi), %cl                 /* cl = b8 pixel1                  */  ; \
         shrb $3, %cl                     /* cl = b5 pixel1                  */  ; \
         movb pixsize_plus_2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shll $16, %eax                   /* eax = r8b5 pixel2 << 16         */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shrl $5, %ebx                    /* ebx = g6 pixel2 | g6 pixel1     */  ; \
         movb 2(%esi), %ch                /* ecx = r8b5 pixel1               */  ; \
         orl  %ecx, %eax                  /* eax = r8b5 pixel2 | r8b5 pixel1 */  ; \
         addl $pixsize_times_2, %esi      /* 2 pixels read                   */  ; \
         andl $0xf81ff81f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x07e007e0, %ebx           /* clean g6 pixel2 | g6 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %edx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
      popl %ecx                                                                  ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl %ecx                                                                  ; \
      jnz next_line_##name


/* void _update_32_to_16 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
#ifdef ALLEGRO_MMX
_align_
_update_32_to_16_no_mmx:
#else
FUNC (_update_32_to_16)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2, LOOP_RATIO_2)  
   CONV_TRUE_TO_16_NO_MMX(32_to_16_no_mmx, 4, 5, 6, 8)
   DESTROY_STACK_FRAME
   ret


/* void _update_24_to_16 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_24_to_16)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2, LOOP_RATIO_2)
   CONV_TRUE_TO_16_NO_MMX(24_to_16_no_mmx, 3, 4, 5, 6)
   DESTROY_STACK_FRAME
   ret


/* void _update_8_to_16 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
/* void _update_8_to_15 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
#ifdef ALLEGRO_MMX
_align_
_update_8_to_16_no_mmx:
#else
FUNC (_update_8_to_16)
FUNC (_update_8_to_15)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_2, LOOP_RATIO_4)
   movl GLOBL(allegro_palette), %ebp
   movl $0, %eax  /* init first line */

   _align_
   next_line_8_to_16_no_mmx:
      movl MYLOCAL1, %edx        
      pushl %ecx
                 
      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_16_no_mmx:
         movl $0, %ebx
         movb (%esi), %al             /* al = pixel1            */
         movl $0, %ecx
         movb 1(%esi), %bl            /* bl = pixel2            */
         movb 2(%esi), %cl            /* cl = pixel3            */
         movl (%ebp,%eax,4), %eax     /* lookup: ax = pixel1    */
         movl 1024(%ebp,%ebx,4), %ebx /* lookup: bx = pixel2    */
         addl $4, %esi                /* 4 pixels read          */
         orl  %ebx, %eax              /* eax = pixel2..pixel1   */
         movl $0, %ebx
         movl %eax, (%edi)            /* write pixel1, pixel2   */
         movb -1(%esi), %bl           /* bl = pixel4            */
         movl (%ebp,%ecx,4), %ecx     /* lookup: cx = pixel3    */
         movl $0, %eax
         movl 1024(%ebp,%ebx,4), %ebx /* lookup: bx = pixel4    */
         addl $8, %edi                /* 4 pixels written       */
         orl  %ebx, %ecx              /* ecx = pixel4..pixel3   */
         decl %edx
         movl %ecx, -4(%edi)          /* write pixel3, pixel4   */
         jnz next_block_8_to_16_no_mmx

      popl %ecx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %ecx
      jnz next_line_8_to_16_no_mmx
 
   DESTROY_STACK_FRAME
   ret


#define CONV_TRUE_TO_15_NO_MMX(name, pixsize_plus_0, pixsize_plus_1, pixsize_plus_2, pixsize_times_2)  \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %edx                                                        ; \
      pushl %ecx                                                                 ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 11 cycles = 10 cycles/2 pixels + 1 cycle loop */                        ; \
      next_block_##name:                                                         ; \
         movb pixsize_plus_0(%esi), %al   /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb pixsize_plus_1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb pixsize_plus_2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shrb $1, %ah                     /* eax = r7b5 pixel2               */  ; \
         movb (%esi), %cl                 /* cl = b8 pixel1                  */  ; \
         shrb $3, %cl                     /* cl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shll $16, %eax                   /* eax = r7b5 pixel2 << 16         */  ; \
         movb 2(%esi), %ch                /* ecx = r8b5 pixel1               */  ; \
         shrb $1, %ch                     /* ecx = r7b5 pixel1               */  ; \
         addl $pixsize_times_2, %esi      /* 2 pixels read                   */  ; \
         shrl $6, %ebx                    /* ebx = g5 pixel2 | g5 pixel1     */  ; \
         orl  %ecx, %eax                  /* eax = r7b5 pixel2 | r7b5 pixel1 */  ; \
         andl $0x7c1f7c1f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x03e003e0, %ebx           /* clean g5 pixel2 | g5 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %edx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
      popl %ecx                                                                  ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl %ecx                                                                  ; \
      jnz next_line_##name


/* void _update_32_to_15 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
#ifdef ALLEGRO_MMX
_align_
_update_32_to_15_no_mmx:
#else
FUNC (_update_32_to_15)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2, LOOP_RATIO_2)  
   CONV_TRUE_TO_15_NO_MMX(32_to_15_no_mmx, 4, 5, 6, 8)
   DESTROY_STACK_FRAME
   ret


/* void _update_24_to_15 (LPDDSURFACEDESC src_desc, LPDDSURFACEDESC dest_desc)
 */
FUNC (_update_24_to_15)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2, LOOP_RATIO_2)
   CONV_TRUE_TO_15_NO_MMX(24_to_15_no_mmx, 3, 4, 5, 6)
   DESTROY_STACK_FRAME
   ret
