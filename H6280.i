//
//  H6280.i
//  ARMH6280
//
//  Created by Fredrik Ahlström on 2003-01-01.
//  Copyright © 2003-2021 Fredrik Ahlström. All rights reserved.
//

							;@ r0,r1,r2=temp regs
	h6280nz		.req r3			;@ bit 31=N, Z=1 if bits 0-7=0
	h6280a		.req r4			;@ bits  0-23=0, also used to clear bytes in memory (vdc.s)
	h6280x		.req r5			;@ bits  0-23=0
	h6280y		.req r6			;@ bits  0-23=0
	h6280sp		.req r7			;@ bits 24-31=SP, bit 0=1.
	cycles		.req r8
	h6280pc		.req r9
	h6280optbl	.req r10
	h6280zpage	.req r11		;@ PCE_RAM
	addy		.req r12		;@ keep this at r12 (scratch for APCS)

	.struct -128				// Changes section so make sure it's set before real code.
h6280Regs:
h6280RegNz:			.long 0
h6280RegA:			.long 0
h6280RegX:			.long 0
h6280RegY:			.long 0
h6280RegSP:			.long 0
h6280RegCy:			.long 0
h6280RegPC:			.long 0
h6280RegZP:			.long 0
h6280MapperState:	.space 8
h6280TimerCycles:	.long 0
h6280IrqPending:	.long 0
h6280IrqDisable:	.byte 0
h6280IoBuffer:		.byte 0
h6280TimerLatch:	.byte 0
h6280TimerEnable:	.byte 0
h6280ClockSpeed:	.byte 0
h6280NMIPin:		.byte 0
h6280Padding:		.space 2

h6280LastBank:		.space 4
h6280OldCycles:		.space 4
h6280NextTimeout_:	.space 4
h6280NextTimeout:	.space 4
h6280End:
h6280RomMap:		.space 32
#define h6280Size (h6280End-h6280Regs)
//h6280_opz:			.space 256*4

;@----------------------------------------------------------------------------
