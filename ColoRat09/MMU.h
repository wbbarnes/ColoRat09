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
******************************************************************************/
#pragma once

#include <cstdint>


class MMU
{
private:
protected:
public:

private:
protected:
public:
	virtual ~MMU() {};

	virtual uint8_t Read(uint16_t address, bool readOnly = false) = 0;
	virtual void Write(uint16_t address, uint8_t byte) = 0;

};

