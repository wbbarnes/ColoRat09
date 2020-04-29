#pragma once
#include <cstdint>

struct CPU_Vectors
{
	int16_t fff0;	// 6809 - reserved;		6309 native - Illegal Opcode and Division by Zero Trap (exception)
	int16_t fff2;	// Software Interrupt 3 vector
	int16_t fff4;	// Software Interrupt 2 vector
	int16_t fff6;	// Fast Interrupt Request vector
	int16_t fff8;	// Interrupt Request vector
	int16_t fffa;	// Software Interrupt [1] vector
	int16_t fffc;	// Non-Maskable Interrupt vector
	int16_t fffe;	// Reset vector
};
