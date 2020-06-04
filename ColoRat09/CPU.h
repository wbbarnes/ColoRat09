/******************************************************************************
*		   File:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author:
*		Created:
*	  Copyright:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Modifications: (Who, whenm, what)
*
******************************************************************************/
#pragma once

#include <cstdint>
#include "MMU.h"


class CPU
{
public:
	virtual ~CPU() {};

	virtual void Clock() = 0;

	virtual uint8_t HardwareRESET() = 0;
	virtual uint8_t IRQ() = 0;

	virtual uint8_t Fetch(const uint16_t address) = 0;
	virtual void SetMMU(MMU* device) = 0;

	virtual uint8_t Read(const uint16_t address, const bool readOnly = false) = 0;
	virtual void Write(const uint16_t address, const uint8_t byte) = 0;
};

