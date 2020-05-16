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
#pragma once

#include <string>
#include <vector>
#include "CPU.h"
#include "MMU.h"

#define MC6809E

class Cpu6809 //: public CPU
{
private:
	MMU* bus;

	uint8_t (Cpu6809::*exec)();						// Mnemonic function from interpreted Opcode
	uint8_t (Cpu6809::*mode)(uint8_t adjClock);	// addressing mode of the oppcode			- XXX *** XXX


	union
	{
		struct
		{
			uint8_t opcode_lo;		// scratch opcode storage low byte	(INTERNAL CPU USE ONLY)
			uint8_t opcode_hi;		// scratch opcode storage hi byte	(INTERNAL CPU USE ONLY)
		};
		uint16_t opcode;			// scratch opcode storage			(INTERNAL CPU USE ONLY)
	};
	union
	{
		struct
		{
			uint8_t offset_lo;		// scratch offset storage low byte	(INTERNAL CPU USE ONLY)
			uint8_t offset_hi;		// scratch offset storage hi byte	(INTERNAL CPU USE ONLY)
		};
		uint16_t offset;			// scratch offset storage			(INTERNAL CPU USE ONLY)
	};

	enum CC : uint8_t
	{
		C = (1 << 0),	// Carry       - indicates that a carry or a borrow was generated from bit seven
		V = (1 << 1),	// Overflow    - indicates previous operation caused a signed arithmetic overflow.
		Z = (1 << 2),	// Zero        - indicates that the result of the previous operation was zero.
		N = (1 << 3),	// Negative    - indicates the most-significant bit of the result of the previous data operation.
		I = (1 << 4),	// IRQ Mask    - indicates if an IRQ will be recognized (1 for disabled, 0 for enabled)
		H = (1 << 5),	// Half Carry  - indicates that a carry was generated from bit three in the alu as a result of an 8-bit addition. This bit is undefined in all subtract-like instructions.
		F = (1 << 6),	// FIRQ Mask   - indicates if an FIRQ will be recognized  (1 for disabled, 0 for enabled)
		E = (1 << 7),	// Entire Flag - indicates All registers are stored on the stack during an interrupt ( 1 for all, 0 for CC and PC only)
	};

	enum REG : uint8_t
	{
		D = 0X00,
		X,
		Y,
		U,
		S,
		PC,
		A = 0x08,
		B,
		CC,
		DP
	};

protected:
	uint8_t clocksLeft;
	uint8_t clocksUsed;


	// Registers, Program/App accessible and internal to CPU only
	uint8_t reg_CC;				// Condition Code register		(Program Accessible)	0B
	uint8_t reg_DP;				// Direct Page register.		(Program Accessible)	0A
	union
	{
		struct
		{
			uint8_t reg_B;		// register B, low byte of D	(Program Accessible)	09
			uint8_t reg_A;		// register A, hi byte of D		(Program Accessible)	08
		};
		uint16_t reg_D;			// register D, combo of A and B	(Program Accessible)	00
	};

	union
	{
		struct
		{
			uint8_t X_lo;		// index register X low byte	(INTERNAL CPU USE ONLY)
			uint8_t X_hi;		// index register X hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_X;			// index register X				(Program Accessible)	01
	};
	union
	{
		struct
		{
			uint8_t Y_lo;		// index register X low byte	(INTERNAL CPU USE ONLY)
			uint8_t Y_hi;		// index register X hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_Y;			// index register Y				(Program Accessible)	02
	};
	union
	{
		struct
		{
			uint8_t U_lo;		// User Stack Pointer low byte		(INTERNAL CPU USE ONLY)
			uint8_t U_hi;		// User Stack Pointer hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_U;			// register U User Stack Pointer	(Program Accessible)	03
	};
	union
	{
		struct
		{
			uint8_t S_lo;		// System Stack Pounter low byte	(INTERNAL CPU USE ONLY)
			uint8_t S_hi;		// System Stack Pounter hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_S;			// register S, System Stack Pounter	(Program Accessible)	04
	};
	union
	{
		struct
		{
			uint8_t PC_lo;		// Program Counter low byte			(INTERNAL CPU USE ONLY)
			uint8_t PC_hi;		// Program Counter hi byte			(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_PC;		// Program Counter					(Program Accessible)	05
	};
	union
	{
		struct
		{
			uint8_t scratch_lo;		// scratch register low byte	(INTERNAL CPU USE ONLY)
			uint8_t scratch_hi;		// scratch register hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_scratch;		// scratch register				(INTERNAL CPU USE ONLY)
	};

	struct OPCODE
	{
		std::string name;
		uint8_t (Cpu6809::*opcode)();						// Mnemonic function from interpreted Opcode
		uint8_t minCycles;
		uint8_t pgmBytes;
		uint8_t (Cpu6809::*addrMode)(uint8_t adjClock);	// addressing mode of the oppcode
	};

	std::vector<Cpu6809::OPCODE> OpCodeP1;
	std::vector<OPCODE> OpCodeP2;
	std::vector<OPCODE> OpCodeP3;

	std::vector<OPCODE> table;

public:
	bool resetTriggered;
	bool nmiTriggered;
	bool firqTriggered;
	bool irqTriggered;
	bool haltTriggered;

private:
	enum ADDR_MODE : uint8_t
	{
		Illegal,

		Inherent,
		Immediate,
		Direct,
		Extended,
		Indexed,
		Relative,
		Register,

		IdxInherent,
		IdxImmediate,
		IdxDirect,
		IdxExtended,
		IdxRelative,
		IdxRegister,
	};
	uint8_t INH(uint8_t adjClock);
	uint8_t IMM(uint8_t adjClock);
	uint8_t DIR(uint8_t adjClock);
	uint8_t EXT(uint8_t adjClock);
	uint8_t IDX(uint8_t adjClock);
	uint8_t REL(uint8_t adjClock);
	uint8_t Ill(uint8_t adjClock);

protected:
	uint8_t Fetch(const uint16_t address);

	uint8_t HALT();			// hardware halt
	uint8_t RESET();		// hardware Reset
	uint8_t NMI();			// hardware NMI
	uint8_t FIRQ();			// hardware FIRQ
	uint8_t IRQ();			// hardware IRQ

	uint8_t SoftRESET();	// Reset				- undocumented opcode: $3E
	uint8_t SWI();			// Software Interrupt
	uint8_t SWI2();			// Software Interrupt 2
	uint8_t SWI3();			// Software Interrupt 3

	uint8_t RTI();			// Return from Interrupt

	uint8_t SYNC();			// Sync																***	To-Do, Needs Work
	uint8_t CWAI();			// Start an interrupt sequence, but then wait for an interrupt.		***	To-Do, Needs Work

	uint8_t BSR();			// Branch to subroutine
	uint8_t LBSR();			// Long Branch to Subroutine
	uint8_t JSR();			// Jump to Subrourine												***	To-Do, Needs Work
	uint8_t RTS();			// Return from Subroutine

	uint8_t BRA();			// Branch Always
	uint8_t LBRA();			// Long Branch Always
	uint8_t JMP();			// Jump to memory location											***	To-Do, Needs Work

	uint8_t BRN();			// Branch Never
	uint8_t LBRN();			// Long Branch Never
	uint8_t NOP();          // No Op, No Operation, Does nothing, affects PC only.

    uint8_t BCC();          // Branch on Carry Clear
    uint8_t LBCC();         // Long Branch on Carry Clear
    uint8_t BCS();          // Branch on Carry Set
    uint8_t LBCS();         // Long Branch on Carry Set
    uint8_t BEQ();          // Branch if Equal
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
    uint8_t BNE();          // Branch if Not Equal
    uint8_t LBNE();         // Long Branch if Not Equal (Z = 0)
    uint8_t BPL();          // Branch if Plus/Positive
    uint8_t LBPL();         // Long Branch if Plus/Positive
    uint8_t BVC();          // Branch if no overflow
    uint8_t LBVC();         // Long Branch if no overflow (V is clear)
    uint8_t BVS();          // Branch on overflow
    uint8_t LBVS();         // Long Branch on overflow (V is set)

							// (increment or decrement S by a given value. Use  an index register to load another)
	uint8_t LEAS();         // Load Effective Address S											***	To-Do, Needs Work
	uint8_t LEAU();         // Load Effective Address U											***	To-Do, Needs Work
	uint8_t LEAX();         // Load Effective Address X											***	To-Do, Needs Work
	uint8_t LEAY();         // Load Effective Address Y											***	To-Do, Needs Work
							// (increment or decrement S by a given value. Use  an index register to load another)

	uint8_t ABX();          // Add B to X
	uint8_t ASLA();         // Arithmetic Shift Left A (Logical Shift Left fill LSb with 0)
	uint8_t ASLB();         // Arithmetic Shift Left B (Logical Shift Left fill LSb with 0)
	uint8_t ASRA();         // Arithmetic Shift Right A (fill MSb with Sign bit)
	uint8_t ASRB();         // Arithmetic Shift Right B (fill MSb with Sign bit)
	uint8_t BITA();         // Bit Test on A with a specific value (by AND)						***	To-Do, Needs Work
	uint8_t BITB();         // Bit Test on B with a specific value (by AND)						***	To-Do, Needs Work
	uint8_t CLRA();         // Clear register A
	uint8_t CLRB();         // Clear register B
	uint8_t COMA();         // 1's Compliment A
	uint8_t COMB();         // 1's Compliment B
	uint8_t DAA();          // Decimal Adjust A													***	To-Do, Needs Work
	uint8_t DECA();         // Decrement A
	uint8_t DECB();         // Decrement B
	uint8_t EXG();          // Exchange any two registers of the same size						***	To-Do, Needs Work
	uint8_t INCA();         // Increment A
	uint8_t INCB();         // Increment B
	uint8_t LSLA();         // Logical Shift Left register A (LSb is loaded with 0)
	uint8_t LSLB();         // Logical Shift Left register B (LSb is loaded with 0)
	uint8_t LSRA();         // Logical Shift Right register A (MSb is loaded with 0)
	uint8_t LSRB();         // Logical Shift Right register B (MSb is loaded with 0)
	uint8_t MUL();          // Multiply register A * register B, store in register D			***	To-Do, Needs Work
	uint8_t NEGA();         // 2's Complement (negate) register A
	uint8_t NEGB();         // 2's Complement (negate) register B
	uint8_t ROLA();         // Rotate Register A Left one bit through the Carry flag (9 bit rotate)
	uint8_t ROLB();         // Rotate Register B Left one bit through the Carry flag (9 bit rotate)
	uint8_t RORA();         // Rotate Register A Right one bit through the Carry flag (9 bit rotate)
	uint8_t RORB();         // Rotate Register B Right one bit through the Carry flag (9 bit rotate)
	uint8_t SEX();          // Sign Extend B into A ( A = Sign bit set on B ? 0xFF : 0x00)
	uint8_t TFR();          // Transfer/Copy one register to another (of the same size)			***	To-Do, Needs Work
	uint8_t TSTA();         // Test Register A, adjust N and Z Condition codes based on content
	uint8_t TSTB();         // Test Register B, adjust N and Z Condition codes based on content

	uint8_t ADCA();         // Add to A + Carry													***	To-Do, Needs Work
	uint8_t ADCB();         // Add to A + Carry													***	To-Do, Needs Work
	uint8_t ADDA();         // Add to A															***	To-Do, Needs Work
	uint8_t ADDB();         // Add to B															***	To-Do, Needs Work
	uint8_t ADDD();         // Add to D (A << 8 | B)											***	To-Do, Needs Work
	uint8_t ANDA();         // And A															***	To-Do, Needs Work
	uint8_t ANDB();         // And B															***	To-Do, Needs Work
	uint8_t ANDCC();        // And Condition Codes (clear one or more flags)					***	To-Do, Needs Work
	uint8_t ASL();          // Arithmetic Shift Left Memory location (Logical Shift Left fill LSb with 0)			***	To-Do, Needs Work
	uint8_t ASR();          // Arithmetic Shift Right Memory location (fill MSb with Sign bit)						***	To-Do, Needs Work
	uint8_t CLR();          // Clear memory location																***	To-Do, Needs Work
	uint8_t CMPA();         // Compare register A to memory location or given value(CC H unaffected)				***	To-Do, Needs Work
	uint8_t CMPB();         // Compare register B to memory location or given value (CC H unaffected)				***	To-Do, Needs Work
	uint8_t CMPD();         // Compare register D ( A <<8 | B) to memory locations or given value (CC H unaffected)	***	To-Do, Needs Work
	uint8_t CMPS();         // Compare register S to memory locations or given value (CC H unaffected)				***	To-Do, Needs Work
	uint8_t CMPU();         // Compare register U to memory locations or given value (CC H unaffected)				***	To-Do, Needs Work
	uint8_t CMPX();         // Compare register X to memory locations or given value (CC H unaffected)				***	To-Do, Needs Work
	uint8_t CMPY();         // Compare register Y to memory locations or given value (CC H unaffected)				***	To-Do, Needs Work
	uint8_t COM();          // 1's Compliment Memory location (i.e. XOR A with 0x00 or 0xFF)						***	To-Do, Needs Work
	uint8_t DEC();          // Decrement Memory location										***	To-Do, Needs Work
	uint8_t EORA();         // Logical Exclusive OR register A									***	To-Do, Needs Work
	uint8_t EORB();         // Logical Exclusive OR register B									***	To-Do, Needs Work
	uint8_t INC();          // Increment Memory location										***	To-Do, Needs Work
	uint8_t LDA();          // Load Register A													***	To-Do, Needs Work
	uint8_t LDB();          // Load Register A													***	To-Do, Needs Work
	uint8_t LDD();          // Load Register D  ( A << 8 | B)									***	To-Do, Needs Work
	uint8_t LDS();          // Load Register S													***	To-Do, Needs Work
	uint8_t LDU();          // Load Register U													***	To-Do, Needs Work
	uint8_t LDX();          // Load Register X													***	To-Do, Needs Work
	uint8_t LDY();          // Load Register Y													***	To-Do, Needs Work
	uint8_t LSL();          // Logical Shift Left memory location (LSb is loaded with 0)		***	To-Do, Needs Work
	uint8_t LSR();          // Logical Shift Right memory location (MSb is loaded with 0)		***	To-Do, Needs Work
	uint8_t NEG();          // 2's Complement (negate) memory location							***	To-Do, Needs Work
	uint8_t ORA();          // Logical OR register A											***	To-Do, Needs Work
	uint8_t ORB();          // Logical OR register B											***	To-Do, Needs Work
	uint8_t ORCC();         // Logical OR Condition Codes (set one or more flags)				***	To-Do, Needs Work
	uint8_t PSHS();         // Push one or more registers onto the System Stack					***	To-Do, Needs Work
	uint8_t PSHU();         // Push one or more registers onto the User Stack					***	To-Do, Needs Work
	uint8_t PULS();         // Pull one or more registers from the System Stack					***	To-Do, Needs Work
	uint8_t PULU();         // Pull one or more registers from the User Stack					***	To-Do, Needs Work
	uint8_t ROL();          // Rotate memory location Left one bit through the Carry flag (9 bit rotate)			***	To-Do, Needs Work
	uint8_t ROR();          // Rotate memory location Right one bit through the Carry flag (9 bit rotate)			***	To-Do, Needs Work
	uint8_t SBCA();         // Subtract with carry (borrow) - register A						***	To-Do, Needs Work
	uint8_t SBCB();         // Subtract with carry (borrow) - register B						***	To-Do, Needs Work
	uint8_t STA();          // Store Register A													***	To-Do, Needs Work
	uint8_t STB();          // Store Register B 												***	To-Do, Needs Work
	uint8_t STD();          // Store Register D     (A << 8 | B)								***	To-Do, Needs Work
	uint8_t STS();          // Store Register S													***	To-Do, Needs Work
	uint8_t STU();          // Store Register U													***	To-Do, Needs Work
	uint8_t STX();          // Store Register x													***	To-Do, Needs Work
	uint8_t STY();          // Store Register Y													***	To-Do, Needs Work
	uint8_t SUBA();         // Subtract from register A											***	To-Do, Needs Work
	uint8_t SUBB();         // Subtract from register A											***	To-Do, Needs Work
	uint8_t SUBD();         // Subtract from register D     (A << 8 | B)						***	To-Do, Needs Work
	uint8_t TST();          // Test memory location, adjust N and Z Condition codes based on content				***	To-Do, Needs Work

	uint8_t XXX();          // INVALID INSTRUCTION!											***	To-Do, Needs Work

public:
	Cpu6809(MMU* device = nullptr);
	~Cpu6809();

	void SetMMU(MMU* device);

	virtual void Clock();

	uint8_t Read(const uint16_t address, const bool readOnly = false);
	void Write(const uint16_t address, const uint8_t byte);

};

