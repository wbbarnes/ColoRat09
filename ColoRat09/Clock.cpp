/******************************************************************************
*		   File: Clock
*		Project: ColoRat09
*	   Solution:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author: William Barnes
*		Created: 2020/05/05
*	  Copyright: 2020 - under Apache 2.0
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Modifications: (Who, whenm, what)
*
*******************************************************************************
* Solution Summary:
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Project Summary:
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* File Summary:
*
******************************************************************************/
#include "Clock.h"
#include <chrono>
#include <iostream>
#include <thread>


//*****************************************************************************
//	Clock()
//*****************************************************************************
//	Initializes the clocks needed for the emulator.
//
// NOTE: The pointers within this class for the CPU and the VDP are NOT OWNED
//		by this class. DO NOT delete them (free their memory) from here
//*****************************************************************************
Clock::Clock(SYS_CLOCK clockSpeed)
{
	primaryClock = clockSpeed;

	cpuClockDivider = 1;
	vdpClockDivider = 1;

	SetMainSpeed(clockSpeed);

	// Standard TV/NTSC timing
	vdpVSyncTime = 1/60;
	vdpHSyncTime = 1/15000;

	// pointer initiation
	cpu = nullptr;
	vdp = nullptr;
}


//*****************************************************************************
//	~Clock()
//*****************************************************************************
//	Cleans up after the clocks needed for the emulator (timers.)
//
// NOTE: The pointers within this class for the CPU and the VDP are NOT OWNED
//		by this class. DO NOT delete them (free their memory) from here
//*****************************************************************************
Clock::~Clock()
{
	cpu = nullptr;
	vdp = nullptr;
}


//*****************************************************************************
//	SetMainSpeed()
//*****************************************************************************
//	Sets the memory bus (aka, MMU since the MMU handles memory mapping.)
//*****************************************************************************
// Params:
//	SYS_CLOCK	- System Clock, the primary clock that all others are derrived
//					from.
//*****************************************************************************
void Clock::SetMainSpeed(SYS_CLOCK clockSpeed)
{
	primaryClock = clockSpeed;
	primaryCycleTime = SetSpeed(primaryClock, 1);

	cpuCycleTime = SetSpeed(primaryClock, cpuClockDivider);
	vdpCycleTime = SetSpeed(primaryClock, vdpClockDivider);
}


//*****************************************************************************
//	Add()
//*****************************************************************************
//	Sets the memory bus (aka, MMU since the MMU handles memory mapping.)
//*****************************************************************************
// Params:
//	CPU* 	- The CPU that we need to trigger clock access for
//*****************************************************************************
void Clock::Add(CPU* processorType, float divider)
{
	cpu = processorType;
	cpuClockDivider = divider;

	cpuCycleTime = SetSpeed(primaryClock, cpuClockDivider);
}


//*****************************************************************************
//	Add()
//*****************************************************************************
//	Sets the memory bus (aka, MMU since the MMU handles memory mapping.)
//*****************************************************************************
// Params:
//	VDP* 	- The Video Display Processor that we need to trigger clock access
//				for
//*****************************************************************************
void Clock::Add(VDP* vdpType, float divider)
{
	vdp = vdpType;
	vdpClockDivider = divider;

	vdpCycleTime = SetSpeed(primaryClock, vdpClockDivider);
}


//*****************************************************************************
//	SetSpeed()
//*****************************************************************************
//	Sets the memory bus (aka, MMU since the MMU handles memory mapping.)
//*****************************************************************************
// Params:
//	SYS_CLOCK	- the root system clock speed designator
//	float		- the divider for the system clock (default 1)
// Returns:
//	uint16_t	- The time in nanoseconds that a clock cycle should take
//*****************************************************************************
uint16_t Clock::SetSpeed(SYS_CLOCK clockSpeed, float divider)
{
	uint16_t cycleTime;
	switch (clockSpeed)
	{
	case SYS_CLOCK::clk_10K:	//   10uS	0.010MHz (10KHz)
		cycleTime = uint16_t(10000 / divider);
		break;
	case SYS_CLOCK::clk_890K:	// 1123nS	0.89MHz (890KHz)
		cycleTime = uint16_t( 1123 / divider);
		break;
	case SYS_CLOCK::clk_1M:		//    1uS	1MHz
		cycleTime = uint16_t( 1000 / divider);
		break;
	case SYS_CLOCK::clk_1M5:	//  666nS	1.5MHz
		cycleTime = uint16_t(  666 / divider);
		break;
	case SYS_CLOCK::clk_1M78:	//  529nS	1.788MHz
		cycleTime = uint16_t(  529 / divider);
		break;
	case SYS_CLOCK::clk_2M:		//  500nS	2MHz
		cycleTime = uint16_t(  500 / divider);
		break;
	case SYS_CLOCK::clk_3M:		//  333nS	3MHz
		cycleTime = uint16_t(  333 / divider);
		break;
	case SYS_CLOCK::clk_4M:		//  250nS	4MHz
		cycleTime = uint16_t(  250 / divider);
		break;
	case SYS_CLOCK::clk_4M77:	//  209nS	4.77MHz
		cycleTime = uint16_t(  209 / divider);
		break;
	case SYS_CLOCK::clk_6M:		//  166nS	6MHz
		cycleTime = uint16_t(  166 / divider);
		break;
	case SYS_CLOCK::clk_8M:		//  125nS	8MHz
		cycleTime = uint16_t(  125 / divider);
		break;
	case SYS_CLOCK::clk_10M:	//  100nS	10MHz
		cycleTime = uint16_t(  100 / divider);
		break;
	case SYS_CLOCK::clk_16M:	//   62nS	16MHz
		cycleTime = uint16_t(   62 / divider);
		break;
	default:
		cycleTime = uint16_t( 1000 / divider);
		break;
	}
	return(cycleTime);
}


//*****************************************************************************
//	Run()
//*****************************************************************************
//	Runs the clock(s) triggering functionality based on the clock.
//*****************************************************************************
// Params:
//	SYS_CLOCK	- the root system clock speed designator
//	float		- the divider for the system clock (default 1)
// Returns:
//	uint16_t	- The time in nanoseconds that a clock cycle should take
//*****************************************************************************
bool Clock::Run()
{
	uint16_t time[2];

	if (vdpCycleTime <= cpuCycleTime)
	{
		time[0] = vdpCycleTime;
		time[1] = cpuCycleTime;
	}
	else
	{
		time[0] = cpuCycleTime;
		time[1] = vdpCycleTime;
	}

	bool run = true;
	while (run)
	{
		std::chrono::steady_clock::time_point _start(std::chrono::steady_clock::now());

		run = Execute();

		std::chrono::steady_clock::time_point _end(std::chrono::steady_clock::now());
		auto timeDifference = _end - _start;
		auto timelapse = std::chrono::duration_cast<std::chrono::nanoseconds>(timeDifference);
		auto cycleTime = (std::chrono::nanoseconds)(primaryCycleTime);
		if (timelapse < cycleTime)
			std::this_thread::sleep_for((cycleTime - timelapse));
		else
			std::cout << "WARN: Cycle exceeded cycle by " << (timelapse - cycleTime).count() << " nanoseconds" << std::endl;
	}
	return(run);
}


bool Clock::Execute()
{
	return(true);
}
