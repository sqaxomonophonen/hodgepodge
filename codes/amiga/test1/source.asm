ExecBase = 4
OpenLibrary = -552
CloseLibrary = -414
AvailMem = -216
Write = -48
Output = -60


; http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_2._guide/node0089.html
; #define MEMF_ANY    (0L)	/* Any type of memory will do */
; #define MEMF_PUBLIC (1L<<0)
; #define MEMF_CHIP   (1L<<1)
; #define MEMF_FAST   (1L<<2)
; #define MEMF_LOCAL  (1L<<8)	/* Memory that does not go away at RESET */
; #define MEMF_24BITDMA (1L<<9)	/* DMAable memory within 24 bits of address */
; 
; #define MEMF_CLEAR   (1L<<16)	/* AllocMem: NULL out area before return */
; #define MEMF_LARGEST (1L<<17)	/* AvailMem: return the largest chunk size */
; #define MEMF_REVERSE (1L<<18)	/* AllocMem: allocate from the top down */
; #define MEMF_TOTAL   (1L<<19)	/* AvailMem: return total size of memory */

MEMF_CHIP = 1<<1
MEMF_FAST = 1<<2
MEMF_LARGEST = 1<<17
LARGEST_CHIP = MEMF_CHIP | MEMF_LARGEST
LARGEST_FAST = MEMF_FAST | MEMF_LARGEST

init:
        move.l  ExecBase,a2
        move.l  a2,a6
        lea     doslib,a1
        moveq   #0,d0
        jsr     OpenLibrary(a6)
        tst.l   d0
        beq.s   done

        move.l  d0,dosbase
        move.l  d0,a6       ; a6 keeps DosBase while we use it

        jsr     Output(a6)
        move.l  d0,d1
        move.l  #msg,d2
        move.l  #msglen,d3
        jsr     Write(a6)

	move.l  a2,a6
	move.l  #LARGEST_CHIP,d1
	jsr	AvailMem(a6)

	move.l	d0,d1 ; d1 = available memory
	moveq	#7,d3
        move.l  #hex,a4
	move.l	#msg+9,a5
	move.b	#10,-(a5)
	;move.l	#$cafed00d,d1
hexloop:
	move.l	d1,d2
	andi	#15,d2
	move.b	(0,a4,d2.w),d4 ; 32-bit instruction, using "brief extension word", 14 total cycles?
	move.b	d4,-(a5)
	lsr.l	#4,d1
	dbra	d3,hexloop

	move.l  dosbase,a6
        jsr     Output(a6)
        move.l  d0,d1
        move.l  #msg,d2
        moveq   #9,d3
        jsr     Write(a6)

        move.l  a6,a1
        move.l  a2,a6                   ; Exec call
        jsr     CloseLibrary(a6)

done:
	moveq.l #0,d0
        rts

dosbase	dc.l 0

hex:	dc.b	"0123456789abcdef"
doslib:	dc.b    "dos.library",0
msg:	dc.b    "Hej verden!",10,"CHIP MEM available: " ; XXX must be at least 9 bytes long
msglen = *-msg
