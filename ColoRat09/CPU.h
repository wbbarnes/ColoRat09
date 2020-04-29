/******************************************************************************
*		   File:
*		Project:
*	   Solution:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author:
*		Created:
*	  Copyright:
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
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Notes:
*	Datasheets report opecodes,memory used, and cycles. This can be used for
* emulation by opcode functionality in one cycle padded by the cycle time
* required, OR, as here, instruction-data balanced with clock cycles, for
* real-time emulated CPU timing.
******************************************************************************/
#pragma once

#include <cstdint>
#include "MMU.h"


class CPU
{
public:
	virtual ~CPU() {};

	virtual void Clock() = 0;

	virtual uint8_t External_Reset() = 0;
	virtual uint8_t External_Irq() = 0;

	virtual uint8_t Fetch(const uint16_t address) = 0;
	virtual void SetMMU(MMU* device) = 0;

	virtual uint8_t Read(const uint16_t address, const bool readOnly = false) = 0;
	virtual void Write(const uint16_t address, const uint8_t byte) = 0;
};

