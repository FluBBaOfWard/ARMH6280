
#include "H6280.i"
							;@ ARM flags
	.equ PSR_N, 0x80000000		;@ Negative (Sign)
	.equ PSR_Z, 0x40000000		;@ Zero
	.equ PSR_C, 0x20000000		;@ Carry
	.equ PSR_V, 0x10000000		;@ Overflow


							;@ HuC6280 flags
	.equ N, 0x80				;@ Sign (negative)
	.equ V, 0x40				;@ Overflow
	.equ T, 0x20				;@ T opcode?
	.equ B, 0x10				;@ Interrupt by BRK opcode?
	.equ D, 0x08				;@ Decimal mode
	.equ I, 0x04				;@ Interrup Disable
	.equ Z, 0x02				;@ Zero
	.equ C, 0x01				;@ Carry

;@----------------------------------------------------------------------------
	.equ RES_VECTOR, 0xFFFE		;@ RESET interrupt vector address
	.equ NMI_VECTOR, 0xFFFC		;@ NMI interrupt vector address
	.equ TIM_VECTOR, 0xFFFA		;@ TIMER interrupt vector address
	.equ IRQ_VECTOR, 0xFFF8		;@ VDC interrupt vector address
	.equ BRK_VECTOR, 0xFFF6		;@ BRK/CD interrupt vector address
;@----------------------------------------------------------------------------
	.equ NOCPUHACK, 2			;@ don't use JMP hack
;@----------------------------------------------------------------------------
	.equ CYC_SHIFT, 8
	.equ CYCLE, 1<<CYC_SHIFT	;@ one cycle
	.equ CYC_MASK, CYCLE-1		;@ Mask
;@----------------------------------------------------------------------------
;@ cycle flags- (stored in cycles reg for speed)

	.equ CYC_C, 0x01			;@ Carry bit
	.equ CYC_I, 0x04			;@ IRQ mask
	.equ CYC_D, 0x08			;@ Decimal bit
	.equ CYC_V, 0x40			;@ Overflow bit
;@----------------------------------------------------------------------------


	.macro alignOpCode
	.align 6
	.endm

	.macro setIrqPin x
	ldrb r0,[h6280optbl,#h6280IrqPending]
	orr r0,r0,#(\x)
	strb r0,[h6280optbl,#h6280IrqPending]
	.endm

	.macro clearIrqPin x
	ldrb r0,[h6280optbl,#h6280IrqPending]
	bic r0,r0,#(\x)
	strb r0,[h6280optbl,#h6280IrqPending]
	.endm

	.macro loadLastBank reg
	ldr \reg,[h6280optbl,#h6280LastBank]
	.endm

	.macro storeLastBank reg
	str \reg,[h6280optbl,#h6280LastBank]
	.endm

	.macro encodePC				;@ Translate h6280pc from HuC6280 PC to rom offset
	bl translatePCToOffset		;@ In=h6280pc, out=r0
	.endm

	.macro reEncodePC			;@ Retranslate h6280pc romoffset
	loadLastBank r0
	sub h6280pc,h6280pc,r0
	encodePC
	.endm

	.macro encodeP extra	;@ Pack HuC6280 flags into r0
	and r0,cycles,#CYC_D+CYC_I+CYC_C+CYC_V
	tst h6280nz,#PSR_N
	orrne r0,r0,#N				;@ N
	tst h6280nz,#0xff
	orreq r0,r0,#Z				;@ Z
	orr r0,r0,#\extra			;@ B...
	.endm

	.macro decodePF			;@ Unpack HuC6280 flags from r0
	bic cycles,cycles,#CYC_D+CYC_I+CYC_C+CYC_V
	and r1,r0,#D+I+C+V
	orr cycles,cycles,r1		;@ DICV
	bic h6280nz,r0,#0xFD			;@ r0 is signed
	eor h6280nz,h6280nz,#Z
	.endm

	.macro getNextOpcode
	ldrb r0,[h6280pc],#1
	.endm

	.macro executeOpCode count
	subs cycles,cycles,#(\count)*4*CYCLE
	addpl pc,h6280optbl,r0,lsl#6
	b outOfCycles
	.endm

	.macro executeOpCode_c count
	sbcs cycles,cycles,#(\count)*4*CYCLE
	addpl pc,h6280optbl,r0,lsl#6
	b outOfCycles
	.endm

	.macro fetch count
	getNextOpcode
	executeOpCode \count
	.endm

	.macro fetch_c count	;@ Same as fetch except it adds the Carry (bit 0) also.
	getNextOpcode
	executeOpCode_c \count
	.endm

	.macro eatcycles count
	sub cycles,cycles,#(\count)*4*CYCLE
	.endm

	.macro clearcycles
	and cycles,cycles,#CYC_MASK	;@ Save CPU bits
	.endm

/*	.macro readmemabs
	and r1,addy,#0xe000
	adr lr,%F0
	ldr pc,[h6280rmem,r1,lsr#11]	;@ In: addy,r1=addy&0xe000
0								;@ Out: r0=val (bits 8-31=0 (LSR,ROR,INC,DEC,ASL)), addy preserved for RMW instructions
	.endm*/

	.macro readmemabs
	bl mem_R8
	.endm

	.macro readmemiix
	bl mem_R8IIX
	.endm

	.macro readmemiiy
	bl mem_R8IIY
	.endm

	.macro readmemzpa
	bl mem_R8ZPI
	.endm

	.macro readmemaiy
	bl mem_R8AIY
	.endm

	.macro readmemaix
	bl mem_R8AIX
	.endm

	.macro readmemzp
	ldrb r0,[h6280zpage,addy]
	.endm

	.macro readmemzpi
	ldrb r0,[h6280zpage,addy,lsr#24]
	.endm

	.macro readmemzps
	ldrsb h6280nz,[h6280zpage,addy]
	.endm

	.macro readmemimm
	ldrb r0,[h6280pc],#1
	.endm

	.macro readmemimms
	ldrsb h6280nz,[h6280pc],#1
	.endm

	.macro readmem
	.ifeq AddressMode-_ABS
		readmemabs
	.endif
	.ifeq AddressMode-_ZP
		readmemzp
	.endif
	.ifeq AddressMode-_ZPI
		readmemzpi
	.endif
	.ifeq AddressMode-_IMM
		readmemimm
	.endif
	.endm

	.macro readmems
	.ifeq AddressMode-_ABS
		readmemabs
		orr h6280nz,r0,r0,lsl#24
	.endif
	.ifeq AddressMode-_ZP
		readmemzps
	.endif
	.ifeq AddressMode-_IMM
		readmemimms
	.endif
	.endm


/*	.macro writememabs
	and r1,addy,#0xe000
	adr r2,writemem_tbl
	adr lr,%F0
	ldr pc,[r2,r1,lsr#11]		;@ In: addy,r0=val(bits 8-31=?),r1=addy&0xe000(for CDRAM_W)
0								;@ Out: r0,r1,r2,addy=?
	.endm*/

	.macro writememabs
	bl mem_W8
	.endm

	.macro writememzp
	strb r0,[h6280zpage,addy]
	.endm

	.macro writememzpi
	strb r0,[h6280zpage,addy,lsr#24]
	.endm

	.macro writemem
	.ifeq AddressMode-_ABS
		writememabs
	.endif
	.ifeq AddressMode-_ZP
		writememzp
	.endif
	.ifeq AddressMode-_ZPI
		writememzpi
	.endif
	.endm
;@----------------------------------------------------------------------------

	.macro push16			;@ Push r0
	mov r1,r0,lsr#8
	strb r1,[h6280zpage,h6280sp,ror#24]
	sub h6280sp,h6280sp,#0x01000000
	strb r0,[h6280zpage,h6280sp,ror#24]
	sub h6280sp,h6280sp,#0x01000000
	.endm						;@ r1,r2=?

	.macro push8 reg
	strb \reg,[h6280zpage,h6280sp,ror#24]
	sub h6280sp,h6280sp,#0x01000000
	.endm						;@r2=?

	.macro pop16			;@ Pop h6280pc
	add h6280sp,h6280sp,#0x01000000
	ldrb h6280pc,[h6280zpage,h6280sp,ror#24]
	add h6280sp,h6280sp,#0x01000000
	ldrb r0,[h6280zpage,h6280sp,ror#24]
	orr h6280pc,h6280pc,r0,lsl#8
	.endm						;@ r0,r1=?

	.macro pop8 reg
	add h6280sp,h6280sp,#0x01000000
	mov r2,h6280sp,ror#24
	ldrsb \reg,[h6280zpage,r2]	;@ Signed for PLA, PLX, PLY, PLP & RTI
	.endm						;@ r2=?

;@----------------------------------------------------------------------------
;@ doXXX: Load addy, increment h6280pc


	.equ _IMM,	1				;@ Immediate
	.equ _ZP,	2				;@ Zero page
	.equ _ZPI,	3				;@ Zero page indexed
	.equ _ABS,	4				;@ Absolute

	.macro doABS			;@ Absolute				$nnnn
	.set AddressMode, _ABS
	ldrb addy,[h6280pc],#1
	ldrb r0,[h6280pc],#1
	orr addy,addy,r0,lsl#8
	.endm

	.macro doAIX			;@ Absolute indexed X	$nnnn,X
	.set AddressMode, _ABS
	ldrb addy,[h6280pc],#1
	ldrb r0,[h6280pc],#1
	add addy,addy,h6280x,lsr#24
	add addy,addy,r0,lsl#8
;@	bic addy,addy,#0xff0000
	.endm

	.macro doAIY			;@ Absolute indexed Y	$nnnn,Y
	.set AddressMode, _ABS
	ldrb addy,[h6280pc],#1
	ldrb r0,[h6280pc],#1
	add addy,addy,h6280y,lsr#24
	add addy,addy,r0,lsl#8
;@	bic addy,addy,#0xff0000
	.endm

	.macro doIMM			;@ Immediate			#$nn
	.set AddressMode, _IMM
	.endm

	.macro doIIX			;@ Indexed indirect X	($nn,X)
	.set AddressMode, _ABS
	ldrb r0,[h6280pc],#1
	add r0,h6280x,r0,lsl#24
	ldrb addy,[h6280zpage,r0,lsr#24]
	add r0,r0,#0x01000000
	ldrb r1,[h6280zpage,r0,lsr#24]
	orr addy,addy,r1,lsl#8
	.endm

	.macro doIIY			;@ Indirect indexed Y	($nn),Y
	.set AddressMode, _ABS
	ldrb r0,[h6280pc],#1
	ldrb addy,[r0,h6280zpage]!
	ldrb r1,[r0,#1]
	add addy,addy,h6280y,lsr#24
	add addy,addy,r1,lsl#8
;@	bic addy,addy,#0xff0000
	.endm

	.macro doZPI			;@ Zeropage indirect	($nn)
	.set AddressMode, _ABS
	ldrb r0,[h6280pc],#1
	ldrb addy,[r0,h6280zpage]!
	ldrb r1,[r0,#1]
	orr addy,addy,r1,lsl#8
	.endm

	.macro doZ				;@ Zero page			$nn
	.set AddressMode, _ZP
	ldrb addy,[h6280pc],#1
	.endm

	.macro doZ2				;@ Zero page			$nn
	.set AddressMode, _ZP
	ldrb addy,[h6280pc],#2		;@ Ugly thing for bbr/bbs
	.endm

	.macro doZIX			;@ Zero page indexed X	$nn,X
	.set AddressMode, _ZP
	ldrb addy,[h6280pc],#1
	add addy,addy,h6280x,lsr#24
	and addy,addy,#0xff
	.endm

	.macro doZIXf			;@ Zero page indexed X	$nn,X
	.set AddressMode, _ZPI
	ldrb addy,[h6280pc],#1
	add addy,h6280x,addy,lsl#24
	.endm

	.macro doZIY			;@ Zero page indexed Y	$nn,Y
	.set AddressMode, _ZP
	ldrb addy,[h6280pc],#1
	add addy,addy,h6280y,lsr#24
	and addy,addy,#0xff
	.endm

	.macro doZIYf			;@ Zero page indexed Y	$nn,Y
	.set AddressMode, _ZPI
	ldrb addy,[h6280pc],#1
	add addy,h6280y,addy,lsl#24
	.endm

;@----------------------------------------------------------------------------

	.macro opADC cyc
	tst cycles,#CYC_D
	bne opADC_Dec

	movs r1,cycles,lsr#1		;@ Get C
	subcs r0,r0,#0x00000100
	orr cycles,cycles,#CYC_C+CYC_V	;@ Prepare C & V
	adcs h6280a,h6280a,r0,ror#8
	bicvc cycles,cycles,#CYC_V	;@ V
	getNextOpcode
	mov h6280nz,h6280a,asr#24		;@ NZ
	executeOpCode_c \cyc
	.endm

	.macro opADCD				;@ Doesn't affect V
	orrs r0,r0,cycles,lsl#31
	orrmi h6280a,h6280a,#0x00800000
	mov h6280a,h6280a,ror#28
	adds h6280a,h6280a,r0,ror#4
	cmncc h6280a,#0x60000000
	addcs h6280a,h6280a,#0x60000001
	mov h6280a,h6280a,ror#5
	movs h6280a,h6280a,lsl#1
	cmncc h6280a,#0x60000000
	addscs h6280a,h6280a,#0x60000000
	bic cycles,cycles,#CYC_C	;@ Clear C
	orrcs cycles,cycles,#CYC_C	;@ C

	mov h6280nz,h6280a,asr#24 	;@ NZ
	.endm


	.macro opADCT cyc
	readmem
	tst cycles,#CYC_D
	bne opADCT_Dec

	ldrb r2,[h6280zpage,h6280x,lsr#24]
	mov r2,r2,lsl#24

	movs r1,cycles,lsr#1		;@ Get C
	subcs r0,r0,#0x00000100
	orr cycles,cycles,#CYC_C+CYC_V	;@ Prepare C & V
	adcs r2,r2,r0,ror#8
	bicvc cycles,cycles,#CYC_V	;@ V

	mov h6280nz,r2,asr#24 		;@ NZ

	strb h6280nz,[h6280zpage,h6280x,lsr#24]
	fetch_c \cyc
	.endm

	.macro opADCTD				;@ Doesn't affect V
	ldrb r2,[h6280zpage,h6280x,lsr#24]
	mov r2,r2,lsl#24

	orrs r0,r0,cycles,lsl#31
	orrmi r2,r2,#0x00800000
	mov r2,r2,ror#28
	adds r2,r2,r0,ror#4
	cmncc r2,#0x60000000
	addcs r2,r2,#0x60000001
	mov r2,r2,ror#5
	movs r2,r2,lsl#1
	cmncc r2,#0x60000000
	addscs r2,r2,#0x60000000
	bic cycles,cycles,#CYC_C	;@ Clear C
	orrcs cycles,cycles,#CYC_C	;@ C

	mov h6280nz,r2,asr#24 		;@ NZ

	strb h6280nz,[h6280zpage,h6280x,lsr#24]
	.endm


	.macro opAND cyc
	readmem
	and h6280a,h6280a,r0,lsl#24
	getNextOpcode
	mov h6280nz,h6280a,asr#24		;@ NZ
	executeOpCode \cyc
	.endm

	.macro opANDT cyc
	readmem
	ldrb r2,[h6280zpage,h6280x,lsr#24]
	and r2,r2,r0
	orr h6280nz,r2,r2,lsl#24		;@ NZ
	getNextOpcode
	strb r2,[h6280zpage,h6280x,lsr#24]
	executeOpCode \cyc
	.endm


	.macro opASL cyc
	readmem
	 mov cycles,cycles,lsr#1	;@ Get C
	 add r0,r0,r0
	 orrs h6280nz,r0,r0,lsl#24	;@ NZ
	 adc cycles,cycles,cycles	;@ Set C
	writemem
	fetch \cyc
	.endm


	.macro opBBR bit
	doZ
	readmemzp
	ldrsb r1,[h6280pc],#1
	tst r0,#1<<(\bit)
	addeq h6280pc,h6280pc,r1
	getNextOpcode
	subeq cycles,cycles,#2*4*CYCLE
	executeOpCode 6
	.endm

	.macro opBBRx bit
	doZ
	readmemzp
	ldrsb r1,[h6280pc],#1
	tst r0,#1<<(\bit)
	subeq cycles,cycles,#2*4*CYCLE
	addeq h6280pc,h6280pc,r1
	cmpeq r1,#-3					;@ Ninja Spirit/Impossamole speed hack.
	andeq cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 6
	.endm

	.macro opBBS bit
	doZ
	readmemzp
	ldrsb r1,[h6280pc],#1
	tst r0,#1<<(\bit)
	addne h6280pc,h6280pc,r1
	getNextOpcode
	subne cycles,cycles,#2*4*CYCLE
	executeOpCode 6
	.endm

	.macro opBBSx bit
	doZ2
	readmemzp
	tst r0,#1<<(\bit)
	beq nobbranch
	ldrsbne r0,[h6280pc,#-1]
	addne h6280pc,h6280pc,r0
	cmp r0,#-3						;@ Bloody Wolf speed hack.
	andeq cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 8
	.endm

	.macro opBIT cyc
	readmem
	bic cycles,cycles,#CYC_V	;@ Clear V
	tst r0,#V
	and h6280nz,r0,h6280a,lsr#24	;@ Z
	orr h6280nz,h6280nz,r0,lsl#24	;@ N
	getNextOpcode
	orrne cycles,cycles,#CYC_V	;@ V
	executeOpCode \cyc
	.endm

	.macro opBRA
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
	fetch 4						;@ +1 if pageboundary crossed?
	.endm

	.macro opBCC
	ldrsb r0,[h6280pc],#1
	tst cycles,#CYC_C			;@ Test Carry
	subeq cycles,cycles,#2*4*CYCLE
	addeq h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opBCS
	ldrsb r0,[h6280pc],#1
	tst cycles,#CYC_C			;@ Test Carry
	subne cycles,cycles,#2*4*CYCLE
	addne h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opBEQ
	ldrsb r0,[h6280pc],#1
	tst h6280nz,#0xff
	subeq cycles,cycles,#2*4*CYCLE
	addeq h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opBMI
	ldrsb r0,[h6280pc],#1
	tst h6280nz,#0x80000000
	subne cycles,cycles,#2*4*CYCLE
	addne h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opBNE
	ldrsb r0,[h6280pc],#1
	tst h6280nz,#0xff
	subne cycles,cycles,#2*4*CYCLE
	addne h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opBPL
	ldrsb r0,[h6280pc],#1
	tst h6280nz,#0x80000000
	subeq cycles,cycles,#2*4*CYCLE
	addeq h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opBVC
	ldrsb r0,[h6280pc],#1
	tst cycles,#CYC_V
	subeq cycles,cycles,#2*4*CYCLE
	addeq h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opBVS
	ldrsb r0,[h6280pc],#1
	tst cycles,#CYC_V
	subne cycles,cycles,#2*4*CYCLE
	addne h6280pc,h6280pc,r0
	fetch 2
	.endm

	.macro opCOMP reg cyc
	readmem
	subs h6280nz,\reg,r0,lsl#24
	orr cycles,cycles,#CYC_C	;@ Prepare C
	getNextOpcode
	mov h6280nz,h6280nz,asr#24	;@ NZ
	executeOpCode_c \cyc
	.endm

	.macro opDEC cyc
	readmem
	sub r0,r0,#1
	orr h6280nz,r0,r0,lsl#24		;@ NZ
	writemem
	fetch \cyc
	.endm

	.macro opEOR cyc
	readmem
	eor h6280a,h6280a,r0,lsl#24
	getNextOpcode
	mov h6280nz,h6280a,asr#24		;@ NZ
	executeOpCode \cyc
	.endm

	.macro opEORT cyc
	readmem
	ldrb r2,[h6280zpage,h6280x,lsr#24]
	eor r2,r2,r0
	orr h6280nz,r2,r2,lsl#24
	getNextOpcode
	strb r2,[h6280zpage,h6280x,lsr#24]
	executeOpCode \cyc
	.endm


	.macro opINC cyc
	readmem
	add r0,r0,#1
	orr h6280nz,r0,r0,lsl#24		;@ NZ
	writemem
	fetch \cyc
	.endm

	.macro opLOAD reg cyc
	readmems
	getNextOpcode
	mov \reg,h6280nz,lsl#24
	executeOpCode \cyc
	.endm

	.macro opLSR cyc
	.ifeq AddressMode-_ABS
		readmemabs
		movs r0,r0,lsr#1
		orr cycles,cycles,#CYC_C	;@ Prepare C
		biccc cycles,cycles,#CYC_C
		mov h6280nz,r0				;@ Z, (N=0)
		writememabs
		fetch \cyc
	.endif
	.ifeq AddressMode-_ZP
		ldrb h6280nz,[h6280zpage,addy]
		orr cycles,cycles,#CYC_C	;@ Prepare C
		movs h6280nz,h6280nz,lsr#1	;@ Z, (N=0)
		getNextOpcode
		strb h6280nz,[h6280zpage,addy]
		executeOpCode_c \cyc
	.endif
	.ifeq AddressMode-_ZPI
		ldrb h6280nz,[h6280zpage,addy,lsr#24]
		orr cycles,cycles,#CYC_C	;@ Prepare C
		movs h6280nz,h6280nz,lsr#1	;@ Z, (N=0)
		getNextOpcode
		strb h6280nz,[h6280zpage,addy,lsr#24]
		executeOpCode_c \cyc
	.endif
	.endm

	.macro opORA cyc
	readmem
	orr h6280a,h6280a,r0,lsl#24
	getNextOpcode
	mov h6280nz,h6280a,asr#24		;@ NZ
	executeOpCode \cyc
	.endm

	.macro opORAT cyc
	readmem
	ldrb r2,[h6280zpage,h6280x,lsr#24]
	orr r2,r2,r0
	orr h6280nz,r2,r2,lsl#24
	getNextOpcode
	strb r2,[h6280zpage,h6280x,lsr#24]
	executeOpCode \cyc
	.endm

	.macro opRMB bit
	doZ
	readmemzp
	bic r0,r0,#1<<(\bit)
	writememzp
	fetch 7
	.endm

	.macro opROL cyc
	readmem
	 movs cycles,cycles,lsr#1	;@ Get C
	 adc r0,r0,r0
	 orrs h6280nz,r0,r0,lsl#24	;@ NZ
	 adc cycles,cycles,cycles	;@ Set C
	writemem
	fetch \cyc
	.endm

	.macro opROR cyc
	readmem
	 movs cycles,cycles,lsr#1	;@ Get C
	 orrcs r0,r0,#0x100
	 movs r0,r0,lsr#1
	 adc cycles,cycles,cycles	;@ Set C
	 orr h6280nz,r0,r0,lsl#24	;@ NZ
	writemem
	fetch \cyc
	.endm

	.macro opSBC cyc
	tst cycles,#CYC_D
	bne opSBC_Dec

	movs r1,cycles,lsr#1		;@ Get C
	sbcs h6280a,h6280a,r0,lsl#24
	orr cycles,cycles,#CYC_C+CYC_V	;@ Prepare C & V
	and h6280a,h6280a,#0xff000000
	bicvc cycles,cycles,#CYC_V	;@ V
	getNextOpcode
	mov h6280nz,h6280a,asr#24		;@ NZ
	executeOpCode_c \cyc
	.endm

	.macro opSBCD				;@ Doesn't affect V
	mov r1,#0
	movs cycles,cycles,lsr#1	;@ Get C
	mov r0,r0,ror#31
	orrcc r0,r0,#0x01
	mov h6280a,h6280a,ror#28
	subs h6280a,h6280a,r0,lsl#23
	movcc r1,#0x06000000

	sbc r0,h6280a,r0,lsr#5
	movs r0,r0,lsl#28
	adc cycles,cycles,cycles	;@ C
	eor cycles,cycles,#CYC_C	;@ C

	orrcs r1,r1,#0x60000000
	orr h6280a,r0,h6280a,lsr#4
	sub h6280a,h6280a,r1

	and h6280a,h6280a,#0xff000000
	mov h6280nz,h6280a,asr#24 	;@ NZ
	.endm

	.macro opSMB bit
	doZ
	readmemzp
	orr r0,r0,#1<<(\bit)
	writememzp
	fetch 7
	.endm

	.macro opSTORE reg cyc
	mov r0,\reg,lsr#24
	writemem
	fetch \cyc
	.endm

	.macro opSTZ cyc
	mov r0,#0
	writemem
	fetch \cyc
	.endm

	.macro opSWAP reg1 reg2
	eor \reg1,\reg1,\reg2
	eor \reg2,\reg2,\reg1
	getNextOpcode
	eor \reg1,\reg1,\reg2
	executeOpCode 3
	.endm

	.macro opTRB cyc
	readmem
	 bic cycles,cycles,#CYC_V		;@ Clear V
	 tst r0,#V
	 orrne cycles,cycles,#CYC_V		;@ V
	 bic h6280nz,r0,h6280a,lsr#24	;@ Z
	 orr h6280nz,h6280nz,r0,lsl#24	;@ N
	 bic r0,r0,h6280a,lsr#24		;@ Result
	writemem
	fetch \cyc
	.endm

	.macro opTSB cyc
	readmem
	bic cycles,cycles,#CYC_V	;@ Clear V
	tst r0,#V
	orrne cycles,cycles,#CYC_V	;@ V
	orr h6280nz,r0,h6280a,lsr#24	;@ Z
	orr h6280nz,h6280nz,r0,lsl#24	;@ N
	orr r0,r0,h6280a,lsr#24		;@ Result
	writemem
	fetch \cyc
	.endm

	.macro opTST cyc			;@ Needs a h6280pc++ before
	readmem
	bic cycles,cycles,#CYC_V	;@ Clear V
	tst r0,#V
	and h6280nz,r0,h6280nz		;@ Z
	orr h6280nz,h6280nz,r0,lsl#24	;@ N
	getNextOpcode
	orrne cycles,cycles,#CYC_V	;@ V
	executeOpCode \cyc
	.endm


	.macro transferStart		;@ Transfer Start
	stmfd sp!,{r3,r5,r6}

//	mov r1,r6,lsr#24
//	strb r1,[h6280zpage,h6280sp,ror#24]	;@ Push Y
//	sub r0,h6280sp,#0x01000000
//	mov r1,r4,lsr#24
//	strb r1,[h6280zpage,r0,ror#24]	;@ Push A
//	sub r0,h6280sp,#0x02000000
//	mov r1,r5,lsr#24
//	strb r1,[h6280zpage,r0,ror#24]	;@ Push X

	ldrb r3,[h6280pc],#1
	ldrb r1,[h6280pc],#1
	orr r3,r3,r1,lsl#8			;@ Load r3 = source
	ldrb r5,[h6280pc],#1
	ldrb r1,[h6280pc],#1
	orr r5,r5,r1,lsl#8			;@ Load r5 = destination
	ldrb r6,[h6280pc],#1
	ldrb r1,[h6280pc],#1
	orrs r6,r6,r1,lsl#8			;@ Load r6 = length

	moveq r6,#0x10000
	mov r1,#6*4*CYCLE
	mul r1,r6,r1
	sub cycles,cycles,r1		;@ cycles=r8
	orr r3,r3,r3,lsl#16
	orr r5,r5,r5,lsl#16
	bx lr
	.endm

	.macro doTAI				;@ Transfer Alt Inc
	bl transferPre
	add r3,r3,#0x10000
0:
	mov r3,r3,ror#16
	mov addy,r3,lsr#16
	readmemabs

	mov addy,r5,lsr#16
	writememabs

	add r5,r5,#0x10000
	subs r6,r6,#1
	bne 0b						;@ In: addy,r0=val(bits 8-31=?)
								;@ Out: r0,r1,r2=?
	ldmfd sp!,{r3,r5,r6}
	fetch 17
	.endm


	.macro doTDD				;@ Transfer Dec Dec
	bl transferPre
0:
	mov addy,r3,lsr#16
	readmemabs

	mov addy,r5,lsr#16
	writememabs

	sub r3,r3,#0x10000
	sub r5,r5,#0x10000
	subs r6,r6,#1
	bne 0b

	ldmfd sp!,{r3,r5,r6}
	fetch 17
	.endm


	.macro doTIA				;@ Transfer Inc Alt
	bl transferPre
	add r5,r5,#0x10000
0:
	mov r5,r5,ror#16
	mov addy,r3,lsr#16
	readmemabs

	mov addy,r5,lsr#16
	writememabs

	add r3,r3,#0x10000
	subs r6,r6,#1
	bne 0b

	ldmfd sp!,{r3,r5,r6}
	fetch 17
	.endm


	.macro doTII				;@ Transfer Inc Inc
	bl transferPre
0:
	mov addy,r3,lsr#16
	readmemabs

	mov addy,r5,lsr#16
	writememabs

	add r3,r3,#0x10000
	add r5,r5,#0x10000
	subs r6,r6,#1
	bne 0b

	ldmfd sp!,{r3,r5,r6}
	fetch 17
	.endm


	.macro doTIN				;@ Transfer Inc None
	bl transferPre
0:
	mov addy,r3,lsr#16
	readmemabs

	mov addy,r5,lsr#16
	writememabs

	add r3,r3,#0x10000
	subs r6,r6,#1
	bne 0b

	ldmfd sp!,{r3,r5,r6}
	fetch 17
	.endm

;@----------------------------------------------------------------------------
