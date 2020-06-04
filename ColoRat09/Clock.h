/******************************************************************************
*		   File: Clock
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author: William Barnes
*		Created: 2020/05/05
*	  Copyright: 2020 - under Apache 2.0
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Modifications: (Who, whenm, what)
*
******************************************************************************/
#pragma once

#include <cstdint>

#include "VDP.h"
#include "CPU.h"
#include "ConfigData.h"

class Clock
{
private:
	SYS_CLOCK primaryClock;

	float cpuClockDivider;
	float vdpClockDivider;

	// time in nanoseconds for process to run.
	uint16_t primaryCycleTime;
	uint16_t cpuCycleTime;
	uint16_t vdpCycleTime;
	uint16_t vdpVSyncTime;
	uint16_t vdpHSyncTime;

	CPU* cpu;
	VDP* vdp;

protected:
public:

private:
	uint16_t SetSpeed(SYS_CLOCK clockSpeed, float divider = 1);
	bool Execute();
protected:
public:
	Clock(SYS_CLOCK clockSpeed = SYS_CLOCK::clk_890K);
	~Clock();

	void SetMainSpeed(SYS_CLOCK clockSpeed = SYS_CLOCK::clk_890K);

	void Add(CPU* processorType, float divider);
	void Add(VDP* vdpType, float divider);

	bool Run();
};

