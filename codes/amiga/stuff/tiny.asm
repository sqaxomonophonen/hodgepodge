

init:

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	macro	PC_MARKER
	printt \1
	printv *
	endm

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	macro	BEGIN_DATA
	PC_MARKER DATA_BEGIN
	endm

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	macro	END_DATA
	PC_MARKER DATA_END
	endm

	macro	FXMUL
	printt "macro:FXMUL",\1,\2
	printv *
	muls.w	\1,\2
	clr.w	\2
	swap	\2
	endm
	


	move.l	#$10000,a2
	moveq	#$42,d2
	moveq	#8,d1
loop:
	move.b	d2,(a2)+
	dbra	d1,loop

	; stop
	move.l	#$ddd000,a3
	move.b	#0,(a3)


	BEGIN_DATA
	dc.b $1,$2,$3,$4
	END_DATA

	FXMUL d0,d1
	FXMUL d0,d1
	FXMUL d1,d0
	FXMUL d6,d0

	or.l d0,(a0)+
	or.l d0,(a0)+
	or.l d0,(a0)+

