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
 *      X-windows bank switching code. These routines will be called 
 *      with a line number in %eax and a pointer to the bitmap in %edx.
 *      The bank switcher should select the appropriate bank for the 
 *      line, and replace %eax with a pointer to the start of the line.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro/platform/alunixac.h"
#include "../i386/asmdefs.inc"

.text


#ifndef ALLEGRO_NO_ASM
	/* Use only with ASM calling convention.  */

FUNC(_xwin_write_line_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(_xwin_write_line)
	popl %edx
	popl %ecx
	popl %ecx
	ret


FUNC(_xwin_unwrite_line_asm)
	pushl %eax
	pushl %ecx
	pushl %edx
	call GLOBL(_xwin_unwrite_line)
	popl %edx
	popl %ecx
	popl %eax
	ret

#ifdef ALLEGRO_XWINDOWS_WITH_XF86DGA

FUNC(_xdga_switch_bank_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(_xdga_switch_bank)
	popl %edx
	popl %ecx
	popl %ecx
	ret

#endif

#endif

