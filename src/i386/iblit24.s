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
 *      24 bit bitmap blitting (written for speed, not readability :-)
 *
 *      By Michal Mertl.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "blit.inc"

#ifdef ALLEGRO_COLOR24

.text



/* void _linear_clear_to_color24(BITMAP *bitmap, int color);
 *  Fills a linear bitmap with the specified color.
 */
FUNC(_linear_clear_to_color24)
   pushl %ebp
   movl %esp, %ebp
   subl $12, %esp                /* 12 bytes temp space for local variables */
   pushl %edi
   pushl %ebx
   pushw %es 

   movl ARG1, %edx               /* edx = bmp */
   movl BMP_CT(%edx), %ebx       /* line to start at */

   movw BMP_SEG(%edx), %es       /* select segment */

   cld

   movl ARG2, %eax              /* pixel color */
   movl %eax, -8(%ebp)
   movl %eax, -5(%ebp)
   movl %ebx, ARG2               /* line to start at (in ARG2 is line now)*/
   movl BMP_CB(%edx), %eax
   subl %ebx, %eax
   movl %eax, -12(%ebp)          /* line counter */
   movl BMP_CR(%edx), %ecx
   subl BMP_CL(%edx), %ecx       /* line length */

   _align_
clear_loop:
   pushl %ecx
   movl ARG2, %eax
   movl ARG1, %edx
   WRITE_BANK()                  /* select bank */
   movl BMP_CL(%edx), %ebx       /* xstart */
   leal (%ebx, %ebx, 2), %ebx    /* xstart *= 3 */
   leal (%eax, %ebx), %edi       /* address = lineoffset+xstart*3 */
   movl -8(%ebp), %eax 
   movl -7(%ebp), %ebx
   movl -6(%ebp), %edx
   cmpl $4, %ecx 
   jb clear_short_line 
   subl $4, %ecx                 /* test loop for sign flag */
   cld

   _align_
clear_inner_loop:                /* plots 4 pixels an once */
   movl %eax, %es:(%edi)
   movl %ebx, %es:4(%edi)
   movl %edx, %es:8(%edi) 
   addl $12, %edi
   subl $4, %ecx
   jns clear_inner_loop
   addl $4, %ecx                 /* add the helper back */
   jz clear_normal_line

   _align_
clear_short_line:
   movw %ax, %es:(%edi)          /* plots remaining pixels (max 3) */
   movb %dl, %es:2(%edi) 
   addl $3, %edi
   decl %ecx
   jg clear_short_line

   _align_
clear_normal_line:
   popl %ecx
   incl ARG2                     /* inc current line */
   decl -12(%ebp)                /* dec loop counter */ 
   jnz clear_loop                /* and loop */

   popw %es

   movl ARG1, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_clear_to_color() */




/* void _linear_blit24(BITMAP *source, BITMAP *dest, int source_x, source_y,
 *                                     int dest_x, dest_y, int width, height);
 *  Normal forwards blitting routine for linear bitmaps.
 */
FUNC(_linear_blit24)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushw %es

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movl B_DEST_X, %edi
   leal (%edi, %edi, 2), %edi
   movl %edi, B_DEST_X
   movl B_SOURCE_X, %esi
   leal (%esi, %esi, 2), %esi
   movl %esi, B_SOURCE_X
   movl B_WIDTH, %ecx
   leal (%ecx, %ecx, 2), %ecx
   movl %ecx, B_WIDTH
   movw %ds, %bx                 /* save data segment selector */
   cld                           /* for forward copy */

   _align_
blit_loop_blitter24:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   WRITE_BANK()                  /* select bank */
   movl B_DEST_X, %edi           /* x offset */
   leal (%eax, %edi), %edi       /* edi = eax+3*edi */
   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   READ_BANK()                   /* select bank */
   movl B_SOURCE_X, %esi         /* x offset */
   leal (%eax, %esi), %esi       /* esi = eax+3*esi */
   movl B_WIDTH, %ecx            /* x loop counter */
   movw BMP_SEG(%edx), %ds       /* load data segment */
   shrl $1, %ecx
   jnc notcarry1
   movsb
   _align_
notcarry1:
   shrl $1, %ecx
   jnc notcarry2
   movsw
   _align_
notcarry2:
   rep ; movsl
   movw %bx, %ds                 /* restore data segment */
   incl B_SOURCE_Y
   incl B_DEST_Y
   decl B_HEIGHT
   jg blit_loop_blitter24        /* and loop */

   popw %es

   movl B_SOURCE, %edx
   UNREAD_BANK()

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit24() */




/* void _linear_blit_backward24(BITMAP *source, BITMAP *dest, int source_x,
 *                      int source_y, int dest_x, dest_y, int width, height);
 *  Reverse blitting routine, for overlapping linear bitmaps.
 */
FUNC(_linear_blit_backward24)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushw %es

   movl B_HEIGHT, %eax           /* y values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_Y
   addl %eax, B_DEST_Y

   movl B_WIDTH, %eax            /* x values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_X
   addl %eax, B_DEST_X
   movl B_DEST_X, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, B_DEST_X
   movl B_SOURCE_X, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, B_SOURCE_X
   movl B_WIDTH, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, B_WIDTH

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movw %ds, %bx                 /* save data segment selector */

   _align_
blit_backwards_loop:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   WRITE_BANK()                  /* select bank */
   movl B_DEST_X, %edi           /* x offset */
   leal (%eax, %edi), %edi       /* edi = eax+3*esi */
   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   READ_BANK()                   /* select bank */
   movl B_SOURCE_X, %esi         /* x offset */
   leal (%eax, %esi), %esi       /* esi = eax+3*esi */
   movl B_WIDTH, %ecx            /* x loop counter */
   movw BMP_SEG(%edx), %ds       /* load data segment */
   std                           /* backwards */
   shrl $1, %ecx
   jnc  not_carry1
   movsb
   _align_
not_carry1:
   shrl $1, %ecx
   jnc not_carry2
   movsw
   _align_
not_carry2:
   rep ; movsl
   movw %bx, %ds                 /* restore data segment */
   decl B_SOURCE_Y
   decl B_DEST_Y
   decl B_HEIGHT
   jg blit_backwards_loop        /* and loop */

   cld                           /* finished */

   popw %es

   movl B_SOURCE, %edx
   UNREAD_BANK()

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit_backward24() */

FUNC(_linear_blit24_end)
   ret




/* void _linear_masked_blit24(BITMAP *source, *dest, int source_x, source_y,
 *                            int dest_x, dest_y, int width, height);
 *  Masked (skipping zero pixels) blitting routine for linear bitmaps.
 */
FUNC(_linear_masked_blit24)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushw %es

   movl B_DEST, %edx
   movw BMP_SEG(%edx), %es 
   movw %ds, %bx 
   cld 

   movl B_DEST_X, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, B_DEST_X
   movl B_SOURCE_X, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, B_SOURCE_X

   _align_
blit_loop_blitter:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   WRITE_BANK()                  /* select bank */
   movl B_DEST_X, %edi           /* x offset */
   addl %eax, %edi
   movl B_SOURCE,%edx
   movw BMP_SEG(%edx), %ds       /* load data segment */
   movl B_SOURCE_Y, %eax         /* line number */
   READ_BANK()                   /* select bank */
   movl B_SOURCE_X, %esi         /* x offset */
   addl %eax, %esi
   movl B_WIDTH, %ecx            /* x loop counter */

   _align_
inner:
   lodsl
   decl %esi
   andl $0xFFFFFF, %eax
   cmpl $MASK_COLOR_24, %eax
   je masked_blit_skip
   movw %ax, %es:(%edi)
   shrl $16, %eax
   movb %al, %es:2(%edi)
   addl $3, %edi
   jmp befloop

   _align_
masked_blit_skip:
   addl $3, %edi

   _align_
befloop:
   loop inner

   movw %bx, %ds                 /* restore data segment */
   incl B_SOURCE_Y
   incl B_DEST_Y
   decl B_HEIGHT
   jg blit_loop_blitter          /* and loop */

   popw %es

   /* the source must be a memory bitmap, no need for
    *  movl B_SOURCE, %edx
    *  UNREAD_BANK()
    */

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_masked_blit24() */




#endif      /* ifdef ALLEGRO_COLOR24 */
