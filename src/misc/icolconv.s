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
 *      Support for blocks of any width, optimizations and additional
 *       MMX routines by Robert J Ohannessian.
 *
 *      See readme.txt for copyright information.
 */

#include "src/i386/asmdefs.inc"


.text


/* local variables */
#define LOCAL1   -4(%esp)
#define LOCAL2   -8(%esp)
#define LOCAL3   -12(%esp)
#define LOCAL4   -16(%esp)


#ifdef ALLEGRO_MMX

/* it seems pand is broken in GAS 2.8.1 */
#define PAND(src, dst)   \
   .byte 0x0f, 0xdb    ; \
   .byte 0xc0 + 8*dst + src  /* mod field */

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
      movl ARG1, %eax                  /* eax = src_rect                 */        ; \
      movl GFXRECT_WIDTH(%eax), %ecx   /* ecx = src_rect->width          */        ; \
      movl GFXRECT_HEIGHT(%eax), %edx  /* edx = src_rect->height         */        ; \
      movl GFXRECT_DATA(%eax), %esi    /* esi = src_rect->data           */        ; \
      movl GFXRECT_PITCH(%eax), %eax   /* eax = src_rect->pitch          */        ; \
      shll $2, %ecx                    /* ecx = SCREEN_W * 4             */        ; \
      subl %ecx, %eax                  /* eax = (src_rect->pitch) - ecx  */        ; \
                                                                                   ; \
      movl ARG2, %ebx                  /* ebx = dest_rect                */        ; \
      shrl $1, %ecx                    /* ecx = SCREEN_W * 2             */        ; \
      movl GFXRECT_DATA(%ebx), %edi    /* edi = dest_rect->data          */        ; \
      movl GFXRECT_PITCH(%ebx), %ebx   /* ebx = dest_rect->pitch         */        ; \
      subl %ecx, %ebx                  /* ebx = (dest_rect->pitch) - ecx */        ; \
      shrl $1, %ecx                    /* ecx = SCREEN_W                 */


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
      movl ARG1, %eax                  /* eax = src_rect                 */        ; \
      movl GFXRECT_WIDTH(%eax), %ecx   /* ecx = src_rect->width          */        ; \
      movl GFXRECT_HEIGHT(%eax), %edx  /* edx = src_rect->height         */        ; \
      movl GFXRECT_DATA(%eax), %esi    /* esi = src_rect->data           */        ; \
      movl GFXRECT_PITCH(%eax), %eax   /* eax = src_rect->pitch          */        ; \
      addl %ecx, %ecx                  /* ecx = SCREEN_W * 2             */        ; \
      subl %ecx, %eax                  /* eax = (src_rect->pitch) - ecx  */        ; \
                                                                                   ; \
      movl ARG2, %ebx                  /* ebx = dest_rect                */        ; \
      addl %ecx, %ecx                  /* edx = SCREEN_W * 4             */        ; \
      movl GFXRECT_DATA(%ebx), %edi    /* edi = dest_rect->data          */        ; \
      movl GFXRECT_PITCH(%ebx), %ebx   /* ebx = dest_rect->pitch         */        ; \
      subl %ecx, %ebx                  /* ebx = (dest_rect->pitch) - ecx */        ; \
      shrl $2, %ecx                    /* ecx = SCREEN_W                 */

#endif


/* void _colorconv_memcpy (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect, int bpp)
 */
FUNC (_colorconv_memcpy)
   
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi
   
   movl ARG3, %ebx
   movl ARG1, %eax                  /* eax = src_rect                 */
   movl GFXRECT_WIDTH(%eax), %eax   
   mull %ebx
   movl %eax, %ecx                  /* ecx = src_rect->width * bpp    */
   movl ARG1, %eax
   movl GFXRECT_HEIGHT(%eax), %edx  /* edx = src_rect->height         */
   movl GFXRECT_DATA(%eax), %esi    /* esi = src_rect->data           */
   movl GFXRECT_PITCH(%eax), %eax   /* eax = src_rect->pitch          */
   subl %ecx, %eax                  /* eax = (src_rect->pitch) - ecx  */

   movl ARG2, %ebx                  /* ebx = dest_rect                */
   movl GFXRECT_DATA(%ebx), %edi    /* edi = dest_rect->data          */
   movl GFXRECT_PITCH(%ebx), %ebx   /* ebx = dest_rect->pitch         */
   subl %ecx, %ebx                  /* ebx = (dest_rect->pitch) - ecx */
   
   pushl %ecx

#ifdef ALLEGRO_MMX
   movl GLOBL(cpu_mmx), %ecx        /* if MMX is enabled (or not disabled :) */
   orl %ecx, %ecx
   jz _colorconv_memcpy_no_mmx

   popl %ecx
   movd %ecx, %mm7       /* Save for later */
   shrl $5, %ecx        /* We work with 32 pixels at a time */
   movd %ecx, %mm6
   
   _align_
   _colorconv_memcpy_line:
      movd %mm6, %ecx
      orl %ecx, %ecx
      jz _colorconv_memcpy_one_byte
   
   _align_
   _colorconv_memcpy_loop:
      movq (%esi),    %mm0   /* Read */
      movq 8(%esi),   %mm1
      addl $32, %esi
      movq -16(%esi), %mm2
      movq -8(%esi),  %mm3
      
      movq %mm0,    (%edi)   /* Write */
      movq %mm1,   8(%edi)
      addl $32, %edi
      movq %mm2, -16(%edi)
      movq %mm3,  -8(%edi)
      
      decl %ecx
      jnz _colorconv_memcpy_loop
      
   _colorconv_memcpy_one_byte:
      movd %mm7, %ecx
      andl $31, %ecx
      jz _colorconv_memcpy_end_line
      
      shrl $1, %ecx
      jnc _colorconv_memcpy_two_bytes

      movsb      
      
   _colorconv_memcpy_two_bytes:
      shrl $1, %ecx
      jnc _colorconv_memcpy_four_bytes
      
      movsb      
      movsb      
     
   _align_
   _colorconv_memcpy_four_bytes:
      shrl $1, %ecx
      jnc _colorconv_memcpy_eight_bytes
      
      movsl
   
   _align_
   _colorconv_memcpy_eight_bytes:
      shrl $1, %ecx
      jnc _colorconv_memcpy_sixteen_bytes
      
      movq (%esi), %mm0
      addl $8, %esi
      movq %mm0, (%edi)
      addl $8, %edi
   
   _align_
   _colorconv_memcpy_sixteen_bytes:
      shrl $1, %ecx
      jnc _colorconv_memcpy_end_line
      
      movq (%esi), %mm0
      movq 8(%esi), %mm1
      addl $16, %esi
      movq %mm0, (%edi)
      movq %mm1, 8(%edi)
      addl $16, %edi
      
   _align_
   _colorconv_memcpy_end_line:
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz _colorconv_memcpy_loop
      
      emms
      jmp _colorconv_memcpy_end
#endif

   _align_
   _colorconv_memcpy_no_mmx:    
      popl %ecx
      pushl %ecx
      shrl $2, %ecx
      
      orl %ecx, %ecx
      jz _colorconv_memcpy_no_mmx_one_byte
      
      rep; movsl
      
     _colorconv_memcpy_no_mmx_one_byte:
      popl %ecx
      pushl %ecx
      
      andl $3, %ecx
      jz _colorconv_memcpy_no_mmx_end_line
      shrl $1, %ecx
      jnc _colorconv_memcpy_no_mmx_two_bytes
      
      movsb
      
     _colorconv_memcpy_no_mmx_two_bytes:
      shrl $1, %ecx
      jnc _colorconv_memcpy_no_mmx_end_line
      
      movsb
      movsb
      
   _align_
   _colorconv_memcpy_no_mmx_end_line:
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz _colorconv_memcpy_no_mmx
      
      popl %ecx

_colorconv_memcpy_end:
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret
   

/* void _colorconv_blit_16_to_16 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 * Can also be used for 15->15
 */
FUNC (_colorconv_blit_16_to_16)

   pushl %ebp
   movl %esp, %ebp
   
   pushl $2
   pushl ARG2
   pushl ARG1
   
   call GLOBL(_colorconv_memcpy)
   addl $12, %esp
   
   popl %ebp
   ret
   
/* void _colorconv_blit_24_to_24 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_24)

   pushl %ebp
   movl %esp, %ebp

   pushl $3
   pushl ARG2
   pushl ARG1
   
   call GLOBL(_colorconv_memcpy)
   addl $12, %esp
   
   popl %ebp
   ret

/* void _colorconv_blit_32_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_32)

   pushl %ebp
   movl %esp, %ebp

   pushl $4
   pushl ARG2
   pushl ARG1
   
   call GLOBL(_colorconv_memcpy)
   addl $12, %esp
   
   popl %ebp
   ret

#ifdef ALLEGRO_MMX   

/* void _colorconv_blit_32_to_16 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_16)

   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_32_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_1 ($0xf80000, $0x00fc00, $0x0000f8);

   /* 32 bit to 16 bit conversion:
    we have:
    esi = src_rect->data
    edi = dest_rect->data
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
   */
   
   movd %ecx, %mm7    /* Save for later */
   shrl $1, %ecx      /* Work in packs of 2 pixels */
   
   _align_
   next_line_32_to_16:
      orl %ecx, %ecx
      jz _colorconv_32_to_16_one_pixel  /* 1 pixel? Can't use dual-pixel code */
   
   _align_
   next_block_32_to_16:
      movq (%esi), %mm0  /* Read 2 pixels */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 - Get Blue component */
      PAND (3, 1)        /* pand %mm3, %mm1 - Get Red component */
      PAND (4, 2)        /* pand %mm4, %mm2 - Get Green component */
      
      psrld $3, %mm0     /* Adjust Red, Green and Blue to correct positions */
      psrld $5, %mm1
      psrld $8, %mm2     
      por %mm1, %mm0     /* Combine Red and Blue */
      addl $8, %esi
      por %mm2, %mm0     /* And Green */
      
      movq %mm0, %mm6    /* Make the pixels fit in the first 32 bits */
      psrlq $16, %mm0
      por %mm0, %mm6
      addl $4, %edi
      movd %mm6, -4(%edi) /* Write */

      decl %ecx
      jnz next_block_32_to_16

      movd %mm7, %ecx
      shrl $1, %ecx
      jnc end_of_line_32_to_16

   _colorconv_32_to_16_one_pixel:
      movd (%esi), %mm0
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 - Get Blue component */
      PAND (3, 1)        /* pand %mm3, %mm1 - Get Red component */
      PAND (4, 2)        /* pand %mm4, %mm2 - Get Green component */
      
      psrld $3, %mm0     /* Adjust Red, Green and Blue to correct positions */
      psrld $5, %mm1
      psrld $8, %mm2
      por %mm1, %mm0     /* Combine Red and Blue */
      addl $4, %esi
      por %mm2, %mm0     /* And Green */
      
      movq %mm0, %mm6    /* Make the pixels fit in the first 32 bits */
      psrlq $16, %mm0
      por %mm0, %mm6
      
      movd %mm6, %ecx
      addl $2, %edi
      movw %cx, -2(%edi) /* Write */
      
      movd %mm7, %ecx    /* Restore the clobbered %ecx */
      shrl $1, %ecx
      
      _align_
   end_of_line_32_to_16:
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz next_line_32_to_16

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



/* void _colorconv_blit_32_to_15 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_15)

   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_32_to_15_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_1 ($0xf80000, $0x00f800, $0x0000f8);

   /* 32 bit to 15 bit conversion:
    we have:
    esi = src_rect->data
    edi = dest_rect->data
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
   */
   
   movd %ecx, %mm7    /* Save for later */
   shrl $1, %ecx      /* Work in packs of 2 pixels */
   
   _align_
   next_line_32_to_15:
      orl %ecx, %ecx
      jz _colorconv_32_to_15_one_pixel  /* 1 pixel? Can't use dual-pixel code */
   
   _align_
   next_block_32_to_15:
      movq (%esi), %mm0  /* Read 2 pixels */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 - Get Blue component */
      PAND (3, 1)        /* pand %mm3, %mm1 - Get Red component */
      PAND (4, 2)        /* pand %mm4, %mm2 - Get Green component */
      
      psrld $3, %mm0     /* Adjust Red, Green and Blue to correct positions */
      psrld $6, %mm1
      psrld $9, %mm2
      por %mm1, %mm0     /* Combine Red and Blue */
      addl $8, %esi
      por %mm2, %mm0     /* And Green */
      
      movq %mm0, %mm6    /* Make the pixels fit in the first 32 bits */
      psrlq $16, %mm0
      por %mm0, %mm6
      addl $4, %edi
      movd %mm6, -4(%edi) /* Write */

      decl %ecx
      jnz next_block_32_to_15

      movd %mm7, %ecx
      shrl $1, %ecx
      jnc end_of_line_32_to_15

   _colorconv_32_to_15_one_pixel:
      movd (%esi), %mm0
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 - Get Blue component */
      PAND (3, 1)        /* pand %mm3, %mm1 - Get Red component */
      PAND (4, 2)        /* pand %mm4, %mm2 - Get Green component */
      
      psrld $3, %mm0     /* Adjust Red, Green and Blue to correct positions */
      psrld $6, %mm1
      psrld $9, %mm2
      por %mm1, %mm0     /* Combine Red and Blue */
      addl $4, %esi
      por %mm2, %mm0     /* And Green */
      
      movq %mm0, %mm6    /* Make the pixels fit in the first 32 bits */
      psrlq $16, %mm0
      por %mm0, %mm6
      
      movd %mm6, %ecx
      addl $2, %edi
      movw %cx, -2(%edi) /* Write */
      
      movd %mm7, %ecx    /* Restore the clobbered %ecx */
      shrl $1, %ecx
      
      _align_
   end_of_line_32_to_15:
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz next_line_32_to_15

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret


/* void _colorconv_blit_16_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_16_to_32)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_16_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_2 ($0xf800, $0x07e0, $0x001f);

   /* 16 bit to 32 bit conversion:
    we have:
    esi = src_rect->data
    edi = dest_rect->data
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
   */
   movl $0x70307, %ebp
   movd %ebp, %mm7
   punpckldq %mm7, %mm7  /* Build add mask */
   
   movl %ecx, %ebp    /* Save for later */
   shrl $2, %ecx      /* Work in packs of 4 pixels */

   
   _align_
   next_line_16_to_32:
      orl %ecx, %ecx
      jz _colorconv_16_to_32_one_pixel  /* 1 pixel? Can't use dual-pixel code */

   _align_
   next_block_16_to_32:
      movq (%esi), %mm0    /* mm0 = [rgb1][rgb2][rgb3][rgb4] */
      movq %mm0, %mm6
      punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb3] xxxx [rgb4]  (x don't matter) */
      punpckhwd %mm6, %mm6 /* mm6 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm0
      pslld $5, %mm1
      pslld $8, %mm2
      por %mm1, %mm0
      addl $8, %esi
      por %mm2, %mm0
      movq %mm6, %mm1
      paddd %mm7, %mm0   /* Add adjustment factor */
      movq %mm6, %mm2
      
      movq %mm0, (%edi)  /* Write */
      
      PAND (5, 6)        /* pand %mm5, %mm6 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm6
      pslld $5, %mm1
      pslld $8, %mm2
      por %mm1, %mm6
      por %mm2, %mm6
      addl $16, %edi
      paddd %mm7, %mm6   /* Add adjustment factor */
      
      movq %mm6, -8(%edi) /* Write */
      
      decl %ecx
      jnz next_block_16_to_32
      
    _colorconv_16_to_32_one_pixel:
      movl %ebp, %ecx
      andl $3, %ecx
      jz end_of_line_16_to_32
      
      shrl $1, %ecx
      jnc _colorconv_16_to_32_two_pixels
      
      movzwl (%esi), %ecx
      movd %ecx, %mm0      /* mm0 = xxxx xxxx [rgb1][rgb2] */
      punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm0
      pslld $5, %mm1
      pslld $8, %mm2
      por %mm1, %mm0
      addl $2, %esi
      por %mm2, %mm0
      paddd %mm7, %mm0   /* Add adjustment factor */
      movd %mm0, (%edi) /* Write */
      addl $4, %edi
      
      movl %ebp, %ecx
      shrl $1, %ecx
      
    _colorconv_16_to_32_two_pixels:
      shrl $1, %ecx
      jnc end_of_line_16_to_32
      
      movd (%esi), %mm0    /* mm0 = xxxx xxxx [rgb1][rgb2] */
      punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm0
      pslld $5, %mm1
      pslld $8, %mm2
      por %mm1, %mm0
      addl $4, %esi
      por %mm2, %mm0
      paddd %mm7, %mm0   /* Add adjustment factor */
      movq %mm0, (%edi) /* Write */
      addl $8, %edi

    _align_
    end_of_line_16_to_32:
      movl %ebp, %ecx
      shrl $2, %ecx
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz next_line_16_to_32
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret


/* void _colorconv_blit_15_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_32)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_15_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_2 ($0x7c00, $0x03e0, $0x001f);

   /* 15 bit to 32 bit conversion:
    we have:
    esi = src_rect->data
    edi = dest_rect->data
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
   */
   movl $0x70707, %ebp
   movd %ebp, %mm7
   punpckldq %mm7, %mm7  /* Build add mask */
   
   movl %ecx, %ebp    /* Save for later */
   shrl $2, %ecx      /* Work in packs of 4 pixels */
   
   _align_
   next_line_15_to_32:
      orl %ecx, %ecx
      jz _colorconv_15_to_32_one_pixel  /* 1 pixel? Can't use dual-pixel code */

   _align_
   next_block_15_to_32:
      movq (%esi), %mm0    /* mm0 = [rgb1][rgb2][rgb3][rgb4] */
      movq %mm0, %mm6
      punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb3] xxxx [rgb4]  (x don't matter) */
      punpckhwd %mm6, %mm6 /* mm6 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm0
      pslld $6, %mm1
      pslld $9, %mm2
      por %mm1, %mm0
      addl $8, %esi
      por %mm2, %mm0
      movq %mm6, %mm1
      paddd %mm7, %mm0   /* Add adjustment factor */
      movq %mm6, %mm2
      
      movq %mm0, (%edi)  /* Write */
      
      PAND (5, 6)        /* pand %mm5, %mm6 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm6
      pslld $6, %mm1
      pslld $9, %mm2
      por %mm1, %mm6
      por %mm2, %mm6
      addl $16, %edi
      paddd %mm7, %mm6   /* Add adjustment factor */
      
      movq %mm6, -8(%edi) /* Write */
      
      decl %ecx
      jnz next_block_15_to_32
      
    _colorconv_15_to_32_one_pixel:
      movl %ebp, %ecx
      andl $3, %ecx
      jz end_of_line_15_to_32
      
      shrl $1, %ecx
      jnc _colorconv_15_to_32_two_pixels
      
      movzwl (%esi), %ecx
      movd %ecx, %mm0      /* mm0 = xxxx xxxx [rgb1][rgb2] */
      punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm0
      pslld $6, %mm1
      pslld $9, %mm2
      por %mm1, %mm0
      addl $2, %esi
      por %mm2, %mm0
      paddd %mm7, %mm0   /* Add adjustment factor */
      movd %mm0, (%edi) /* Write */
      addl $4, %edi
      
      movl %ebp, %ecx
      shrl $1, %ecx
      
    _colorconv_15_to_32_two_pixels:
      shrl $1, %ecx
      jnc end_of_line_15_to_32
      
      movd (%esi), %mm0    /* mm0 = xxxx xxxx [rgb1][rgb2] */
      punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
      movq %mm0, %mm1
      movq %mm0, %mm2
      
      PAND (5, 0)        /* pand %mm5, %mm0 */
      PAND (3, 1)        /* pand %mm3, %mm1 */
      PAND (4, 2)        /* pand %mm4, %mm2 */
      
      pslld $3, %mm0
      pslld $6, %mm1
      pslld $9, %mm2
      por %mm1, %mm0
      addl $4, %esi
      por %mm2, %mm0
      paddd %mm7, %mm0   /* Add adjustment factor */
      movq %mm0, (%edi) /* Write */
      addl $8, %edi

    _align_
    end_of_line_15_to_32: 
      movl %ebp, %ecx
      shrl $2, %ecx
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz next_line_15_to_32
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret


/* void _colorconv_blit_8_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_8_to_32)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_8_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax
   movl %eax, LOCAL2                  /* LOCAL2 = src_rect->pitch - SCREEN_W */

   movl ARG2, %ebx                    /* ebx = &*dest_rect      */
   shll $2, %ecx                      /* ecx = SCREEN_W * 4     */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   movl %ebx, LOCAL3                  /* LOCAL3 = (dest_rect->pitch) - (SCREEN_W * 4) */
   shrl $2, %ecx                      /* ecx = SCREEN_W         */
   movl GLOBL(_colorconv_indexed_palette), %ebx  /* ebx = _colorconv_indexed_palette  */


   /* 8 bit to 32 bit conversion:
    we have:
    ecx = SCREEN_W
    edx = SCREEN_H
    ebx = _colorconv_indexed_palette
    esi = src_rect->data
    edi = dest_rect->data
    LOCAL2 = offset from the end of a line to the beginning of the next
    LOCAL3 = same as LOCAL2, but for the dest bitmap
   */
   movd %ecx, %mm7              /* Save width for later */
   
   movl $0xFF, %ecx             /* Get byte mask in mm6 */
   movd %ecx, %mm6
   
   movd %mm7, %ecx              /* Restore depth */

   _align_   
   next_line_8_to_32:
      shrl $2, %ecx            /* Work with packs of 4 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_8_to_32  /* Less than 4 pixels? Can't work with the main loop */

   _align_   
   next_block_8_to_32:
      movd (%esi), %mm0         /* Read 4 pixels */
      
      movq %mm0, %mm1           /* Get pixel 1 */
      pand %mm6, %mm0
      movd %mm0, %eax
      
      movd (%ebx,%eax,4), %mm2  /* mm2 = xxxxxxxxxx [    1    ] */
      
      psrlq $8, %mm1
      movq %mm1, %mm0           /* Get pixel 2 */
      pand %mm6, %mm1
      movd %mm1, %eax
      
      movd (%ebx,%eax,4), %mm3  /* mm3 = xxxxxxxxxx [    2    ] */
      
      psrlq $8, %mm0
      movq %mm0, %mm1           /* Get pixel 3 */
      pand %mm6, %mm0
      punpckldq %mm3, %mm2      /* mm2 = [    2    ][    1    ] */
      movd %mm0, %eax
      
      movq %mm2, (%edi)

      movd (%ebx,%eax,4), %mm3  /* mm3 = xxxxxxxxxx [    3    ] */
      
      psrlq $8, %mm1
      movq %mm1, %mm0           /* Get pixel 4 */
      pand %mm6, %mm1
      movd %mm1, %eax

      movd (%ebx,%eax,4), %mm4  /* mm4 = xxxxxxxxxx [    4    ] */
      
      addl $4, %esi
      punpckldq %mm4, %mm3      /* mm3 = [    4    ][    3    ] */
      addl $16, %edi
      
      movq %mm3, -8(%edi)       /* Write 2 pixels */
      
      decl %ecx
      jnz next_block_8_to_32
      
   do_one_pixel_8_to_32:
      movd %mm7, %ecx           /* Restore width */
      andl $3, %ecx
      jz end_of_line_8_to_32    /* Nothing to do? */

      shrl $1, %ecx
      jnc do_two_pixels_8_to_32
      
      movzbl (%esi), %eax       /* Convert 1 pixel */
      movl (%ebx,%eax,4), %eax
      incl %esi
      addl $4, %edi
      movl %eax, -4(%edi)
   
   do_two_pixels_8_to_32:
      shrl $1, %ecx
      jnc end_of_line_8_to_32
      
      movzbl (%esi), %ecx       /* Convert 2 pixels */
      movzbl 1(%esi), %eax
      
      movl (%ebx,%ecx,4), %ecx
      movl (%ebx,%eax,4), %eax
      
      addl $2, %esi
      movl %ecx, (%edi)
      movl %eax, 4(%edi)
      addl $8, %edi

   _align_
   end_of_line_8_to_32:

      addl LOCAL2, %esi
      movd %mm7, %ecx           /* Restore width */
      addl LOCAL3, %edi
      decl %edx
      jnz next_line_8_to_32
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret




/* void _colorconv_blit_8_to_16 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
/* void _colorconv_blit_8_to_15 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_8_to_16)
FUNC (_colorconv_blit_8_to_15)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_8_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax
   movl %eax, LOCAL2                  /* LOCAL2 = src_rect->pitch - SCREEN_W */

   movl ARG2, %ebx                    /* ebx = &*dest_rect      */
   shll $1, %ecx                      /* ecx = SCREEN_W * 2     */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   movl %ebx, LOCAL3                  /* LOCAL3 = (dest_rect->pitch) - (SCREEN_W * 2) */
   shrl $1, %ecx                      /* ecx = SCREEN_W         */
   movl GLOBL(_colorconv_indexed_palette), %ebx  /* ebx = _colorconv_indexed_palette  */


   /* 8 bit to 16 bit conversion:
    we have:
    ecx = SCREEN_W
    edx = SCREEN_H
    ebx = _colorconv_indexed_palette
    esi = src_rect->data
    edi = dest_rect->data
    LOCAL2 = offset from the end of a line to the beginning of the next
    LOCAL3 = same as LOCAL2, but for the dest bitmap
   */
   movd %ecx, %mm7              /* Save width for later */
   
   movl $0xFF, %ecx             /* Get byte mask in mm6 */
   movd %ecx, %mm6
   
   movd %mm7, %ecx              /* Restore depth */

   _align_   
   next_line_8_to_16:
      shrl $2, %ecx            /* Work with packs of 4 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_8_to_16  /* Less than 4 pixels? Can't work with the main loop */

   _align_   
   next_block_8_to_16:
      movd (%esi), %mm0         /* Read 4 pixels */
      
      movq %mm0, %mm1           /* Get pixel 1 */
      pand %mm6, %mm0
      movd %mm0, %eax
      
      movd (%ebx,%eax,4), %mm2  /* mm2 = xxxxxxxxxx xxxxx[ 1 ] */
      
      psrlq $8, %mm1
      movq %mm1, %mm0           /* Get pixel 2 */
      pand %mm6, %mm1
      movd %mm1, %eax
      
      movd (%ebx,%eax,4), %mm3  /* mm3 = xxxxxxxxxx xxxxx[ 2 ] */
      
      psrlq $8, %mm0
      movq %mm0, %mm1           /* Get pixel 3 */
      pand %mm6, %mm0
      punpcklwd %mm3, %mm2      /* mm2 = xxxxxxxxxx [ 2 ][ 1 ] */
      movd %mm0, %eax

      movd (%ebx,%eax,4), %mm3  /* mm3 = xxxxxxxxxx xxxxx[ 3 ] */
      
      psrlq $8, %mm1
      movq %mm1, %mm0           /* Get pixel 4 */
      pand %mm6, %mm1
      movd %mm1, %eax

      movd (%ebx,%eax,4), %mm4  /* mm4 = xxxxxxxxxx xxxxx[ 4 ] */
      
      addl $4, %esi
      punpcklwd %mm4, %mm3      /* mm3 = xxxxxxxxxx [ 4 ][ 3 ] */
      addl $8, %edi
      punpckldq %mm3, %mm2      /* mm2 = [ 4 ][ 3 ] [ 2 ][ 1 ] */
      
      movq %mm2, -8(%edi)       /* Write first 4 pixels */
      
      decl %ecx
      jnz next_block_8_to_16
      
   do_one_pixel_8_to_16:
      movd %mm7, %ecx           /* Restore width */
      andl $3, %ecx
      jz end_of_line_8_to_16    /* Nothing to do? */

      shrl $1, %ecx
      jnc do_two_pixels_8_to_16
      
      movzbl (%esi), %eax       /* Convert 1 pixel */
      movl (%ebx,%eax,4), %eax
      incl %esi
      addl $2, %edi
      movw %ax, -2(%edi)
   
   do_two_pixels_8_to_16:
      shrl $1, %ecx
      jnc end_of_line_8_to_16
      
      movzbl (%esi), %ecx       /* Convert 2 pixels */
      movzbl 1(%esi), %eax
      
      movl (%ebx,%ecx,4), %ecx
      movl (%ebx,%eax,4), %eax
      
      shll $16, %eax
      addl $2, %esi
      orl %eax, %ecx
      addl $4, %edi
      movl %ecx, -4(%edi)

   _align_
   end_of_line_8_to_16:

      addl LOCAL2, %esi
      movd %mm7, %ecx           /* Restore width */
      addl LOCAL3, %edi
      decl %edx
      jnz next_line_8_to_16
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret


/* void _colorconv_blit_24_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_32)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_24_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   leal (%ecx, %ecx, 2), %ebx         /* ebx = SCREEN_W * 3     */
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ebx, %eax

   movl ARG2, %ebx                    /* ebx = &*dest_rect      */
   shll $2, %ecx                      /* ecx = SCREEN_W * 4     */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   shrl $2, %ecx


   /* 24 bit to 32 bit conversion:
    we have:
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = source line pitch - width
    ebx = dest line pitch - width
    esi = src_rect->data
    edi = dest_rect->data
   */
   movd %ecx, %mm7              /* Save width for later */


   _align_   
   next_line_24_to_32:
      shrl $2, %ecx             /* Work with packs of 4 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_24_to_32  /* Less than 4 pixels? Can't work with the main loop */

   _align_   
   /* 11.5 cycles + jmp */
   next_block_24_to_32:
      movq (%esi), %mm0         /* Read 4 pixels */
      movd 8(%esi), %mm1        /* mm1 = [..0..][RGB3][R2] */
      
      movq %mm0, %mm2           /* mm0 = [GB2][RGB1][RGB0] */
      movq %mm0, %mm3
      movq %mm1, %mm4

      psllq $16, %mm2
      psllq $40, %mm0
      psrlq $40, %mm2
      psrlq $40, %mm0           /* mm0 = [....0....][RGB0] */
      psllq $32, %mm2           /* mm2 = [..][RGB1][..0..] */
      psrlq $8, %mm1
      psrlq $48, %mm3           /* mm3 = [.....0....][GB2] */
      psllq $56, %mm4
      psllq $32, %mm1           /* mm1 = [.RGB3][....0...] */
      psrlq $40, %mm4           /* mm4 = [....0...][R2][0] */
      
      por %mm3, %mm1
      por %mm2, %mm0            /* mm0 = [.RGB1][.RGB0] */
      por %mm4, %mm1            /* mm1 = [.RGB3][.RGB2] */
      
      movq %mm0, (%edi)         /* Write */
      movq %mm1, 8(%edi)
      
      addl $12, %esi
      addl $16, %edi

      decl %ecx
      jnz next_block_24_to_32
      
   do_one_pixel_24_to_32:
      movd %mm7, %ecx           /* Restore width */
      andl $3, %ecx
      jz end_of_line_24_to_32   /* Nothing to do? */

      shrl $1, %ecx
      jnc do_two_pixels_24_to_32
      
      xorl %ecx, %ecx           /* Warning, partial registar stalls ahead, 6 cycle penalty on the 686 */
      movzwl (%esi), %ebp
      movb  2(%esi), %cl
      movw  %bp, (%edi)
      movw  %cx, 2(%edi)
      
      addl $3, %esi
      addl $4, %edi

      movd %mm7, %ecx
      shrl $1, %ecx             /* Restore width */
   
   do_two_pixels_24_to_32:
      shrl $1, %ecx
      jnc end_of_line_24_to_32
      
      movd (%esi), %mm0         /* Read 2 pixels */
      movzwl 4(%esi), %ebp      
      movd %ebp, %mm1
      
      movq %mm0, %mm2
      
      pslld $8, %mm1
      pslld $8, %mm0
      psrld $24, %mm2
      psrld $8, %mm0
      
      por %mm2, %mm1
      psllq $32, %mm1
      por %mm1, %mm0
      
      movq %mm0, (%edi)
      
      addl $6, %esi
      addl $8, %edi

   _align_
   end_of_line_24_to_32:

      addl %eax, %esi
      movd %mm7, %ecx           /* Restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_24_to_32
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret

/* void _colorconv_blit_32_to_24 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_24)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_32_to_24_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   shll $2, %ecx
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax

   movl ARG2, %ebx                    /* ebx = &*dest_rect      */
   shrl $2, %ecx                      /* ecx = SCREEN_W * 4     */
   leal (%ecx, %ecx, 2), %ebp         /* ebp = SCREEN_W * 3     */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ebp, %ebx


   /* 32 bit to 24 bit conversion:
    we have:
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = source line pitch - width
    ebx = dest line pitch - width
    esi = src_rect->data
    edi = dest_rect->data
   */
   movd %ecx, %mm7              /* Save width for later */


   _align_   
   next_line_32_to_24:
      shrl $2, %ecx             /* Work with packs of 4 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_32_to_24  /* Less than 4 pixels? Can't work with the main loop */

   _align_   
   /* 12 cycles + jmp */
   next_block_32_to_24:
      movq (%esi), %mm0         /* Read 4 pixels */
      movq 8(%esi), %mm1        /* mm1 = [.RGB3][.RGB2] */
      
      movq %mm0, %mm2           /* mm0 = [.RGB1][.RGB0] */
      movq %mm1, %mm3
      movq %mm1, %mm4
      
      psllq $48, %mm3
      psllq $40, %mm0
      psllq $8, %mm2
      psrlq $40, %mm0
      psrlq $16, %mm2
      
      por %mm3, %mm0
      por %mm2, %mm0
      
      psllq $8, %mm4
      psllq $40, %mm1
      psrlq $32, %mm4
      psrlq $56, %mm1
      psllq $8, %mm4
      
      por %mm4, %mm1
      
      movq %mm0, (%edi)
      movd %mm1, 8(%edi)
      
      addl $16, %esi
      addl $12, %edi

      decl %ecx
      jnz next_block_32_to_24
      
   do_one_pixel_32_to_24:
      movd %mm7, %ecx           /* Restore width */
      andl $3, %ecx
      jz end_of_line_32_to_24   /* Nothing to do? */

      shrl $1, %ecx
      jnc do_two_pixels_32_to_24

      movl (%esi), %ecx
      addl $4, %esi
      movw %cx, (%edi)
      shrl $16, %ecx
      addl $3, %edi
      movb %cl, -1(%edi)
      
      movd %mm7, %ecx
      shrl $1, %ecx             /* Restore width */
   
   do_two_pixels_32_to_24:
      shrl $1, %ecx
      jnc end_of_line_32_to_24

      movq (%esi), %mm0         /* Read 2 pixels */
      
      movq %mm0, %mm1
      
      psllq $40, %mm0
      psrlq $32, %mm1
      psrlq $40, %mm0
      psllq $24, %mm1
      
      por %mm1, %mm0
      
      movd %mm0, (%edi)
      psrlq $32, %mm0
      movd %mm0, %ecx
      movw %cx, 2(%edi)
      
      addl $8, %esi
      addl $6, %edi

   _align_
   end_of_line_32_to_24:

      addl %eax, %esi
      movd %mm7, %ecx           /* Restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_32_to_24
      
   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret


/* void _colorconv_blit_16_to_15 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_16_to_15)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_16_to_15_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   shll $1, %ecx
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax

   movl ARG2, %ebx                    /* ebx = &*dest_rect      */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   shrl $1, %ecx


   /* 16 bit to 15 bit conversion:
    we have:
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = source line pitch - width
    ebx = dest line pitch - width
    esi = src_rect->data
    edi = dest_rect->data
   */
   movd %ecx, %mm7              /* Save width for later */
   
   movl $0xFFC0FFC0, %ecx
   movd %ecx, %mm6
   punpckldq %mm6, %mm6         /* mm6 = reg-green mask */
   movl $0x001F001F, %ecx
   movd %ecx, %mm5
   punpckldq %mm5, %mm5         /* mm4 = blue mask */
   
   movd %mm7, %ecx

   _align_   
   next_line_16_to_15:
      shrl $3, %ecx             /* Work with packs of 8 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_16_to_15  /* Less than 8 pixels? Can't work with the main loop */

   _align_   
   next_block_16_to_15:
      movq (%esi), %mm0         /* Read 8 pixels */
      movq 8(%esi), %mm1        /* mm1 = [rgb7][rgb6][rgb5][rgb4] */

      addl $16, %esi
      addl $16, %edi

      movq %mm0, %mm2           /* mm0 = [rgb3][rgb2][rgb1][rgb0] */
      movq %mm1, %mm3

      pand %mm6, %mm0           /* Isolate red-green */
      pand %mm6, %mm1
      pand %mm5, %mm2           /* Isolate blue */
      pand %mm5, %mm3

      psrlq $1, %mm0            /* Shift red-green by 1 bit to the right */
      psrlq $1, %mm1

      por %mm2, %mm0            /* Recombine components */
      por %mm3, %mm1

      movq %mm0, -16(%edi)      /* Write result */
      movq %mm1, -8(%edi)
      decl %ecx
      jnz next_block_16_to_15

    do_one_pixel_16_to_15:
       movd %mm7, %ecx          /* Anything left to do? */
       andl $7, %ecx
       jz end_of_line_16_to_15

       shrl $1, %ecx            /* Do one pixel */
       jnc do_two_pixels_16_to_15

       movzwl (%esi), %ecx      /* Read one pixel */
       addl $2, %esi

       movd %ecx, %mm0       
       movd %ecx, %mm2

       pand %mm6, %mm0
       pand %mm5, %mm2

       psrlq $1, %mm0
       por %mm2, %mm0

       movd %mm0, %ecx
       addl $2, %edi

       movw %cx, -2(%edi)

       movd %mm7, %ecx
       shrl $1, %ecx

     do_two_pixels_16_to_15:
       shrl $1, %ecx
       jnc do_four_pixels_16_to_15

       movd (%esi), %mm0      /* Read two pixel */
       addl $4, %esi

       movq %mm0, %mm2

       pand %mm6, %mm0
       pand %mm5, %mm2

       psrlq $1, %mm0
       por %mm2, %mm0

       addl $4, %edi

       movd %mm0, -4(%edi)

     _align_
     do_four_pixels_16_to_15:
       shrl $1, %ecx
       jnc end_of_line_16_to_15

       movq (%esi), %mm0      /* Read four pixel */
       addl $8, %esi

       movq %mm0, %mm2

       pand %mm6, %mm0
       pand %mm5, %mm2

       psrlq $1, %mm0
       por %mm2, %mm0

       addl $8, %edi

       movd %mm0, -8(%edi)

   _align_
   end_of_line_16_to_15:

      addl %eax, %esi
      movd %mm7, %ecx           /* Restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_16_to_15

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp
      
   ret


/* void _colorconv_blit_15_to_16 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_16)
   movl GLOBL(cpu_mmx), %eax     /* if MMX is enabled (or not disabled :) */
   test %eax, %eax
   jz _colorconv_blit_15_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   shll $1, %ecx
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax

   movl ARG2, %ebx                    /* ebx = &*dest_rect      */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   shrl $1, %ecx


   /* 15 bit to 16 bit conversion:
    we have:
    ecx = SCREEN_W
    edx = SCREEN_H
    eax = source line pitch - width
    ebx = dest line pitch - width
    esi = src_rect->data
    edi = dest_rect->data
   */
   movd %ecx, %mm7              /* Save width for later */
   
   movl $0x7FE07FE0, %ecx
   movd %ecx, %mm6
   movl $0x00200020, %ecx       /* Addition to green component */
   punpckldq %mm6, %mm6         /* mm6 = reg-green mask */
   movd %ecx, %mm4
   movl $0x001F001F, %ecx
   punpckldq %mm4, %mm4         /* mm4 = green add mask */
   movd %ecx, %mm5
   punpckldq %mm5, %mm5         /* mm5 = blue mask */

   movd %mm7, %ecx

   _align_   
   next_line_15_to_16:
      shrl $3, %ecx             /* Work with packs of 8 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_15_to_16  /* Less than 8 pixels? Can't work with the main loop */

   _align_   
   next_block_15_to_16:
      movq (%esi), %mm0         /* Read 8 pixels */
      movq 8(%esi), %mm1        /* mm1 = [rgb7][rgb6][rgb5][rgb4] */

      addl $16, %esi
      addl $16, %edi

      movq %mm0, %mm2           /* mm0 = [rgb3][rgb2][rgb1][rgb0] */
      movq %mm1, %mm3

      pand %mm6, %mm0           /* Isolate red-green */
      pand %mm6, %mm1
      pand %mm5, %mm2           /* Isolate blue */
      pand %mm5, %mm3

      psllq $1, %mm0            /* Shift red-green by 1 bit to the left */
      psllq $1, %mm1
      por %mm4, %mm0            /* Set missing bit to 1 */
      por %mm4, %mm1

      por %mm2, %mm0            /* Recombine components */
      por %mm3, %mm1

      movq %mm0, -16(%edi)      /* Write result */
      movq %mm1, -8(%edi)
      decl %ecx
      jnz next_block_15_to_16

    do_one_pixel_15_to_16:
       movd %mm7, %ecx          /* Anything left to do? */
       andl $7, %ecx
       jz end_of_line_15_to_16

       shrl $1, %ecx            /* Do one pixel */
       jnc do_two_pixels_15_to_16

       movzwl (%esi), %ecx      /* Read one pixel */
       addl $2, %esi

       movd %ecx, %mm0       
       movd %ecx, %mm2

       pand %mm6, %mm0
       pand %mm5, %mm2

       psllq $1, %mm0
       por %mm4, %mm0
       por %mm2, %mm0

       addl $2, %edi

       movd %mm0, %ecx

       movw %cx, -2(%edi)

       movd %mm7, %ecx
       shrl $1, %ecx

     do_two_pixels_15_to_16:
       shrl $1, %ecx
       jnc do_four_pixels_15_to_16

       movd (%esi), %mm0      /* Read two pixel */
       addl $4, %esi

       movq %mm0, %mm2

       pand %mm6, %mm0
       pand %mm5, %mm2

       psllq $1, %mm0
       addl $4, %edi
       por %mm4, %mm0
       por %mm2, %mm0

       movd %mm0, -4(%edi)

     _align_
     do_four_pixels_15_to_16:
       shrl $1, %ecx
       jnc end_of_line_15_to_16

       movq (%esi), %mm0      /* Read four pixel */
       addl $8, %esi

       movq %mm0, %mm2

       pand %mm6, %mm0
       pand %mm5, %mm2

       psllq $1, %mm0
       por %mm4, %mm0
       por %mm2, %mm0

       addl $8, %edi

       movq %mm0, -8(%edi)

   _align_
   end_of_line_15_to_16:

      addl %eax, %esi
      movd %mm7, %ecx           /* Restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_15_to_16

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

#define INIT_REGISTERS_NO_MMX(src_mul_code, dest_mul_code)                        \
   movl ARG1, %eax                  /* eax    = src_rect                    */  ; \
   movl GFXRECT_WIDTH(%eax), %ebx   /* ebx    = src_rect->width             */  ; \
   movl GFXRECT_HEIGHT(%eax), %edx  /* edx    = src_rect->height            */  ; \
   movl GFXRECT_PITCH(%eax), %ecx   /* ecx    = src_rect->pitch             */  ; \
   movl %ebx, %edi                  /* edi    = width                       */  ; \
   src_mul_code                     /* ebx    = width*x                     */  ; \
   movl GFXRECT_DATA(%eax), %esi    /* esi    = src_rect->data              */  ; \
   subl %ebx, %ecx                                                              ; \
   movl %edi, %ebx                                                              ; \
   movl ARG2, %eax                  /* eax    = dest_rect                   */  ; \
   movl %edi, MYLOCAL1              /* LOCAL1 = width                       */  ; \
   movl %ecx, MYLOCAL2              /* LOCAL2 = src_rect->pitch - width*x   */  ; \
   dest_mul_code                    /* ebx    = width*y                     */  ; \
   movl GFXRECT_PITCH(%eax), %ecx   /* ecx    = dest_rect->pitch            */  ; \
   subl %ebx, %ecx                                                              ; \
   movl GFXRECT_DATA(%eax), %edi    /* edi    = dest_rect->data             */  ; \
   movl %ecx, MYLOCAL3              /* LOCAL3 = dest_rect->pitch - width*y  */

  /* registers state after initialization:
    eax: free 
    ebx: free
    ecx: free (for the inner loop counter)
    edx: (int) height
    esi: (char *) source surface pointer
    edi: (char *) destination surface pointer
    ebp: free (for the lookup table base pointer)
    LOCAL1: (const int) width
    LOCAL2: (const int) offset from the end of a line to the beginning of next
    LOCAL3: (const int) same as LOCAL2, but for the dest bitmap
   */
   


/* void _colorconv_blit_24_to_32_no_mmx (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_24_to_32_no_mmx:
#else
FUNC (_colorconv_blit_24_to_32)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_4)
   
   movl MYLOCAL1, %ecx             /* init first line */
   
   _align_
   next_line_24_to_32_no_mmx:
   
      pushl %edx
      
      shrl $2, %ecx
      orl %ecx, %ecx
      jz colorconv_24_to_32_no_mmx_one_pixel


      _align_
      /* 100% Pentium pairable loop */
      /* 9 cycles = 8 cycles/4 pixels + 1 cycle loop */
      next_block_24_to_32_no_mmx:
         movl 4(%esi), %ebx
         movl (%esi), %eax         /* eax = pixel1              */
         shll $8, %ebx             /* ebx = r8g8b0 pixel2       */
         movl 8(%esi), %edx        /* edx = pixel4 | r8 pixel 3 */
         movl %eax, (%edi)         /* write pixel1              */
         movb 3(%esi), %bl         /* ebx = pixel2              */ 
         movl %edx, %eax           /* eax = r8 pixel3           */
         shll $16, %eax            /* eax = r8g0b0 pixel3       */
         movl %ebx, 4(%edi)        /* write pixel2              */
         shrl $8, %edx             /* ecx = pixel4              */
         movb 6(%esi), %al         /* eax = r8g0b8 pixel3       */
         movb 7(%esi), %ah         /* eax = r8g8b8 pixel3       */ 
         movl %edx, 12(%edi)       /* write pixel4              */
         addl $12, %esi            /* 4 pixels read             */
         movl %eax, 8(%edi)        /* write pixel3              */
         addl $16, %edi            /* 4 pixels written          */
         decl %ecx
         jnz next_block_24_to_32_no_mmx
         
       colorconv_24_to_32_no_mmx_one_pixel:
         popl %edx                /* Need to restore the stack before reading out of it */
         movl MYLOCAL1, %ecx      /* Restore width */
         pushl %edx
         andl $3, %ecx
         jz end_of_line_24_to_32_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_24_to_32_no_mmx_two_pixels
         
         movzwl (%esi), %eax       /* Read one pixel */
         movzbl 2(%esi), %ebx
         addl $3, %esi
         shll $16, %ebx            /* Convert */
         orl %ebx, %eax
         movl %eax, (%edi)         /* Write */
         addl $4, %edi
         
       colorconv_24_to_32_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_24_to_32_no_mmx
         
         movl (%esi), %eax         /* Read 2 pixels */
         movzwl 4(%esi), %ebx
         
         movl %eax, %edx           /* Convert */
         shll $8, %ebx
         shll $8, %eax
         andl $0xFF, %edx
         shrl $8, %eax
         addl $6, %esi
         orl %edx, %ebx
         
         movl %eax, (%edi)         /* Write */
         movl %ebx, 4(%edi)
         
         addl $8, %edi

      _align_
      end_of_line_24_to_32_no_mmx:
   
      popl %edx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      movl MYLOCAL1, %ecx /* Init next line */
      decl %edx
      jnz next_line_24_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_15_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
/* void _colorconv_blit_16_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_15_to_32_no_mmx:
_colorconv_blit_16_to_32_no_mmx:
#else
FUNC (_colorconv_blit_15_to_32)
FUNC (_colorconv_blit_16_to_32)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_4)
   movl GLOBL(_colorconv_rgb_scale_5x35), %ebp
   xorl %eax, %eax /* init first line */  

   _align_
   next_line_16_to_32_no_mmx:
   
      movl MYLOCAL1, %ecx
      
      pushl %edx
      
      shrl $1, %ecx
      jz colorconv_16_to_32_no_mmx_one_pixel

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */
      next_block_16_to_32_no_mmx:
         xorl %ebx, %ebx
         movb (%esi), %al               /* al = low byte pixel1        */
         xorl %edx, %edx
         movb 1(%esi), %bl              /* bl = high byte pixel1       */
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel1 */
         movb 2(%esi), %dl              /* dl = low byte pixel2        */
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel1 */
         addl $8, %edi                  /* 2 pixels written            */
         addl %ebx, %eax                /* eax = r8g8b8 pixel1         */
         xorl %ebx, %ebx
         movl 1024(%ebp,%edx,4), %edx   /* lookup: ecx = r0g8b8 pixel2 */
         movb 3(%esi), %bl              /* bl = high byte pixel2       */  
         movl %eax, -8(%edi)            /* write pixel1                */
         xorl %eax, %eax
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel2 */
         addl $4, %esi                  /* 4 pixels read               */
         addl %ebx, %edx                /* edx = r8g8b8 pixel2         */ 
         decl %ecx
         movl %edx, -4(%edi)            /* write pixel2                */
         jnz next_block_16_to_32_no_mmx
         
       colorconv_16_to_32_no_mmx_one_pixel:
         popl %edx                /* Need to restore the stack before reading out of it */
         movl MYLOCAL1, %ecx      /* Restore width */
         shrl $1, %ecx
         jnc end_of_line_16_to_32_no_mmx

         movzbl (%esi), %eax            /* al = low byte pixel1        */
         addl $4, %edi                  /* 2 pixels written            */
         movzbl 1(%esi), %ebx           /* bl = high byte pixel1       */
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel1 */
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel1 */
         addl $2, %esi
         addl %ebx, %eax                /* eax = r8g8b8 pixel1         */
         movl %eax, -4(%edi)            /* write pixel1                */

      _align_
      end_of_line_16_to_32_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_16_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_8_to_32 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_8_to_32_no_mmx:
#else
FUNC (_colorconv_blit_8_to_32)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_4)
   xorl %eax, %eax     /* init first line */
   movl GLOBL(_colorconv_indexed_palette), %ebp
   

   _align_
   next_line_8_to_32_no_mmx:
      movl MYLOCAL1, %ecx
      pushl %edx
      
      shrl $2, %ecx
      jz colorconv_8_to_32_no_mmx_one_pixel

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_32_no_mmx:
         movb (%esi), %al           /* init first line */
         xorl %ebx, %ebx
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel1 */
         movb 1(%esi), %bl          /* bl = pixel2          */
         movl %eax, (%edi)          /* write pixel1         */
         addl $16, %edi             /* 4 pixels written     */
         xorl %eax, %eax
         movl (%ebp,%ebx,4), %ebx   /* lookup: ebx = pixel2 */
         movb 2(%esi), %al          /* al = pixel3          */
         movl %ebx, -12(%edi)       /* write pixel2         */
         xorl %edx, %edx
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel3 */
         movb 3(%esi), %dl          /* dl = pixel4          */
         movl %eax, -8(%edi)        /* write pixel3         */
         xorl %eax, %eax
         movl (%ebp,%edx,4), %edx   /* lookup: edx = pixel4 */
         addl $4, %esi              /* 4 pixels read        */
         movl %edx, -4(%edi)        /* write pixel4         */
         decl %ecx
         jnz next_block_8_to_32_no_mmx
         
       colorconv_8_to_32_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         
         andl $3, %ecx
         jz end_of_line_8_to_32_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_8_to_32_no_mmx_two_pixels
         
         movb (%esi), %al           /* read one pixel */
         incl %esi
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel */
         addl $4, %edi
         movl %eax, -4(%edi)        /* write pixel */
         
       colorconv_8_to_32_no_mmx_two_pixels:
         
         movb (%esi), %al           /* read one pixel */
         addl $2, %esi
         movb -1(%esi), %bl         /* read another pixel */
         
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel */
         movl (%ebp,%ebx,4), %ebx   /* lookup: ebx = pixel */
         addl $8, %edi
         movl %eax, -8(%edi)        /* write pixel */
         movl %ebx, -4(%edi)        /* write pixel */
         
      _align_
      end_of_line_8_to_32_no_mmx:
        
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_8_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_32_to_24_no_mmx (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_32_to_24_no_mmx:
#else
FUNC (_colorconv_blit_32_to_24)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_3)
   movl MYLOCAL1, %ecx          /* init first line */

   _align_
   next_line_32_to_24_no_mmx:
   
      shrl $2, %ecx
      pushl %ecx

      orl %ecx, %ecx
      jz colorconv_32_to_24_no_mmx_one_pixel
   
      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_32_to_24_no_mmx:
         movl 4(%esi), %ebx
         addl $12, %edi         /* 4 pixels written                */ 
         movl %ebx, %ebp        /* ebp = pixel2                    */
         shll $24, %ebx         /* ebx = b8 pixel2 << 24           */
         movl 12(%esi), %edx    /* edx = pixel4                    */
         shll $8, %edx          /* ecx = pixel4 << 8               */
         movl (%esi), %eax      /* eax = pixel1                    */
         movb 10(%esi), %dl     /* edx = pixel4 | r8 pixel3        */
         orl  %eax, %ebx        /* ebx = b8 pixel2 | pixel1        */
         movl %ebx, -12(%edi)   /* write pixel1..b8 pixel2         */
         movl %ebp, %eax        /* eax = pixel2                    */ 
         shrl $8, %eax          /* eax = r8g8 pixel2               */
         movl 8(%esi), %ebx     /* ebx = pixel 3                   */
         shll $16, %ebx         /* ebx = g8b8 pixel3 << 16         */
         movl %edx, -4(%edi)    /* write r8 pixel3..pixel4         */
         orl  %ebx, %eax        /* eax = g8b8 pixel3 | r8g8 pixel2 */
         addl $16, %esi         /* 4 pixels read                   */
         movl %eax, -8(%edi)    /* write g8r8 pixel2..b8g8 pixel3  */
         decl %ecx
         jnz next_block_32_to_24_no_mmx
         
       colorconv_32_to_24_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         
         andl $3, %ecx
         jz end_of_line_32_to_24_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_32_to_24_no_mmx_two_pixels
         
         movl (%esi), %eax      /* Read one pixel */
         addl $4, %esi
         movw %ax, (%edi)       /* Write bottom 24 bits */
         shrl $16, %eax
         addl $3, %edi
         movb %al, -1(%edi)
         
       colorconv_32_to_24_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_32_to_24_no_mmx
         
         movl (%esi), %eax      /* Read two pixel */
         movl 4(%esi), %ebx
         
         addl $8, %esi          /* Convert 32->24 */
         shll $8, %eax
         movl %ebx, %ecx
         shrl $8, %eax
         shrl $24, %ebx
         shrl $8, %ecx
         orl %ebx, %eax
         addl $6, %edi
         
         movl %eax, -6(%edi)    /* Write bottom 48 bits */
         movw %ax,  -2(%edi)

      _align_
      end_of_line_32_to_24_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      movl MYLOCAL1, %ecx /* init next line */
      decl %edx
      jnz next_line_32_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret
 


/* void _colorconv_blit_15_to_24 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_24)
/* void _colorconv_blit_16_to_24 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_16_to_24)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_3)
   movl GLOBL(_colorconv_rgb_scale_5x35), %ebp

   next_line_16_to_24_no_mmx:
      movl MYLOCAL1, %ecx
      pushl %edx
      
      shrl $2, %ecx
      orl %ecx, %ecx
      jz colorconv_16_to_24_no_mmx_one_pixel

      _align_
      /* 100% Pentium pairable loop */
      /* 22 cycles = 20 cycles/4 pixels + 1 cycle stack + 1 cycle loop */
      next_block_16_to_24_no_mmx:
         movl %ecx, -16(%esp)           /* fake pushl %edx                  */
         xorl %eax, %eax
         xorl %ebx, %ebx
         movb 7(%esi), %bl              /* bl = high byte pixel4            */
         xorl %edx, %edx
         movb 6(%esi), %al              /* al = low byte pixel4             */         
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel4      */
         movb 4(%esi), %dl              /* dl = low byte pixel3             */         
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel4      */
         xorl %ecx, %ecx
         addl %ebx, %eax                /* eax = r8g8b8 pixel4              */
         movb 5(%esi), %cl              /* cl = high byte pixel3            */          
         shll $8, %eax                  /* eax = r8g8b8 pixel4 << 8         */
         movl 5120(%ebp,%edx,4), %edx   /* lookup: edx = g8b800r0 pixel3    */         
         movl 4096(%ebp,%ecx,4), %ecx   /* lookup: ecx = g8b000r8 pixel3    */
         xorl %ebx, %ebx
         addl %ecx, %edx                /* edx = g8b800r8 pixel3            */
         movb %cl, %al                  /* eax = pixel4 << 8 | r8 pixel3    */
         movl %eax, 8(%edi)             /* write r8 pixel3..pixel4          */   
         movb 3(%esi), %bl              /* bl = high byte pixel2            */          
         xorl %eax, %eax
         xorl %ecx, %ecx
         movb 2(%esi), %al              /* al = low byte pixel2             */
         movl 2048(%ebp,%ebx,4), %ebx   /* lookup: ebx = b000r8g8 pixel2    */
         movb 1(%esi), %cl              /* cl = high byte pixel1            */
         addl $12, %edi                 /* 4 pixels written                 */
         movl 3072(%ebp,%eax,4), %eax   /* lookup: eax = b800r0g8 pixel2    */
         addl $8, %esi                  /* 4 pixels read                    */
         addl %ebx, %eax                /* eax = b800r8g8 pixel2            */
         movl (%ebp,%ecx,4), %ecx       /* lookup: ecx = r8g8b0 pixel1      */
         movb %al, %dl                  /* edx = g8b8 pixel3 | 00g8 pixel2  */
         xorl %ebx, %ebx
         movb %ah, %dh                  /* edx = g8b8 pixel3 | r8g8 pixel2  */
         movb -8(%esi), %bl             /* bl = low byte pixel1             */
         movl %edx, -8(%edi)            /* write g8r8 pixel2..b8g8 pixel3   */
         andl $0xff000000, %eax         /* eax = b8 pixel2 << 24            */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel1      */
         addl %ecx, %ebx                /* ebx = r8g8b8 pixel1              */ 
         movl -16(%esp), %ecx           /* fake popl %ecx                   */
         orl  %ebx, %eax                /* eax = b8 pixel2 << 24 | pixel1   */
         decl %ecx
         movl %eax, -12(%edi)           /* write pixel1..b8 pixel2          */
         jnz next_block_16_to_24_no_mmx
         
       colorconv_16_to_24_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         
         andl $3, %ecx
         jz end_of_line_16_to_24_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_16_to_24_no_mmx_two_pixels
         
         xorl %eax, %eax
         xorl %ebx, %ebx
         movb 1(%esi), %al              /* al = high byte pixel1            */
         addl $3, %edi                  /* 1 pixel written                  */
         addl $2, %esi                  /* 1 pixel  read                    */
         movl (%ebp,%eax,4), %eax       /* lookup: eax = r8g8b0 pixel1      */
         movb -2(%esi), %bl             /* bl = low byte pixel1             */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel1      */
         addl %eax, %ebx                /* ebx = r8g8b8 pixel1              */ 
         movl %ebx, -3(%edi)            /* write pixel1                     */
         
       colorconv_16_to_24_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_16_to_24_no_mmx
         
         xorl %eax, %eax
         xorl %ebx, %ebx
         movb 1(%esi), %al              /* al = high byte pixel1            */
         addl $6, %edi                  /* 1 pixel written                  */
         addl $4, %esi                  /* 1 pixel  read                    */
         movl (%ebp,%eax,4), %eax       /* lookup: eax = r8g8b0 pixel1      */
         movb -4(%esi), %bl             /* bl = low byte pixel1             */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel1      */
         addl %eax, %ebx                /* ebx = r8g8b8 pixel1              */ 
         xorl %eax, %eax
         movw %bx, -6(%edi)             /* write pixel1                     */
         shrl $16, %ebx
         movb -1(%esi), %al             /* al = high byte pixel2            */
         movb %bl, -4(%edi)

         xorl %ebx, %ebx
         movl (%ebp,%eax,4), %eax       /* lookup: eax = r8g8b0 pixel2      */
         movb -2(%esi), %bl             /* bl = low byte pixel2             */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel2      */
         addl %eax, %ebx                /* ebx = r8g8b8 pixel2              */ 
         movw %bx, -3(%edi)             /* write pixel2                     */
         shrl $16, %ebx
         movb %bl, -1(%edi)

    _align_
    end_of_line_16_to_24_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_16_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_8_to_24 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_8_to_24)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_3)
   movl GLOBL(_colorconv_indexed_palette), %ebp
   xorl %eax, %eax  /* init first line */

   _align_
   next_line_8_to_24_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      shrl $2, %ecx
      jz colorconv_8_to_24_no_mmx_one_pixel

      _align_
      /* 100% Pentium pairable loop */
      /* 12 cycles = 11 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_24_no_mmx:
         xorl %ebx, %ebx
         movb 3(%esi), %al               /* al = pixel4                     */
         movb 2(%esi), %bl               /* bl = pixel3                     */
         xorl %edx, %edx
         movl 3072(%ebp,%eax,4), %eax    /* lookup: eax = pixel4 << 8       */
         movb 1(%esi), %dl               /* dl = pixel 2                    */
         movl 2048(%ebp,%ebx,4), %ebx    /* lookup: ebx = g8b800r8 pixel3   */ 
         addl $12, %edi                  /* 4 pixels written                */ 
         movl 1024(%ebp,%edx,4), %edx    /* lookup: edx = b800r8g8 pixel2   */
         movb %bl, %al                   /* eax = pixel4 << 8 | r8 pixel3   */ 
         movl %eax, -4(%edi)             /* write r8 pixel3..pixel4         */  
         xorl %eax, %eax
         movb %dl, %bl                   /* ebx = g8b8 pixel3 | 00g8 pixel2 */
         movb (%esi), %al                /* al = pixel1                     */
         movb %dh, %bh                   /* ebx = g8b8 pixel3 | r8g8 pixel2 */
         shrl $24, %edx                  /* edx = b8 pixel2                 */
         movl %ebx, -8(%edi)             /* write g8r8 pixel2..b8g8 pixel3  */
         shll $24, %edx                  /* edx = b8 pixel2 << 24           */
         movl (%ebp,%eax,4), %eax        /* lookup: eax = pixel1            */ 
         orl  %eax, %edx                 /* edx = b8 pixel2 << 24 | pixel1  */
         xorl %eax, %eax
         movl %edx, -12(%edi)            /* write pixel1..b8 pixel2         */
         addl $4, %esi                   /* 4 pixels read                   */ 
         decl %ecx
         jnz next_block_8_to_24_no_mmx
         
       colorconv_8_to_24_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_8_to_24_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_8_to_24_no_mmx_two_pixels
         
         xorl %eax, %eax
         movb (%esi), %al                /* al = pixel1                     */
         incl %esi
         movl (%ebp,%eax,4), %eax        /* lookup: eax = pixel1            */
         movl %eax, (%edi)               /* write pixel1                    */
         addl $3, %edi
         
       colorconv_8_to_24_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_8_to_24_no_mmx
         
         xorl %eax, %eax
         xorl %ebx, %ebx
         movb (%esi), %al                /* al = pixel1                     */
         movb 1(%esi), %bl               /* bl = pixel2                     */
         addl $2, %esi
         movl (%ebp,%eax,4), %eax        /* lookup: eax = pixel1            */
         movl (%ebp,%ebx,4), %ebx        /* lookup: ebx = pixel2            */
         movl %eax, (%edi)               /* write pixel1                    */
         movl %ebx, 3(%edi)              /* write pixel2                    */
         addl $6, %edi
      
      _align_
      end_of_line_8_to_24_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_8_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret



#define CONV_TRUE_TO_16_NO_MMX(name, bytes_ppixel)                                 \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %ecx                                                        ; \
      pushl %edx                                                                 ; \
                                                                                 ; \
      shrl $1, %ecx                                                              ; \
      jz colorconv_##name##_one_pixel                                            ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */                         ; \
      next_block_##name:                                                         ; \
         movb bytes_ppixel(%esi), %al     /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb bytes_ppixel+1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb (%esi), %dl                 /* dl = b8 pixel1                  */  ; \
         shrb $3, %dl                     /* dl = b5 pixel1                  */  ; \
         movb bytes_ppixel+2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shll $16, %eax                   /* eax = r8b5 pixel2 << 16         */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shrl $5, %ebx                    /* ebx = g6 pixel2 | g6 pixel1     */  ; \
         movb 2(%esi), %dh                /* edx = r8b5 pixel1               */  ; \
         orl  %edx, %eax                  /* eax = r8b5 pixel2 | r8b5 pixel1 */  ; \
         addl $bytes_ppixel*2, %esi       /* 2 pixels read                   */  ; \
         andl $0xf81ff81f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x07e007e0, %ebx           /* clean g6 pixel2 | g6 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %ecx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
       colorconv_##name##_one_pixel:                                             ; \
         popl %edx                                                               ; \
         movl MYLOCAL1, %ecx                                                     ; \
                                                                                 ; \
         shrl $1, %ecx                                                           ; \
         jnc end_of_line_##name                                                  ; \
                                                                                 ; \
         xorl %ecx, %ecx                                                         ; \
         xorl %ebx, %ebx                                                         ; \
         movb (%esi), %cl                 /* cl = b8 pixel1                  */  ; \
         addl $2, %edi                                                           ; \
         shrb $3, %cl                     /* cl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel1                 */  ; \
         shrl $5, %ebx                    /* ebx = g6 pixel1                 */  ; \
         movb 2(%esi), %ch                /* ecx = r8b5 pixel1               */  ; \
         addl $bytes_ppixel, %esi         /* 1 pixel read                    */  ; \
         andl $0xf81f, %ecx               /* eax = r5b5 pixel1               */  ; \
         andl $0x07e0, %ebx               /* clean g6 pixel1                 */  ; \
         orl  %ebx, %ecx                  /* ecx = pixel1                    */  ; \
         movw %cx, -2(%edi)               /* write pixel1                    */  ; \
                                                                                 ; \
      _align_                                                                    ; \
      end_of_line_##name:                                                        ; \
                                                                                 ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl %edx                                                                  ; \
      jnz next_line_##name



/* void _colorconv_blit_32_to_16 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_32_to_16_no_mmx:
#else
FUNC (_colorconv_blit_32_to_16)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2)  
   CONV_TRUE_TO_16_NO_MMX(32_to_16_no_mmx, 4)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_24_to_16 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_16)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2)
   CONV_TRUE_TO_16_NO_MMX(24_to_16_no_mmx, 3)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_8_to_16 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
/* void _colorconv_blit_8_to_15 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_8_to_16_no_mmx:
#else
FUNC (_colorconv_blit_8_to_16)
FUNC (_colorconv_blit_8_to_15)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_2)
   movl GLOBL(_colorconv_indexed_palette), %ebp
   xorl %eax, %eax  /* init first line */

   _align_
   next_line_8_to_16_no_mmx:
      movl MYLOCAL1, %ecx
      pushl %edx
      
      shrl $2, %ecx
      orl %ecx, %ecx
      jz colorconv_8_to_16_no_mmx_one_pixel
                 
      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_16_no_mmx:
         xorl %ebx, %ebx
         movb (%esi), %al             /* al = pixel1            */
         xorl %edx, %edx
         movb 1(%esi), %bl            /* bl = pixel2            */
         movb 2(%esi), %dl            /* dl = pixel3            */
         movl (%ebp,%eax,4), %eax     /* lookup: ax = pixel1    */
         movl 1024(%ebp,%ebx,4), %ebx /* lookup: bx = pixel2    */
         addl $4, %esi                /* 4 pixels read          */
         orl  %ebx, %eax              /* eax = pixel2..pixel1   */
         xorl %ebx, %ebx
         movl %eax, (%edi)            /* write pixel1, pixel2   */
         movb -1(%esi), %bl           /* bl = pixel4            */
         movl (%ebp,%edx,4), %edx     /* lookup: dx = pixel3    */
         xorl %eax, %eax
         movl 1024(%ebp,%ebx,4), %ebx /* lookup: bx = pixel4    */
         addl $8, %edi                /* 4 pixels written       */
         orl  %ebx, %edx              /* ecx = pixel4..pixel3   */
         decl %ecx
         movl %edx, -4(%edi)          /* write pixel3, pixel4   */
         jnz next_block_8_to_16_no_mmx
         
       colorconv_8_to_16_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         
         andl $3, %ecx
         jz end_of_line_8_to_16_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_8_to_16_no_mmx_two_pixels
         
         xorl %eax, %eax
         incl %esi
         movb -1(%esi), %al
         movl 1024(%ebp, %eax, 4), %eax
         addl $2, %edi
         movw %ax, -1(%edi)
         
       colorconv_8_to_16_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_8_to_16_no_mmx
         
         xorl %eax, %eax
         xorl %ebx, %ebx
         movb (%esi), %bl
         movb 1(%esi), %al
         addl $2, %esi
         movl 1024(%ebp, %ebx, 4), %ebx
         movl 1024(%ebp, %eax, 4), %eax
         addl $4, %edi
         shrl $16, %eax
         orl %ebx, %eax
         movl %eax, -4(%edi)
         
    _align_
    end_of_line_8_to_16_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_8_to_16_no_mmx
 
   DESTROY_STACK_FRAME
   ret



#define CONV_TRUE_TO_15_NO_MMX(name, bytes_ppixel)                                 \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %ecx                                                        ; \
      pushl %edx                                                                 ; \
                                                                                 ; \
      shrl $1, %ecx                                                              ; \
      jz colorconv_##name##_one_pixel                                            ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */                         ; \
      next_block_##name:                                                         ; \
         movb bytes_ppixel(%esi), %al     /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb bytes_ppixel+1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb bytes_ppixel+2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shrb $1, %ah                     /* eax = r7b5 pixel2               */  ; \
         movb (%esi), %dl                 /* dl = b8 pixel1                  */  ; \
         shrb $3, %dl                     /* dl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shll $16, %eax                   /* eax = r7b5 pixel2 << 16         */  ; \
         movb 2(%esi), %dh                /* edx = r8b5 pixel1               */  ; \
         shrb $1, %dh                     /* edx = r7b5 pixel1               */  ; \
         addl $bytes_ppixel*2, %esi       /* 2 pixels read                   */  ; \
         shrl $6, %ebx                    /* ebx = g5 pixel2 | g5 pixel1     */  ; \
         orl  %edx, %eax                  /* eax = r7b5 pixel2 | r7b5 pixel1 */  ; \
         andl $0x7c1f7c1f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x03e003e0, %ebx           /* clean g5 pixel2 | g5 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %ecx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
       colorconv_##name##_one_pixel:                                             ; \
         popl %edx                                                               ; \
         movl MYLOCAL1, %ecx                                                     ; \
                                                                                 ; \
         shrl $1, %ecx                                                           ; \
         jnc end_of_line_##name                                                  ; \
                                                                                 ; \
         xorl %ecx, %ecx                                                         ; \
         xorl %ebx, %ebx                                                         ; \
         movb (%esi), %cl                 /* cl = b8 pixel1                  */  ; \
         addl $2, %edi                                                           ; \
         shrb $3, %cl                     /* cl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel1                 */  ; \
         movb 2(%esi), %ch                /* ecx = r8b5 pixel1               */  ; \
         shrb $1, %ch                     /* ecx = r7b5 pixel1               */  ; \
         addl $bytes_ppixel, %esi         /* 1 pixel read                    */  ; \
         shrl $6, %ebx                    /* ebx = g5 pixel2 | g5 pixel1     */  ; \
         andl $0x7c1f, %eax               /* eax = r5b5 pixel1               */  ; \
         andl $0x03e0, %ebx               /* clean g5 pixel1                 */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         movl %eax, -2(%edi)              /* write pixel1..pixel2            */  ; \
                                                                                 ; \
      _align_                                                                    ; \
      end_of_line_##name:                                                        ; \
                                                                                 ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl %edx                                                                  ; \
      jnz next_line_##name



/* void _colorconv_blit_32_to_15 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_32_to_15_no_mmx:
#else
FUNC (_colorconv_blit_32_to_15)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2)  
   CONV_TRUE_TO_15_NO_MMX(32_to_15_no_mmx, 4)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_24_to_15 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_15)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2)
   CONV_TRUE_TO_15_NO_MMX(24_to_15_no_mmx, 3)
   DESTROY_STACK_FRAME
   ret



#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_15_to_16_no_mmx:
#else
FUNC (_colorconv_blit_15_to_16)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_2)

   _align_
   next_line_15_to_16_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      shrl $2, %ecx
      jz colorconv_15_to_16_no_mmx_one_pixel

      _align_
      /* 100% Pentium pairable loop */
      /* 9 cycles = 8 cycles/4 pixels + 1 cycle loop */
      next_block_15_to_16_no_mmx:
         movl (%esi), %eax
         movl 4(%esi), %edx
         movl %eax, %ebx
         andl $0x7FE07FE0, %eax
         andl $0x001F001F, %ebx
         shll $1, %eax
         addl $8, %esi
         orl  $0x00200020, %eax
         orl  %ebx, %eax
         movl %edx, %ebx
         andl $0x7FE07FE0, %edx
         andl $0x001F001F, %ebx
         shll $1, %edx
         addl $8, %edi
         orl  $0x00200020, %edx
         orl  %edx, %ebx
         movl %eax, -8(%edi)
         movl %ebx, -4(%edi)
         decl %ecx
         jnz next_block_15_to_16_no_mmx

       colorconv_15_to_16_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_15_to_16_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_15_to_16_no_mmx_two_pixels

         movzwl (%esi), %eax
         addl $2, %edi
         movl %eax, %ebx
         andl $0x7FE0, %eax
         andl $0x001F, %ebx
         shll $1, %eax
         addl $2, %esi
         orl $0x0020, %eax
         orl %ebx, %eax
         movw %ax, -2(%edi)
         
       colorconv_15_to_16_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_15_to_16_no_mmx
         
         movl (%esi), %eax
         addl $4, %edi
         movl %eax, %ebx
         andl $0x7FE07FE0, %eax
         andl $0x001F001F, %ebx
         shll $1, %eax
         addl $4, %esi
         orl $0x00200020, %eax
         orl %ebx, %eax
         movl %eax, -4(%edi)

      _align_
      end_of_line_15_to_16_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_15_to_16_no_mmx

   DESTROY_STACK_FRAME
   ret




/* void _colorconv_blit_16_to_15 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_16_to_15_no_mmx:
#else
FUNC (_colorconv_blit_16_to_15)
#endif
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_2)

   _align_
   next_line_16_to_15_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      shrl $2, %ecx
      jz colorconv_16_to_15_no_mmx_one_pixel

      _align_
      /* 100% Pentium pairable loop */
      /* 9 cycles = 8 cycles/4 pixels + 1 cycle loop */
      next_block_16_to_15_no_mmx:
         movl (%esi), %eax
         movl 4(%esi), %edx
         movl %eax, %ebx
         andl $0xFFC0FFC0, %eax
         andl $0x001F001F, %ebx
         shrl $1, %eax
         addl $8, %esi
         orl %ebx, %eax
         movl %edx, %ebx
         andl $0xFFC0FFC0, %edx
         andl $0x001F001F, %ebx
         shrl $1, %edx
         addl $8, %edi
         orl %edx, %ebx
         movl %eax, -8(%edi)
         movl %ebx, -4(%edi)
         decl %ecx
         jnz next_block_16_to_15_no_mmx

       colorconv_16_to_15_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_16_to_15_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_16_to_15_no_mmx_two_pixels

         movzwl (%esi), %eax
         addl $2, %edi
         movl %eax, %ebx
         andl $0xFFC0FFC0, %eax
         andl $0x001F001F, %ebx
         shrl $1, %eax
         addl $2, %esi
         orl %ebx, %eax
         movw %ax, -2(%edi)
         
       colorconv_16_to_15_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_16_to_15_no_mmx
         
         movl (%esi), %eax
         addl $4, %edi
         movl %eax, %ebx
         andl $0xFFC0FFC0, %eax
         andl $0x001F001F, %ebx
         shrl $1, %eax
         addl $4, %esi
         orl %ebx, %eax
         movl %eax, -4(%edi)

      _align_
      end_of_line_16_to_15_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_16_to_15_no_mmx

   DESTROY_STACK_FRAME
   ret




/* void _colorconv_blit_8_to_8 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_8_to_8)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_1)
   
   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_8_to_8_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      shrl $2, %ecx                    /* Work in packs of 4 pixels */
      jz colorconv_8_to_8_no_mmx_one_pixel

      _align_
      /* 100% Pentium pairable loop */
      /* 13 cycles = 12 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_8_no_mmx:
         movl (%esi), %eax         /* Read 4 pixels */

         addl $4, %esi
         addl $4, %edi

         movzbl %al, %ebx          /* Pick out 2x bottom 8 bits */
         movzbl %ah, %edx

         shrl $16, %eax

         movb (%ebp, %ebx), %bl    /* Lookup the new palette entry */
         movb (%ebp, %edx), %dl

         shll $8, %edx
         orl %edx, %ebx            /* Combine them */

         movzbl %ah, %edx          /* Repeat for the top 16 bits */
         andl $0xFF, %eax

         movb (%ebp, %edx), %dl
         movb (%ebp, %eax), %al

         shll $24, %edx
         shll $16, %eax

         orl %edx, %ebx            /* Put everything together */
         orl %ebx, %eax

         movl %eax, -4(%edi)       /* Write 4 pixels */

         decl %ecx
         jnz next_block_8_to_8_no_mmx

       colorconv_8_to_8_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_8_to_8_no_mmx
         
         shrl $1, %ecx
         jnc colorconv_8_to_8_no_mmx_two_pixels

         xorl %eax, %eax
         movb (%esi), %al          /* Read 1 pixel */

         incl %edi
         incl %esi

         movb (%ebp, %eax), %al    /* Lookup the new palette entry */

         movb %al, -1(%edi)        /* Write 1 pixels */
         
       colorconv_8_to_8_no_mmx_two_pixels:
         shrl $1, %ecx
         jnc end_of_line_8_to_8_no_mmx
         
         xorl %eax, %eax
         xorl %ebx, %ebx
         
         movb (%esi), %al          /* Read 2 pixels */
         movb 1(%esi), %bl

         addl $2, %edi
         addl $2, %esi

         movb (%ebp, %eax), %al    /* Lookup the new palette entry */
         movb (%ebp, %ebx), %bl

         movb %al, -2(%edi)       /* Write 2 pixels */
         movb %bl, -1(%edi)

      _align_
      end_of_line_8_to_8_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_8_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_15_to_8 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_8)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_1)
   
   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_15_to_8_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      shrl $1, %ecx                    /* Work in packs of 2 pixels */
      jz colorconv_15_to_8_no_mmx_one_pixel

      _align_
      next_block_15_to_8_no_mmx:
         movl (%esi), %eax         /* Read 2 pixels */

         addl $4, %esi
         addl $2, %edi

         movl %eax, %ebx           /* Get bottom 16 bits */
         movl %eax, %edx

         andl $0x781E, %ebx
         andl $0x03C0, %edx

         shrb $1, %bl              /* Shift to correct positions */
         shrb $3, %bh
         shrl $2, %edx

         shrl $16, %eax

         orl %edx, %ebx            /* Combine to get a 4.4.4 number */
         
         movl %eax, %edx
         movb (%ebp, %ebx), %bl    /* Look it up */
         
         andl $0x781F, %eax
         andl $0x03C0, %edx
         
         shrb $1, %al              /* Shift to correct positions */
         shrb $3, %ah
         shrl $2, %edx

         orl %edx, %eax            /* Combine to get a 4.4.4 number */
         movb (%ebp, %eax), %bh    /* Look it up */

         movw %bx, -2(%edi)        /* Write 2 pixels */

         decl %ecx
         jnz next_block_15_to_8_no_mmx

       colorconv_15_to_8_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx

         shrl $1, %ecx
         jnc end_of_line_15_to_8_no_mmx

         movzwl (%esi), %eax       /* Read 2 pixels */

         addl $2, %esi
         incl %edi

         movl %eax, %ebx           /* Get bottom 16 bits */

         andl $0x781E, %ebx
         andl $0x03C0, %eax

         shrb $1, %bl              /* Shift to correct positions */
         shrb $3, %bh
         shrl $2, %eax

         orl %eax, %ebx            /* Combine to get a 4.4.4 number */
         
         movb (%ebp, %ebx), %bl    /* Look it up */
         
         movb %bl, -1(%edi)        /* Write 2 pixels */

      _align_
      end_of_line_15_to_8_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_15_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_16_to_8 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_16_to_8)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_1)
   
   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_16_to_8_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      shrl $1, %ecx                    /* Work in packs of 2 pixels */
      jz colorconv_16_to_8_no_mmx_one_pixel

      _align_
      next_block_16_to_8_no_mmx:
         movl (%esi), %eax         /* Read 2 pixels */

         addl $4, %esi
         addl $2, %edi

         movl %eax, %ebx           /* Get bottom 16 bits */
         movl %eax, %edx

         andl $0xF01E, %ebx
         andl $0x0780, %edx

         shrb $1, %bl              /* Shift to correct positions */
         shrb $4, %bh
         shrl $3, %edx

         shrl $16, %eax

         orl %edx, %ebx            /* Combine to get a 4.4.4 number */
         
         movl %eax, %edx
         movb (%ebp, %ebx), %bl    /* Look it up */
         
         andl $0xF01E, %eax
         andl $0x0780, %edx
         
         shrb $1, %al              /* Shift to correct positions */
         shrb $4, %ah
         shrl $3, %edx

         orl %edx, %eax            /* Combine to get a 4.4.4 number */
         movb (%ebp, %eax), %bh    /* Look it up */

         movw %bx, -2(%edi)        /* Write 2 pixels */

         decl %ecx
         jnz next_block_16_to_8_no_mmx

       colorconv_16_to_8_no_mmx_one_pixel:
         popl %edx
         movl MYLOCAL1, %ecx

         shrl $1, %ecx
         jnc end_of_line_15_to_8_no_mmx

         movzwl (%esi), %eax       /* Read 2 pixels */

         addl $2, %esi
         incl %edi

         movl %eax, %ebx           /* Get bottom 16 bits */

         andl $0xF01E, %ebx
         andl $0x0780, %eax

         shrb $1, %bl              /* Shift to correct positions */
         shrb $4, %bh
         shrl $3, %eax

         orl %eax, %ebx            /* Combine to get a 4.4.4 number */
         
         movb (%ebp, %ebx), %bl    /* Look it up */
         
         movb %bl, -1(%edi)        /* Write 2 pixels */

      _align_
      end_of_line_16_to_8_no_mmx:

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_16_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret


/* void _colorconv_blit_32_to_8 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_8)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_1)
   
   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_32_to_8_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      _align_
      next_block_32_to_8_no_mmx:
         movzbl (%esi), %eax       /* Read 1 pixel */
         movzbl 1(%esi), %ebx
         movzbl 2(%esi), %edx

         addl $4, %esi
         incl %edi

         shrl $4, %edx
         andl $0xF0, %ebx
         shll $8, %edx
         shrl $4, %eax
         
         orl %ebx, %edx            /* Combine to get 4.4.4 */
         orl %edx, %eax

         movb (%ebp, %eax), %al    /* Look it up */
         
         movb %al, -1(%edi)        /* Write 1 pixel */

         decl %ecx
         jnz next_block_32_to_8_no_mmx

      end_of_line_32_to_8_no_mmx:
      
      popl %edx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_32_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret


/* void _colorconv_blit_24_to_8 (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_8)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_1)
   
   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_24_to_8_no_mmx:
      movl MYLOCAL1, %ecx      
      pushl %edx
      
      _align_
      next_block_24_to_8_no_mmx:
         movzbl (%esi), %eax         /* Read 1 pixel */
         movzbl 1(%esi), %ebx
         movzbl 2(%esi), %edx

         addl $3, %esi
         incl %edi

         shrl $4, %edx
         andl $0xF0, %ebx
         shll $8, %edx
         shrl $4, %eax
         
         orl %ebx, %edx            /* Combine to get 4.4.4 */
         orl %edx, %eax

         movb (%ebp, %eax), %al    /* Look it up */
         
         movb %al, -1(%edi)        /* Write 1 pixel */

         decl %ecx
         jnz next_block_24_to_8_no_mmx

      end_of_line_24_to_8_no_mmx:
      
      popl %edx
      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl %edx
      jnz next_line_24_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret

