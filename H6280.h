//
//  H6280.h
//  ARMH6280
//
//  Created by Fredrik Ahlström on 2003-01-01.
//  Copyright © 2003-2026 Fredrik Ahlström. All rights reserved.
//
#ifndef H6280_HEADER
#define H6280_HEADER

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
//	u32 regs[8]
	u32 regNz;
	u32 regA;
	u32 regX;
	u32 regY;
	u32 regSP;
	u32 regCy;
	u8 *regPC;
	u8 *regZP;
	u8 mapperState[8];
	u32 timerCycles;
	u32 irqPending;
	u8 irqDisable;
	u8 ioBuffer;
	u8 timerLatch;
	u8 timerEnable;
	u8 clockSpeed;
	u8 nmiPin;
	u8 padding[2];

	u8 *lastBank;
	int oldCycles;
	void (*nextTimeout_)(void);
	void (*nextTimeout)(void);
	void (*st1Func)(u8 val);
	void (*st2Func)(u8 val);
	u32 romMap[8];
} H6280Core;

extern H6280Core m6280OpTable;

void h6280Reset(int type);

/**
 * Saves the state of the cpu to the destination.
 * @param  *destination: Where to save the state.
 * @param  *cpu: The H6280Core cpu to save.
 * @return The size of the state.
 */
int h6280SaveState(void *destination, const H6280Core *cpu);

/**
 * Loads the state of the cpu from the source.
 * @param  *cpu: The H6280Core cpu to load a state into.
 * @param  *source: Where to load the state from.
 * @return The size of the state.
 */
int h6280LoadState(H6280Core *cpu, const void *source);

/**
 * Gets the state size of an H6280Core cpu.
 * @return The size of the state.
 */
int h6280GetStateSize(void);

void h6280SetNMIPin(bool set);
void h6280SetIRQPin(bool set);
void h6280RestoreAndRunXCycles(int cycles);
void h6280RunXCycles(int cycles);
void h6280CheckIrqs(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // H6280_HEADER
