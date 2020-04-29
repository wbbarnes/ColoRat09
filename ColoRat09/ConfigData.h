/******************************************************************************
*		   File: ConfigData.h
*		Project: ColoRat09
*	   Solution: ColoRat
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author: William Barnes
*		Created: 2020/04/23
*	  Copyright: UNPUBLISHED (2020)
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
*	Configuration options for the ColoRat computer
* (exists as emulation currently)
******************************************************************************/
#pragma once

enum SYS_RAM
{
	ram_64K,	//  64KB RAM			Typical MAX for an 8-bit CPU w/o paging MMU
	ram_128K,	// 128KB RAM
	ram_512K,	// 512KB RAM
	ram_1M,		//   1MB RAM
	ram_2M,		//   2MB RAM
	ram_4M,		//   4MB RAM
	ram_8M,		//   8MB RAM
	ram_16M		//  16MB RAM
};


enum SYS_CLOCK
{
	clk_10K,	//  0.010MHz (10KHz)	TESTING SPEED
	clk_890K,	//  0.89MHz (890KHz)	CoCo 1, 2, & 3 normal speed
	clk_1M,		//  1MHz				68xx, 65xx, 63xx
	clk_1M5,	//  1.5MHz				68Axx 65Axx, 63Axx
	clk_1M78,	//  1.788MHz			CoCo 3 double speed
	clk_2M,		//	2MHz				63Bxx, 68Bxx, 65Bxx
	clk_3M,		//	3MHz				63Cxx
	clk_4M,		//  4MHz
	clk_4M77,	//  4.77MHz				Z80
	clk_6M,		//  6MHz				680xx
	clk_8M,		//  8MHz				680xx
	clk_10M,	// 10MHz				680xx
	clk_16M,	// 16MHz				680xx
};

enum SYS_CPU
{
	cpu_z80,	// Zilog	 Z80		8-bit
	cpu_6502,	// Mostek	 M6502		8-bit
	cpu_6510,	// Commodore MC6809		8-bit
	cpu_6800,	// Motorola  MC6800		8-bit
	cpu_6802,	// Motorola  MC6802		8-bit
	cpu_6803,	// Motorola  MC6803		8-bit
	cpu_6809,	// Motorola  MC6809		8-bit data,  8/16-bit internal
	cpu_6309,	// Hitachi	 MC6309		8-bit data,  8/16-bit internal
	cpu_68008,	// Motorola  MC68008	8-bit data, 16/32-bit internal
};
