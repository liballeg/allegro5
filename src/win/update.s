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
 *      See readme.txt for copyright information.
 */

#include "..\i386\asmdefs.inc"



.text

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
FUNC (_update_8_to_16)
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
