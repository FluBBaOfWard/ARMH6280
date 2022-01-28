//
//  H6280.h
//  ARMH6280
//
//  Created by Fredrik Ahlström on 2003-01-01.
//  Copyright © 2003-2021 Fredrik Ahlström. All rights reserved.
//

#ifndef H6280_HEADER
#define H6280_HEADER

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
// m6280Regs[8]
	u32 h6280RegNz;
	u32 h6280RegA;
	u32 h6280RegX;
	u32 h6280RegY;
	u32 h6280RegSP;
	u32 h6280RegCy;
	u32 h6280RegPC;
	u32 h6280RegZP;
	u8 h6280MapperState[8];
	u32 h6280TimerCycles;
	u32 h6280IrqPending;
	u8 h6280IrqDisable;
	u8 h6280IoBuffer;
	u8 h6280TimerLatch;
	u8 h6280TimerEnable;
	u8 h6280ClockSpeed;
	u8 h6280NmiPin;
	u8 h6280Padding[2];

	void *h6280LastBank;
	int h6280OldCycles;
	void *h6280NextTimeout_;
	void *h6280NextTimeout;
	u32 h6280RomMap[8];
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
