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
 *      QNX asm line switcher routines.
 *
 *      By Angelo Mottola.
 *
 *      Window updating system derived from Win32 version by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text


#ifndef ALLEGRO_NO_ASM
/* Use only with ASM calling convention.  */

FUNC (phd_write_line_asm)
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz phd_already_idle

   orl $BMP_ID_LOCKED, BMP_ID(%edx)
   pushal
   call GLOBL(PgWaitHWIdle)
   popal

phd_already_idle:
   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax
   ret



FUNC (phd_acquire_asm)
   pushl %ebp
   movl %esp, %ebp

   movl 8(%ebp), %eax
   testl $BMP_ID_LOCKED, BMP_ID(%eax)
   jnz phd_already_acquired
   
   orl $BMP_ID_LOCKED, BMP_ID(%eax)
   pushal
   call GLOBL(PgWaitHWIdle)
   popal
   
phd_already_acquired:
   popl %ebp

   ret
   

FUNC (phd_release_asm)
   pushl %ebp
   movl %esp, %ebp

   movl 8(%ebp), %eax
   andl $~BMP_ID_LOCKED, BMP_ID(%eax)

   popl %ebp
   ret



FUNC (ph_write_line_asm)

   pushal
   movl GLOBL(ph_dirty_lines), %ebx
   addl BMP_YOFFSET(%edx), %ebx
   movb $1, (%ebx,%eax)
   
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz ph_already_idle

   orl $BMP_ID_LOCKED, BMP_ID(%edx)
   orl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   call GLOBL(PgWaitHWIdle)

ph_already_idle:
   popal
   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax
      
   ret



FUNC (ph_unwrite_line_asm)
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz dont_update

do_update:
   andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)
   andl $~BMP_ID_LOCKED, BMP_ID(%edx)
   pushal
   call update_dirty_lines
   popal

dont_update: 
   ret



/* ph_acquire_asm:
 *  arg1 = bmp
 */
FUNC(ph_acquire_asm)
   pushl %ebp
   movl %esp, %ebp

   movl 8(%ebp), %eax
   testl $BMP_ID_LOCKED, BMP_ID(%eax)
   jnz ph_already_acquired

   orl $BMP_ID_LOCKED, BMP_ID(%eax)
   pushal
   call GLOBL(PgWaitHWIdle)
   popal
   
ph_already_acquired:
   popl %ebp
   ret



/* ph_release_asm:
 *  arg1 = bmp
 */
FUNC(ph_release_asm)
   pushl %ebp
   movl %esp, %ebp
   
   movl 8(%ebp), %eax
   testl $BMP_ID_LOCKED, BMP_ID(%eax)
   jz ph_already_released
   
   andl $~BMP_ID_LOCKED, BMP_ID(%eax)
   andl $~BMP_ID_AUTOLOCK, BMP_ID(%eax)
   pushal
   call update_dirty_lines
   popal

ph_already_released:
   popl %ebp
   ret



#define RECT_UL_X    (%esp)
#define RECT_UL_Y   2(%esp)
#define RECT_LR_X   4(%esp)
#define RECT_LR_Y   6(%esp)

update_dirty_lines:
   subl $8, %esp  /* allocate a RECT structure */

   movw $0, RECT_UL_X
   movl GLOBL(ph_window_w), %ecx
   movw %cx, RECT_LR_X
   movl GLOBL(ph_dirty_lines), %ebx
   movl GLOBL(ph_window_h), %esi
   movl $0, %edi  
      
   _align_
next_line:
   movb (%ebx,%edi), %al  /* al = ph_dirty_lines[edi] */
   testb %al, %al         /* is dirty? */
   jz test_end            /* no ! */
   movw %di, RECT_UL_Y

   _align_
loop_dirty_lines:
   movb $0, (%ebx,%edi)   /* ph_dirty_lines[edi] = 0  */
   incl %edi
   movb (%ebx,%edi), %al  /* al = ph_dirty_lines[edi] */
   testb %al, %al         /* is still dirty? */
   jnz loop_dirty_lines   /* yes ! */

   movw %di, RECT_LR_Y
   leal RECT_UL_X, %eax
   pushl %eax
   call *GLOBL(ph_update_window)
   popl %eax
      
   _align_
test_end:
   incl %edi
   cmpl %edi, %esi  /* last line? */
   jge next_line    /* no ! */
   
   addl $8, %esp
   call GLOBL(PgFlush)

   ret



#endif

