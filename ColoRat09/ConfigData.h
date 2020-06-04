/******************************************************************************
*		   File: ConfigData.h
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author: William Barnes
*		Created: 2020/04/23
*	  Copyright: 2020 - under Apache 2.0
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Modifications: (Who, whenm, what)
*
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
	clk_10K,	//   10uS	0.010MHz (10KHz)	TESTING SPEED
	clk_890K,	// 1123nS	0.89MHz (890KHz)	CoCo 1, 2, & 3 normal speed
	clk_1M,		//    1uS	1MHz				68xx, 65xx, 63xx
	clk_1M5,	//  666nS	1.5MHz				68Axx 65Axx, 63Axx
	clk_1M78,	//  529nS	1.788MHz			CoCo 3 double speed
	clk_2M,		//  500nS	2MHz				63Bxx, 68Bxx, 65Bxx
	clk_3M,		//  333nS	3MHz				63Cxx
	clk_4M,		//  250nS	4MHz
	clk_4M77,	//  209nS	4.77MHz				Z80
	clk_6M,		//  166nS	6MHz				680xx
	clk_8M,		//  125nS	8MHz				680xx
	clk_10M,	//  100nS	10MHz				680xx
	clk_16M,	//   62nS	16MHz				680xx
};

enum SYS_CPU
{
	cpu_z80,	// Zilog	 Z80		8-bit
	cpu_6502,	// Mostek	 M6502		8-bit
	cpu_6510,	// Commodore MC6809		8-bit
	cpu_6800,	// Motorola  MC6800		8-bit
	cpu_6801,	// Motorola  MC6803		8-bit
	cpu_6802,	// Motorola  MC6802		8-bit
	cpu_6803,	// Motorola  MC6803		8-bit
	cpu_6805,	// Motorola  MC6803		8-bit
	cpu_6808,	// Motorola  MC6803		8-bit
	cpu_6809,	// Motorola  MC6809		8-bit data,  8/16-bit internal
	cpu_6309,	// Hitachi	 MC6309		8-bit data,  8/16-bit internal
	cpu_68008,	// Motorola  MC68008	8-bit data, 16/32-bit internal
};
