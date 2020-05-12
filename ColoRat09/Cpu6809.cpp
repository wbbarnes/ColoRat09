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
	mode = nullptr;

	using op = Cpu6809;

	OpCodeP1 =
	{
		{0x00, "NEG"  , &op::NEG  , &op::DIR}, {0x01, "???"  , &op::XXX  , &op::Ill}, {0x02, "???"  , &op::XXX  , &op::Ill}, {0x03, "COM"  , &op::COM  , &op::DIR}, {0x04, "LSR"  , &op::LSR  , &op::DIR}, {0x05, "???"  , &op::XXX  , &op::Ill}, {0x06, "ROR"  , &op::ROR  , &op::DIR}, {0x07, "ASR"  , &op::ASR  , &op::DIR}, {0x08, "ASL"  , &op::ASL  , &op::DIR}, {0x09, "ROL"  , &op::ROL  , &op::DIR}, {0x0A, "DEC"  , &op::DEC  , &op::DIR}, {0x0B, "???"  , &op::XXX  , &op::Ill}, {0x0C, "INC"  , &op::INC  , &op::DIR}, {0x0D, "TST"  , &op::TST  , &op::DIR}, {0x0E, "JMP"  , &op::JMP  , &op::DIR}, {0x0F, "CLR"  , &op::CLR  , &op::DIR},
		{0x10, "pg2"  , nullptr   , nullptr }, {0x11, "pg3"  , nullptr   , nullptr }, {0x12, "Nop"  , &op::NOP  , &op::INH}, {0x13, "SYNC" , &op::SYNC , &op::INH}, {0x14, "???"  , &op::XXX  , &op::Ill}, {0x15, "???"  , &op::XXX  , &op::Ill}, {0x16, "LBRA" , &op::LBRA , &op::REL}, {0x17, "LBSR" , &op::LBSR , &op::REL}, {0x18, "???"  , &op::XXX  , &op::Ill}, {0x19, "DAA"  , &op::DAA  , &op::INH}, {0x1A, "ORCC" , &op::ORCC , &op::IMM}, {0x1B, "???"  , &op::XXX  , &op::Ill}, {0x1C, "ANDCC", &op::ANDCC, &op::IMM}, {0x1D, "SEX"  , &op::SEX  , &op::INH}, {0x1E, "EXG"  , &op::EXG  , &op::IMM}, {0x1F, "TFR"  , &op::TFR  , &op::IMM},
		{0x20, "BRA"  , &op::BRA  , &op::REL}, {0x21, "BRN"  , &op::BRN  , &op::REL}, {0x22, "BHI"  , &op::BHI  , &op::REL}, {0x23, "BLS"  , &op::BLS  , &op::REL}, {0x24, "BCC"  , &op::BCC  , &op::REL}, {0x25, "BCS"  , &op::BCS  , &op::REL}, {0x26, "BNE"  , &op::BNE  , &op::REL}, {0x27, "BEQ"  , &op::BEQ  , &op::REL}, {0x28, "BVC"  , &op::BVC  , &op::REL}, {0x29, "BVS"  , &op::BVS  , &op::REL}, {0x2A, "BPL"  , &op::BPL  , &op::REL}, {0x2B, "BMI"  , &op::BMI  , &op::REL}, {0x2C, "BGE"  , &op::BGE  , &op::REL}, {0x2D, "BLT"  , &op::BLT  , &op::REL}, {0x2E, "BGT"  , &op::BGT  , &op::REL}, {0x2F, "BLE"  , &op::BLE  , &op::REL},
		{0x30, "LEAX" , &op::LEAX , &op::IDX}, {0x31, "LEAY" , &op::LEAY , &op::IDX}, {0x32, "LEAS" , &op::LEAS , &op::IDX}, {0x33, "LEAU" , &op::LEAU , &op::IDX}, {0x34, "PSHS" , &op::PSHS , &op::IMM}, {0x35, "PULS" , &op::PULS , &op::IMM}, {0x36, "PSHU" , &op::PSHU , &op::IMM}, {0x37, "PULU" , &op::PULU , &op::IMM}, {0x38, "???"  , &op::XXX  , &op::Ill}, {0x39, "RTS"  , &op::RTS  , &op::INH}, {0x3A, "ABX"  , &op::ABX  , &op::INH}, {0x3B, "RTI"  , &op::RTI  , &op::INH}, {0x3C, "CWAI" , &op::CWAI , &op::INH}, {0x3D, "MUL"  , &op::MUL  , &op::INH}, {0x3E, "RESET", &op::RESET, &op::INH}, {0x3F, "SWI"  , &op::SWI  , &op::INH},
		{0x40, "NEG"  , &op::NEG  , &op::INH}, {0x41, "???"  , &op::XXX  , &op::Ill}, {0x42, "???"  , &op::XXX  , &op::Ill}, {0x43, "COMA" , &op::COMA , &op::INH}, {0x44, "LSRA" , &op::LSRA , &op::INH}, {0x45, "???"  , &op::XXX  , &op::Ill}, {0x46, "RORA" , &op::RORA , &op::INH}, {0x47, "ASRA" , &op::ASRA , &op::INH}, {0x48, "ASLA(LSLA)" , &op::ASLA , &op::INH}, {0x49, "ROLA" , &op::ROLA , &op::INH}, {0x4A, "DECA" , &op::DECA , &op::INH}, {0x4B, "???"  , &op::XXX  , &op::Ill}, {0x4C, "INCA" , &op::INCA , &op::INH}, {0x4D, "TSTA" , &op::TSTA , &op::INH}, {0x4E, "???"  , &op::XXX  , &op::Ill}, {0x4F, "CLRA" , &op::CLRA , &op::INH},
		{0x50, "NEGB" , &op::NEGB , &op::INH}, {0x51, "???"  , &op::XXX  , &op::Ill}, {0x52, "???"  , &op::XXX  , &op::Ill}, {0x53, "COMB" , &op::COMB , &op::INH}, {0x54, "LSRB" , &op::LSRB , &op::INH}, {0x55, "???"  , &op::XXX  , &op::Ill}, {0x56, "RORB" , &op::RORB , &op::INH}, {0x57, "ASRB" , &op::ASRB , &op::INH}, {0x58, "ASLB(LSLB)" , &op::ASLB , &op::INH}, {0x59, "ROLB" , &op::ROLB , &op::INH}, {0x5A, "DECB" , &op::DECB , &op::INH}, {0x5B, "???"  , &op::XXX  , &op::Ill}, {0x5C, "INCB" , &op::INCB , &op::INH}, {0x5D, "TSTB" , &op::TSTB , &op::INH}, {0x5E, "???"  , &op::XXX  , &op::Ill}, {0x5F, "CLRB" , &op::CLRB , &op::INH},
		{0x60, "NEG"  , &op::NEG  , &op::IDX}, {0x61, "???"  , &op::XXX  , &op::Ill}, {0x62, "???"  , &op::XXX  , &op::Ill}, {0x63, "COM"  , &op::COM  , &op::IDX}, {0x64, "LSR"  , &op::LSR  , &op::IDX}, {0x65, "???"  , &op::XXX  , &op::Ill}, {0x66, "ROR"  , &op::ROR  , &op::IDX}, {0x67, "ASR"  , &op::ASR  , &op::IDX}, {0x68, "ALS(LSL)"   , &op::ASL  , &op::IDX}, {0x69, "ROL"  , &op::ROL  , &op::IDX}, {0x6A, "DEC"  , &op::DEC  , &op::IDX}, {0x6B, "???"  , &op::XXX  , &op::Ill}, {0x6C, "INC"  , &op::INC  , &op::IDX}, {0x6D, "TST"  , &op::TST  , &op::IDX}, {0x6E, "JMP " , &op::JMP  , &op::IDX}, {0x6F, "CLR " , &op::CLR  , &op::IDX},
		{0x70, "NEG"  , &op::NEG  , &op::EXT}, {0x71, "???"  , &op::XXX  , &op::Ill}, {0x72, "???"  , &op::XXX  , &op::Ill}, {0x73, "COM"  , &op::COM  , &op::EXT}, {0x74, "LSR"  , &op::LSR  , &op::EXT}, {0x75, "???"  , &op::XXX  , &op::Ill}, {0x76, "ROR"  , &op::ROR  , &op::EXT}, {0x77, "ASR"  , &op::ASR  , &op::EXT}, {0x78, "ASL(LSL)"   , &op::ASL  , &op::EXT}, {0x79, "ROL"  , &op::ROL  , &op::EXT}, {0x7A, "DEC"  , &op::DEC  , &op::EXT}, {0x7B, "???"  , &op::XXX  , &op::Ill}, {0x7C, "INC"  , &op::INC  , &op::EXT}, {0x7D, "TST"  , &op::TST  , &op::EXT}, {0x7E, "JMP " , &op::JMP  , &op::EXT}, {0x7F, "CLR " , &op::CLR  , &op::EXT},
		{0x80, "SUBA" , &op::SUBA , &op::IMM}, {0x81, "CMPA" , &op::CMPA , &op::IMM}, {0x82, "SBCA" , &op::SBCA , &op::IMM}, {0x83, "SUBD" , &op::SUBD , &op::IMM}, {0x84, "ANDA" , &op::ANDA , &op::IMM}, {0x85, "BITA" , &op::BITA , &op::IMM}, {0x86, "LDA"  , &op::LDA  , &op::IMM}, {0x87, "???"  , &op::XXX  , &op::Ill}, {0x88, "EORA" , &op::EORA , &op::IMM}, {0x89, "ADCA" , &op::ADCA , &op::IMM}, {0x8A, "ORA"  , &op::ORA  , &op::IMM}, {0x8B, "ADDA" , &op::ADDA , &op::IMM}, {0x8C, "CMPX" , &op::CMPX , &op::IMM}, {0x8D, "BSR"  , &op::BSR  , &op::REL}, {0x8E, "LDX"  , &op::LDX  , &op::IMM}, {0x8F, "???"  , &op::XXX  , &op::Ill},
		{0x90, "SUBA" , &op::SUBA , &op::DIR}, {0x91, "CMPA" , &op::CMPA , &op::DIR}, {0x92, "SBCA" , &op::SBCA , &op::DIR}, {0x93, "SUBD" , &op::SUBD , &op::DIR}, {0x94, "ANDA" , &op::ANDA , &op::DIR}, {0x95, "BITA" , &op::BITA , &op::DIR}, {0x96, "LDA"  , &op::LDA  , &op::DIR}, {0x97, "STA"  , &op::STA  , &op::DIR}, {0x98, "EORA" , &op::EORA , &op::DIR}, {0x99, "ADCA" , &op::ADCA , &op::DIR}, {0x9A, "ORA"  , &op::ORA  , &op::DIR}, {0x9B, "ADDA" , &op::ADDA , &op::DIR}, {0x9C, "CMPX" , &op::CMPX , &op::DIR}, {0x9D, "JSR"  , &op::JSR  , &op::DIR}, {0x9E, "LDX"  , &op::LDX  , &op::DIR}, {0x9F, "STX"  , &op::STX  , &op::DIR},
		{0xA0, "SUBA" , &op::SUBA , &op::IDX}, {0xA1, "CMPA" , &op::CMPA , &op::IDX}, {0xA2, "SBCA" , &op::SBCA , &op::IDX}, {0xA3, "SUBD" , &op::SUBD , &op::IDX}, {0xA4, "ANDA" , &op::ANDA , &op::IDX}, {0xA5, "BITA" , &op::BITA , &op::IDX}, {0xA6, "LDA"  , &op::LDA  , &op::IDX}, {0xA7, "STA"  , &op::STA  , &op::IDX}, {0xA8, "EORA" , &op::EORA , &op::IDX}, {0xA9, "ADCA" , &op::ADCA , &op::IDX}, {0xAA, "ORA"  , &op::ORA  , &op::IDX}, {0xAB, "ADDA" , &op::ADDA , &op::IDX}, {0xAC, "CMPX" , &op::CMPX , &op::IDX}, {0xAD, "JSR"  , &op::JSR  , &op::IDX}, {0xAE, "LDX"  , &op::LDX  , &op::IDX}, {0xAF, "STX"  , &op::STX  , &op::IDX},
		{0xB0, "SUBA" , &op::SUBA , &op::EXT}, {0xB1, "CMPA" , &op::CMPA , &op::EXT}, {0xB2, "SBCA" , &op::SBCA , &op::EXT}, {0xB3, "SUBD" , &op::SUBD , &op::EXT}, {0xB4, "ANDA" , &op::ANDA , &op::EXT}, {0xB5, "BITA" , &op::BITA , &op::EXT}, {0xB6, "LDA"  , &op::LDA  , &op::EXT}, {0xB7, "STA"  , &op::STA  , &op::EXT}, {0xB8, "EORA" , &op::EORA , &op::EXT}, {0xB9, "ADCA" , &op::ADCA , &op::EXT}, {0xBA, "ORA"  , &op::ORA  , &op::EXT}, {0xBB, "ADDA" , &op::ADDA , &op::EXT}, {0xBC, "CMPX" , &op::CMPX , &op::EXT}, {0xBD, "JSR"  , &op::JSR  , &op::EXT}, {0xBE, "LDX"  , &op::LDX  , &op::EXT}, {0xBF, "STX"  , &op::STX  , &op::EXT},
		{0xC0, "SUBB" , &op::SUBB , &op::IMM}, {0xC1, "CMPB" , &op::CMPB , &op::IMM}, {0xC2, "SBCB" , &op::SBCB , &op::IMM}, {0xC3, "ADDD" , &op::ADDD , &op::IMM}, {0xC4, "ANDB" , &op::ANDB , &op::IMM}, {0xC5, "BITB" , &op::BITB , &op::IMM}, {0xC6, "LDB"  , &op::LDB  , &op::IMM}, {0xC7, "???"  , &op::XXX  , &op::Ill}, {0xC8, "EORB" , &op::EORB , &op::IMM}, {0xC9, "ADCB" , &op::ADCB , &op::IMM}, {0xCA, "ORB"  , &op::ORB  , &op::IMM}, {0xCB, "ADDB" , &op::ADDB , &op::IMM}, {0xCC, "LDD"  , &op::LDD  , &op::IMM}, {0xCD, "???"  , &op::XXX  , &op::Ill}, {0xCE, "LDU"  , &op::LDU  , &op::IMM}, {0xCF, "???"  , &op::XXX  , &op::Ill},
		{0xD0, "SUBB" , &op::SUBB , &op::DIR}, {0xD1, "CMPB" , &op::CMPB , &op::DIR}, {0xD2, "SBCB" , &op::SBCB , &op::DIR}, {0xD3, "ADDD" , &op::ADDD , &op::DIR}, {0xD4, "ANDB" , &op::ANDB , &op::DIR}, {0xD5, "BITB" , &op::BITB , &op::DIR}, {0xD6, "LDB"  , &op::LDB  , &op::DIR}, {0xD7, "STB"  , &op::STB  , &op::DIR}, {0xD8, "EORB" , &op::EORB , &op::DIR}, {0xD9, "ADCB" , &op::ADCB , &op::DIR}, {0xDA, "ORB"  , &op::ORB  , &op::DIR}, {0xDB, "STD"  , &op::STD  , &op::DIR}, {0xDC, "LDD"  , &op::LDD  , &op::DIR}, {0xDD, "STD"  , &op::STD  , &op::DIR}, {0xDE, "LDU"  , &op::LDU  , &op::DIR}, {0xDF, "STU"  , &op::STU  , &op::DIR},
		{0xE0, "SUBB" , &op::SUBB , &op::IDX}, {0xE1, "CMPB" , &op::CMPB , &op::IDX}, {0xE2, "SBCB" , &op::SBCB , &op::IDX}, {0xE3, "ADDD" , &op::ADDD , &op::IDX}, {0xE4, "ANDB" , &op::ANDB , &op::IDX}, {0xE5, "BITB" , &op::BITB , &op::IDX}, {0xE6, "LDB"  , &op::LDB  , &op::IDX}, {0xE7, "STB"  , &op::STB  , &op::IDX}, {0xE8, "EORB" , &op::EORB , &op::IDX}, {0xE9, "ADCB" , &op::ADCB , &op::IDX}, {0xEA, "ORB"  , &op::ORB  , &op::IDX}, {0xEB, "ADDB" , &op::ADDB , &op::IDX}, {0xEC, "LDD"  , &op::LDD  , &op::IDX}, {0xED, "STD"  , &op::STD  , &op::IDX}, {0xEE, "LDU"  , &op::LDU  , &op::IDX}, {0xEF, "STU"  , &op::STU  , &op::IDX},
		{0xF0, "SUBB" , &op::SUBB , &op::EXT}, {0xF1, "CMPB" , &op::CMPB , &op::EXT}, {0xF2, "SBCB" , &op::SBCB , &op::EXT}, {0xF3, "ADDD" , &op::ADDD , &op::EXT}, {0xF4, "ANDB" , &op::ANDB , &op::EXT}, {0xF5, "BITB" , &op::BITB , &op::EXT}, {0xF6, "LDB"  , &op::LDB  , &op::EXT}, {0xF7, "STB"  , &op::STB  , &op::EXT}, {0xF8, "EORB" , &op::EORB , &op::EXT}, {0xF9, "ADCB" , &op::ADCB , &op::EXT}, {0xFA, "ORB"  , &op::ORB  , &op::EXT}, {0xFB, "ADDB" , &op::ADDB , &op::EXT}, {0xFC, "LDD"  , &op::LDD  , &op::EXT}, {0xFD, "STD"  , &op::STD  , &op::EXT}, {0xFE, "LDU"  , &op::LDU  , &op::EXT}, {0xFF, "STU"  , &op::STU  , &op::EXT},

	};
	OpCodeP2 =
	{
		{0X00, "???"  , &op::XXX  , &op::Ill}, {0X01, "???"  , &op::XXX  , &op::Ill}, {0X02, "???"  , &op::XXX  , &op::Ill}, {0X03, "???"  , &op::XXX  , &op::Ill}, {0X04, "???"  , &op::XXX  , &op::Ill}, {0X05, "???"  , &op::XXX  , &op::Ill}, {0X06, "???"  , &op::XXX  , &op::Ill}, {0X07, "???"  , &op::XXX  , &op::Ill}, {0X08, "???"  , &op::XXX  , &op::Ill}, {0X09, "???"  , &op::XXX  , &op::Ill}, {0X0A, "???"  , &op::XXX  , &op::Ill}, {0X0B, "???"  , &op::XXX  , &op::Ill}, {0X0C, "???"  , &op::XXX  , &op::Ill}, {0X0D, "???"  , &op::XXX  , &op::Ill}, {0X0E, "???"  , &op::XXX  , &op::Ill}, {0X0F, "???"  , &op::XXX  , &op::Ill},
		{0X10, "???"  , &op::XXX  , &op::Ill}, {0X11, "???"  , &op::XXX  , &op::Ill}, {0X12, "???"  , &op::XXX  , &op::Ill}, {0X13, "???"  , &op::XXX  , &op::Ill}, {0X14, "???"  , &op::XXX  , &op::Ill}, {0X15, "???"  , &op::XXX  , &op::Ill}, {0X16, "???"  , &op::XXX  , &op::Ill}, {0X17, "???"  , &op::XXX  , &op::Ill}, {0X18, "???"  , &op::XXX  , &op::Ill}, {0X19, "???"  , &op::XXX  , &op::Ill}, {0X1A, "???"  , &op::XXX  , &op::Ill}, {0X1B, "???"  , &op::XXX  , &op::Ill}, {0X1C, "???"  , &op::XXX  , &op::Ill}, {0X1D, "???"  , &op::XXX  , &op::Ill}, {0X1E, "???"  , &op::XXX  , &op::Ill}, {0X1F, "???"  , &op::XXX  , &op::Ill},
		{0X20, "???"  , &op::XXX  , &op::Ill}, {0X21, "LBRN" , &op::LBRN , &op::REL}, {0X22, "LBHI" , &op::LBHI , &op::REL}, {0X23, "LBLS" , &op::LBLS , &op::REL}, {0X24, "LBHS" , &op::LBHS , &op::REL}, {0X25, "LBLO" , &op::LBLO , &op::REL}, {0X26, "LBNE" , &op::LBNE , &op::REL}, {0X27, "LBEQ" , &op::LBEQ , &op::REL}, {0X28, "LBVC" , &op::LBVC , &op::REL}, {0X29, "LBVS" , &op::LBVS , &op::REL}, {0X2A, "LBPL" , &op::LBPL , &op::REL}, {0X2B, "LBMI" , &op::LBMI , &op::REL}, {0X2C, "LBGE" , &op::LBGE , &op::REL}, {0X2D, "LBLT" , &op::LBLT , &op::REL}, {0X2E, "LBGT" , &op::LBGT , &op::REL}, {0X2F, "LBLE" , &op::LBLE , &op::REL},
		{0X30, "???"  , &op::XXX  , &op::Ill}, {0X31, "???"  , &op::XXX  , &op::Ill}, {0X32, "???"  , &op::XXX  , &op::Ill}, {0X33, "???"  , &op::XXX  , &op::Ill}, {0X34, "???"  , &op::XXX  , &op::Ill}, {0X35, "???"  , &op::XXX  , &op::Ill}, {0X36, "???"  , &op::XXX  , &op::Ill}, {0X37, "???"  , &op::XXX  , &op::Ill}, {0X38, "???"  , &op::XXX  , &op::Ill}, {0X39, "???"  , &op::XXX  , &op::Ill}, {0X3A, "???"  , &op::XXX  , &op::Ill}, {0X3B, "???"  , &op::XXX  , &op::Ill}, {0X3C, "???"  , &op::XXX  , &op::Ill}, {0X3D, "???"  , &op::XXX  , &op::Ill}, {0X3E, "???"  , &op::XXX  , &op::Ill}, {0X3F, "SWI2" , &op::SWI2 , &op::INH},
		{0X40, "???"  , &op::XXX  , &op::Ill}, {0X41, "???"  , &op::XXX  , &op::Ill}, {0X42, "???"  , &op::XXX  , &op::Ill}, {0X43, "???"  , &op::XXX  , &op::Ill}, {0X44, "???"  , &op::XXX  , &op::Ill}, {0X45, "???"  , &op::XXX  , &op::Ill}, {0X46, "???"  , &op::XXX  , &op::Ill}, {0X47, "???"  , &op::XXX  , &op::Ill}, {0X48, "???"  , &op::XXX  , &op::Ill}, {0X49, "???"  , &op::XXX  , &op::Ill}, {0X4A, "???"  , &op::XXX  , &op::Ill}, {0X4B, "???"  , &op::XXX  , &op::Ill}, {0X4C, "???"  , &op::XXX  , &op::Ill}, {0X4D, "???"  , &op::XXX  , &op::Ill}, {0X4E, "???"  , &op::XXX  , &op::Ill}, {0X4F, "???"  , &op::XXX  , &op::Ill},
		{0X50, "???"  , &op::XXX  , &op::Ill}, {0X51, "???"  , &op::XXX  , &op::Ill}, {0X52, "???"  , &op::XXX  , &op::Ill}, {0X53, "???"  , &op::XXX  , &op::Ill}, {0X54, "???"  , &op::XXX  , &op::Ill}, {0X55, "???"  , &op::XXX  , &op::Ill}, {0X56, "???"  , &op::XXX  , &op::Ill}, {0X57, "???"  , &op::XXX  , &op::Ill}, {0X58, "???"  , &op::XXX  , &op::Ill}, {0X59, "???"  , &op::XXX  , &op::Ill}, {0X5A, "???"  , &op::XXX  , &op::Ill}, {0X5B, "???"  , &op::XXX  , &op::Ill}, {0X5C, "???"  , &op::XXX  , &op::Ill}, {0X5D, "???"  , &op::XXX  , &op::Ill}, {0X5E, "???"  , &op::XXX  , &op::Ill}, {0X5F, "???"  , &op::XXX  , &op::Ill},
		{0X60, "???"  , &op::XXX  , &op::Ill}, {0X61, "???"  , &op::XXX  , &op::Ill}, {0X62, "???"  , &op::XXX  , &op::Ill}, {0X63, "???"  , &op::XXX  , &op::Ill}, {0X64, "???"  , &op::XXX  , &op::Ill}, {0X65, "???"  , &op::XXX  , &op::Ill}, {0X66, "???"  , &op::XXX  , &op::Ill}, {0X67, "???"  , &op::XXX  , &op::Ill}, {0X68, "???"  , &op::XXX  , &op::Ill}, {0X69, "???"  , &op::XXX  , &op::Ill}, {0X6A, "???"  , &op::XXX  , &op::Ill}, {0X6B, "???"  , &op::XXX  , &op::Ill}, {0X6C, "???"  , &op::XXX  , &op::Ill}, {0X6D, "???"  , &op::XXX  , &op::Ill}, {0X6E, "???"  , &op::XXX  , &op::Ill}, {0X6F, "???"  , &op::XXX  , &op::Ill},
		{0X70, "???"  , &op::XXX  , &op::Ill}, {0X71, "???"  , &op::XXX  , &op::Ill}, {0X72, "???"  , &op::XXX  , &op::Ill}, {0X73, "???"  , &op::XXX  , &op::Ill}, {0X74, "???"  , &op::XXX  , &op::Ill}, {0X75, "???"  , &op::XXX  , &op::Ill}, {0X76, "???"  , &op::XXX  , &op::Ill}, {0X77, "???"  , &op::XXX  , &op::Ill}, {0X78, "???"  , &op::XXX  , &op::Ill}, {0X79, "???"  , &op::XXX  , &op::Ill}, {0X7A, "???"  , &op::XXX  , &op::Ill}, {0X7B, "???"  , &op::XXX  , &op::Ill}, {0X7C, "???"  , &op::XXX  , &op::Ill}, {0X7D, "???"  , &op::XXX  , &op::Ill}, {0X7E, "???"  , &op::XXX  , &op::Ill}, {0X7F, "???"  , &op::XXX  , &op::Ill},
		{0X80, "???"  , &op::XXX  , &op::Ill}, {0X81, "???"  , &op::XXX  , &op::Ill}, {0X82, "???"  , &op::XXX  , &op::Ill}, {0X83, "CMPD" , &op::CMPD , &op::IMM}, {0X84, "???"  , &op::XXX  , &op::Ill}, {0X85, "???"  , &op::XXX  , &op::Ill}, {0X86, "???"  , &op::XXX  , &op::Ill}, {0X87, "???"  , &op::XXX  , &op::Ill}, {0X88, "???"  , &op::XXX  , &op::Ill}, {0X89, "???"  , &op::XXX  , &op::Ill}, {0X8A, "???"  , &op::XXX  , &op::Ill}, {0X8B, "???"  , &op::XXX  , &op::Ill}, {0X8C, "CMPY" , &op::CMPY , &op::IMM}, {0X8D, "???"  , &op::XXX  , &op::Ill}, {0X8E, "LDY"  , &op::LDY  , &op::IMM}, {0X8F, "???"  , &op::XXX  , &op::Ill},
		{0X90, "???"  , &op::XXX  , &op::Ill}, {0X91, "???"  , &op::XXX  , &op::Ill}, {0X92, "???"  , &op::XXX  , &op::Ill}, {0X93, "CMPD" , &op::CMPD , &op::DIR}, {0X94, "???"  , &op::XXX  , &op::Ill}, {0X95, "???"  , &op::XXX  , &op::Ill}, {0X96, "???"  , &op::XXX  , &op::Ill}, {0X97, "???"  , &op::XXX  , &op::Ill}, {0X98, "???"  , &op::XXX  , &op::Ill}, {0X99, "???"  , &op::XXX  , &op::Ill}, {0X9A, "???"  , &op::XXX  , &op::Ill}, {0X9B, "???"  , &op::XXX  , &op::Ill}, {0X9C, "CMPY" , &op::CMPY , &op::DIR}, {0X9D, "???"  , &op::XXX  , &op::Ill}, {0X9E, "LDY"  , &op::LDY  , &op::DIR}, {0X9F, "STY"  , &op::STY  , &op::DIR},
		{0XA0, "???"  , &op::XXX  , &op::Ill}, {0XA1, "???"  , &op::XXX  , &op::Ill}, {0XA2, "???"  , &op::XXX  , &op::Ill}, {0XA3, "CMPD" , &op::CMPD , &op::IDX}, {0XA4, "???"  , &op::XXX  , &op::Ill}, {0XA5, "???"  , &op::XXX  , &op::Ill}, {0XA6, "???"  , &op::XXX  , &op::Ill}, {0XA7, "???"  , &op::XXX  , &op::Ill}, {0XA8, "???"  , &op::XXX  , &op::Ill}, {0XA9, "???"  , &op::XXX  , &op::Ill}, {0XAA, "???"  , &op::XXX  , &op::Ill}, {0XAB, "???"  , &op::XXX  , &op::Ill}, {0XAC, "CMPY" , &op::CMPY , &op::IDX}, {0XAD, "???"  , &op::XXX  , &op::Ill}, {0XAE, "LDY"  , &op::LDY  , &op::IDX}, {0XAF, "STY"  , &op::STY  , &op::IDX},
		{0XB0, "???"  , &op::XXX  , &op::Ill}, {0XB1, "???"  , &op::XXX  , &op::Ill}, {0XB2, "???"  , &op::XXX  , &op::Ill}, {0XB3, "CMPD" , &op::CMPD , &op::EXT}, {0XB4, "???"  , &op::XXX  , &op::Ill}, {0XB5, "???"  , &op::XXX  , &op::Ill}, {0XB6, "???"  , &op::XXX  , &op::Ill}, {0XB7, "???"  , &op::XXX  , &op::Ill}, {0XB8, "???"  , &op::XXX  , &op::Ill}, {0XB9, "???"  , &op::XXX  , &op::Ill}, {0XBA, "???"  , &op::XXX  , &op::Ill}, {0XBB, "???"  , &op::XXX  , &op::Ill}, {0XBC, "CMPY" , &op::CMPY , &op::EXT}, {0XBD, "???"  , &op::XXX  , &op::Ill}, {0XBE, "LDY"  , &op::LDY  , &op::EXT}, {0XBF, "STY"  , &op::STY  , &op::EXT},
		{0XC0, "???"  , &op::XXX  , &op::Ill}, {0XC1, "???"  , &op::XXX  , &op::Ill}, {0XC2, "???"  , &op::XXX  , &op::Ill}, {0XC3, "???"  , &op::XXX  , &op::Ill}, {0XC4, "???"  , &op::XXX  , &op::Ill}, {0XC5, "???"  , &op::XXX  , &op::Ill}, {0XC6, "???"  , &op::XXX  , &op::Ill}, {0XC7, "???"  , &op::XXX  , &op::Ill}, {0XC8, "???"  , &op::XXX  , &op::Ill}, {0XC9, "???"  , &op::XXX  , &op::Ill}, {0XCA, "???"  , &op::XXX  , &op::Ill}, {0XCB, "???"  , &op::XXX  , &op::Ill}, {0XCC, "???"  , &op::XXX  , &op::Ill}, {0XCD, "???"  , &op::XXX  , &op::Ill}, {0XCE, "LDS"  , &op::LDS  , &op::IMM}, {0XCF, "???"  , &op::XXX  , &op::Ill},
		{0XD0, "???"  , &op::XXX  , &op::Ill}, {0XD1, "???"  , &op::XXX  , &op::Ill}, {0XD2, "???"  , &op::XXX  , &op::Ill}, {0XD3, "???"  , &op::XXX  , &op::Ill}, {0XD4, "???"  , &op::XXX  , &op::Ill}, {0XD5, "???"  , &op::XXX  , &op::Ill}, {0XD6, "???"  , &op::XXX  , &op::Ill}, {0XD7, "???"  , &op::XXX  , &op::Ill}, {0XD8, "???"  , &op::XXX  , &op::Ill}, {0XD9, "???"  , &op::XXX  , &op::Ill}, {0XDA, "???"  , &op::XXX  , &op::Ill}, {0XDB, "???"  , &op::XXX  , &op::Ill}, {0XDC, "???"  , &op::XXX  , &op::Ill}, {0XDD, "???"  , &op::XXX  , &op::Ill}, {0XDE, "LDS"  , &op::LDS  , &op::DIR}, {0XDF, "STS"  , &op::STS  , &op::DIR},
		{0XE0, "???"  , &op::XXX  , &op::Ill}, {0XE1, "???"  , &op::XXX  , &op::Ill}, {0XE2, "???"  , &op::XXX  , &op::Ill}, {0XE3, "???"  , &op::XXX  , &op::Ill}, {0XE4, "???"  , &op::XXX  , &op::Ill}, {0XE5, "???"  , &op::XXX  , &op::Ill}, {0XE6, "???"  , &op::XXX  , &op::Ill}, {0XE7, "???"  , &op::XXX  , &op::Ill}, {0XE8, "???"  , &op::XXX  , &op::Ill}, {0XE9, "???"  , &op::XXX  , &op::Ill}, {0XEA, "???"  , &op::XXX  , &op::Ill}, {0XEB, "???"  , &op::XXX  , &op::Ill}, {0XEC, "???"  , &op::XXX  , &op::Ill}, {0XED, "???"  , &op::XXX  , &op::Ill}, {0XEE, "LDS"  , &op::LDS  , &op::IDX}, {0XEF, "STS"  , &op::STS  , &op::IDX},
		{0XF0, "???"  , &op::XXX  , &op::Ill}, {0XF1, "???"  , &op::XXX  , &op::Ill}, {0XF2, "???"  , &op::XXX  , &op::Ill}, {0XF3, "???"  , &op::XXX  , &op::Ill}, {0XF4, "???"  , &op::XXX  , &op::Ill}, {0XF5, "???"  , &op::XXX  , &op::Ill}, {0XF6, "???"  , &op::XXX  , &op::Ill}, {0XF7, "???"  , &op::XXX  , &op::Ill}, {0XF8, "???"  , &op::XXX  , &op::Ill}, {0XF9, "???"  , &op::XXX  , &op::Ill}, {0XFA, "???"  , &op::XXX  , &op::Ill}, {0XFB, "???"  , &op::XXX  , &op::Ill}, {0XFC, "???"  , &op::XXX  , &op::Ill}, {0XFD, "???"  , &op::XXX  , &op::Ill}, {0XFE, "LDS"  , &op::LDS  , &op::EXT}, {0XFF, "STS"  , &op::STS  , &op::EXT},
	};
	OpCodeP3 =
	{
		{0x00, "???"  , &op::XXX  , &op::Ill}, {0x01, "???"  , &op::XXX  , &op::Ill}, {0x02, "???"  , &op::XXX  , &op::Ill}, {0x03, "???"  , &op::XXX  , &op::Ill}, {0x04, "???"  , &op::XXX  , &op::Ill}, {0x05, "???"  , &op::XXX  , &op::Ill}, {0x06, "???"  , &op::XXX  , &op::Ill}, {0x07, "???"  , &op::XXX  , &op::Ill}, {0x08, "???"  , &op::XXX  , &op::Ill}, {0x09, "???"  , &op::XXX  , &op::Ill}, {0x0A, "???"  , &op::XXX  , &op::Ill}, {0x0B, "???"  , &op::XXX  , &op::Ill}, {0x0C, "???"  , &op::XXX  , &op::Ill}, {0x0D, "???"  , &op::XXX  , &op::Ill}, {0x0E, "???"  , &op::XXX  , &op::Ill}, {0x0F, "???"  , &op::XXX  , &op::Ill},
		{0x10, "???"  , &op::XXX  , &op::Ill}, {0x11, "???"  , &op::XXX  , &op::Ill}, {0x12, "???"  , &op::XXX  , &op::Ill}, {0x13, "???"  , &op::XXX  , &op::Ill}, {0x14, "???"  , &op::XXX  , &op::Ill}, {0x15, "???"  , &op::XXX  , &op::Ill}, {0x16, "???"  , &op::XXX  , &op::Ill}, {0x17, "???"  , &op::XXX  , &op::Ill}, {0x18, "???"  , &op::XXX  , &op::Ill}, {0x19, "???"  , &op::XXX  , &op::Ill}, {0x1A, "???"  , &op::XXX  , &op::Ill}, {0x1B, "???"  , &op::XXX  , &op::Ill}, {0x1C, "???"  , &op::XXX  , &op::Ill}, {0x1D, "???"  , &op::XXX  , &op::Ill}, {0x1E, "???"  , &op::XXX  , &op::Ill}, {0x1F, "???"  , &op::XXX  , &op::Ill},
		{0x20, "???"  , &op::XXX  , &op::Ill}, {0x21, "???"  , &op::XXX  , &op::Ill}, {0x22, "???"  , &op::XXX  , &op::Ill}, {0x23, "???"  , &op::XXX  , &op::Ill}, {0x24, "???"  , &op::XXX  , &op::Ill}, {0x25, "???"  , &op::XXX  , &op::Ill}, {0x26, "???"  , &op::XXX  , &op::Ill}, {0x27, "???"  , &op::XXX  , &op::Ill}, {0x28, "???"  , &op::XXX  , &op::Ill}, {0x29, "???"  , &op::XXX  , &op::Ill}, {0x2A, "???"  , &op::XXX  , &op::Ill}, {0x2B, "???"  , &op::XXX  , &op::Ill}, {0x2C, "???"  , &op::XXX  , &op::Ill}, {0x2D, "???"  , &op::XXX  , &op::Ill}, {0x2E, "???"  , &op::XXX  , &op::Ill}, {0x2F, "???"  , &op::XXX  , &op::Ill},
		{0x30, "???"  , &op::XXX  , &op::Ill}, {0x31, "???"  , &op::XXX  , &op::Ill}, {0x32, "???"  , &op::XXX  , &op::Ill}, {0x33, "???"  , &op::XXX  , &op::Ill}, {0x34, "???"  , &op::XXX  , &op::Ill}, {0x35, "???"  , &op::XXX  , &op::Ill}, {0x36, "???"  , &op::XXX  , &op::Ill}, {0x37, "???"  , &op::XXX  , &op::Ill}, {0x38, "???"  , &op::XXX  , &op::Ill}, {0x39, "???"  , &op::XXX  , &op::Ill}, {0x3A, "???"  , &op::XXX  , &op::Ill}, {0x3B, "???"  , &op::XXX  , &op::Ill}, {0x3C, "???"  , &op::XXX  , &op::Ill}, {0x3D, "???"  , &op::XXX  , &op::Ill}, {0x3E, "???"  , &op::XXX  , &op::Ill}, {0x3F, "SWI3" , &op::SWI3 , &op::INH},
		{0x40, "???"  , &op::XXX  , &op::Ill}, {0x41, "???"  , &op::XXX  , &op::Ill}, {0x42, "???"  , &op::XXX  , &op::Ill}, {0x43, "???"  , &op::XXX  , &op::Ill}, {0x44, "???"  , &op::XXX  , &op::Ill}, {0x45, "???"  , &op::XXX  , &op::Ill}, {0x46, "???"  , &op::XXX  , &op::Ill}, {0x47, "???"  , &op::XXX  , &op::Ill}, {0x48, "???"  , &op::XXX  , &op::Ill}, {0x49, "???"  , &op::XXX  , &op::Ill}, {0x4A, "???"  , &op::XXX  , &op::Ill}, {0x4B, "???"  , &op::XXX  , &op::Ill}, {0x4C, "???"  , &op::XXX  , &op::Ill}, {0x4D, "???"  , &op::XXX  , &op::Ill}, {0x4E, "???"  , &op::XXX  , &op::Ill}, {0x4F, "???"  , &op::XXX  , &op::Ill},
		{0x50, "???"  , &op::XXX  , &op::Ill}, {0x51, "???"  , &op::XXX  , &op::Ill}, {0x52, "???"  , &op::XXX  , &op::Ill}, {0x53, "???"  , &op::XXX  , &op::Ill}, {0x54, "???"  , &op::XXX  , &op::Ill}, {0x55, "???"  , &op::XXX  , &op::Ill}, {0x56, "???"  , &op::XXX  , &op::Ill}, {0x57, "???"  , &op::XXX  , &op::Ill}, {0x58, "???"  , &op::XXX  , &op::Ill}, {0x59, "???"  , &op::XXX  , &op::Ill}, {0x5A, "???"  , &op::XXX  , &op::Ill}, {0x5B, "???"  , &op::XXX  , &op::Ill}, {0x5C, "???"  , &op::XXX  , &op::Ill}, {0x5D, "???"  , &op::XXX  , &op::Ill}, {0x5E, "???"  , &op::XXX  , &op::Ill}, {0x5F, "???"  , &op::XXX  , &op::Ill},
		{0x60, "???"  , &op::XXX  , &op::Ill}, {0x61, "???"  , &op::XXX  , &op::Ill}, {0x62, "???"  , &op::XXX  , &op::Ill}, {0x63, "???"  , &op::XXX  , &op::Ill}, {0x64, "???"  , &op::XXX  , &op::Ill}, {0x65, "???"  , &op::XXX  , &op::Ill}, {0x66, "???"  , &op::XXX  , &op::Ill}, {0x67, "???"  , &op::XXX  , &op::Ill}, {0x68, "???"  , &op::XXX  , &op::Ill}, {0x69, "???"  , &op::XXX  , &op::Ill}, {0x6A, "???"  , &op::XXX  , &op::Ill}, {0x6B, "???"  , &op::XXX  , &op::Ill}, {0x6C, "???"  , &op::XXX  , &op::Ill}, {0x6D, "???"  , &op::XXX  , &op::Ill}, {0x6E, "???"  , &op::XXX  , &op::Ill}, {0x6F, "???"  , &op::XXX  , &op::Ill},
		{0x70, "???"  , &op::XXX  , &op::Ill}, {0x71, "???"  , &op::XXX  , &op::Ill}, {0x72, "???"  , &op::XXX  , &op::Ill}, {0x73, "???"  , &op::XXX  , &op::Ill}, {0x74, "???"  , &op::XXX  , &op::Ill}, {0x75, "???"  , &op::XXX  , &op::Ill}, {0x76, "???"  , &op::XXX  , &op::Ill}, {0x77, "???"  , &op::XXX  , &op::Ill}, {0x78, "???"  , &op::XXX  , &op::Ill}, {0x79, "???"  , &op::XXX  , &op::Ill}, {0x7A, "???"  , &op::XXX  , &op::Ill}, {0x7B, "???"  , &op::XXX  , &op::Ill}, {0x7C, "???"  , &op::XXX  , &op::Ill}, {0x7D, "???"  , &op::XXX  , &op::Ill}, {0x7E, "???"  , &op::XXX  , &op::Ill}, {0x7F, "???"  , &op::XXX  , &op::Ill},
		{0x80, "???"  , &op::XXX  , &op::Ill}, {0x81, "???"  , &op::XXX  , &op::Ill}, {0x82, "???"  , &op::XXX  , &op::Ill}, {0x83, "CMPU" , &op::CMPU , &op::IMM}, {0x84, "???"  , &op::XXX  , &op::Ill}, {0x85, "???"  , &op::XXX  , &op::Ill}, {0x86, "???"  , &op::XXX  , &op::Ill}, {0x87, "???"  , &op::XXX  , &op::Ill}, {0x88, "???"  , &op::XXX  , &op::Ill}, {0x89, "???"  , &op::XXX  , &op::Ill}, {0x8A, "???"  , &op::XXX  , &op::Ill}, {0x8B, "???"  , &op::XXX  , &op::Ill}, {0x8C, "CMPS" , &op::CMPS , &op::IMM}, {0x8D, "???"  , &op::XXX  , &op::Ill}, {0x8E, "???"  , &op::XXX  , &op::Ill}, {0x8F, "???"  , &op::XXX  , &op::Ill},
		{0x90, "???"  , &op::XXX  , &op::Ill}, {0x91, "???"  , &op::XXX  , &op::Ill}, {0x92, "???"  , &op::XXX  , &op::Ill}, {0x93, "CMPU" , &op::CMPU , &op::DIR}, {0x94, "???"  , &op::XXX  , &op::Ill}, {0x95, "???"  , &op::XXX  , &op::Ill}, {0x96, "???"  , &op::XXX  , &op::Ill}, {0x97, "???"  , &op::XXX  , &op::Ill}, {0x98, "???"  , &op::XXX  , &op::Ill}, {0x99, "???"  , &op::XXX  , &op::Ill}, {0x9A, "???"  , &op::XXX  , &op::Ill}, {0x9B, "???"  , &op::XXX  , &op::Ill}, {0x9C, "CMPS" , &op::CMPS , &op::DIR}, {0x9D, "???"  , &op::XXX  , &op::Ill}, {0x9E, "???"  , &op::XXX  , &op::Ill}, {0x9F, "???"  , &op::XXX  , &op::Ill},
		{0xA0, "???"  , &op::XXX  , &op::Ill}, {0xA1, "???"  , &op::XXX  , &op::Ill}, {0xA2, "???"  , &op::XXX  , &op::Ill}, {0xA3, "CMPU" , &op::CMPU , &op::IDX}, {0xA4, "???"  , &op::XXX  , &op::Ill}, {0xA5, "???"  , &op::XXX  , &op::Ill}, {0xA6, "???"  , &op::XXX  , &op::Ill}, {0xA7, "???"  , &op::XXX  , &op::Ill}, {0xA8, "???"  , &op::XXX  , &op::Ill}, {0xA9, "???"  , &op::XXX  , &op::Ill}, {0xAA, "???"  , &op::XXX  , &op::Ill}, {0xAB, "???"  , &op::XXX  , &op::Ill}, {0xAC, "CMPS" , &op::CMPS , &op::IDX}, {0xAD, "???"  , &op::XXX  , &op::Ill}, {0xAE, "???"  , &op::XXX  , &op::Ill}, {0xAF, "???"  , &op::XXX  , &op::Ill},
		{0xB0, "???"  , &op::XXX  , &op::Ill}, {0xB1, "???"  , &op::XXX  , &op::Ill}, {0xB2, "???"  , &op::XXX  , &op::Ill}, {0xB3, "CMPU" , &op::CMPU , &op::EXT}, {0xB4, "???"  , &op::XXX  , &op::Ill}, {0xB5, "???"  , &op::XXX  , &op::Ill}, {0xB6, "???"  , &op::XXX  , &op::Ill}, {0xB7, "???"  , &op::XXX  , &op::Ill}, {0xB8, "???"  , &op::XXX  , &op::Ill}, {0xB9, "???"  , &op::XXX  , &op::Ill}, {0xBA, "???"  , &op::XXX  , &op::Ill}, {0xBB, "???"  , &op::XXX  , &op::Ill}, {0xBC, "CMPS" , &op::CMPS , &op::EXT}, {0xBD, "???"  , &op::XXX  , &op::Ill}, {0xBE, "???"  , &op::XXX  , &op::Ill}, {0xBF, "???"  , &op::XXX  , &op::Ill},
		{0xC0, "???"  , &op::XXX  , &op::Ill}, {0xC1, "???"  , &op::XXX  , &op::Ill}, {0xC2, "???"  , &op::XXX  , &op::Ill}, {0xC3, "???"  , &op::XXX  , &op::Ill}, {0xC4, "???"  , &op::XXX  , &op::Ill}, {0xC5, "???"  , &op::XXX  , &op::Ill}, {0xC6, "???"  , &op::XXX  , &op::Ill}, {0xC7, "???"  , &op::XXX  , &op::Ill}, {0xC8, "???"  , &op::XXX  , &op::Ill}, {0xC9, "???"  , &op::XXX  , &op::Ill}, {0xCA, "???"  , &op::XXX  , &op::Ill}, {0xCB, "???"  , &op::XXX  , &op::Ill}, {0xCC, "???"  , &op::XXX  , &op::Ill}, {0xCD, "???"  , &op::XXX  , &op::Ill}, {0xCE, "???"  , &op::XXX  , &op::Ill}, {0xCF, "???"  , &op::XXX  , &op::Ill},
		{0xD0, "???"  , &op::XXX  , &op::Ill}, {0xD1, "???"  , &op::XXX  , &op::Ill}, {0xD2, "???"  , &op::XXX  , &op::Ill}, {0xD3, "???"  , &op::XXX  , &op::Ill}, {0xD4, "???"  , &op::XXX  , &op::Ill}, {0xD5, "???"  , &op::XXX  , &op::Ill}, {0xD6, "???"  , &op::XXX  , &op::Ill}, {0xD7, "???"  , &op::XXX  , &op::Ill}, {0xD8, "???"  , &op::XXX  , &op::Ill}, {0xD9, "???"  , &op::XXX  , &op::Ill}, {0xDA, "???"  , &op::XXX  , &op::Ill}, {0xDB, "???"  , &op::XXX  , &op::Ill}, {0xDC, "???"  , &op::XXX  , &op::Ill}, {0xDD, "???"  , &op::XXX  , &op::Ill}, {0xDE, "???"  , &op::XXX  , &op::Ill}, {0xDF, "???"  , &op::XXX  , &op::Ill},
		{0xE0, "???"  , &op::XXX  , &op::Ill}, {0xE1, "???"  , &op::XXX  , &op::Ill}, {0xE2, "???"  , &op::XXX  , &op::Ill}, {0xE3, "???"  , &op::XXX  , &op::Ill}, {0xE4, "???"  , &op::XXX  , &op::Ill}, {0xE5, "???"  , &op::XXX  , &op::Ill}, {0xE6, "???"  , &op::XXX  , &op::Ill}, {0xE7, "???"  , &op::XXX  , &op::Ill}, {0xE8, "???"  , &op::XXX  , &op::Ill}, {0xE9, "???"  , &op::XXX  , &op::Ill}, {0xEA, "???"  , &op::XXX  , &op::Ill}, {0xEB, "???"  , &op::XXX  , &op::Ill}, {0xEC, "???"  , &op::XXX  , &op::Ill}, {0xED, "???"  , &op::XXX  , &op::Ill}, {0xEE, "???"  , &op::XXX  , &op::Ill}, {0xEF, "???"  , &op::XXX  , &op::Ill},
		{0xF0, "???"  , &op::XXX  , &op::Ill}, {0xF1, "???"  , &op::XXX  , &op::Ill}, {0xF2, "???"  , &op::XXX  , &op::Ill}, {0xF3, "???"  , &op::XXX  , &op::Ill}, {0xF4, "???"  , &op::XXX  , &op::Ill}, {0xF5, "???"  , &op::XXX  , &op::Ill}, {0xF6, "???"  , &op::XXX  , &op::Ill}, {0xF7, "???"  , &op::XXX  , &op::Ill}, {0xF8, "???"  , &op::XXX  , &op::Ill}, {0xF9, "???"  , &op::XXX  , &op::Ill}, {0xFA, "???"  , &op::XXX  , &op::Ill}, {0xFB, "???"  , &op::XXX  , &op::Ill}, {0xFC, "???"  , &op::XXX  , &op::Ill}, {0xFD, "???"  , &op::XXX  , &op::Ill}, {0xFE, "???"  , &op::XXX  , &op::Ill}, {0xFF, "???"  , &op::XXX  , &op::Ill},
	};
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
		else
		{

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
	opcode_lo = Read(reg_PC);

	if (opcode_lo == 0x10)
	{
		table = OpCodeP2;
		opcode_hi = opcode_lo;
		++clocksUsed;
	}
	else if (opcode_lo == 0x11)
	{
		table = OpCodeP3;
		opcode_hi = opcode_lo;
		++clocksUsed;
	}
	else
	{
		exec = table[opcode_lo].opcode;
		mode = table[opcode_lo].addrMode;
		++clocksUsed;
	}
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
//	Software Reset.		- undocumented opcode: 0x3E
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
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
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
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
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
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
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
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
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
//	SYNC()
//*****************************************************************************
// Synchronize to interrupt
//
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
//	CWAI()
//*****************************************************************************
// Wait for Interrupt
//
// Mask reg_CC. Push all registers onto System Stack. Wait for interrupt.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes: All may be affected
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
		break;
	case 2:		// R	CC Mask				PC+1
		scratch_lo = Read(++reg_PC);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			PC+2
		reg_CC &= scratch_lo;
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
//	JSR()
//*****************************************************************************
//	Jump to Subroutine
//*****************************************************************************
// MODIFIES:
//		  Registers:	None
//	Condition Codes:	None
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
	if (mode == &Cpu6809::DIR)
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
	if (mode == &Cpu6809::EXT)
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
	if (mode == &Cpu6809::IDX)
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
//	JMP()
//*****************************************************************************
//	Jump to location in memory
//*****************************************************************************
// MODIFIES:
//		  Registers:	None
//	Condition Codes:	None
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
	if (mode == &Cpu6809::DIR)
	{
		switch (clocksUsed)
		{
		case 1:
		case 2:
		case 3:
			break;
		}
	}
	if (mode == &Cpu6809::EXT)
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
	if (mode == &Cpu6809::IDX)
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
// Branch never and Nop
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
//	Nop()
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
//	BCC()
//*****************************************************************************
// Branch on Carry Clear
//
// CC::C is not set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BCC()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBCC()
//*****************************************************************************
// Long Branch on Carry Clear
//
// CC::C is not set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBCC()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C) 
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BCS()
//*****************************************************************************
// Branch on Carry Set
//
// CC::C is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BCS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBCS()
//*****************************************************************************
// Long Branch on Carry Set
//
// CC::C is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBCS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BEQ()
//*****************************************************************************
// Branch if Equal 
//
// CC::Z is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BEQ()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) == CC::Z)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBEQ()
//*****************************************************************************
// Long Branch if Equal
//
// Reg_CC CC::Z is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBEQ()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) != CC::Z)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BGE()
//*****************************************************************************
// Branch if Greater or Equal (signed)
//
// CC::N and CC::V are either both set or both clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BGE()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ( ((reg_CC & CC::C) == CC::C) == ((reg_CC & CC::V) == CC::V) )
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBGE()
//*****************************************************************************
// Long Branch  if Greater or Equal
//
// CC::N and CC::V are either both set or both clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBGE()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) == CC::C) == ((reg_CC & CC::V) == CC::V)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BGT()
//*****************************************************************************
// Branch if Greater than (signed)
//
// CC::Z is clear  AND  CC::N and CC::V are either both set or both clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BGT()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ( (((reg_CC & CC::N) == CC::N) && ((reg_CC & CC::V) == CC::V)) && ((reg_CC & CC::Z) == 0))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LGBT()
//*****************************************************************************
// Long Branch if Greater than
//
// CC::Z is clear  AND  CC::N and CC::V are either both set or clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBGT()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if (!((((reg_CC & CC::N) == CC::N) && ((reg_CC & CC::V) == CC::V)) && ((reg_CC & CC::Z) == 0)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BHI()
//*****************************************************************************
// Branch if Higher (unsigned)
//
// both CC::C and CC::Z are clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BHI()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ( ((reg_CC & CC::C) != CC::C) && ((reg_CC & CC::Z) != CC::Z))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBHI()
//*****************************************************************************
// Long Branch if Higher
//
// reg_CC both CC::C and CC::Z are clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBHI()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) != CC::C) && ((reg_CC & CC::Z) != CC::Z)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BHS()
//*****************************************************************************
// Branch if higher or same (unsigned)
//
// CC::C is clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BHS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBHS()
//*****************************************************************************
// Long Branch if higher or same
//
// CC::C is clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBHS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BLE()
//*****************************************************************************
// Branch if Less than or Equal (signed)
//
// CC::Z is set  OR  either CC::N or CC::V (not both) is set.
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BLE()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if (((reg_CC & CC::Z) == CC::Z) || 
			(((reg_CC & CC::N) == CC::N) && (reg_CC & CC::V) != CC::V) || 
			(((reg_CC & CC::N) != CC::N) && (reg_CC & CC::V) == CC::V))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBLE()
//*****************************************************************************
// Long Branch if Less than or Equal (signed)
//
// CC::Z is set  OR  either CC::N or CC::V (not both) is set.
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBLE()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::Z) == CC::Z) ||
			(((reg_CC & CC::N) == CC::N) && (reg_CC & CC::V) != CC::V) ||
			(((reg_CC & CC::N) != CC::N) && (reg_CC & CC::V) == CC::V)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BLO()
//*****************************************************************************
// Branch if Lower (unsigned)
//
// CC::C is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BLO()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBLO()
//*****************************************************************************
// Long Branch if Lower (unsigned)
//
// CC::C is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBLO()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BLS()
//*****************************************************************************
// Branch if Lower or Same (unsigned)
//
// CC::C is set or CC::Z is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BLS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if (((reg_CC & CC::C) == CC::C) || ((reg_CC & CC::Z) == CC::Z))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBLS()
//*****************************************************************************
// Long Branch if Lower or Same (unsigned)
//
// CC::C is set or CC::Z is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBLS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) == CC::C) || ((reg_CC & CC::Z) == CC::Z)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BLT()
//*****************************************************************************
// Branch if less than (signed)
//
// either CC::N or CC::V is set, not both
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BLT()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if (((reg_CC & CC::C) == CC::C) != ((reg_CC & CC::N) == CC::N))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBLT()
//*****************************************************************************
// Long Branch if less than (signed)
//
// either CC::N or CC::V is set, not both
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBLT()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) == CC::C) != ((reg_CC & CC::N) == CC::N)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BMI()
//*****************************************************************************
// Branch on Minus 
//	
// CC::N is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BMI()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) == CC::N)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBMI()
//*****************************************************************************
// Long Branch on Minus
//	
// CC::N is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBMI()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) != CC::N)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BNE()
//*****************************************************************************
// Branch if Not Equal
//
//  CC::Z is not set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BNE()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) != CC::Z)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBNE()
//*****************************************************************************
// Long Branch if Not Equal
//
//  CC::Z is not set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBNE()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) == CC::Z)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BPL()
//*****************************************************************************
// Branch if Plus/Positive
//
// CC::N is clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BPL()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) != CC::N)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBPL()
//*****************************************************************************
// Long Branch if Plus/Positive
//
// CC::N is clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBPL()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) == CC::N)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BVC()
//*****************************************************************************
//	// Branch if no overflow
//
// CC::V is clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BVC()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) != CC::V)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBVC()
//*****************************************************************************
// Long Branch if no overflow
//
// CC::V is clear
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBVC()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) == CC::V)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BVS()
//*****************************************************************************
// Branch on overflow
//
// CC::V is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::BVS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Offset				PC+1
		scratch_lo = Read(++reg_PC);
		scratch_hi = ((offset_lo & 0x80) == 0x80) ? 0xff : 0x00;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) == CC::V)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBVS()
//*****************************************************************************
// Long Branch on overflow
//
// CC::V is set
//*****************************************************************************
// MODIFIES:
//		  Registers: potentially PC
//	Condition Codes: None
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::LBVS()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		++reg_PC;
		break;
	case 3:		// R	Offset High			PC+2
		scratch_hi = Read(++reg_PC);
		break;
	case 4:		// R	Offset Low			PC+3
		scratch_lo = Read(++reg_PC);
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) != CC::V)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Load Effective Address Mode opcodes
//*********************************************************************************************************************************

//*****************************************************************************
//	LEAS()
//*****************************************************************************
// Load Effective Address S		(system stack)
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
uint8_t Cpu6809::LEAS()
{
	return(clocksUsed);
}


//*****************************************************************************
//	LEAU()
//*****************************************************************************
// Load Effective Address U		(user stack)
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
uint8_t Cpu6809::LEAU()
{
	return(clocksUsed);
}


//*****************************************************************************
//	LEAX()
//*****************************************************************************
// Load Effective Address X		(index register X)
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
uint8_t Cpu6809::LEAX()
{
	return(clocksUsed);
}


//*****************************************************************************
//	LEAY()
//*****************************************************************************
// Load Effective Address Y		(index register Y)
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
uint8_t Cpu6809::LEAY()
{
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Inherent Address Mode opcodes
//*********************************************************************************************************************************

//*****************************************************************************
//	ABX()
//*****************************************************************************
//	Add B to X
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
uint8_t Cpu6809::ABX()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			(PC)
		break;
	case 2:		// R don't care				(PC+1)
		++reg_PC;
		break;
	case 3:
		reg_X += reg_B;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASLA()
//*****************************************************************************
// Arithmetic Shift Left A (Logical Shift Left fill LSb with 0)
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
uint8_t Cpu6809::ASLA()
{
	switch(++clocksUsed)
	{
	case 1:		// R Opcode Fetch			(PC)
		break;
	case 2:
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = reg_A << 1;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (((reg_A >> 6) & 1) != ((reg_A >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASLB()
//*****************************************************************************
// Arithmetic Shift Left B (Logical Shift Left fill LSb with 0)
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
uint8_t Cpu6809::ASLB()
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = reg_B << 1;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (((reg_B >> 6) & 1) != ((reg_B >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASRA()
//*****************************************************************************
// Arithmetic Shift Right A (fill MSb with Sign bit)
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
uint8_t Cpu6809::ASRA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = reg_A & 0x80;
		reg_CC = ((reg_A & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((reg_A >> 1) & 0x7f) | scratch_lo;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASRB()
//*****************************************************************************
// Arithmetic Shift Right B (fill MSb with Sign bit)
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
uint8_t Cpu6809::ASRB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = reg_B & 0x80;
		reg_CC = ((reg_B & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = ((reg_B >> 1) & 0x7f) | scratch_lo;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);

		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BITA()
//*****************************************************************************
// Bit Test on A with a specific value (by AND)
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
uint8_t Cpu6809::BITA()
{
	return(clocksUsed);
}


//*****************************************************************************
//	BITB()
//*****************************************************************************
// Bit Test on B with a specific value (by AND)
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
uint8_t Cpu6809::BITB()
{
	return(clocksUsed);
}


//*****************************************************************************
//	CLRA()
//*****************************************************************************
// Clear register/accumulator A
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
uint8_t Cpu6809::CLRA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_A = 0;
		reg_CC = (reg_CC & (CC::H | CC::I | CC::E | CC::F)) | CC::Z | CC::C;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CLRB()
//*****************************************************************************
// Clear register/accumulator B
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
uint8_t Cpu6809::CLRB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_B = 0;
		reg_CC = (reg_CC & (CC::H | CC::I | CC::E | CC::F)) | CC::Z | CC::C;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	COMA()
//*****************************************************************************
// 1's Compliment A (i.e. XOR A with 0x00 or 0xFF)
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
uint8_t Cpu6809::COMA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_A ^= 0xff;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	COMB()
//*****************************************************************************
// 1's Compliment B (i.e. XOR B with 0x00 or 0xFF)
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
uint8_t Cpu6809::COMB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_B ^= 0xff;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	DAA()
//*****************************************************************************
// Decimal Adjust A		(contents of A -> BCD... BCD operation should be prior)
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
uint8_t Cpu6809::DAA()
{
	uint8_t carry = 0;
	uint8_t cfLsn = 0;
	uint8_t cfMsn = 0;
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = reg_A & 0x0f;
		scratch_hi = (reg_A & 0xf0) >> 4;
		carry = reg_CC & CC::C;
		if (((reg_CC & CC::H) == CC::H) || (scratch_lo > 9))
			cfLsn = 6;
		if ((carry == CC::C) || (scratch_hi > 9) || ((scratch_hi > 8) && (scratch_lo > 9)))
			cfMsn = 6;

		reg_A += cfLsn;			// fixes lsn
		reg_A += (cfMsn << 4);	// fixes msn
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC = (cfMsn == 6 || carry) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);

		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	DECA()
//*****************************************************************************
//	Decrement accumulator A
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
uint8_t Cpu6809::DECA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		--reg_A;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	DECB()
//*****************************************************************************
//	Decrement accumulator B
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
uint8_t Cpu6809::DECB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		--reg_B;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	EXG()
//*****************************************************************************
// Exchange any two registers of the same size
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
uint8_t Cpu6809::EXG()
{
	uint8_t data_hi = 0;
	uint8_t data_lo = 0;
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Post scratch_lo			PC+1
		scratch_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		data_hi = scratch_lo & 0xf0;
		data_lo = scratch_lo & 0x0f;
		break;
	case 4:
		switch (data_hi)
		{
		case REG::A:
			scratch_lo = reg_A;
			switch (data_lo)
			{
			case REG::B:
				reg_A = reg_B;
				reg_B = scratch_lo;
				break;
			case REG::CC:
				reg_A = reg_CC;
				reg_CC = scratch_lo;
				break;
			case REG::DP:
				reg_A = reg_DP;
				reg_DP = scratch_lo;
				break;
			}
			break;
		case REG::B:
			scratch_lo = reg_A;
			switch (data_lo)
			{
			case REG::A:
				reg_B = reg_A;
				reg_A = scratch_lo;
				break;
			case REG::CC:
				reg_B = reg_CC;
				reg_CC = scratch_lo;
				break;
			case REG::DP:
				reg_B = reg_DP;
				reg_DP = scratch_lo;
				break;
			}
			break;
		}
		break;
	case 5:
		switch (data_hi)
		{
		case REG::CC:
			scratch_lo = reg_CC;
			switch (data_lo)
			{
			case REG::A:
				reg_CC = reg_A;
				reg_A = scratch_lo;
				break;
			case REG::B:
				reg_CC = reg_B;
				reg_B = scratch_lo;
				break;
			case REG::DP:
				reg_CC = reg_DP;
				reg_DP = scratch_lo;
				break;
			}
			break;
		case REG::DP:
			scratch_lo = reg_DP;
			switch (data_lo)
			{
			case REG::A:
				reg_DP = reg_A;
				reg_A = scratch_lo;
				break;
			case REG::B:
				reg_DP = reg_B;
				reg_B = scratch_lo;
				break;
			case REG::CC:
				reg_DP = reg_CC;
				reg_CC = scratch_lo;
				break;
			}
			break;
		}
		break;
	case 6:
		switch (data_hi)
		{
		case REG::X:
			reg_scratch = reg_X;
			switch (data_lo)
			{
			case REG::Y:
				reg_X = reg_Y;
				reg_Y = reg_scratch;
				break;
			case REG::U:
				reg_X = reg_U;
				reg_U = reg_scratch;
				break;
			case REG::S:
				reg_X = reg_S;
				reg_S = reg_scratch;
				break;
			case REG::D:
				reg_X = reg_D;
				reg_D = reg_scratch;
				break;
			case REG::PC:
				reg_X = reg_PC;
				reg_PC = reg_scratch;
				break;
			}
			break;
		case REG::Y:
			reg_scratch = reg_Y;
			switch (data_lo)
			{
			case REG::X:
				reg_Y = reg_X;
				reg_X = reg_scratch;
				break;
			case REG::U:
				reg_Y = reg_U;
				reg_U = reg_scratch;
				break;
			case REG::S:
				reg_Y = reg_S;
				reg_S = reg_scratch;
				break;
			case REG::D:
				reg_Y = reg_D;
				reg_D = reg_scratch;
				break;
			case REG::PC:
				reg_Y = reg_PC;
				reg_PC = reg_scratch;
				break;
			}
			break;
		}
		break;
	case 7:
		switch (data_hi)
		{
		case REG::U:
			reg_scratch = reg_U;
			switch (data_lo)
			{
			case REG::X:
				reg_U = reg_X;
				reg_X = reg_scratch;
				break;
			case REG::Y:
				reg_U = reg_Y;
				reg_Y = reg_scratch;
				break;
			case REG::S:
				reg_U = reg_S;
				reg_S = reg_scratch;
				break;
			case REG::D:
				reg_U = reg_D;
				reg_D = reg_scratch;
				break;
			case REG::PC:
				reg_U = reg_PC;
				reg_PC = reg_scratch;
				break;
			}
			break;
		case REG::S:
			reg_scratch = reg_S;
			switch (data_lo)
			{
			case REG::X:
				reg_S = reg_X;
				reg_X = reg_scratch;
				break;
			case REG::Y:
				reg_S = reg_Y;
				reg_Y = reg_scratch;
				break;
			case REG::U:
				reg_S = reg_U;
				reg_U = reg_scratch;
				break;
			case REG::D:
				reg_S = reg_D;
				reg_D = reg_scratch;
				break;
			case REG::PC:
				reg_S = reg_PC;
				reg_PC = reg_scratch;
				break;
			}
			break;
		}
		break;
	case 8:
		switch (data_hi)
		{
		case REG::D:
			reg_scratch = reg_D;
			switch (data_lo)
			{
			case REG::X:
				reg_D = reg_PC;
				reg_PC = reg_scratch;
				break;
			case REG::Y:
				reg_D = reg_Y;
				reg_Y = reg_scratch;
				break;
			case REG::U:
				reg_D = reg_U;
				reg_U = reg_scratch;
				break;
			case REG::S:
				reg_D = reg_S;
				reg_S = reg_scratch;
				break;
			case REG::PC:
				reg_D = reg_PC;
				reg_PC = reg_scratch;
				break;
			}
			break;
		case REG::PC:
			reg_scratch = reg_PC;
			switch (data_lo)
			{
			case REG::X:
				reg_PC = reg_X;
				reg_X = reg_scratch;
				break;
			case REG::Y:
				reg_PC = reg_Y;
				reg_Y = reg_scratch;
				break;
			case REG::U:
				reg_PC = reg_U;
				reg_U = reg_scratch;
				break;
			case REG::S:
				reg_PC = reg_S;
				reg_S = reg_scratch;
				break;
			case REG::D:
				reg_PC = reg_D;
				reg_D = reg_scratch;
				break;
			}
			break;
		}
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	INCA()
//*****************************************************************************
// Increment accumulator A
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
uint8_t Cpu6809::INCA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		++reg_A;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	INCB()
//*****************************************************************************
// Increment accumulator B
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
uint8_t Cpu6809::INCB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		++reg_B;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSLA()
//*****************************************************************************
 // Logical Shift Left register A (LSb is loaded with 0)
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
uint8_t Cpu6809::LSLA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_CC = (((reg_A >> 6) & 1) != ((reg_A >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_A = reg_A << 1;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSLB()
//*****************************************************************************
// Logical Shift Left register B (LSb is loaded with 0)
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
uint8_t Cpu6809::LSLB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_CC = (((reg_B >> 6) & 1) != ((reg_B >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_B = reg_B << 1;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSRA()
//*****************************************************************************
// Logical Shift Right register A (MSb is loaded with 0)
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
uint8_t Cpu6809::LSRA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = reg_A & 0x80;
		reg_CC = ((reg_A & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((reg_A >> 1) & 0x7f);
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC &= ~CC::N;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSRB()
//*****************************************************************************
// Logical Shift Right register B (MSb is loaded with 0)
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
uint8_t Cpu6809::LSRB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = reg_B & 0x80;
		reg_CC = ((reg_B & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = ((reg_B >> 1) & 0x7f);
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC &= ~CC::N;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	MUL()
//*****************************************************************************
// Multiply register A * register B, store in register D (A << 8 | B)
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
uint8_t Cpu6809::MUL()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		reg_D = reg_A * reg_B;
		break;
	case 4:		// R	Don't Care			$ffff
		reg_CC = (reg_D == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		break;
	case 5:		// R	Don't Care			$ffff
		reg_CC = ((reg_D & 0x0080) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		break;
	case 6:		// R	Don't Care			$ffff
		break;
	case 7:		// R	Don't Care			$ffff
		break;
	case 8:		// R	Don't Care			$ffff
		break;
	case 9:		// R	Don't Care			$ffff
		break;
	case 10:	// R	Don't Care			$ffff
		break;
	case 11:	// R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NEGA()
//*****************************************************************************
// 2's Complement (negate) register A
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
uint8_t Cpu6809::NEGA()
{
	switch(++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (reg_A == 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		reg_A = 0 - reg_A;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NEGB()
//*****************************************************************************
// 2's Complement (negate) register B
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
uint8_t Cpu6809::NEGB()         // 2's Complement (negate) register B
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (reg_B == 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		reg_B = 0 - reg_B;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROLA()
//*****************************************************************************
// Rotate Register A Left one bit through the Carry flag (9 bit rotate)
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
uint8_t Cpu6809::ROLA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = ((reg_CC & CC::C) != 0) ? 1 : 0;
		reg_CC = (((reg_A >> 6) & 1) != ((reg_A >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = (reg_A << 1) | scratch_lo;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROLB()
//*****************************************************************************
// Rotate Register B Left one bit through the Carry flag (9 bit rotate)
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
uint8_t Cpu6809::ROLB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = ((reg_CC & CC::C) != 0) ? 1 : 0;
		reg_CC = (((reg_B >> 6) & 1) != ((reg_B >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = (reg_B << 1) | scratch_lo;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RORA()
//*****************************************************************************
// Rotate Register A Right one bit through the Carry flag (9 bit rotate)
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
uint8_t Cpu6809::RORA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((reg_A & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = (reg_A >> 1) | scratch_lo;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RORB()
//*****************************************************************************
// Rotate Register B Right one bit through the Carry flag (9 bit rotate)
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
uint8_t Cpu6809::RORB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		scratch_lo = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((reg_B & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = (reg_B >> 1) | scratch_lo;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SEX()
//*****************************************************************************
// Sign Extend B into A ( A = Sign bit set on B ? 0xFF : 0x00 )
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
uint8_t Cpu6809::SEX()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
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
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TFR()
//*****************************************************************************
// Transfer/Copy one register to another (of the same size)
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
uint8_t Cpu6809::TFR()
{
	switch(++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Post Byte			PC+1
		scratch_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		scratch_hi = (scratch_lo & 0xf0) >> 4;
		scratch_lo &= 0x0f;
		break;
	case 4:		// R	Don't Care			$ffff
		switch (scratch_hi)
		{
		case REG::A:
			switch (scratch_lo)
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
			switch (scratch_lo)
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
			switch (scratch_lo)
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
			switch (scratch_lo)
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
	case 5:		// R	Don't Care			$ffff
		switch (scratch_hi)
		{
		case REG::D:
			switch (scratch_lo)
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
			switch (scratch_lo)
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
			switch (scratch_lo)
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
	case 6:		// R	Don't Care			$ffff
		switch (scratch_hi)
		{
		case REG::U:
			switch (scratch_lo)
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
			switch (scratch_lo)
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
			switch (scratch_lo)
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
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TSTA()
//*****************************************************************************
// Test Register A, adjust N and Z Condition codes based on content
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
uint8_t Cpu6809::TSTA()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC &= ~CC::V;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TSTB()
//*****************************************************************************
// Test Register B, adjust N and Z Condition codes based on content
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
uint8_t Cpu6809::TSTB()
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Don't Care			PC+1
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC &= ~CC::V;
		++reg_PC;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


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
uint8_t Cpu6809::ADCA()         // Add to A + Carry
{ return(255); }


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
uint8_t Cpu6809::ADCB()         // Add to A + Carry
{ return(255); }


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
uint8_t Cpu6809::ADDA()         // Add to A
{ return(255); }


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
uint8_t Cpu6809::ADDB()         // Add to B
{ return(255); }


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
uint8_t Cpu6809::ADDD()         // Add to D (A << 8 | B)
{ return(255); }


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
uint8_t Cpu6809::ANDA()         // And A
{ return(255); }


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
uint8_t Cpu6809::ANDB()         // And B
{ return(255); }


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
uint8_t Cpu6809::ANDCC()        // And Condition Codes (clear one or more flags)
{ return(255); }


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
uint8_t Cpu6809::ASL()          // Arithmetic Shift Left Memory location (Logical Shift Left fill LSb with 0)
{ return(255); }


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
uint8_t Cpu6809::ASR()          // Arithmetic Shift Right Memory location (fill MSb with Sign bit)
{ return(255); }


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
uint8_t Cpu6809::CLR()          // Clear memory location
{ return(255); }


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
uint8_t Cpu6809::CMPA()         // Compare register A to memory location or given value(CC H unaffected)
{ return(255); }


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
uint8_t Cpu6809::CMPB()         // Compare register B to memory location or given value (CC H unaffected)
{ return(255); }


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
uint8_t Cpu6809::CMPD()         // Compare register D ( A <<8 | B) to memory locations or given value (CC H unaffected)
{ return(255); }


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
uint8_t Cpu6809::CMPS()         // Compare register S to memory locations or given value (CC H unaffected)
{ return(255); }


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
uint8_t Cpu6809::CMPU()         // Compare register U to memory locations or given value (CC H unaffected)
{ return(255); }


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
uint8_t Cpu6809::CMPX()         // Compare register X to memory locations or given value (CC H unaffected)
{ return(255); }


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
uint8_t Cpu6809::CMPY()         // Compare register Y to memory locations or given value (CC H unaffected)
{ return(255); }


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
uint8_t Cpu6809::COM()          // 1's Compliment Memory location (i.e. XOR A with 0x00 or 0xFF)
{ return(255); }


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
uint8_t Cpu6809::DEC()          // Decrement Memory location
{ return(255); }


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
uint8_t Cpu6809::EORA()         // Logical Exclusive OR register A
{ return(255); }


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
uint8_t Cpu6809::EORB()         // Logical Exclusive OR register B
{ return(255); }


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
uint8_t Cpu6809::INC()          // Increment Memory location
{ return(255); }


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
uint8_t Cpu6809::LDA()          // Load Register A
{ return(255); }


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
uint8_t Cpu6809::LDB()          // Load Register A
{ return(255); }


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
uint8_t Cpu6809::LDD()          // Load Register D  ( A << 8 | B)
{ return(255); }


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
uint8_t Cpu6809::LDS()          // Load Register S
{ return(255); }


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
uint8_t Cpu6809::LDU()          // Load Register U
{ return(255); }


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
uint8_t Cpu6809::LDX()          // Load Register X
{ return(255); }


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
uint8_t Cpu6809::LDY()          // Load Register Y
{ return(255); }


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
uint8_t Cpu6809::LSL()          // Logical Shift Left memory location (LSb is loaded with 0)
{ return(255); }


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
uint8_t Cpu6809::LSR()          // Logical Shift Right memory location (MSb is loaded with 0)
{ return(255); }


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
uint8_t Cpu6809::NEG()          // 2's Complement (negate) memory location
{ return(255); }


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
uint8_t Cpu6809::ORA()          // Logical OR register A
{ return(255); }


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
uint8_t Cpu6809::ORB()          // Logical OR register B
{ return(255); }


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
uint8_t Cpu6809::ORCC()         // Logical OR Condition Codes (set one or more flags)
{ return(255); }


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
uint8_t Cpu6809::PSHS()         // Push one or more registers onto the System Stack
{ return(255); }


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
uint8_t Cpu6809::PSHU()         // Push one or more registers onto the User Stack
{ return(255); }


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
uint8_t Cpu6809::PULS()         // Pull one or more registers from the System Stack
{ return(255); }


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
uint8_t Cpu6809::PULU()         // Pull one or more registers from the User Stack
{ return(255); }


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
uint8_t Cpu6809::ROL()          // Rotate memory location Left one bit through the Carry flag (9 bit rotate)
{ return(255); }


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
uint8_t Cpu6809::ROR()          // Rotate memory location Right one bit through the Carry flag (9 bit rotate)
{ return(255); }


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
uint8_t Cpu6809::SBCA()         // Subtract with carry (borrow) - register A
{ return(255); }


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
uint8_t Cpu6809::SBCB()         // Subtract with carry (borrow) - register B
{ return(255); }


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
uint8_t Cpu6809::STA()          // Store Register A
{ return(255); }


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
uint8_t Cpu6809::STB()          // Store Register B 
{ return(255); }


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
uint8_t Cpu6809::STD()          // Store Register D     (A << 8 | B)
{ return(255); }


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
uint8_t Cpu6809::STS()          // Store Register S
{ return(255); }


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
uint8_t Cpu6809::STU()          // Store Register U
{ return(255); }


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
uint8_t Cpu6809::STX()          // Store Register x
{ return(255); }


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
uint8_t Cpu6809::STY()          // Store Register Y
{ return(255); }


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
uint8_t Cpu6809::SUBA()         // Subtract from register A
{ return(255); }


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
uint8_t Cpu6809::SUBB()         // Subtract from register A
{ return(255); }


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
uint8_t Cpu6809::SUBD()         // Subtract from register D     (A << 8 | B)
{ return(255); }


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
uint8_t Cpu6809::TST()          // Test memory location, adjust N and Z Condition codes based on content
{ return(255); }


//*********************************************************************************************************************************
// Invented Mnemonic for invalid Opcode
//*********************************************************************************************************************************

//*****************************************************************************
//	XXX()
//*****************************************************************************
//	INVALID INSTRUCTION!
//
// On the 6309, this would trigger the invalid operation exception vector.
//
// On the 6809, this is undefined behavior and does not trigger any such
// exceptions, as the invalid instruction exception trap does not exist. (we
// try to define undefined opcodes for the CPU, but not all have any known
// "hidden" functionality, so we wind up here instead.
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
uint8_t Cpu6809::XXX()
{
	return(clocksUsed);
}


//*********************************************************************************************************************************


//*********************************************************************************************************************************
//	Address Modes (If Opcode's mnemonic is associated with only one opcode, the function may handle it internally, otherwise it
// invokes one of thse address mode functions.
//*********************************************************************************************************************************

//*****************************************************************************
//	INH()
//*****************************************************************************
//	Inherent Address mode - Opcode defines what is affected, no other bytes of
//	memory are required to perform an operation
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of adjusted clock cycles used.
//*****************************************************************************
uint8_t Cpu6809::INH(uint8_t adjClock)
{
	// don't have to do anything...
	return(adjClock);
}


//*****************************************************************************
//	Immediate()
//*****************************************************************************
//	Immediate Address mode... opcode(s) followed by two byte address or a
// single or double byte value to store in a register.
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	uint8_t	- the number of adjusted clock cycles used.
//*****************************************************************************
uint8_t Cpu6809::IMM(uint8_t adjClock)
{
	switch (adjClock)
	{
	case 0:
		offset_hi = Read(reg_PC);
		break;
	case 1:
		offset_lo = Read(reg_PC);
		break;
	}
	return(adjClock);
}


//*****************************************************************************
//	DIR()
//*****************************************************************************
//	Direct [page] Address mode reg_DP (Direct Page register) holds the
// high-byte (MSB) of the address to access  and the byte following the
// opcode(s) forms the low-byte (LSB) of the memory address accessed.
//
// NOTE: This mode can only access 256 bytes of RAM, if a page boarder is
//		crossed, the given results may not be what is expected.
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	uint8_t	- the number of adjusted clock cycles used.
//*****************************************************************************
uint8_t Cpu6809::DIR(uint8_t adjClock)
{
	switch (adjClock)
	{
	case 0:
		offset_hi = reg_DP;
	case 1:
		offset_lo = Read(reg_DP);
		break;
	}
	return(adjClock);
}


//*****************************************************************************
//	()
//*****************************************************************************
//	. Address mode
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	uint8_t	- the number of adjusted clock cycles used.
//*****************************************************************************
uint8_t Cpu6809::EXT(uint8_t adjClock)
{ return(255); }


//*****************************************************************************
//	()
//*****************************************************************************
//	. Address mode
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	uint8_t	- the number of adjusted clock cycles used.
//*****************************************************************************
uint8_t Cpu6809::IDX(uint8_t adjClock)
{ return(255); }


//*****************************************************************************
//	()
//*****************************************************************************
//	. Address mode
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	uint8_t	- the number of adjusted clock cycles used.
//*****************************************************************************
uint8_t Cpu6809::REL(uint8_t adjClock)
{ return(255); }


//*****************************************************************************
//	()
//*****************************************************************************
//	. Address mode
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	uint8_t	- the number of adjusted clock cycles used.
//*****************************************************************************
uint8_t Cpu6809::Ill(uint8_t adjClock)
{ return(255); }
