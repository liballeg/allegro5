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
 *      Joystick driver for the Microsoft Sidewinder.
 *
 *      Originally by Robert Grubbs, AT&T conversion by Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text



/* ParityCheckAndStore:
 *  Entry: edx=button status, edi=button status ptr.
 *  Exit: edi is advanced, and button status may or may not be stored.
 * 
 *  This routine checks to see if the parity of the data coming back from
 *  the sidewinder is stored properly. If it isn't, the button status is not
 *  updated. Otherwise, it is!
 */
ParityCheckAndStore:
   movl %edx, %ebx               /* get button status from edx */
   movl %ebx, %ecx               /* dupe button status */
   andb $0x7F, %ch               /* knock out parity bit from data */
   xorb %ch, %cl
   setp %cl
   btl $15, %ebx
   setb %ch
   cmpb %ch, %cl
   jne noStore

   movl %edx, (%edi)             /* store our button status */

noStore:
   addl $4, %edi
   ret



/* int _get_sidewinder_status(int output[4]);
 *  C callable routine to get the attached Sidewinder status and fill in
 *  the status structure pointed to by the input parameter.
 */
FUNC(_get_sidewinder_status)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx                    /* save important registers */
   pushl %ecx
   pushl %edx
   pushl %esi
   pushl %edi

   movl ARG1, %eax
   movl %eax, %ebx               /* store the address of our structure */

   movl $GLOBL(_sidewinder_dump), %edi
   movl $200, %ecx               /* size of dump */
   movw $0x201, %dx              /* joystick I/O port */
   cld 

   pushf 
   cli                           /* stop ints so we get accurate readings */

sampleLoop:
   outb %al, (%dx)               /* read strobe (trigger) */
   inb (%dx), %al                /* get our data */
   movb %al, (%edi)              /* store our received data */
   incl %edi                     /* increment our pointer */
   decl %ecx                     /* decrement our count */
   jne sampleLoop                /* keep going until we're done */

   popf                          /* restore interrupt status */

   /* now we look for 15 high cycles in the stream we've just read in */
   movl $GLOBL(_sidewinder_dump), %esi
   movl $200, %ecx               /* # of bytes we've tried */
   xorl %edx, %edx               /* zero our high cycle count */

cycleLoop:
   movb (%esi), %al              /* get our received data */
   incl %esi                     /* increment our source pointer */
   decl %ecx                     /* decrement our count */
   je noSidewinder               /* no sidewinder attached! */

   testb $0x10, %al              /* look for zero (active low) */
   jne strobeHigh                /* strobe is high! */

   xorl %edx, %edx               /* zero our count */
   jmp cycleLoop                 /* continue until we find it */

strobeHigh:
   incl %edx                     /* increment our high cycle count */
   cmpl $15, %edx                /* 15 highs yet? */
   jne cycleLoop                 /* nope! Keep going until we find it */

   /* we now have our 15 cycles in a row */
   /* now look for the leading edge of the data strobe */
   xorl %edi, %edi

dataStrobeLoop:
   movb (%esi), %al              /* get our data */
   incl %esi                     /* next ptr, please */

   decl %ecx                     /* decrement our remaining byte count */
   je noSidewinder               /* if we're out of data, no sidewinder */

   testb $0x10, %al              /* anything there? */
   jne dataStrobeLoop            /* nope! */

   xorl %edx, %edx               /* zero our count */

   /* we have the beginning of our data strobe! */
   /* start looking for a string of 15 zeros */
highStrobeLoop:
   incl %edx                     /* increment our count */
   cmpl $15, %edx                /* 15 cycles? */
   je modeCheck                  /* go do a Sidewinder mode check */

   movb (%esi), %al              /* get our data */
   incl %esi                     /* increment our pointer */

   decl %ecx                     /* decrement our remaining byte count */
   je noSidewinder               /* if we're out of data, no sidewinder */

   testb $0x10, %al              /* anything there? */
   je highStrobeLoop             /* nope.... */

   movb %al, GLOBL(_sidewinder_data)(%edi)
   incl %edi                     /* increment our pointer */
   jmp dataStrobeLoop

noSidewinder:
   movl $-1, %eax                /* not found */

swReturn:
   popl %edi
   popl %esi
   popl %edx
   popl %ecx
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret 


   /* now we know we've got valid data. Let's go through and see what
    * we've got in terms of controllers, and update any of the appropriate
    * controller info.
    */
modeCheck:
   movl $GLOBL(_sidewinder_data), %esi
   movl %edi, %ecx               /* store this here! */
   movl %ebx, %edi               /* our target storage address */

   cmpl $0x3C, %ecx              /* mode A with 4 sidewinders? */
   je ModeA4
   cmpl $0x2D, %ecx              /* mode A with 3 sidewinders? */
   je ModeA3
   cmpl $0x1E, %ecx              /* mode A with 2 sidewinders? */
   je ModeA2
   cmpl $0xF, %ecx               /* mode A with 1 sidewinder? */
   je ModeA1
   cmpl $0x14, %ecx              /* mode B with 3 sidewinders? */
   je ModeB3
   cmpl $0xA, %ecx               /* mode B with 2 sidewinders? */
   je ModeB2
   cmpl $0x5, %ecx               /* mode B with 1 sidewinder? */
   je ModeB1
   jmp noSidewinder              /* not recognized - not installed */


   /* 4 Sidewinders - Mode A */
ModeA4:
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   movl $4, %eax
   jmp swReturn


   /* 3 Sidewinders - Mode A */
ModeA3:
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   movl $0, (%edi)
   movl $3, %eax
   jmp swReturn


   /* 2 Sidewinders - Mode A */
ModeA2:
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   movl $0, (%edi)
   movl $0, 4(%edi)
   movl $2, %eax
   jmp swReturn


   /* 1 Sidewinder - Mode A */
ModeA1:
   call ModeAConvert             /* go do a Mode A conversion */
   call ParityCheckAndStore
   movl $0, (%edi)
   movl $0, 4(%edi)
   movl $0, 8(%edi)
   movl $1, %eax
   jmp swReturn


   /* 3 Sidewinders - Mode B */
ModeB3:
   call ModeBConvert             /* go do a Mode B conversion */
   call ParityCheckAndStore
   call ModeBConvert             /* go do a Mode B conversion */
   call ParityCheckAndStore
   call ModeBConvert             /* go do a Mode B conversion */
   call ParityCheckAndStore
   movl $0, (%edi)
   movl $3, %eax
   jmp swReturn


   /* 2 Sidewinders - Mode B */
ModeB2:
   call ModeBConvert             /* go do a Mode B conversion */
   call ParityCheckAndStore
   call ModeBConvert             /* go do a Mode B conversion */
   call ParityCheckAndStore
   movl $0, (%edi)
   movl $0, 4(%edi)
   movl $2, %eax
   jmp swReturn


   /* 1 Sidewinder - Mode B */
ModeB1:
   call ModeBConvert             /* go do a Mode B conversion */
   call ParityCheckAndStore
   movl $0, (%edi)
   movl $0, 4(%edi)
   movl $0, 8(%edi)
   movl $1, %eax
   jmp swReturn



/* ModeAConvert:
 *  Entry: esi=source data ptr.
 *  Exit: edx=button status.
 * 
 *  This routine does the Mode A type conversion.
 */
ModeAConvert:
   xorl %edx, %edx               /* zero our mask */
   xorl %eax, %eax               /* zero this for the OR */
   movl $15, %ecx                /* 15 Bits! */

bitLoop:
   movb (%esi), %al              /* get our source bye */
   incl %esi
   rcrb $6,%al
   cmc 
   rcrw %dx
   decl %ecx
   jne bitLoop

   ret 



/* ModeBConvert:
 *  Entry: esi=source data.
 *  Exit: edx=button data.
 * 
 *  This routine does the Mode B type conversion.
 */
ModeBConvert:
   xorl %edx, %edx               /* zero our mask */
   movb $5, %ch                  /* 5 Loops of 3 loops */

startLoop:
   movb $0x20, %al               /* start with this bitmask */
   movb $3, %cl                  /* 3 Loops! */

innerLoop:
   testb %al, (%esi)             /* is it on? */
   clc 
   jne notOn
   stc 

notOn:
   rcrw %dx                      /* suck up our new bit */

   decb %cl
   shlb %al                      /* move our bitmask over */
   jne innerLoop                 /* keep looping! */

   incl %esi                     /* next byte, please */
   decb %ch                      /* decrement our outer loop */
   jne startLoop                 /* try again */

   ret 
