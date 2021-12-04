#ifdef __arm__

#include "H6280mac.h"

	.syntax unified

#ifdef ARM9
	.section .itcm				;@ For the NDS
#elif GBA
	.section .iwram				;@ For the GBA
#else
	.section .text
#endif
	.arm
	.align 2

	.global h6280Hacks

;@----------------------------------------------------------------------------
noBranch:
	add h6280pc,h6280pc,#1
	fetch 2
;@----------------------------------------------------------------------------
_10y:	;@ BPL *
;@----------------------------------------------------------------------------
	tst h6280nz,#0x80000000
	bne noBranch
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
	cmp r0,#-8						;@ speed hack 2 Street Fighter II CE
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 4
;@----------------------------------------------------------------------------
_30y:	;@ BMI
;@----------------------------------------------------------------------------
	tst h6280nz,#0x80000000
	beq noBranch
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
	cmp r0,#-7						;@ Speed hack Aoi Blue Blink
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 4
;@----------------------------------------------------------------------------
_4Cx:	;@ JMP $nnnn
;@----------------------------------------------------------------------------
	loadLastBank r2
	ldrb r0,[h6280pc],#1
	ldrb r1,[h6280pc],#1
	sub r2,h6280pc,r2
	orr h6280pc,r0,r1,lsl#8
	sub r2,h6280pc,r2
	cmp r2,#-8						;@ "Circus Lido" wants -9
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits

	encodePC
	fetch 4
;@----------------------------------------------------------------------------
_58x:	;@ CLI				patch for "Maniac Pro Wrestling (J)"
;@----------------------------------------------------------------------------
	bic cycles,cycles,#CYC_I
	fetch 2
;@----------------------------------------------------------------------------
_80y:	;@ BRA				branch always
;@----------------------------------------------------------------------------
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
	cmp r0,#-5						;@ Speed hack Darius, Toyshop Boys
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 4
;@----------------------------------------------------------------------------
_90y:	;@ BCC *
;@----------------------------------------------------------------------------
	tst cycles,#CYC_C				;@ Test Carry
	bne noBranch
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
	cmp r0,#-8						;@ Speed hack 2 Ninja Gaiden, Super Wolleyball
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 4
;@----------------------------------------------------------------------------
_B0y:	;@ BCS *
;@----------------------------------------------------------------------------
	tst cycles,#CYC_C				;@ Test Carry
	beq noBranch
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
	cmp r0,#-8						;@ Speed hack "Andre Panza Kick Boxing" & "Power Sports"
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 4
;@----------------------------------------------------------------------------
_D0y:	;@ BNE *
;@----------------------------------------------------------------------------
	tst h6280nz,#0xff
	beq noBranch
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
_D0z:	cmp r0,#-7					;@ Speed hack 2 Xeviuos -7
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 4
;@----------------------------------------------------------------------------
_F0y:	;@ BEQ *
;@----------------------------------------------------------------------------
	tst h6280nz,#0xff
	bne noBranch
	ldrsb r0,[h6280pc],#1
	add h6280pc,h6280pc,r0
_F0z:	cmp r0,#-7					;@ Speed hack 2 (-7)
	andcs cycles,cycles,#CYC_MASK	;@ Save CPU bits
	fetch 4
;@----------------------------------------------------------------------------
_E3x:	;@ TIA $nnnn,$nnnn,#$nnnn
;@----------------------------------------------------------------------------
	ldrb r2,[h6280pc,#2]
	ldrb r0,[h6280pc,#3]
	orr r2,r2,r0				;@ Destination
;@	ldr r1,=0x404
;@	cmp r2,r1					;@ VCE pal write
	cmp r2,#4					;@ VCE pal write
	beq _E3z
	cmp r2,#2					;@ VDC access
	bne _E3
	ldr r2,=vdcRegister
	ldrb r2,[r2]
	cmp r2,#2					;@ VRAM write
	bne _E3
_E3z:
	stmfd sp!,{r3-r7,r11}

	ldrb r3,[h6280pc],#1
	ldrb r0,[h6280pc],#3
	orr r3,r3,r0,lsl#8			;@ Load r3 = source
	mov r1,r3,lsr#13
	add r6,h6280optbl,#h6280RomMap
	ldr r0,[r6,r1,lsl#2]
	add r3,r3,r0				;@ r3 = real address.

	ldrb r5,[h6280pc],#1
	ldrb r0,[h6280pc],#1
	orrs r5,r5,r0,lsl#8			;@ load r5 = length

	moveq r5,#0x10000
	mov r0,#7*3*CYCLE			;@ Should it be 6*3 or 7*3?
	mul r0,r5,r0
	sub cycles,cycles,r0		;@ Cycles=r8

	cmp r2,#4					;@ VCE pal write
	beq _E3y

	ldr r6,=vdcAdrInc
	ldrb r6,[r6]
	mov r6,r6,lsl#16
	ldr r11,=vram_w_adr
	ldr addy,[r11]

	ldr r7,=DIRTYTILES
	ldr r2,=PCE_VRAM
;@	b VRAMcpy
;@----------------------------------------------------------------------------
VRAMcpy:
	movs addy,addy,asr#15

	strbpl r4,[r7,addy,lsr#7]	;@ dirty table.
	ldrb r0,[r3],#1				;@ Source
	ldrb r1,[r3],#1
	orrpl r0,r0,r1,lsl#8
	strhpl r0,[r2,addy]			;@ Write to virtual PCE_VRAM

	add addy,r6,addy,lsl#15
	subs r5,r5,#2
	bhi VRAMcpy

	str addy,[r11]

	ldmfd sp!,{r3-r7,r11}
	fetch 17

;@----------------------------------------------------------------------------
_E3y:	;@ TIA $nnnn,$nnnn,#$nnnn
;@----------------------------------------------------------------------------
	ldr r6,=vceAddress
	ldr r2,[r6]
	ldr r7,=vcePaletteRam
;@----------------------------------------------------------------------------
PaletteCpy:
	ldrb r0,[r3],#1				;@ Source
	ldrb r1,[r3],#1
	orr r0,r0,r1,lsl#8

	mov r4,r2,lsr#22
	strh r0,[r7,r4]
	add r2,r2,#0x00800000
	subs r5,r5,#2
	bhi PaletteCpy

	str r2,[r6]					;@ palettePtr
	ldmfd sp!,{r3-r7,r11}
	fetch 17
;@----------------------------------------------------------------------------


#ifdef ARM9
	.section .text				;@ For the NDS
#endif
	.align 2

;@----------------------------------------------------------------------------
h6280Hacks:			;@ called by CPU_reset (r0-r9 are free to use)
;@----------------------------------------------------------------------------
//	str lr,[sp,#-4]!

;@---cpu reset
	ldr r8,=gHackFlags
	ldr r8,[r8]
	ldr r1,midHack7
	tst r8,#0x40000000			;@ Check hack flags for F0 hack12.
	ldrne r1,superHack12
	tst r8,#0x08000000			;@ Check hack flags for F0 hack8.
	ldrne r1,midHack8
	tst r8,#0x04000000			;@ Check hack flags for F0 hack11.
	ldrne r1,superHack11
	ldr r2,=_F0z
	str r1,[r2]

	ldr r1,midHack7				;@ D0 special hacks
	tst r8,#0x02000000			;@ Check hack flags for D0-0xFB hack.
	ldrne r1,lowHack5
	tst r8,#0x20000000			;@ Check hack flags for D0-0xF6 hack+.
	ldrne r1,superHack10
	tst r8,#0x10000000			;@ Check hack flags for D0-0xFC hack-.
	ldrne r1,lowHack4
	ldr r2,=_D0z
	str r1,[r2]

	ldr r0,=gHwFlags
	ldr r0,[r0]
	tst r0,#NOCPUHACK			;@ Load opcode set
	adr r2,hackOps
	adrne r2,normalOps			;@ This makes all cpuhacks go away if it's choosen.
	tsteq r8,#0x80000000		;@ Check hack flags for nojump hack.
	adreq r1,jmpOps
	adrne r1,normalOps
	adr r3,opIndex
	mov r7,#1
	mov r4,#11					;@ Number of hacks
nr0:
	tst r8,r7,lsl r4
	ldrne r5,[r2,r4,lsl#2]		;@ Hacks
	ldreq r5,[r1,r4,lsl#2]		;@ Normal
	ldr r6,[r3,r4,lsl#2]
	str r5,[r6]
	subs r4,r4,#1
	bpl nr0

	bx lr
//	ldr pc,[sp],#4
;@----------------------------------------------------------------------------
normalOps:
	.long _10,_30,_4C,_50,_70,_80,_90,_B0,_D0,_F0,_58,_E3
jmpOps:
	.long _10,_30,_4Cx,_50,_70,_80,_90,_B0,_D0,_F0,_58,_E3x
hackOps:
	.long _10y,_30y,0,0,0,_80y,_90y,_B0y,_D0y,_F0y,_58x,_E3
opIndex:
//	.long h6280OpTable+0x10*4,h6280OpTable+0x30*4,h6280OpTable+0x4C*4,h6280OpTable+0x50*4,h6280OpTable+0x70*4
//	.long h6280OpTable+0x80*4,h6280OpTable+0x90*4,h6280OpTable+0xB0*4,h6280OpTable+0xD0*4,h6280OpTable+0xF0*4,h6280OpTable+0x58*4,h6280OpTable+0xE3*4
superHack12:
	cmp r0,#-12					;@ Speed hack+ for SF2CE, Deep Blue(D0)
superHack11:
	cmp r0,#-11					;@ Speed hack+ for Morita Shogi PC (F0)
superHack10:
	cmp r0,#-10					;@ Speed hack+ for Galaga(D0)
midHack8:
	cmp r0,#-8					;@ Speed hack normal
midHack7:
	cmp r0,#-7					;@ Speed hack normal
lowHack5:
	cmp r0,#-5					;@ Speed hack low
lowHack4:
	cmp r0,#-4					;@ Speed hack low
;@----------------------------------------------------------------------------

#endif // #ifdef __arm__
