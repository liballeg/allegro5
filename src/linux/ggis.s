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
 *      GGI bank switching code. These routines will be called 
 *      with a line number in %eax and a pointer to the bitmap in %edx.
 *      The bank switcher should select the appropriate bank for the 
 *      line, and replace %eax with a pointer to the start of the line.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro/alunixac.h"
#include "../i386/asmdefs.inc"


.text


#if (!defined ALLEGRO_NO_ASM) && (defined ALLEGRO_LINUX_GGI)


FUNC(ggi_read_bank_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(ggi_read_bank)
	popl %edx
	popl %ecx
	popl %ecx
	ret


FUNC(ggi_write_bank_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(ggi_write_bank)
	popl %edx
	popl %ecx
	popl %ecx
	ret


FUNC(ggi_unwrite_bank_asm)
	pushl %eax
	pushl %ecx
	pushl %edx
	call GLOBL(ggi_unwrite_bank)
	popl %edx
	popl %ecx
	popl %eax
	ret


FUNC(ggi_write_fake_bank_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(ggi_write_fake_bank)
	popl %edx
	popl %ecx
	popl %ecx
	ret


FUNC(ggi_unfake_bank_asm)
	pushl %eax
	pushl %ecx
	pushl %edx
	call GLOBL(ggi_unfake_bank)
	popl %edx
	popl %ecx
	popl %eax
	ret


#endif

