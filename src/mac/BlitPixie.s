
	MACRO
	MakeFunction &fnName
		EXPORT &fnName[DS]
 		EXPORT .&fnName[PR]
		
		TC &fnName[TC], &fnName[DS]
			
		CSECT &fnName[DS]
			DC.L .&fnName[PR]
 			DC.L TOC[tc0]
		
		CSECT .&fnName[PR]
		FUNCTION .&fnName[PR]	
		
	ENDM

bitmap 	record
w		DS.L	1
h		DS.L	1
clip	DS.L	1
cl		DS.L	1
cr		DS.L	1
ct		DS.L	1
cb		DS.L	1
vtable	DS.L	1
wbk		DS.L	1
rbk		DS.L	1
dat		DS.L	1
id		DS.L	1
extra	DS.L	1
xoff	DS.L	1
yoff	DS.L	1
seg		DS.L	1
lin		DS.L	1
		endr


linkageArea:		set 24	; constant comes from the PowerPC Runtime Architecture Document
CalleesParams:		set	32	; always leave space for GPR's 3-10
CalleesLocalVars:	set 0	; ClickHandler doesn't have any
numGPRs:			set 0	; num volitile GPR's (GPR's 13-31) used by ClickHandler
numFPRs:			set 0	; num volitile FPR's (FPR's 14-31) used by ClickHandler
spaceToSave:	set linkageArea + CalleesParams + CalleesLocalVars + 4*numGPRs + 8*numFPRs  


	MakeFunction	Clear8
;		mflr	r0					; Get link register
;		stw		r0, 0x0008(SP)		; Store the link resgister on the stack
;		stwu	SP, -spaceToSave(SP); skip over the stack space where the caller		
									; might have saved stuff
;r3 long color
;r4 unsigned char *dst
;r5 long w
;r6 rowBytes
;r7 last byte
		
		sub		r9,r6,r5			;inc=rowBytes-w
		srawi	r5,r5,2				;w/4
		mfctr	r10

		mtctr	r5
		b		clear_loop_x
clear_loop_y:				
		add		r4,r4,r9			;p+=inc
		mtctr	r5
		
clear_loop_x:
		stw		r3,0(r4)
;		dcbz	0,(r4)
		addi	r4,r4,+4
		bdnz	clear_loop_x
loop_x_end:		
		cmp		cr0,r4,r7
		blt		cr0,clear_loop_y

		mtctr	r10
		blr								; return via the link register
	csect .Clear8[pr]
		dc.b 'Clear8'
	

	MakeFunction	ClearChunk
;dst r3
;w
;
;
		b		iloop:
eloop:
		add		r3,r3,r5
		add		r4,r4,r6
iloop:	
		cmp		cr0,r3,r4
		dcbz	0,(r3)
		dcbst	0,(r3)
		addi	r3,r3,+32
		blt		cr0,iloop
		cmp		cr1,r3,r7
		blt		cr1,eloop:
		blr
	
	csect .ClearChunk[pr]
		dc.b 'ClearChunck'

;**********************************************************************

;	MakeFunction	_mac_sys_clear_to_color8
;r_bmp	equ r3
;r_color equ r4
;r_w		equ	r5
;r_h		equ	r6
;r_rb	equ	r7
;c_dst	equ	r8
;r_last	equ	r9
;r_inc 	equ r10
;r_tmp	equ r10
;r_tmp2	equ r9
;		
;;		mflr	r0					; Get link register
;		rlwinm	r_color,r_color, 0, 24, 31	;color=color&0xFF		
;		addi	r_tmp, r0, 0x101
;		addis	r_tmp, r_tmp, 0x101
;		mullw	r_color,r_tmp,r_color			;color=color*0x01010101
;		
;		lwz		c_dst,bitmap.lin(r_bmp)		;
;		lwz		r_tmp2,bitmap.lin+4(r_bmp)		;
;		sub		r_rb,r_tmp2,c_dst				;
;		
;		lwz		r_w,bitmap.w(r_bmp)			;
;		addi	r_w,r_w,3					;
;		rlwinm	r_w,r_w,0,0,29				;w=(w+3)&0xFFFFFFFC
;		
;		lwz		r_h,bitmap.h(r_bmp)			;
;		
;		add		r_last,c_dst,r_w			;
;		sub		r_inc,r_rb,r_w				;
;		addi	r_inc,r_inc,-4				;
;		b		@clear_loop_x				;
;@clear_loop_y:					
;		add		c_dst,c_dst,r_inc			;dst+=inc
;		add		r_last,r_last,r_rb			;last1+=rowBytes
;@clear_loop_x:
;		cmp		cr5,c_dst,r_last			;do
;		stw		r_color,0(c_dst)			;*dst=color
;		addi	c_dst,c_dst,+4				;dst++
;		blt		cr5,@clear_loop_x			;while dst<last
;@loop_x_end:
;		addi	r_h,r_h,-1					;
;		cmpi	cr0,r_h,0					;
;		bne		@clear_loop_y				
;;		mtlr	r0							; Reset the link register
;		blr
;
;	csect ._mac_sys_clear_to_color8[pr]
;		dc.b '_mac_sys_clear_to_color8'
;
;
