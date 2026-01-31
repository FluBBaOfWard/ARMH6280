//
//  H6280.i
//  ARMH6280
//
//  Created by Fredrik Ahlström on 2003-01-01.
//  Copyright © 2003-2026 Fredrik Ahlström. All rights reserved.
//
#if !__ASSEMBLER__
	#error This header file is only for use in assembly files!
#endif

							;@ r0,r1,r2=temp regs
	h6280nz		.req r3			;@ bit 31=N, Z=1 if bits 0-7=0
	h6280a		.req r4			;@ bits  0-23=0, also used to clear bytes in memory (vdc.s)
	h6280x		.req r5			;@ bits  0-23=0
	h6280y		.req r6			;@ bits  0-23=0
	h6280sp		.req r7			;@ bits 24-31=SP, bit 0=1.
	cycles		.req r8
	h6280pc		.req r9
	h6280ptr	.req r10
	h6280zpage	.req r11		;@ pceRAM
	addy		.req r12		;@ keep this at r12 (scratch for APCS)

;@----------------------------------------------------------------------------
							;@ ARM flags
	.equ PSR_N, 0x80000000		;@ Negative (Sign)
	.equ PSR_Z, 0x40000000		;@ Zero
	.equ PSR_C, 0x20000000		;@ Carry
	.equ PSR_V, 0x10000000		;@ Overflow


							;@ HuC6280 flags
	.equ N, 0x80				;@ Sign (negative)
	.equ V, 0x40				;@ Overflow
	.equ T, 0x20				;@ T opcode
	.equ B, 0x10				;@ Interrupt by BRK opcode
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
;@ IRQ flags
	.equ BRKIRQ_F, 0x01			;@ External IRQ (CD-ROM) flag
	.equ VDCIRQ_F, 0x02			;@ VDC IRQ Flag
	.equ TIMIRQ_F, 0x04			;@ Timer IRQ flag
	.equ NMI_F,    0x08			;@ NMI flag
	.equ RESET_F,  0x10			;@ Reset flag
;@----------------------------------------------------------------------------

	.struct -128				// Changes section so make sure it's set before real code.
h6280StateStart:
h6280Regs:
h6280RegNz:			.long 0
h6280RegA:			.long 0
h6280RegX:			.long 0
h6280RegY:			.long 0
h6280RegSP:			.long 0
h6280Cycles:		.long 0
h6280RegPC:			.long 0
h6280ZeroPage:		.long 0
h6280IrqPending:	.byte 0
h6280IrqMask:		.byte 0
h6280Padding0:		.skip 1
h6280NMIPin:		.byte 0
h6280MapperState:	.space 8
h6280TimerCycles:	.long 0
h6280TimerLatch:	.byte 0
h6280TimerEnable:	.byte 0
h6280IrqDisable:	.byte 0
h6280ClockSpeed:	.byte 0
h6280IoBuffer:		.byte 0
h6280Padding:		.skip 3

h6280LastBank:		.long 0
h6280OldCycles:		.long 0
h6280NextTimeout_:	.long 0
h6280NextTimeout:	.long 0
h6280StateEnd:
h6280ST1Func:		.long 0
h6280ST2Func:		.long 0
h6280End:
h6280RomMap:		.space 8*4
h6280Size  = h6280End-h6280Regs
h6280StateSize = h6280StateEnd-h6280StateStart
//h6280_opz:			.space 256*4

;@----------------------------------------------------------------------------
