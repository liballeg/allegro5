# Check if assembler understands MMX and SSE instructions.
#  check_MMX(VARIABLE)
#  check_SSE(VARIABLE)
#  VARIABLE - variable to store the result to
#

macro(check_MMX var)
    if("${var}" MATCHES "^${var}$")
	asm_test(${var} "
	    .globl _dummy_function
	    _dummy_function:
	    pushl %ebp
	    movl %esp, %ebp
	    movq -8(%ebp), %mm0
	    movd %edi, %mm1
	    punpckldq %mm1, %mm1
	    psrld $ 19, %mm7
	    pslld $ 10, %mm6
	    por %mm6, %mm7
	    paddd %mm1, %mm0
	    emms
	    popl %ebp
	    ") 
	set(${var} "${${var}}" CACHE INTERNAL "Assembler understands MMX")
    endif("${var}" MATCHES "^${var}$")
endmacro(check_MMX)

macro(check_SSE var)
    if("${var}" MATCHES "^${var}$")
        asm_test(${var} "
            .globl _dummy_function
            _dummy_function:
            pushl %ebp
            movl %esp, %ebp
            movq -8(%ebp), %mm0
            movd %edi, %mm1
            punpckldq %mm1, %mm1
            psrld $ 10, %mm3
            maskmovq %mm3, %mm1
            paddd %mm1, %mm0
            emms
            popl %ebp
            ret
            ")
	set(${var} "${${var}}" CACHE INTERNAL "Assembler understands SSE")
    endif("${var}" MATCHES "^${var}$")
endmacro(check_SSE)
