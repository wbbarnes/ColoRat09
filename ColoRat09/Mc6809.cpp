#include "Mc6809.h"

Mc6809::Mc6809()
{
	reg_A = 0x00;
	reg_B = 0x00;
	reg_D = 0x0000;
	reg_X = 0x0000;
	reg_Y = 0x0000;
	reg_S = 0x0000;
	reg_U = 0x0000;
	reg_PC = 0xfffe;
	reg_DP = 0x00;
	reg_CC = 0x00;

	data_hi = 0;
	data_lo = 0;
	data = 0x0000;

	instruction_hi = 0xff;
	instruction_lo = 0xff;
	instruction = 0xffff;

	cyclesLeft = 0;
	mmu = nullptr;

	opcode = nullptr;

	haltTriggered = true;
	resetTriggered = true;
	nmiTriggered = false;
	firqTriggered = false;
	irqTriggered = false;
}

Mc6809::~Mc6809()
{}


//*********************************************************************************************************************************
// Hardware requests
//*********************************************************************************************************************************


//*****************************************************************************
//	Interrupts:		/Reset, /NMI, SWI, /FIRQ, /IRQ, SWI2, SWI3,
//					<reserved, not implemented>
// in the order of priority.
// Reset and NIM, FIRQ, and IRQ interrupts are hardware (pin inputs on 6x09.)
// SWI, SWI2, SWI3 are software interrupts
//*****************************************************************************


//*********************************************************************************************************************************
// "externally generated" reset, halt, and interrupts 
//*****************************************************************************

//*****************************************************************************
//	TriggerHalt()
//*****************************************************************************
// set trigger flag for halt
//
// Condition Codes Affected: None
//*****************************************************************************
void Mc6809::TriggerHalt()
{
	haltTriggered = true;
}


//*****************************************************************************
//	ReleaseHalt()
//*****************************************************************************
// reset trigger flag for halt
//
// Condition Codes Affected: None
//*****************************************************************************
void Mc6809::ReleaseHalt()
{
	haltTriggered = false;
}


//*****************************************************************************
//	TriggerReset()
//*****************************************************************************
// set trigger flag for reset
//
// Condition Codes Affected: None
//*****************************************************************************
void Mc6809::TriggerReset()
{
	resetTriggered = true;
}


//*****************************************************************************
//	TriggerNMI()
//
// Condition Codes Affected: None
//*****************************************************************************
// set trigger flag for NMI
//*****************************************************************************
void Mc6809::TriggerNMI()
{
	nmiTriggered = true;
}


//*****************************************************************************
//	TriggerIRQ()
//
// Condition Codes Affected: None
//*****************************************************************************
// set trigger flag for IRQ
//*****************************************************************************
void Mc6809::TriggerIRQ()
{
	irqTriggered = true;
}


//*****************************************************************************
//	TriggerFIRQ()
//*****************************************************************************
// set trigger flag for FIRQ
//
// Condition Codes Affected: None
//*****************************************************************************
void Mc6809::TriggerFIRQ()
{
	firqTriggered = true;
}


//*********************************************************************************************************************************
// Actual reset, halt, interrupt hardware routines
//*****************************************************************************

//*****************************************************************************
//	External_Reset()
//*****************************************************************************
//	Resets the CPU to a functional state and starts executing code at a given
// address via a jump vector table @ $FFFE (MSB) and $FFFF (LSB)
// Highest priority after /halt signal.
// Also an undocumented Opcode: $3E
//
// Address Mode: Inherent?
// Condition Codes Affected: 
//	
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::External_Reset()
{
	// COLD reset - cycle count is not really known.
	reg_CC = CC::I | CC::F;

	reg_PC_hi = Read(InterruptVector.RESET_hi);
	reg_PC_lo = Read(InterruptVector.RESET_lo);

	cyclesLeft = 0;

	reg_CC = 0x00;

	return(cyclesLeft);
}


//*****************************************************************************
//	External_Nmi()
//*****************************************************************************
//	Non-Maskable Interrupt Request. Stores registers and starts executing code
// at a given address via a jump vector table @ $FFFC (MSB) and $FFFD (LSB)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	E, I, F
//	
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::External_Nmi()
{
	switch (cyclesLeft)
	{
	case 19:	// R unknown				(PC)
		break;
	case 18:	// R unknown				(PC)
		break;
	case 17:	// R don't care				($ffff)
		reg_CC |= CC::E;
		break;
	case 16:	// W push PC low	onto S	(SP-1)	- Program Counter
		Write(--reg_S, reg_PC_lo);
		break;
	case 15:	// W push PC hi		onto S	(SP-2)
		Write(--reg_S, reg_PC_hi);
		break;
	case 14:	// W push U low		onto S	(SP-3)	- User Stack
		Write(--reg_S, reg_U_lo);
		break;
	case 13:	// W push U hi		onto S	(SP-4)
		Write(--reg_S, reg_U_hi);
		break;
	case 12:	// W push Y low		onto S	(SP-5)	- Index Register Y
		Write(--reg_S, reg_Y_lo);
		break;
	case 11:	// W push Y hi		onto S	(SP-6)
		Write(--reg_S, reg_Y_hi);
		break;
	case 10:	// W push X low		onto S	(SP-7)	- Index Register X
		Write(--reg_S, reg_X_lo);
		break;
	case 9:		// W push X hi		onto S	(SP-8)
		Write(--reg_S, reg_X_hi);
		break;
	case 8:		// W push DP		onto S	(SP-9)	- Direct Page register
		Write(--reg_S, reg_DP);
		break;
	case 7:		// W push B			onto S	(SP-10)	- Accumulator B
		Write(--reg_S, reg_B);
		break;
	case 6:		// W push A			onto S	(SP-11)	- Accumulator A
		Write(--reg_S, reg_A);
		break;
	case 5:		// W push CC		onto S	(SP-12)	- Condition Codes register
		Write(--reg_S, reg_CC);
		break;
	case 4:		// R don't care				($ffff)
		reg_CC |= (CC::I | CC::F);
		break;
	case 3:		// R NMI Vector hi			($FFFC)
		reg_PC_hi = Read(InterruptVector.NMI_hi);
		break;
	case 2:		// R NMI Vector lo			($FFFD)
		reg_PC_lo = Read(InterruptVector.NMI_lo);
		break;
	case 1:		// R don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	External_Firq()
//*****************************************************************************
//	Maskable Fast Interrupt Request. Stores registers  CC and PC and starts
// executing code at a given address via a jump vector  table @ $FFF6 (MSB) and
// $FFF7 (LSB)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	E, I, F
//	
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::External_Firq()
{
	switch (cyclesLeft)
	{
	case 10:	// R unknown				(PC)	- already done by the time we get here
		if ((reg_CC & CC::F) != 0)
			cyclesLeft = 1;
		break;
	case 9:		// R unknown				(PC+1)
		++reg_PC;
		break;
	case 8:		// R don't care				($ffff)
		break;
	case 7:		// W push PC low	onto S	(SP-1)	- Program Counter
		Write(--reg_S, reg_PC_lo);
		break;
	case 6:		// W push PC hi		onto S	(SP-2)
		Write(--reg_S, reg_PC_hi);
		break;
	case 5:		// W push CC		onto S	(SP-3)	- Condition Codes register
		Write(--reg_S, reg_CC);
		break;
	case 4:		// R don't care				($ffff)
		reg_CC |= (CC::I | CC::F);
		break;
	case 3:		// R FIRQ Vector hi	($FFF6)
		reg_PC_hi = Read(InterruptVector.FIRQ_hi);
		break;
	case 2:		// R FIRQ Vector lo	($FFF7)
		reg_PC_lo = Read(InterruptVector.FIRQ_lo);
		break;
	case 1:		// R don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	External_Irq()
//*****************************************************************************
//	Maskable Interrupt Request. Stores registers and starts executing code at a
// given address via a jump vector table @ $FFF8 (MSB) and $FFF9 (LSB)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	E, I, F
//	
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::External_Irq()
{
	switch (cyclesLeft)
	{
	case 19:	// R unknown				(PC)	- already done by the time we get here
		if ((reg_CC & CC::I) != 0)
			cyclesLeft = 1;
		break;
	case 18:	// R unknown				(PC+1)
		++reg_PC;
		break;
	case 17:	// R don't care				($ffff)
		reg_CC |= CC::E;
		break;
	case 16:	// W push PC low	onto S	(SP-1)	- Program Counter
		Write(--reg_S, reg_PC_lo);
		break;
	case 15:	// W push PC hi		onto S	(SP-2)
		Write(--reg_S, reg_PC_hi);
		break;
	case 14:	// W push U low		onto S	(SP-3)	- User Stack
		Write(--reg_S, reg_U_lo);
		break;
	case 13:	// W push U hi		onto S	(SP-4)
		Write(--reg_S, reg_U_hi);
		break;
	case 12:	// W push Y low		onto S	(SP-5)	- Index Register Y
		Write(--reg_S, reg_Y_lo);
		break;
	case 11:	// W push Y hi		onto S	(SP-6)
		Write(--reg_S, reg_Y_hi);
		break;
	case 10:	// W push X low		onto S	(SP-7)	- Index Register X
		Write(--reg_S, reg_X_lo);
		break;
	case 9:		// W push X hi		onto S	(SP-8)
		Write(--reg_S, reg_X_hi);
		break;
	case 8:		// W push DP		onto S	(SP-9)	- Direct Page register
		Write(--reg_S, reg_DP);
		break;
	case 7:		// W push B			onto S	(SP-10)	- Accumulator B
		Write(--reg_S, reg_B);
		break;
	case 6:		// W push A			onto S	(SP-11)	- Accumulator A
		Write(--reg_S, reg_A);
		break;
	case 5:		// W push CC		onto S	(SP-12)	- Condition Codes register
		Write(--reg_S, reg_CC);
		break;
	case 4:		// R don't care				($ffff)
		reg_CC |= (CC::I | CC::F);
		break;
	case 3:		// R retrv Reset Vector hi	($FFF8)
		reg_PC_hi = Read(InterruptVector.IRQ_hi);
		break;
	case 2:		// R retrv Reset Vector lo	($FFF9)
		reg_PC_lo = Read(InterruptVector.IRQ_lo);
		break;
	case 1:		// R don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*********************************************************************************************************************************
// instructions
//*********************************************************************************************************************************


//*********************************************************************************************************************************
// Software Interrupts - Triggered by software only, not hardware
//*****************************************************************************

//*****************************************************************************
//	Swi()
//*****************************************************************************
//	Software Interrupt (1). Stores registers and starts executing code at a
// given address via a jump vector table @ $FFFA (MSB) and $FFFB (LSB)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	E, I, F
//
// NOTE: This is also a software instruction: 3F
// 19 clocks, 1 bytes, inherent address mode
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::SWI()
{
	switch (cyclesLeft)
	{
	case 19:	// R Opcode fetch			(PC) - already done by the time we get here
		break;
	case 18:	// R don't care				(PC+1)
		++reg_PC;
		break;
	case 17:	// R don't care				($FFFF)
		reg_CC |= CC::E;
		break;
	case 16:	// W push PC low	onto S	(SP-1)	- Program Counter
		Write(--reg_S, reg_PC_lo);
		break;
	case 15:	// W push PC hi		onto S	(SP-2)
		Write(--reg_S, reg_PC_hi);
		break;
	case 14:	// W push U low		onto S	(SP-3)	- User Stack
		Write(--reg_S, reg_U_lo);
		break;
	case 13:	// W push U hi		onto S	(SP-4)
		Write(--reg_S, reg_U_hi);
		break;
	case 12:	// W push Y low		onto S	(SP-5)	- Index Register Y
		Write(--reg_S, reg_Y_lo);
		break;
	case 11:	// W push Y hi		onto S	(SP-6)
		Write(--reg_S, reg_Y_hi);
		break;
	case 10:	// W push X low		onto S	(SP-7)	- Index Register X
		Write(--reg_S, reg_X_lo);
		break;
	case 9:		// W push X hi		onto S	(SP-8)
		Write(--reg_S, reg_X_hi);
		break;
	case 8:		// W push DP		onto S	(SP-9)	- Direct Page register
		Write(--reg_S, reg_DP);
		break;
	case 7:		// W push B			onto S	(SP-10)	- Accumulator B
		Write(--reg_S, reg_B);
		break;
	case 6:		// W push A			onto S	(SP-11)	- Accumulator A
		Write(--reg_S, reg_A);
		break;
	case 5:		// W push CC		onto S	(SP-12)	- Condition Codes register
		Write(--reg_S, reg_CC);
		break;
	case 4:		// R don't care				($ffff)
		reg_CC |= (CC::I | CC::F);
		break;
	case 3:		// R retrieve SWI Vector hi ($FFFA)
		reg_PC_hi = Read(InterruptVector.SWI_hi);
		break;
	case 2:		// R retrieve SWI Vector lo	($FFFB)
		reg_PC_lo = Read(InterruptVector.SWI_lo);
		break;
	case 1:		// R don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	SWI2()
//*****************************************************************************
//	Software Interrupt 2. Stores registers and starts executing code at a
// given address via a jump vector table @ $FFF4 (MSB) and $FFF5 (LSB)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	E
//
// NOTE: This is also a software instruction: 103F
// 20 clocks, 2 bytes, inherent address mode
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::SWI2()
{
	
	switch (cyclesLeft)
	{
	case 20:	// R Opcode fetch			(PC)	- already done by the time we get here
		break;
	case 19:	// R Opcode fetch 2nd byte	(PC+1)	- already done by the time we get here
		++reg_PC;
		break;
	case 18:	// R don't care				(PC+2)
		++reg_PC;
		break;
	case 17:	// R don't care				($FFFF)
		reg_CC |= CC::E;
		break;
	case 16:	// W push PC low	onto S	(SP-1)	- Program Counter
		Write(--reg_S, reg_PC_lo);
		break;
	case 15:	// W push PC hi		onto S	(SP-2)
		Write(--reg_S, reg_PC_hi);
		break;
	case 14:	// W push U low		onto S	(SP-3)	- User Stack
		Write(--reg_S, reg_U_lo);
		break;
	case 13:	// W push U hi		onto S	(SP-4)
		Write(--reg_S, reg_U_hi);
		break;
	case 12:	// W push Y low		onto S	(SP-5)	- Index Register Y
		Write(--reg_S, reg_Y_lo);
		break;
	case 11:	// W push Y hi		onto S	(SP-6)
		Write(--reg_S, reg_Y_hi);
		break;
	case 10:	// W push X low		onto S	(SP-7)	- Index Register X
		Write(--reg_S, reg_X_lo);
		break;
	case 9:		// W push X hi		onto S	(SP-8)
		Write(--reg_S, reg_X_hi);
		break;
	case 8:		// W push DP		onto S	(SP-9)	- Direct Page register
		Write(--reg_S, reg_DP);
		break;
	case 7:		// W push B			onto S	(SP-10)	- Accumulator B
		Write(--reg_S, reg_B);
		break;
	case 6:		// W push A			onto S	(SP-11)	- Accumulator A
		Write(--reg_S, reg_A);
		break;
	case 5:		// W push CC		onto S	(SP-12)	- Condition Codes register
		Write(--reg_S, reg_CC);
		break;
	case 4:		// R don't care				($ffff)
		break;
	case 3:		// R retrv SWI2 Vector hi	($FFF4)
		reg_PC_hi = Read(InterruptVector.SWI2_hi);
		break;
	case 2:		// R retrv SWI2 Vector lo	($FFF5)
		reg_PC_lo = Read(InterruptVector.SWI2_lo);
		break;
	case 1:		// R don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	SWI3()
//*****************************************************************************
//	Software Interrupt 3. Stores registers and starts executing code at a
// given address via a jump vector table @ $FFF2 (MSB) and $FFF3 (LSB)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	E
//
// NOTE: This is also a software instruction: 113F
// 20 clocks, 2 bytes, inherent address mode
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::SWI3()
{

	
	switch (cyclesLeft)
	{
	case 20:	// R Opcode fetch			(PC)	- already done by the time we get here
		break;
	case 19:	// R Opcode fetch 2nd byte	(PC+1)	- already done by the time we get here
		++reg_PC;
		break;
	case 18:	// R don't care				(PC+2)
		++reg_PC;
		break;
	case 17:	// R don't care				($FFFF)
		reg_CC |= CC::E;
		break;
	case 16:	// W push PC low	onto S	(SP-1)	- Program Counter
		Write(--reg_S, reg_PC_lo);
		break;
	case 15:	// W push PC hi		onto S	(SP-2)
		Write(--reg_S, reg_PC_hi);
		break;
	case 14:	// W push U low		onto S	(SP-3)	- User Stack
		Write(--reg_S, reg_U_lo);
		break;
	case 13:	// W push U hi		onto S	(SP-4)
		Write(--reg_S, reg_U_hi);
		break;
	case 12:	// W push Y low		onto S	(SP-5)	- Index Register Y
		Write(--reg_S, reg_Y_lo);
		break;
	case 11:	// W push Y hi		onto S	(SP-6)
		Write(--reg_S, reg_Y_hi);
		break;
	case 10:	// W push X low		onto S	(SP-7)	- Index Register X
		Write(--reg_S, reg_X_lo);
		break;
	case 9:		// W push X hi		onto S	(SP-8)
		Write(--reg_S, reg_X_hi);
		break;
	case 8:		// W push DP		onto S	(SP-9)	- Direct Page register
		Write(--reg_S, reg_DP);
		break;
	case 7:		// W push B			onto S	(SP-10)	- Accumulator B
		Write(--reg_S, reg_B);
		break;
	case 6:		// W push A			onto S	(SP-11)	- Accumulator A
		Write(--reg_S, reg_A);
		break;
	case 5:		// W push CC		onto S	(SP-12)	- Condition Codes register
		Write(--reg_S, reg_CC);
		break;
	case 4:		// R don't care				($ffff)
		break;
	case 3:		// R retrv SWI3 Vector hi	($FFF2)
		reg_PC_hi = Read(InterruptVector.SWI3_hi);
		break;
	case 2:		// R retrv SWI3 Vector lo	($FFF3)
		reg_PC_lo = Read(InterruptVector.SWI3_lo);
		break;
	case 1:		// R don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	Reset()
//*****************************************************************************
//	Resets the CPU to a functional state and starts executing code at a given
// address via a jump vector table @ $FFFE (MSB) and $FFFF (LSB)
// Highest priority after /halt signal.
//
// Address Mode: Inherent
// Condition Codes Affected:
//	CC
//
// NOTE: an UNDOCUMENTED Opcode: $3E
//
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::Reset()
{
	switch (cyclesLeft)
	{
	case 19:	// R Opcode fetch			(PC) - already done by the time we get here
		break;
	case 18:	// R don't care				(PC+1)
		++reg_PC;
		break;
	case 17:	// R don't care				($ffff)
		reg_CC |= CC::E;
		break;
	case 16:	// W push PC low	onto S	(SP-1)	- Program Counter
		Write(--reg_S, reg_PC_lo);
		break;
	case 15:	// W push PC hi		onto S	(SP-2)
		Write(--reg_S, reg_PC_hi);
		break;
	case 14:	// W push U low		onto S	(SP-3)	- User Stack
		Write(--reg_S, reg_U_lo);
		break;
	case 13:	// W push U hi		onto S	(SP-4)
		Write(--reg_S, reg_U_hi);
		break;
	case 12:	// W push Y low		onto S	(SP-5)	- Index Register Y
		Write(--reg_S, reg_Y_lo);
		break;
	case 11:	// W push Y hi		onto S	(SP-6)
		Write(--reg_S, reg_Y_hi);
		break;
	case 10:	// W push X low		onto S	(SP-7)	- Index Register X
		Write(--reg_S, reg_X_lo);
		break;
	case 9:		// W push X hi		onto S	(SP-8)
		Write(--reg_S, reg_X_hi);
		break;
	case 8:		// W push DP		onto S	(SP-9)	- Direct Page register
		Write(--reg_S, reg_DP);
		break;
	case 7:		// W push B			onto S	(SP-10)	- Accumulator B
		Write(--reg_S, reg_B);
		break;
	case 6:		// W push A			onto S	(SP-11)	- Accumulator A
		Write(--reg_S, reg_A);
		break;
	case 5:		// W push CC		onto S	(SP-12)	- Condition Codes register
		Write(--reg_S, reg_CC);
		break;
	case 4:		// R don't care				($ffff)
		reg_CC |= (CC::I | CC::F);
		break;
	case 3:		// R retrv Reset Vector hi	($FFFE)
		reg_PC_hi = Read(InterruptVector.RESET_hi);
		break;
	case 2:		// R retrv Reset Vector lo	($FFFF)
		reg_PC_lo = Read(InterruptVector.RESET_lo);
		break;
	case 1:		// R don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*********************************************************************************************************************************
// Returns: from interrupts or subroutines
//*****************************************************************************

//*****************************************************************************
//	RTI()
//*****************************************************************************
//	Return from Interrupt. Reloads all saved registers (exact count depends on
// what the E flag of the retrieved CC register contains)
//
// Address Mode: Inherent
// Condition Codes Affected: Pulled from [System] Stack
//	
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::RTI()
{
	switch (cyclesLeft)
	{
	case 15:	// R Opcode Fetch			(PC)		//1
		break;
	case 14:	// R Don't Care				(PC+1)		//2
		++reg_PC;
		break;
	case 13:	// R CCR					(SP)		//3
		reg_CC = Read(reg_S++);
		break;
	}
	if ((reg_CC | CC::E) != CC::E)
	{
		switch (cyclesLeft)
		{
		case 12:	// R PC hi				(SP+1)		//4
			break;
		case 11:	// R PC lo				(SP+2)		//5
			break;
		case 10:	// R Don't care			($ffff)		//6
			cyclesLeft = 1;
			break;
		}
		--cyclesLeft;
		return(cyclesLeft);
	}
	switch (cyclesLeft)
	{
	case 12:	// R Register A				(SP+1)		//4
		reg_A = data_hi;
		break;
	case 11:	// R Register B				(SP+2)		//5
		reg_B = data_lo;
		break;
	case 10:	// R Register DP			(SP+3)		//6
		reg_DP = Read(reg_S++);
		break;
	case 9:		// R register X hi			(SP+4)		//7
		reg_X_hi = Read(reg_S++);
		break;
	case 8:		// R register X lo			(SP+5)		//8
		reg_X_lo = Read(reg_S++);
		break;
	case 7:		// R register Y hi			(SP+6)		//9
		reg_Y_hi = Read(reg_S++);
		break;
	case 6:		// R register Y lo			(SP+7)		//10
		reg_Y_lo = Read(reg_S++);
		break;
	case 5:		// R register U Hi			(SP+8)		//11
		reg_U_hi = Read(reg_S++);
		break;
	case 4:		// R register U lo			(SP+9)		//12
		reg_U_lo = Read(reg_S++);
		break;
	case 3:		// R register PC hi			(SP+10)		//13
		reg_PC_hi = Read(reg_S++);
		break;
	case 2:		// R register PC lo			(SP+11)		//14
		reg_PC_lo = Read(reg_S++);
		break;
	case 1:		//R Don't care				($ffff)		//15
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	RTS()
//*****************************************************************************
//	Return from Subroutine. Doesn't care if it was BSR, LBSR, or JSR
//
// Address Mode: Inherent
// Condition Codes Affected: None
//	
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::RTS()
{
	switch (cyclesLeft)
	{
	case 5:		// R Opcode Fetch			(PC)
		break;
	case 4:		// R Don't Care				(PC+1)
		++reg_PC;
		break;
	case 3:		// R register PC hi			(SP)
		reg_PC_hi = Read(reg_S++);
		break;
	case 2:		// R register PC lo			(SP+1)
		reg_PC_lo = Read(reg_S++);
		break;
	case 1:		//R Don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*********************************************************************************************************************************
// opcodes - single addressing mode, simple
//*****************************************************************************


//*****************************************************************************
// ABX()
//*****************************************************************************
// Add B to X
//
// Address Mode: Inherent
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::ABX()
{
	switch (cyclesLeft)
	{
	case 3:		// R opcode fetch			(PC)
		break;
	case 2:		// R don't care				(PC+1)
		++reg_PC;
		break;
	case 1:
		reg_X += reg_B;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// ASLA()
//*****************************************************************************
// Arithmetic Shift Left A (Logical Shift Left fill LSb with 0)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - Undefined
//  N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::ASLA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = reg_A << 1;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (((reg_A >> 6) & 1) != ((reg_A >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// ASLB()
//*****************************************************************************
// Arithmetic Shift Left B (Logical Shift Left fill LSb with 0)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - Undefined
//  N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::ASLB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = reg_B << 1;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (((reg_B >> 6) & 1) != ((reg_B >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// ASRA()
//*****************************************************************************
// Arithmetic Shift Right A (fill MSb with Sign bit)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - Undefined
//  N, Z, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::ASRA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		data_lo = reg_A & 0x80;
		reg_CC = ((reg_A & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((reg_A >> 1) & 0x7f) | data_lo;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// ASRB()
//*****************************************************************************
// Arithmetic Shift Right B (fill MSb with Sign bit)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - Undefined
//  N, Z, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::ASRB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		data_lo = reg_B & 0x80;
		reg_CC = ((reg_B & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = ((reg_B >> 1) & 0x7f) | data_lo;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// CLRA()
//*****************************************************************************
// Clear register A
//
// Address Mode: Inherent
// Condition Codes Affected: All
//	H - Undefined
//  N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::CLRA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_A = 0;
		reg_CC = (reg_CC & (CC::H | CC::I | CC::E | CC::N)) | CC::Z | CC::C;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// CLRB()
//*****************************************************************************
// Clear register B
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - Undefined
//  N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::CLRB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_B = 0;
		reg_CC = (reg_CC & (CC::H | CC::I | CC::E | CC::N)) | CC::Z | CC::C;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// COMA()
//*****************************************************************************
// 1's Compliment A (i.e. XOR A with 0x00 or 0xFF)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - Undefined
//  N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::COMA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_A = ~reg_A;
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC &= ~CC::V;
		reg_CC |= CC::C;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// COMB()
//*****************************************************************************
// 1's Compliment B (i.e. XOR B with 0x00 or 0xFF) 
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - Undefined
//  N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::COMB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_B = ~reg_B;
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC &= ~CC::V;
		reg_CC |= CC::C;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


// REVIEW THIS ONE
//*****************************************************************************
// CWAI()
//*****************************************************************************
// Wait for Interrupt
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H, N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::CWAI()
{
	switch (cyclesLeft)
	{
	case 20:		// R opcode fetch			(PC)		//1
		--cyclesLeft;
		break;
	case 19:		// R CC Mask				(PC+1)		//2
		data_lo = Read(reg_PC++);
		--cyclesLeft;
		break;
	case 18:		// R Don't care				(PC+2)		//3
		++reg_PC;
		--cyclesLeft;
		break;
	case 17:		// R Don't care				($ffff)		//4
		reg_CC = (reg_CC & data_lo) | CC::E;
		--cyclesLeft;
		break;
	case 16:		// W push PC low	onto S	(SP-1)		//5
		Write(--reg_S, reg_PC_lo);
		break;
	case 15:		// W push PC hi		onto S	(SP-2)		/6
		Write(--reg_S, reg_PC_hi);
		break;
	case 14:		// W U Low					(SP-3)		//7
		Write(--reg_S, reg_U_lo);
		--cyclesLeft;
		break;
	case 13:		// W U high					(SP-4)		//8
		Write(--reg_S, reg_U_hi);
		--cyclesLeft;
		break;
	case 12:			// W Y Low					(SP-5)		//9
		Write(--reg_S, reg_Y_lo);
		--cyclesLeft;
		break;
	case 11:			// W Y high					(SP-6)		//10
		Write(--reg_S, reg_Y_hi);
		--cyclesLeft;
		break;
	case 10:			// W X Low					(SP-7)		//11
		Write(--reg_S, reg_X_lo);
		--cyclesLeft;
		break;
	case 9:			// W X High					(SP-8)		//12
		Write(--reg_S, reg_PC_hi);
		--cyclesLeft;
		break;
	case 8:			// W DP						(SP-9)		//13
		Write(--reg_S, reg_DP);
		--cyclesLeft;
		break;
	case 7:		// W push B			onto S		(SP-10)		//14
		Write(--reg_S, reg_B);
		--cyclesLeft;
		break;
	case 6:		// W push A			onto S		(SP-11)		//15
		Write(--reg_S, reg_A);
		--cyclesLeft;
		break;
	case 5:		// W push CC		onto S		(SP-12)		//16
		Write(--reg_S, reg_CC);
		--cyclesLeft;
		break;
	case 4:
		// wait for interrupt to take place
		if (resetTriggered || nmiTriggered || firqTriggered || irqTriggered)
			--cyclesLeft;
		break;
	case 3:
		if (resetTriggered)			// external RESET was triggered
			reg_PC_hi = Read(InterruptVector.RESET_hi);
		else if (nmiTriggered)			// external NMI was triggered
			reg_PC_hi = Read(InterruptVector.NMI_hi);
		else if (firqTriggered)			// external FIRQ was triggered
			reg_PC_hi = Read(InterruptVector.FIRQ_hi);
		else if (irqTriggered)			// external IRQ was triggered
			reg_PC_hi = Read(InterruptVector.IRQ_hi);
		--cyclesLeft;
	case 2:
		if (resetTriggered)			// external RESET was triggered
			reg_PC_lo = Read(InterruptVector.RESET_lo);
		else if (nmiTriggered)			// external NMI was triggered
			reg_PC_lo = Read(InterruptVector.NMI_lo);
		else if (firqTriggered)			// external FIRQ was triggered
			reg_PC_lo = Read(InterruptVector.FIRQ_lo);
		else if (irqTriggered)			// external IRQ was triggered
			reg_PC_lo = Read(InterruptVector.IRQ_lo);
		--cyclesLeft;
		break;
	case 1:
		--cyclesLeft;
		break;
	}
	return(cyclesLeft);
}


// REVIEW THIS ONE
//*****************************************************************************
// DAA()
//*****************************************************************************
// Decimal Adjust A (contents of A -> BCD... BCD operation should be prior)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	V - undefined
//	N, Z, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::DAA()
{
	uint8_t cfLsn = 0;
	uint8_t cfMsn = 0;
	uint8_t carry;
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		++reg_PC;
		break;
	case 1:

		data_lo = reg_A & 0x0f;
		data_hi = (reg_A & 0xf0) >> 4;
		carry = reg_CC & CC::C;
		if (((reg_CC & CC::H) == CC::H) || (data_lo > 9))
			 cfLsn= 6;
		if ((carry == CC::C) || (data_hi > 9) || ((data_hi > 8) && (data_lo > 9)))
			cfMsn = 6;

		reg_A += cfLsn;			// fixes lsn
		reg_A += (cfMsn << 4);	// fixes msn
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (cfMsn == 6 || carry) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// DECA()
//*****************************************************************************
// Decrement A (A -= A      A = A - 1   --A     A--)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, V
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::DECA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		if (reg_A == 0)
		{
			reg_A = 0xff;
			reg_CC |= CC::V;
		}
		else
		{
			--reg_A;
			reg_CC &= ~CC::V;
		}
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// DECB()
//*****************************************************************************
// Decrement B (B -= B      B = B - 1   --B     B--)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	V - undefined
//	N, Z, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::DECB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		if (reg_B == 0)
		{
			reg_B = 0xff;
			reg_CC |= CC::V;
		}
		else
		{
			--reg_B;
			reg_CC &= ~CC::V;
		}
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// EXG()
//*****************************************************************************
// Exchange any two registers of the same size
//
// Address Mode: Immediate
// Condition Codes Affected: dependent on if CC is part of the exchange
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::EXG()
{
	switch (cyclesLeft)
	{
	case 8:		// R opcode fetch			(PC)
		break;
	case 7:
		data_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 6:
		data_hi = data_lo & 0xf0;
		data_lo = data_lo & 0x0f;
		if (data_lo & 0x80 != data_hi & 0x80)
			;	// error in match-up
		break;

	case 5:
		switch (data_hi)
		{
		case REG::A:
			byte = reg_A;
			switch (data_lo)
			{
			case REG::B:
				reg_A = reg_B;
				reg_B = byte;
				break;
			case REG::CC:
				reg_A = reg_CC;
				reg_CC = byte;
				break;
			case REG::DP:
				reg_A = reg_DP;
				reg_DP = byte;
				break;
			}
			break;
		case REG::B:
			byte = reg_A;
			switch (data_lo)
			{
			case REG::A:
				reg_B = reg_A;
				reg_A = byte;
				break;
			case REG::CC:
				reg_B = reg_CC;
				reg_CC = byte;
				break;
			case REG::DP:
				reg_B = reg_DP;
				reg_DP = byte;
				break;
			}
			break;
		}
		break;
	case 4:
		switch (data_hi)
		{
		case REG::CC:
			byte = reg_CC;
			switch (data_lo)
			{
			case REG::A:
				reg_CC = reg_A;
				reg_A = byte;
				break;
			case REG::B:
				reg_CC = reg_B;
				reg_B = byte;
				break;
			case REG::DP:
				reg_CC = reg_DP;
				reg_DP = byte;
				break;
			}
			break;
		case REG::DP:
			byte = reg_DP;
			switch (data_lo)
			{
			case REG::A:
				reg_DP = reg_A;
				reg_A = byte;
				break;
			case REG::B:
				reg_DP = reg_B;
				reg_B = byte;
				break;
			case REG::CC:
				reg_DP = reg_CC;
				reg_CC = byte;
				break;
			}
			break;
		}
		break;

	case 3:
		switch (data_hi)
		{
		case REG::X:
			word = reg_X;
			switch (data_lo)
			{
			case REG::Y:
				reg_X = reg_Y;
				reg_CC = word;
				break;
			case REG::U:
				reg_X = reg_U;
				reg_U = word;
				break;
			case REG::S:
				reg_X = reg_S;
				reg_S = word;
				break;
			case REG::D:
				reg_X = reg_D;
				reg_D = word;
				break;
			case REG::PC:
				reg_X = reg_PC;
				reg_PC = word;
				break;
			}
			break;
		case REG::Y:
			word = reg_Y;
			switch (data_lo)
			{
			case REG::X:
				reg_Y = reg_X;
				reg_X = word;
				break;
			case REG::U:
				reg_Y = reg_U;
				reg_U = word;
				break;
			case REG::S:
				reg_Y = reg_S;
				reg_S = word;
				break;
			case REG::D:
				reg_Y = reg_D;
				reg_D = word;
				break;
			case REG::PC:
				reg_Y = reg_PC;
				reg_PC = word;
				break;
			}
			break;
		}
		break;
	case 2:
		switch (data_hi)
		{
		case REG::U:
			word = reg_U;
			switch (data_lo)
			{
			case REG::X:
				reg_U = reg_X;
				reg_X = word;
				break;
			case REG::Y:
				reg_U = reg_Y;
				reg_Y = word;
				break;
			case REG::S:
				reg_U = reg_S;
				reg_S = word;
				break;
			case REG::D:
				reg_U = reg_D;
				reg_D = word;
				break;
			case REG::PC:
				reg_U = reg_PC;
				reg_PC = word;
				break;
			}
			break;
		case REG::S:
			word = reg_S;
			switch (data_lo)
			{
			case REG::X:
				reg_S = reg_X;
				reg_X = word;
				break;
			case REG::Y:
				reg_S = reg_Y;
				reg_Y = word;
				break;
			case REG::U:
				reg_S = reg_U;
				reg_U = word;
				break;
			case REG::D:
				reg_S = reg_D;
				reg_D = word;
				break;
			case REG::PC:
				reg_S = reg_PC;
				reg_PC = word;
				break;
			}
			break;
		}
		break;
	case 1:
		switch (data_hi)
		{
		case REG::D:
			word = reg_D;
			switch (data_lo)
			{
			case REG::X:
				reg_D = reg_PC;
				reg_PC = word;
				break;
			case REG::Y:
				reg_D = reg_Y;
				reg_Y = word;
				break;
			case REG::U:
				reg_D = reg_U;
				reg_U = word;
				break;
			case REG::S:
				reg_D = reg_S;
				reg_S = word;
				break;
			case REG::PC:
				reg_D = reg_PC;
				reg_PC = word;
				break;
			}
			break;
		case REG::PC:
			word = reg_PC;
			switch (data_lo)
			{
			case REG::X:
				reg_PC = reg_X;
				reg_X = word;
				break;
			case REG::Y:
				reg_PC = reg_Y;
				reg_Y = word;
				break;
			case REG::U:
				reg_PC = reg_U;
				reg_U = word;
				break;
			case REG::S:
				reg_PC = reg_S;
				reg_S = word;
				break;
			case REG::D:
				reg_PC = reg_D;
				reg_D = word;
				break;
			}
			break;
		}
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// INCA()
//*****************************************************************************
// Increment A (A += A      A = A + 1   ++A     A++)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, V
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::INCA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		if (reg_A == 0xff)
		{
			reg_A = 0x00;
			reg_CC |= CC::V;
		}
		else
		{
			++reg_A;
			reg_CC &= ~CC::V;
		}
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// INCB()
//*****************************************************************************
// Increment B (B += B      B = B + 1   ++B     B++)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, V
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::INCB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		if (reg_B == 0xff)
		{
			reg_B = 0x00;
			reg_CC |= CC::V;
		}
		else
		{
			++reg_B;
			reg_CC &= ~CC::V;
		}
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************************
// LSLA()
//*****************************************************************************
// Logical Shift Left register A (LSb is loaded with 0)
// Functionally ALSA(), might never be called.
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - undefined
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LSLA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_CC = (((reg_A >> 6) & 1) != ((reg_A >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_A = reg_A << 1;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************************
// LSLB()
//*****************************************************************************
// Logical Shift Left register B (LSb is loaded with 0)
// Functionally ALSB(), might never be called.
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - undefined
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LSLB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_CC = (((reg_B >> 6) & 1) != ((reg_B >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_B = reg_B << 1;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// LSRA()
//*****************************************************************************
// Logical Shift Right register A (MSb is loaded with 0)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LSRA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		++reg_PC;
		break;
	case 1:
		data_lo = reg_A & 0x80;
		reg_CC = ((reg_A & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((reg_A >> 1) & 0x7f);
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC &= ~CC::N;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// LSRB()
//*****************************************************************************
// Logical Shift Right register B (MSb is loaded with 0)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LSRB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		data_lo = reg_B & 0x80;
		reg_CC = ((reg_B & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = ((reg_B >> 1) & 0x7f);
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC &= ~CC::N;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// MUL()
//*****************************************************************************
// Multiply register A * register B, store in register D (A << 8 | B)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	Z, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::MUL()
{
	switch (cyclesLeft)
	{
	case 11:		// R opcode fetch			(PC)
		break;
	case 10:
		data = reg_A * reg_B;
		break;
	case 9:
		reg_D = data;
		break;
	case 8:
		reg_CC = (reg_D == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		break;
	case 7:
		reg_CC = ((reg_D & 0x0080) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		break;
	case 6:
		break;
	case 5:
		break;
	case 4:
		break;
	case 3:
		break;
	case 2:
		break;
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// NEGA()
//*****************************************************************************
// 2's Complement (negate) register A
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - undefined
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::NEGA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (reg_A == 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		reg_A = 0 - reg_A;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// NEGA()
//*****************************************************************************
// 2's Complement (negate) register B
//
// Address Mode: Inherent
// Condition Codes Affected:
//	H - undefined
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::NEGB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (reg_B == 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		reg_B = 0 - reg_B;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// NOP()
//*****************************************************************************
// No Operation, Does nothing, affects PC only.
//
// Address Mode: Inherent
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::NOP()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// ROLA()
//*****************************************************************************
// Rotate Register A Left one bit through the Carry flag (9 bit rotate)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::ROLA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		data_lo = ((reg_CC & CC::C) != 0) ? 1 : 0;
		reg_CC = (((reg_A >> 6) & 1) != ((reg_A >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = (reg_A << 1) | data_lo;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// ROLB()
//*****************************************************************************
// Rotate Register B Left one bit through the Carry flag (9 bit rotate)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::ROLB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		data_lo = ((reg_CC & CC::C) != 0) ? 1 : 0;
		reg_CC = (((reg_B >> 6) & 1) != ((reg_B >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = (reg_B << 1) | data_lo;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// RORA()
//*****************************************************************************
// Rotate Register A Right one bit through the Carry flag (9 bit rotate)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::RORA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		data_lo = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((reg_A & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = (reg_A >> 1) | data_lo;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
// RORB()
//*****************************************************************************
// Rotate Register B Right one bit through the Carry flag (9 bit rotate)
//
// Address Mode: Inherent
// Condition Codes Affected:
//	N, Z, V, C
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::RORB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		data_lo = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((reg_B & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = (reg_B >> 1) | data_lo;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	SEX()
//*****************************************************************************
// Sign Extend B into A ( A = Sign bit set on B ? 0xFF : 0x00)
//
// Address Mode: Inherent
// Condition Codes Affected: 
//	N, Z
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::SEX()
{
	switch (cyclesLeft)
	{
	case 2:
		break;
	case 1:
		if ((reg_B & 0x80) == 0x80)
		{
			reg_A = 0xff;
			reg_CC |= CC::N;
			reg_CC &= ~CC::Z;
		}
		else
		{
			reg_A = 0x00;
			reg_CC &= ~CC::N;
			reg_CC |= CC::Z;
		}
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	SYNC()
//*****************************************************************************
// Synchronize to interrupt
//
// Address Mode: Inherent
// Condition Codes Affected: 
//	N, Z
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::SYNC()
{
	switch (cyclesLeft)
	{
	case 3:		// R opcode fetch			(PC)
		--cyclesLeft;
		break;
	case 2:
		++reg_PC;
		--cyclesLeft;
		break;
	case 1:
		asdjk;
		break;
	}
	return(cyclesLeft);
}


//*****************************************************************************
//	TFR()
//*****************************************************************************
// Transfer/Copy one register to another (of the same size)
//
// Address Mode: Immediate
// Condition Codes Affected: dependent on if CC is targeted in transfer
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::TFR()
{
	switch (cyclesLeft)
	{
	case 6:		// R opcode fetch			(PC)
		break;
	case 5:
		data_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 4:
		data_hi = (data_lo & 0xf0) >> 4;
		data_lo = data_lo & 0x0f;
		if (data_lo & 0x80 != data_hi & 0x80)
			;	// error in match-up
		break;
	case 3:
		switch (data_hi)
		{
		case REG::A:
			switch (data_lo)
			{
			case REG::B:
				reg_B = reg_A;
				break;
			case REG::DP:
				reg_DP = reg_A;
				break;
			case REG::CC:
				reg_CC = reg_A;
				break;
			}
			break;
		case REG::B:
			switch (data_lo)
			{
			case REG::A:
				reg_A = reg_B;
				break;
			case REG::DP:
				reg_DP = reg_B;
				break;
			case REG::CC:
				reg_CC = reg_B;
				break;
			}
			break;
		case REG::DP:
			switch (data_lo)
			{
			case REG::A:
				reg_DP = reg_A;
				break;
			case REG::B:
				reg_DP = reg_B;
				break;
			case REG::CC:
				reg_DP = reg_CC;
				break;
			}
			break;
		case REG::CC:
			switch (data_lo)
			{
			case REG::A:
				reg_CC = reg_A;
				break;
			case REG::B:
				reg_CC = reg_B;
				break;
			case REG::DP:
				reg_CC = reg_DP;
				break;
			}
			break;
		}
		break;
	case 2:
		switch (data_hi)
		{
		case REG::D:
			switch (data_lo)
			{
			case REG::X:
				reg_D = reg_X;
				break;
			case REG::Y:
				reg_D = reg_Y;
				break;
			case REG::U:
				reg_D = reg_U;
				break;
			case REG::S:
				reg_D = reg_S;
				break;
			case REG::PC:
				reg_D = reg_PC;
				break;
			}
			break;
		case REG::X:
			switch (data_lo)
			{
			case REG::D:
				reg_X = reg_D;
				break;
			case REG::Y:
				reg_X = reg_Y;
				break;
			case REG::U:
				reg_X = reg_U;
				break;
			case REG::S:
				reg_X = reg_S;
				break;
			case REG::PC:
				reg_X = reg_PC;
				break;
			}
			break;
		case REG::Y:
			switch (data_lo)
			{
			case REG::D:
				reg_Y = reg_D;
				break;
			case REG::X:
				reg_Y = reg_X;
				break;
			case REG::U:
				reg_Y = reg_U;
				break;
			case REG::S:
				reg_Y = reg_S;
				break;
			case REG::PC:
				reg_Y = reg_PC;
				break;
			}
			break;
		}
		break;
	case 1:
		switch (data_hi)
		{
		case REG::U:
			switch (data_lo)
			{
			case REG::D:
				reg_U = reg_D;
				break;
			case REG::X:
				reg_U = reg_X;
				break;
			case REG::Y:
				reg_U = reg_Y;
				break;
			case REG::S:
				reg_U = reg_S;
				break;
			case REG::PC:
				reg_U = reg_PC;
				break;
			}
			break;
		case REG::S:
			switch (data_lo)
			{
			case REG::D:
				reg_S = reg_D;
				break;
			case REG::X:
				reg_S = reg_X;
				break;
			case REG::Y:
				reg_S = reg_Y;
				break;
			case REG::U:
				reg_S = reg_U;
				break;
			case REG::PC:
				reg_S = reg_PC;
				break;
			}
			break;
		case REG::PC:
			switch (data_lo)
			{
			case REG::D:
				reg_PC = reg_D;
				break;
			case REG::X:
				reg_PC = reg_X;
				break;
			case REG::Y:
				reg_PC = reg_Y;
				break;
			case REG::U:
				reg_PC = reg_U;
				break;
			case REG::S:
				reg_PC = reg_S;
				break;
			}
			break;
		}
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	TSTA()
//*****************************************************************************
// Test Register A, adjust N and Z Condition codes based on content
//
// Address Mode: Inherent
// Condition Codes Affected: 
//	N, Z, V
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::TSTA()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC &= ~CC::V;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	TSTB()
//*****************************************************************************
// Test Register B, adjust N and Z Condition codes based on content
//
// Address Mode: Inherent
// Condition Codes Affected: 
//	N, Z, V
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::TSTB()
{
	switch (cyclesLeft)
	{
	case 2:		// R opcode fetch			(PC)
		break;
	case 1:
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC &= ~CC::V;
		++reg_PC;
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*********************************************************************************************************************************
// opcodes - single addressing mode with submodes
//*****************************************************************************


//*****************************************************************************
//	LEAS()
//*****************************************************************************
// Load Effective Address S (increment or decrement S by a given value. Use  an index register to load another)
//
// Address Mode: Indexed
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LEAS(int s)
{
	switch (cyclesLeft)
	{
	case 3:		// R opcode fetch			(PC)
		break;
	case 2:		// R Post Byte				(PC+1)
		break;

	// index mode, will determine how many more...
	case 1:		// R Don't Care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	LEAU()
//*****************************************************************************
// Load Effective Address U (increment or decrement U by a given value. Use  an index register to load another)
//
// Address Mode: Indexed
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LEAU()
{
	switch (cyclesLeft)
	{
	case 3:		// R opcode fetch			(PC)
		break;
	case 2:		// R Post Byte				(PC+1)
		break;

	// index mode, will determine how many more...
	case 1:		// R Don't Care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	LEAX()
//*****************************************************************************
// Load Effective Address X (increment or decrement X by a given value. Use  an index register to load another)
//
// Address Mode: Indexed
// Condition Codes Affected: 
//	Z
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LEAX()
{
	switch (cyclesLeft)
	{
	case 3:		// R opcode fetch			(PC)
		break;
	case 2:		// R Post Byte				(PC+1)
		break;

	// index mode, will determine how many more...
	case 1:		// R Don't Care				($ffff)
		reg_CC = (reg_X == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*****************************************************************************
//	LEAY()
//*****************************************************************************
// Load Effective Address Y (increment or decrement Y by a given value. Use  an index register to load another)
//
// Address Mode: Indexed
// Condition Codes Affected: 
//	Z
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LEAY()
{
	switch (cyclesLeft)
	{
	case 3:		// R opcode fetch			(PC)
		break;
	case 2:		// R Post Byte				(PC+1)
		break;

	// index mode, will determine how many more...
	case 1:		// R Don't Care				($ffff)
		reg_CC = (reg_Y == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*********************************************************************************************************************************
// opcodes - conditional branches
//*****************************************************************************


//*****************************************************************************
// BCC()
//*****************************************************************************
// Branch on Carry Clear        (C clear)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BCC()
{
	switch (cyclesLeft)
	{
	case 4:		// R opcode fetch			{PC)
		break;
	case 3:		// R Offset					(PC+1)
		data_lo = Read(reg_PC++);
		break;
	case 2:		//R Don't care				($ffff)
		if ((reg_CC & CC::C) == CC::C)		// if carry is clear, we don't branch
			cyclesLeft = 1;
		break;
	case 1:		//R Don't care				($ffff)
		reg_PC +=
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch on Carry Clear
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBCC()
{
	switch (cyclesLeft)
	{
	case 6:		// R opcode fetch			{PC)
		break;
	case 5:		// R opcode byte 2			(PC+1)
		data_hi = Read(reg_PC++);
		break;
	case 4:		// R Offset	hi				(PC+2)
		break;
	case 3:		//R Offset	lo				(PC+3)
		break;
	case 2:		//R Don't care				($ffff)
		if ((reg_CC & CC::C) == CC::C)		// if carry is clear, we don't branch
			cyclesLeft = 1;
		break;
	case 1:		//R Don't care				($ffff)
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch on Carry Set          (C set)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BCS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch on Carry Set
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBCS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Equal              (Z set)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BEQ()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Equal
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBEQ()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Greater or Equal (signed)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BGE()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch  if Greater or Equal
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBGE()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Greater than (signed)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BGT()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Greater than
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBGT()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Higher (unsigned)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BHI()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Higher
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBHI()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if higher or same (unsigned)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BHS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if higher or same
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBHS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Less than or Equal (signed)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BLE()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Less than or Equal (signed)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBLE()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Lower (unsigned)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BLO()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Lower (unsigned)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBLO()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Lower or Same (unsigned)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BLS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Lower or Same (unsigned)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBLS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if less than (signed)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BLT()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if less than (signed)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBLT()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch on Minus 
uint8_t Mc6809::BMI()
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch on Minus
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBMI()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Not Equal (Z = 0)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BNE()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Not Equal (Z = 0)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBNE()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if Plus/Positive
uint8_t Mc6809::BPL()
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if Plus/Positive
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBPL()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch Always
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BRA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch Always
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBRA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch Never (another NOP)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BRN()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch Never (another NOP)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBRN()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch to Subroutine
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BSR()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch to Subroutine
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBSR()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch if no overflow (V is clear)
uint8_t Mc6809::BVC()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch if no overflow (V is clear)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::LBVC()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Branch on overflow (V is set)
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::BVS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Long Branch on overflow (V is set)
uint8_t Mc6809::LBVS()
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Long Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//********************************************************************************************************************************* 
// opcodes - Jumps
//*****************************************************************************


// Unconditional Jump (non-relative) to a given (direct or indirect) address
uint8_t Mc6809::JMP()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Jump to a subroutine (non-relative) at a given (direct or indirect) address
//
// Memory Addressing Mode: Memory immediate
// Address Mode: Relative
// Condition Codes Affected: None
//*****************************************************************************
// Returns:
//	uint8_t - clock cycles remaining
//*****************************************************************************
uint8_t Mc6809::JSR()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*********************************************************************************************************************************
// opcode - multi addressing modes
//*****************************************************************************


// Add to A + Carry
uint8_t Mc6809::ADCA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Add to A + Carry
uint8_t Mc6809::ADCB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Add to A
uint8_t Mc6809::ADDA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Add to B
uint8_t Mc6809::ADDB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Add to D (A << 8 | B)
uint8_t Mc6809::ADDD()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// And A
uint8_t Mc6809::ANDA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// And B
uint8_t Mc6809::ANDB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// And Condition Codes (clear one or more flags)
uint8_t Mc6809::ANDCC()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Arithmetic Shift Left Memory location (Logical Shift Left fill LSb with 0)
uint8_t Mc6809::ASL()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Arithmetic Shift Right Memory location (fill MSb with Sign bit)
uint8_t Mc6809::ASR()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Bit Test on A with a specific value (by AND)
uint8_t Mc6809::BITA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Bit Test on B with a specific value (by AND)
uint8_t Mc6809::BITB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Clear memory location
uint8_t Mc6809::CLR()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Compare register A to memory location or given value(CC H unaffected)
uint8_t Mc6809::CMPA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Compare register B to memory location or given value (CC H unaffected)
uint8_t Mc6809::CMPB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Compare register D ( A <<8 | B) to memory locations or given value (CC H unaffected)
uint8_t Mc6809::CMPD()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Compare register S to memory locations or given value (CC H unaffected)
uint8_t Mc6809::CMPS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Compare register U to memory locations or given value (CC H unaffected)
uint8_t Mc6809::CMPU()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Compare register X to memory locations or given value (CC H unaffected)
uint8_t Mc6809::CMPX()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Compare register Y to memory locations or given value (CC H unaffected)
uint8_t Mc6809::CMPY()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// 1's Compliment Memory location (i.e. XOR A with 0x00 or 0xFF)
uint8_t Mc6809::COM()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Decrement Memory location
uint8_t Mc6809::DEC()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Logical Exclusive OR register A
uint8_t Mc6809::EORA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Logical Exclusive OR register B
uint8_t Mc6809::EORB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Increment Memory location
uint8_t Mc6809::INC()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Load Register A
uint8_t Mc6809::LDA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Load Register A
uint8_t Mc6809::LDB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Load Register D  ( A << 8 | B)
uint8_t Mc6809::LDD()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Load Register S
uint8_t Mc6809::LDS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Load Register U
uint8_t Mc6809::LDU()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Load Register X
uint8_t Mc6809::LDX()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Load Register Y
uint8_t Mc6809::LDY()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Logical Shift Left memory location (LSb is loaded with 0)
uint8_t Mc6809::LSL()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Logical Shift Right memory location (MSb is loaded with 0)
uint8_t Mc6809::LSR()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// 2's Complement (negate) memory location
uint8_t Mc6809::NEG()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


// Logical OR register A
uint8_t Mc6809::ORA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Logical OR register B
uint8_t Mc6809::ORB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Logical OR Condition Codes (set one or more flags)
uint8_t Mc6809::ORCC()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Push one or more registers onto the System Stack
uint8_t Mc6809::PSHS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Push one or more registers onto the User Stack
uint8_t Mc6809::PSHU()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Pull one or more registers from the System Stack
uint8_t Mc6809::PULS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Pull one or more registers from the User Stack
uint8_t Mc6809::PULU()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


// Rotate memory location Left one bit through the Carry flag (9 bit rotate)
uint8_t Mc6809::ROL()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Rotate memory location Right one bit through the Carry flag (9 bit rotate)
uint8_t Mc6809::ROR()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Subtract with carry (borrow) - register A
uint8_t Mc6809::SBCA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Subtract with carry (borrow) - register B
uint8_t Mc6809::SBCB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Store Register A
uint8_t Mc6809::STA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Store Register B 
uint8_t Mc6809::STB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Store Register D     (A << 8 | B)
uint8_t Mc6809::STD()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Store Register S
uint8_t Mc6809::STS()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Store Register U
uint8_t Mc6809::STU()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Store Register X
uint8_t Mc6809::STX()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Store Register Y
uint8_t Mc6809::STY()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Subtract from register A
uint8_t Mc6809::SUBA()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Subtract from register A
uint8_t Mc6809::SUBB()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// Subtract from register D     (A << 8 | B)
uint8_t Mc6809::SUBD()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


// Test memory location, adjust N and Z Condition codes based on content
uint8_t Mc6809::TST()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}

// INVALID INSTRUCTION! on 6309, this would trigger the invalid operation exception vector
uint8_t Mc6809::XXX()
{
	switch (cyclesLeft)
	{
	case 1:
		break;
	}
	--cyclesLeft;
	return(cyclesLeft);
}


//*********************************************************************************************************************************
// INTERNAL - substructure of the emulated CPU/MPU that isn't op-code or
//			external control system
//*****************************************************************************


//*****************************************************************************
//	Clock()
//*****************************************************************************
//	Triggers the functions of the CPU
//*****************************************************************************
void Mc6809::Clock()
{
	// does this need to be moved above the rest? or at least the test to execute "opcode"?
	if (opcode != nullptr)
		if (opcode() == 0)
			opcode == nullptr; 
	else
		if (haltTriggered)
		return;
	else if (resetTriggered)			// external RESET was triggered
		External_Reset();
	else if (nmiTriggered)			// external NMI was triggered
		External_Nmi();
	else if (firqTriggered)			// external FIRQ was triggered
		External_Firq();
	else if (irqTriggered)			// external IRQ was triggered
		External_Irq();
	else 
		Fetch(reg_PC);
}


//*****************************************************************************
//	Fetch()
//*****************************************************************************
// Fetch an instruction from RAM
//*****************************************************************************
// Params:
//	const uint16_t address	- the memory location (from MMU, SAM, or other)
//							  to read from. NOTE: 6809 is 64K only: $0000-$FFFF
//*****************************************************************************
// Returns:
//	uint8_t
//*****************************************************************************
uint8_t Mc6809::Fetch(const uint16_t address)
{
	if (instruction_lo == 0x10 || instruction_lo == 0x11)
		instruction_hi = instruction_lo;
	else
		instruction_hi = 0x00;

	instruction_lo = Read(reg_PC);

	++reg_PC;
	return(0);
}


//*********************************************************************************************************************************
// INTERNAL - Emulator only: not part of the CPU/MPU emulated op-code or
//			external control system
//*****************************************************************************


uint8_t Mc6809::Read(const uint16_t address, const bool readOnly)
{
	if (mmu != nullptr)
		data_lo = mmu->Read(address, readOnly);
	else
		data = 0;
	return(data_lo);
}


void Mc6809::Write(const uint16_t address, const uint8_t byte)
{
	if (mmu != nullptr)
		mmu->Write(address, byte);
}


void Mc6809::SetMMU(MMU* device)
{
	mmu = device;
}


//*********************************************************************************************************************************
