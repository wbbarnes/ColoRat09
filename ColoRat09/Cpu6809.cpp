/******************************************************************************
*		   File:
*		Project:
*	   Solution:
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author: William Barnes
*		Created: 2020/05/02
*	  Copyright: 2020 - under Apache 2.0 Licensing
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
*	The software portion of the CPU emulation. This is being done on a per
* clock-cycle emulation model and not a do everything at once and wait for the
* rignt amount of cycles to pass model.
*
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* NOTES:
*	Datasheets report opecodes,memory used, and cycles. This can be used for
* emulation by exec functionality in one cycle padded by the cycle time
* required, OR, as here, instruction-data balanced with clock cycles, for
* real-time emulated CPU timing.
*
*   All unions that split a WORD (uint16_t) into BYTES (uint8_t) MUST observe
* the following: On the emulated 6809: in memory, a 16-bit word is stored with
* the MSB first... On the hardware the emulator runs on (Intel) this is
* reversed. Motorola chips must observe THIS order as it appears here,
******************************************************************************/

#include "Cpu6809.h"


//*****************************************************************************
//	Cpu6809()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset. Also sets the memory bus (aka MMU since the MMU handles memory
// mapping)
//*****************************************************************************
Cpu6809::Cpu6809(MMU* device)
{
	bus = device;
	Mode = AddressMode::INHERENT;

	haltTriggered = false;
	resetTriggered = true;
	nmiTriggered = false;
	irqTriggered = false;
	firqTriggered = false;

	// set all registers to a clear state.
	reg_CC = 0x00;			// Condition Code Register

	reg_DP = 0x00;			// Direct Page Registe

	reg_A = 0x00;			// (GP)	Accumulator A
	reg_B = 0x00;			// (GP)	Accumulator B
	reg_D = 0x0000;			// (GP)	Accumulator D

	X_hi = 0x00;			// (internal only)
	X_lo = 0x00;			// (internal only)
	reg_X = 0x0000;			// Index Register X

	Y_hi = 0x00;			// (internal only)
	Y_lo = 0x00;			// (internal only)
	reg_Y = 0x0000;			// Index Register Y

	U_hi = 0x00;			// (internal only)
	U_lo = 0x00;			// (internal only)
	reg_U = 0x0000;			// User Stack Pointer

	S_hi = 0x00;			// (internal only)
	S_lo = 0x00;			// (internal only)
	reg_S = 0x0000;			// System Stack Pointer

	PC_hi = 0x00;			// (internal only)
	PC_lo = 0x00;			// (internal only)
	reg_PC = 0x0000;		// Program Counter

	scratch_hi = 0x00;		// (internal only)
	scratch_lo = 0x00;		// (internal only)
	reg_scratch = 0x0000;	// (internal only)

	offset_hi = 0x00;
	offset_lo = 0x00;
	offset = 0x00;

	opcode_hi = 0x00;
	opcode_lo = 0x00;
	opcode = 0x0000;

	clocksUsed = 0;
	clocksLeft = 0;

	exec = nullptr;
}


//*****************************************************************************
//	~Cpu6809()
//*****************************************************************************
//	 Cleans up memory, if owned, on exit.
//*****************************************************************************
Cpu6809::~Cpu6809()
{}


//*****************************************************************************
//	SetMMU()
//*****************************************************************************
//	Sets the memory bus (aka, MMU since the MMU handles memory mapping.)
//*****************************************************************************
// Params:
//	MMU* device	- The memory management unit (or just the bus)
//*****************************************************************************
void Cpu6809::SetMMU(MMU* device)
{
	bus = device;
}


//*********************************************************************************************************************************


//*****************************************************************************
//	Read()
//*****************************************************************************
//	Reads a byte of data from the address bus.
// NOTES: this falls only within CPU address space. If it lies outsied, it still
//		has to be written within CPU address space but the MMU bus will handle
//		any translation to exactly where in memory it will be read from.
//*****************************************************************************
// Params:
//	uint16_t address	- the address in memory to read from.
//	bool readOnly		- the byte to write to memory
// Returns:
//	uint8_t byte		- the byte read from memory
//*****************************************************************************
uint8_t Cpu6809::Read(const uint16_t address, const bool readOnly)
{
	if( address >= 0x0000 && address <= 0xffff)
		return (bus->Read(address, readOnly));
	return(0x00);
}


//*****************************************************************************
//	Write()
//*****************************************************************************
//	Writes a byte of data to the address bus.
// NOTES: this falls only within CPU address space. If it lies outsied, it still
//		has to be written within CPU address space but the MMU bus will handle
//		any translation to exactly where in memory it will be written.
//*****************************************************************************
// Params:
//	uint16_t address	- the address in memory to write to.
//	uint8_t byte		- the byte to write to memory
//*****************************************************************************
void Cpu6809::Write(const uint16_t address, const uint8_t byte)
{
	if (address >= 0x0000 && address <= 0xffff)
		bus->Write(address, byte);
}


//*****************************************************************************
//	Clock()
//*****************************************************************************
//	Triggers the cycle the CPU is carrying out
//*****************************************************************************
void Cpu6809::Clock()
{
	if (exec != nullptr)
	{
		//(obj->*fp)(m, n)
		if (uint8_t result = (this->*exec)() == 255)
		{
			exec = nullptr;
			clocksUsed = 0;
			clocksLeft = 0;
		}
		else if (haltTriggered)
		{
			exec = &Cpu6809::HALT;
		}
		else if (resetTriggered)
		{
			exec = &Cpu6809::RESET;
		}
		else if (nmiTriggered)
		{
			exec = &Cpu6809::NMI;
		}
		else if (firqTriggered)
		{
			exec = &Cpu6809::FIRQ;
		}
		else if (irqTriggered)
		{
			exec = &Cpu6809::IRQ;
		}
		return;
	}
	Fetch(reg_PC++);
	return;
}


//*****************************************************************************
//	Cpu6809()
//*****************************************************************************
//	Retrieve opcodes, and determine addressing and instruction to execute
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//*****************************************************************************
uint8_t Cpu6809::Fetch(const uint16_t address)
{
	opcode_lo = Fetch(reg_PC);
	if (opcode_hi == 0x00 && (opcode_lo == 0x10 || opcode_lo == 0x11))
		opcode_hi = opcode_lo;

	// process opcode and set it to execute it.
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Hardware initiated: Halt, Reset, and interrupts
//*********************************************************************************************************************************

//*****************************************************************************
//	HALT()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//*****************************************************************************
uint8_t Cpu6809::HALT()
{

	clocksUsed = haltTriggered ? 1 : 255;
	return(clocksUsed);
}


//*****************************************************************************
//	RESET()
//*****************************************************************************
//	Hardware reset.
// NOTE: exact cycle length of this function depends on, In Real Life, how long
//		the reset condition is held "low" (active)
//*****************************************************************************
// MODIFIES:
//		  Registers: CC
//	Condition Codes: All may be afffected
//*****************************************************************************
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::RESET()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Don't care			$fffe
		if (resetTriggered)
			--clocksUsed;
		break;
	case 2:		// R	Don't care			$fffe
		reg_CC = (CC::I | CC::F);
		break;
	case 3:		// R	Don't care			$fffe
		break;
	case 4:		// R	Don't care			$fffe
		break;
	case 5:		// R	Int Vector High		$fffe
		PC_hi = Read(0xfffe);
		break;
	case 6:		// R	Int Vector Low		$ffff
		PC_lo = Read(0xffff);
		break;
	case 7:		// R	Don't care			$ffff
		reg_CC = 0x00;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}

//*****************************************************************************
//	Cpu6809()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::NMI()
{
	switch (++clocksUsed)
	{
	case 1:		// R	?					PC
		break;
	case 2:		// R	?					PC
		break;
	case 3:		// R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(--reg_S, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(--reg_S, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(--reg_S, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(--reg_S, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(--reg_S, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(--reg_S, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(--reg_S, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(--reg_S, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(--reg_S, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;
	case 16:	// R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	// R	Int Vector High		$fffc
		PC_hi = Read(0xfffc);
		break;
	case 18:	// R	Int Vector Low		$fffd
		PC_lo = Read(0xfffd);
		break;
	case 19:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	FIRQ()
//*****************************************************************************
//	Fast Interrupt Request	- Only stores reg_PC and reg_CC on the sytem stack
//*****************************************************************************
// MODIFIES:
//		  Registers: S
//	Condition Codes: F, I
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::FIRQ()
{
	switch (++clocksUsed)
	{
	case 1:		// R	?					PC
		break;
	case 2:		// R	?					PC
		break;
	case 3:		// R	Don't care			$ffff
		reg_CC &= ~CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 6:		// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;
	case 7:		// R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 8:		// R	Int Vector High		$fff6
		PC_hi = Read(0xfff6);
		break;
	case 9:		// R	Int Vector Low		$fff7
		PC_lo = Read(0xfff7);
		break;
	case 10:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	IRQ()
//*****************************************************************************
//	Interrupt Request
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::IRQ()
{
	switch (++clocksUsed)
	{
	case 1:		// R	?					PC
		break;
	case 2:		// R	?					PC
		break;
	case 3:		// R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(--reg_S, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(--reg_S, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(--reg_S, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(--reg_S, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(--reg_S, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(--reg_S, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(--reg_S, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(--reg_S, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(--reg_S, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;
	case 16:	// R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	// R	Int Vector High		$fff8
		PC_hi = Read(0xfff8);
		break;
	case 18:	// R	Int Vector Low		$fff9
		PC_lo = Read(0xfff9);
		break;
	case 19:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Software initiated: software (exec) interrupts and other interrupt related Opcodes
//*********************************************************************************************************************************

//*****************************************************************************
//	SoftRESET()
//*****************************************************************************
//	Software Reset.		- undocumented exec: 0x3E
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::SoftRESET()
{
	if (clocksUsed == 0)
		clocksUsed = 1;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Don't care			PC+1	++PC
		++reg_PC;
		break;
	case 3:		// R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(--reg_S, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(--reg_S, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(--reg_S, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(--reg_S, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(--reg_S, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(--reg_S, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(--reg_S, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(--reg_S, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(--reg_S, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;
	case 16:	// R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	// R	Int Vector High		$fffe
		PC_hi = Read(0xfffe);
		break;
	case 18:	// R	Int Vector Low		$ffff
		PC_lo = Read(0xffff);
		break;
	case 19:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}



//*****************************************************************************
//	SWI()
//*****************************************************************************
//	Software Interrupt
//*****************************************************************************
// MODIFIES:
//		  Registers: S
//	Condition Codes: E, F, I
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::SWI()
{
	if (clocksUsed == 0)
		clocksUsed = 1;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Don't care			PC+1	++PC
		++reg_PC;
		break;
	case 3:		// R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(--reg_S, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(--reg_S, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(--reg_S, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(--reg_S, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(--reg_S, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(--reg_S, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(--reg_S, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(--reg_S, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(--reg_S, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;
	case 16:	// R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	// R	Int Vector High		$fffa
		PC_hi = Read(0xfffa);
		break;
	case 18:	// R	Int Vector Low		$fffb
		PC_lo = Read(0xfffb);
		break;
	case 19:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SWI2()
//*****************************************************************************
//	Software Interrupt 2
//*****************************************************************************
// MODIFIES:
//		  Registers: S
//	Condition Codes: E, F, I
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::SWI2()
{
	if (clocksUsed == 0)
		clocksUsed = 2;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Opcode 2nd byte		PC+1	++PC
		++reg_PC;
		break;
	case 3:		// R	Don't care			PC+2	++PC
		++reg_PC;
		break;
	case 4:		// R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 5:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 6:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 7:		// W	User Stack Low		SP-3	--SP
		Write(--reg_S, U_lo);
		break;
	case 8:		// W	User Stack High		SP-4	--SP
		Write(--reg_S, U_hi);
		break;
	case 9:		// W	Y  Register Low		SP-5	--SP
		Write(--reg_S, Y_lo);
		break;
	case 10:	// W	Y  Register High	SP-6	--SP
		Write(--reg_S, Y_hi);
		break;
	case 11:	// W	X  Register Low		SP-7	--SP
		Write(--reg_S, X_lo);
		break;
	case 12:	// W	X  Register High	SP-8	--SP
		Write(--reg_S, X_hi);
		break;
	case 13:	// W	DP Register			SP-9	--SP
		Write(--reg_S, reg_DP);
		break;
	case 14:	// W	B  Register			SP-10	--SP
		Write(--reg_S, reg_B);
		break;
	case 15:	// W	A  Register			SP-11	--SP
		Write(--reg_S, reg_A);
		break;
	case 16:	// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;
	case 17:	// R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 18:	// R	Int Vector High		$fff4
		PC_hi = Read(0xfff4);
		break;
	case 19:	// R	Int Vector Low		$fff5
		PC_lo = Read(0xfff5);
		break;
	case 20:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SWI3()
//*****************************************************************************
//	Software Interrupt 3
//*****************************************************************************
// MODIFIES:
//		  Registers: S
//	Condition Codes: E
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::SWI3()
{
	if (clocksUsed == 0)
		clocksUsed = 2;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Opcode 2nd byte		PC+1	++PC
		++reg_PC;
		break;
	case 3:		// R	Don't care			PC+2	++PC
		++reg_PC;
		break;
	case 4:		// R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 5:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 6:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 7:		// W	User Stack Low		SP-3	--SP
		Write(--reg_S, U_lo);
		break;
	case 8:		// W	User Stack High		SP-4	--SP
		Write(--reg_S, U_hi);
		break;
	case 9:		// W	Y  Register Low		SP-5	--SP
		Write(--reg_S, Y_lo);
		break;
	case 10:	// W	Y  Register High	SP-6	--SP
		Write(--reg_S, Y_hi);
		break;
	case 11:	// W	X  Register Low		SP-7	--SP
		Write(--reg_S, X_lo);
		break;
	case 12:	// W	X  Register High	SP-8	--SP
		Write(--reg_S, X_hi);
		break;
	case 13:	// W	DP Register			SP-9	--SP
		Write(--reg_S, reg_DP);
		break;
	case 14:	// W	B  Register			SP-10	--SP
		Write(--reg_S, reg_B);
		break;
	case 15:	// W	A  Register			SP-11	--SP
		Write(--reg_S, reg_A);
		break;
	case 16:	// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;
	case 17:	// R	Don't Care			$ffff
		break;
	case 18:	// R	Int Vector High		$fff2
		PC_hi = Read(0xfff2);
		break;
	case 19:	// R	Int Vector Low		$fff3
		PC_lo = Read(0xfff3);
		break;
	case 20:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RTI()
//*****************************************************************************
//	Return from Interrupt.	- pull registers and the program counter off the
// system stack. If after we pull the CC register off the stack (the first
// pull) the E flag is set, we need to pull ALL the registers pushed on the
// stack. If the E flag is clear, we just pull the PC from the stack and then
// we've returned to where we left off when the interrupt was made.
//*****************************************************************************
// MODIFIES:
//		  Registers: CC and PC.
//					 If CC's E is set, then A, B, DP, X, Y, and U as well.
//	Condition Codes: All
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::RTI()
{
	if (clocksUsed == 0)
		clocksUsed = 2;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Don't Care			PC+1
		break;
	case 3:		// R	CCR					SP
		reg_CC = Read(reg_S++);
		break;
	}

	// if the E flag is clear (E = 0) we do the short version
	if ((reg_CC & CC::E) != CC::E)
	{
		switch (clocksUsed)
		{
		case 4:	// R	PC High				SP+1
			PC_hi = Read(reg_S++);
			break;
		case 5:	// R	PC low				SP+2
			PC_lo = Read(reg_S++);
			break;
		case 6:	// R	Don't Care			$ffff
			clocksUsed = 255;
			break;
		}
		// Don't touch the long version below.
		return (clocksUsed);
	}

	// if the E flag is set (E = 1) we do the long version
	switch (clocksUsed)
	{
	case 4:		// R	A Register			SP+1
		reg_A = Read(reg_S++);
		break;
	case 5:		// R	B Register			SP+2
		reg_B = Read(reg_S++);
		break;
	case 6:		// R	DP Register			SP+3
		reg_DP = Read(reg_S++);
		break;
	case 7:		// R	X Register High		SP+4
		X_hi = Read(reg_S++);
		break;
	case 8:		// R	X Register Low		SP+5
		X_lo = Read(reg_S++);
		break;
	case 9:		// R	Y Register High		SP+6
		Y_hi = Read(reg_S++);
		break;
	case 10:	// R	Y Register Low		SP+7
		Y_lo = Read(reg_S++);
		break;
	case 11:	// R	User Stack High		SP+8
		U_hi = Read(reg_S++);
		break;
	case 12:	// R	User Stack Low		SP+9
		U_lo = Read(reg_S++);
		break;
	case 13:	// R	PC High				SP+10
		PC_hi = Read(reg_S++);
		break;
	case 14:	// R	PC Low				SP+11
		PC_lo = Read(reg_S++);
		break;
	case 15:	// R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	()
//*****************************************************************************
//	Stop processing Instructions and wait for interrupt.
// IF interrupt is masked OR interrupt signal lasts < 3 cycle, continue from
// next instruction.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes: May be affected by unmasked interrupt.
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::SYNC()
{
	// TODO
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Don't Care			PC+1
		++reg_PC;
		break;
	case 3:		// R	Don't Care			Z
		break;
	case 4:		// R	Don't Care			Z
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::CWAI()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
	case 2:		// R	CC Mask				PC+1
	case 3:		// R	Don't Care			PC+2
		++reg_PC;
	case 4:		// R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 5:		// W	PC Low				SP-1	--SP
		Write(--reg_S, PC_lo);
		break;
	case 6:		// W	PC High				SP-2	--SP
		Write(--reg_S, PC_hi);
		break;
	case 7:		// W	User Stack Low		SP-3	--SP
		Write(--reg_S, U_lo);
		break;
	case 8:		// W	User Stack High		SP-4	--SP
		Write(--reg_S, U_hi);
		break;
	case 9:		// W	Y  Register Low		SP-5	--SP
		Write(--reg_S, Y_lo);
		break;
	case 10:	// W	Y  Register High	SP-6	--SP
		Write(--reg_S, Y_hi);
		break;
	case 11:	// W	X  Register Low		SP-7	--SP
		Write(--reg_S, X_lo);
		break;
	case 12:	// W	X  Register High	SP-8	--SP
		Write(--reg_S, X_hi);
		break;
	case 13:	// W	DP Register			SP-9	--SP
		Write(--reg_S, reg_DP);
		break;
	case 14:	// W	B  Register			SP-10	--SP
		Write(--reg_S, reg_B);
		break;
	case 15:	// W	A  Register			SP-11	--SP
		Write(--reg_S, reg_A);
		break;
	case 16:	// W	CC Register			SP-12	--SP
		Write(--reg_S, reg_CC);
		break;

		// TODO

	case 17:	// R	Don't Care			$ffff
		// wait for interrupt signal
		break;

	case 18:	// R	Int Vector High		$fffx
		// act on signal by grabbing the interrupt's vector

		PC_hi = Read(0xfff4);
		break;
	case 19:	// R	Int Vector Low		$fffx
		PC_lo = Read(0xfff5);
		break;
	case 20:	// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Subroutine branching and return
//*********************************************************************************************************************************

//*****************************************************************************
//	BSR()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BSR()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		break;
	case 4:		// R	Don't Care			Effective Address
		reg_scratch += reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		break;
	case 6:		// W	Retern Address Low	SP-1
		 Write(--reg_S, PC_lo);
		 break;
	case 7:		// W	Return Address High	SP-2
		Write(--reg_S, PC_hi);
		reg_PC = reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBSR()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBSR()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Offset High			PC+1
		scratch_hi = Read(++reg_PC);
		break;
	case 3:		// R	Offset Low			PC+1
		scratch_lo = Read(++reg_PC);
		break;
	case 4:		// R	Don't Care			$ffff
		break;
	case 5:		// R	Don't Care			$ffff
		break;
	case 6:		// R	Don't Care			Effective Address
		reg_scratch += reg_PC;
		break;
	case 7:		// R	Don't Care			$ffff
		break;
	case 8:		// W	Retern Address Low	SP-1
		Write(--reg_S, PC_lo);
		break;
	case 9:		// W	Return Address High	SP-2
		Write(--reg_S, PC_hi);
		reg_PC = reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::JSR()
{
	++clocksUsed;
	if (clocksUsed == 1)
		++reg_PC;
	if (Mode == AddressMode::DIRECT)
	{
		switch (clocksUsed)
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			break;
		}
	}
	if (Mode == AddressMode::EXTENDED)
	{
		switch (clocksUsed)
		{
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			break;
		}
	}
	if (Mode == AddressMode::INDEXED)
	{
		int a = 7;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RTS()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::RTS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Don't Care			PC+1
		++reg_PC;
		break;
	case 3:		// R	PC High				SP
		PC_lo = Read(reg_S++);
		break;
	case 4:		// R	PC Low				SP+1
		PC_hi = Read(reg_S++);
		break;
	case 5:		// R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Subroutine branching and return
//*********************************************************************************************************************************

//*****************************************************************************
//	BRA()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BRA()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBRA()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBRA()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Offset High			PC+1
		scratch_hi = Read(++reg_PC);
		break;
	case 3:		// R	Offset Low			PC+1
		scratch_lo = Read(++reg_PC);
		break;
	case 4:		// R	Don't Care			$ffff
		reg_PC += reg_scratch;
		break;
	case 5:		// R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::JMP()
{
	++clocksUsed;
	if (clocksUsed == 1)
		++reg_PC;
	if (Mode == AddressMode::DIRECT)
	{
		switch (clocksUsed)
		{
		case 1:
		case 2:
		case 3:
			break;
		}
	}
	if (Mode == AddressMode::EXTENDED)
	{
		switch (clocksUsed)
		{
		case 1:
		case 2:
		case 3:
		case 4:
			break;
		}
	}
	if (Mode == AddressMode::INDEXED)
	{
		switch (clocksUsed)
		{
		case 1:
		case 2:

		case 3:
		case 4:
		case 5:
		case 6:
			break;
		}
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Branch never and NOP
//*********************************************************************************************************************************

//*****************************************************************************
//	BRN()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BRN()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		break;
	case 3:		// R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBRN()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBRN()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+1
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+1
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		break;
	case 6:		// R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NOP()
//*****************************************************************************
//	Do Nothing, No Operations performed.
// Uses Memory space and Clock cycles only. 
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::NOP()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch			PC
		++reg_PC;
		break;
	case 2:		// R	Don't Care				PC+1;
		clocksUsed = 255;
		++reg_PC;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Conditional Branch opcodes
//*********************************************************************************************************************************

//*****************************************************************************
//	()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t BCC();          // Branch on Carry Clear        (C clear)
uint8_t LBCC();         // Long Branch on Carry Clear
uint8_t BCS();          // Branch on Carry Set          (C set)
uint8_t LBCS();         // Long Branch on Carry Set
uint8_t BEQ();          // Branch if Equal              (Z set)
uint8_t LBEQ();         // Long Branch if Equal
uint8_t BGE();          // Branch if Greater or Equal (signed)
uint8_t LBGE();         // Long Branch  if Greater or Equal
uint8_t BGT();          // Branch if Greater than (signed)
uint8_t LBGT();         // Long Branch if Greater than
uint8_t BHI();          // Branch if Higher (unsigned)
uint8_t LBHI();         // Long Branch if Higher
uint8_t BHS();          // Branch if higher or same (unsigned)
uint8_t LBHS();         // Long Branch if higher or same
uint8_t BLE();          // Branch if Less than or Equal (signed)
uint8_t LBLE();         // Long Branch if Less than or Equal (signed)
uint8_t BLO();          // Branch if Lower (unsigned)
uint8_t LBLO();         // Long Branch if Lower (unsigned)
uint8_t BLS();          // Branch if Lower or Same (unsigned)
uint8_t LBLS();         // Long Branch if Lower or Same (unsigned)
uint8_t BLT();          // Branch if less than (signed)
uint8_t LBLT();         // Long Branch if less than (signed)
uint8_t BMI();          // Branch on Minus 
uint8_t LBMI();         // Long Branch on Minus
uint8_t BNE();          // Branch if Not Equal (Z = 0)
uint8_t LBNE();         // Long Branch if Not Equal (Z = 0)
uint8_t BPL();          // Branch if Plus/Positive
uint8_t LBPL();         // Long Branch if Plus/Positive
uint8_t BVC();          // Branch if no overflow (V is clear)
uint8_t LBVC();         // Long Branch if no overflow (V is clear)
uint8_t BVS();          // Branch on overflow (V is set)
uint8_t LBVS();         // Long Branch on overflow (V is set)


//*********************************************************************************************************************************
// Load Effective Address Mode opcodes
//*********************************************************************************************************************************

//*****************************************************************************
//	()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t LEAS();         // Load Effective Address S (increment or decrement S by a given value. Use  an index register to load another)
uint8_t LEAU();         // Load Effective Address U (increment or decrement U by a given value. Use  an index register to load another)
uint8_t LEAX();         // Load Effective Address X (increment or decrement X by a given value. Use  an index register to load another)
uint8_t LEAY();         // Load Effective Address Y (increment or decrement Y by a given value. Use  an index register to load another)


//*********************************************************************************************************************************
// Inherent Address Mode opcodes
//*********************************************************************************************************************************

//*****************************************************************************
//	()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t ABX();          // Add B to X
uint8_t ASLA();         // Arithmetic Shift Left A (Logical Shift Left fill LSb with 0)
uint8_t ASLB();         // Arithmetic Shift Left B (Logical Shift Left fill LSb with 0)
uint8_t ASRA();         // Arithmetic Shift Right A (fill MSb with Sign bit)
uint8_t ASRB();         // Arithmetic Shift Right B (fill MSb with Sign bit)
uint8_t BITA();         // Bit Test on A with a specific value (by AND)
uint8_t BITB();         // Bit Test on B with a specific value (by AND)
uint8_t CLRA();         // Clear register A
uint8_t CLRB();         // Clear register B
uint8_t COMA();         // 1's Compliment A (i.e. XOR A with 0x00 or 0xFF)
uint8_t COMB();         // 1's Compliment B (i.e. XOR B with 0x00 or 0xFF) 
uint8_t CWAI();         // Wait for Interrupt
uint8_t DAA();          // Decimal Adjust A (contents of A -> BCD... BCD operation should be prior)
uint8_t DECA();         // Decrement A (A -= A      A = A - 1   --A     A--)
uint8_t DECB();         // Decrement B (B -= B      B = B - 1   --B     B--) 
uint8_t EXG();          // Exchange any two registers of the same size
uint8_t INCA();         // Increment A (A += A      A = A + 1   ++A     A++)
uint8_t INCB();         // Increment B (B += B      B = B + 1   ++B     B++)
uint8_t LSLA();         // Logical Shift Left register A (LSb is loaded with 0)
uint8_t LSLB();         // Logical Shift Left register B (LSb is loaded with 0)
uint8_t LSRA();         // Logical Shift Right register A (MSb is loaded with 0)
uint8_t LSRB();         // Logical Shift Right register B (MSb is loaded with 0)
uint8_t MUL();          // Multiply register A * register B, store in register D (A << 8 | B)
uint8_t NEGA();         // 2's Complement (negate) register A
uint8_t NEGB();         // 2's Complement (negate) register B
uint8_t ROLA();         // Rotate Register A Left one bit through the Carry flag (9 bit rotate)
uint8_t ROLB();         // Rotate Register B Left one bit through the Carry flag (9 bit rotate)
uint8_t RORA();         // Rotate Register A Right one bit through the Carry flag (9 bit rotate)
uint8_t RORB();         // Rotate Register B Right one bit through the Carry flag (9 bit rotate)
uint8_t SEX();          // Sign Extend B into A ( A = Sign bit set on B ? 0xFF : 0x00)
uint8_t SYNC();         // Synchronize to interrupt
uint8_t TFR();          // Transfer/Copy one register to another (of the same size)
uint8_t TSTA();         // Test Register A, adjust N and Z Condition codes based on content
uint8_t TSTB();         // Test Register B, adjust N and Z Condition codes based on content


//*********************************************************************************************************************************
// Multi address mode opcodes
//*********************************************************************************************************************************

//*****************************************************************************
//	()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t ADCA();         // Add to A + Carry
uint8_t ADCB();         // Add to A + Carry
uint8_t ADDA();         // Add to A
uint8_t ADDB();         // Add to B
uint8_t ADDD();         // Add to D (A << 8 | B)
uint8_t ANDA();         // And A
uint8_t ANDB();         // And B
uint8_t ANDCC();        // And Condition Codes (clear one or more flags)
uint8_t ASL();          // Arithmetic Shift Left Memory location (Logical Shift Left fill LSb with 0)
uint8_t ASR();          // Arithmetic Shift Right Memory location (fill MSb with Sign bit)
uint8_t CLR();          // Clear memory location
uint8_t CMPA();         // Compare register A to memory location or given value(CC H unaffected)
uint8_t CMPB();         // Compare register B to memory location or given value (CC H unaffected)
uint8_t CMPD();         // Compare register D ( A <<8 | B) to memory locations or given value (CC H unaffected)
uint8_t CMPS();         // Compare register S to memory locations or given value (CC H unaffected)
uint8_t CMPU();         // Compare register U to memory locations or given value (CC H unaffected)
uint8_t CMPX();         // Compare register X to memory locations or given value (CC H unaffected)
uint8_t CMPY();         // Compare register Y to memory locations or given value (CC H unaffected)
uint8_t COM();          // 1's Compliment Memory location (i.e. XOR A with 0x00 or 0xFF)
uint8_t DEC();          // Decrement Memory location
uint8_t EORA();         // Logical Exclusive OR register A
uint8_t EORB();         // Logical Exclusive OR register B
uint8_t INC();          // Increment Memory location
uint8_t LDA();          // Load Register A
uint8_t LDB();          // Load Register A
uint8_t LDD();          // Load Register D  ( A << 8 | B)
uint8_t LDS();          // Load Register S
uint8_t LDU();          // Load Register U
uint8_t LDX();          // Load Register X
uint8_t LDY();          // Load Register Y
uint8_t LSL();          // Logical Shift Left memory location (LSb is loaded with 0)
uint8_t LSR();          // Logical Shift Right memory location (MSb is loaded with 0)
uint8_t NEG();          // 2's Complement (negate) memory location
uint8_t ORA();          // Logical OR register A
uint8_t ORB();          // Logical OR register B
uint8_t ORCC();         // Logical OR Condition Codes (set one or more flags)
uint8_t PSHS();         // Push one or more registers onto the System Stack
uint8_t PSHU();         // Push one or more registers onto the User Stack
uint8_t PULS();         // Pull one or more registers from the System Stack
uint8_t PULU();         // Pull one or more registers from the User Stack
uint8_t ROL();          // Rotate memory location Left one bit through the Carry flag (9 bit rotate)
uint8_t ROR();          // Rotate memory location Right one bit through the Carry flag (9 bit rotate)
uint8_t SBCA();         // Subtract with carry (borrow) - register A
uint8_t SBCB();         // Subtract with carry (borrow) - register B
uint8_t STA();          // Store Register A
uint8_t STB();          // Store Register B 
uint8_t STD();          // Store Register D     (A << 8 | B)
uint8_t STS();          // Store Register S
uint8_t STU();          // Store Register U
uint8_t STX();          // Store Register x
uint8_t STY();          // Store Register Y
uint8_t SUBA();         // Subtract from register A
uint8_t SUBB();         // Subtract from register A
uint8_t SUBD();         // Subtract from register D     (A << 8 | B)
uint8_t TST();          // Test memory location, adjust N and Z Condition codes based on content


//*********************************************************************************************************************************
// Invented Mnemonic for invalid Opcode
//*********************************************************************************************************************************

//*****************************************************************************
//	()
//*****************************************************************************
//	INVALID INSTRUCTION!
// on the 6309, this would trigger the invalid operation exception vector
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BadOp()
{
	return(clocksUsed);
}
