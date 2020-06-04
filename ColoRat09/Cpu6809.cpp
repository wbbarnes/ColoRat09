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

#define USE_RESET_3E

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
//	clocksLeft = 0;

	exec = nullptr ;
	mode = nullptr ;

	using op = Cpu6809;

	OpCodeP1 =
	{
#ifdef USE_RESET_3E
		{"NEG"  ,&op::NEG  ,6 ,2 ,&op::DIR}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COM"  ,&op::COM  ,6 ,2 ,&op::DIR}, {"LSR"      ,&op::LSR  ,6 ,2 ,&op::DIR}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"ROR"  ,&op::ROR  ,6 ,2 ,&op::DIR}, {"ASR"  ,&op::ASR  ,6 ,2 ,&op::DIR}, {"ASL/LSL"  ,&op::LSL  ,6 ,2 ,&op::DIR}, {"ROL"  ,&op::ROL  ,6 ,2 ,&op::DIR}, {"DEC"  ,&op::DEC  ,6 ,2 ,&op::DIR}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INC"  ,&op::INC  ,6 ,2 ,&op::DIR}, {"TST"  ,&op::TST  ,6 ,2 ,&op::DIR}, {"JMP"  ,&op::JMP      ,3 ,2 ,&op::DIR}, {"CLR"  ,&op::CLR  ,6 ,2 ,&op::DIR},
		{"***"  ,nullptr   ,0 ,0 ,nullptr }, {"***"  ,nullptr   ,0 ,0 ,nullptr }, {"NOP"  ,&op::NOP  ,2 ,1 ,&op::INH}, {"SYNC" ,&op::SYNC ,4 ,1 ,&op::INH}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"LBRA" ,&op::LBRA ,5 ,3 ,&op::REL}, {"LBSR" ,&op::LBSR ,9 ,3 ,&op::REL}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"DAA"  ,&op::DAA  ,2 ,1 ,&op::INH}, {"ORCC" ,&op::ORCC ,3 ,2 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"ANDCC",&op::ANDCC,3 ,2 ,&op::IMM}, {"SEX"  ,&op::SEX  ,2 ,1 ,&op::INH}, {"EXG"  ,&op::EXG      ,8 ,2 ,&op::IMM}, {"TFR"  ,&op::TFR  ,6 ,2 ,&op::IMM},
		{"BRA"  ,&op::BRA  ,3 ,2 ,&op::REL}, {"BRN"  ,&op::BRN  ,3 ,2 ,&op::REL}, {"BHI"  ,&op::BHI  ,3 ,2 ,&op::REL}, {"BLS"  ,&op::BLS  ,3 ,2 ,&op::REL}, {"BHS/BCC"  ,&op::BCC  ,3 ,2 ,&op::REL}, {"BLO/BCS"  ,&op::BCS  ,3 ,2 ,&op::REL}, {"BNE"  ,&op::BNE  ,3 ,2 ,&op::REL}, {"BEQ"  ,&op::BEQ  ,3 ,2 ,&op::REL}, {"BVC"      ,&op::BVC  ,3 ,2 ,&op::REL}, {"BVS"  ,&op::BVS  ,3 ,2 ,&op::REL}, {"BPL"  ,&op::BPL  ,3 ,2 ,&op::REL}, {"BMI"  ,&op::BMI  ,3 ,2 ,&op::REL}, {"BGE"  ,&op::BGE  ,3 ,2 ,&op::REL}, {"BLT"  ,&op::BLT  ,3 ,2 ,&op::REL}, {"BGT"  ,&op::BGT      ,3 ,2 ,&op::REL}, {"BLE"  ,&op::BLE  ,3 ,2 ,&op::REL},
		{"LEAX" ,&op::LEAX ,4 ,2 ,&op::IDX}, {"LEAY" ,&op::LEAY ,4 ,2 ,&op::IDX}, {"LEAS" ,&op::LEAS ,4 ,2 ,&op::IDX}, {"LEAU" ,&op::LEAU ,4 ,2 ,&op::IDX}, {"PSHS"     ,&op::PSHS ,5 ,2 ,&op::IMM}, {"PULS"     ,&op::PULS ,5 ,2 ,&op::IMM}, {"PSHU" ,&op::PSHU ,5 ,2 ,&op::IMM}, {"PULU" ,&op::PULU ,5 ,2 ,&op::IMM}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"RTS"  ,&op::RTS  ,5 ,1 ,&op::INH}, {"ABX"  ,&op::ABX  ,3 ,1 ,&op::INH}, {"RTI"  ,&op::RTI  ,6 ,1 ,&op::INH}, {"CWAI" ,&op::CWAI ,20,2 ,&op::INH}, {"MUL"  ,&op::MUL  ,11,1 ,&op::INH}, {"RESET",&op::SoftRESET,1 ,1 ,nullptr }, {"SWI"  ,&op::SWI  ,19,1 ,&op::INH},
		{"NEGA" ,&op::NEGA ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COMA" ,&op::COMA ,2 ,1 ,&op::INH}, {"LSRA"     ,&op::LSRA ,2 ,1 ,&op::INH}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"RORA" ,&op::RORA ,2 ,1 ,&op::INH}, {"ASRA" ,&op::ASRA ,2 ,1 ,&op::INH}, {"ASLA/LSLA",&op::LSLA ,2 ,1 ,&op::INH}, {"ROLA" ,&op::ROLA ,2 ,1 ,&op::INH}, {"DECA" ,&op::DECA ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INCA" ,&op::INCA ,2 ,1 ,&op::INH}, {"TSTA" ,&op::TSTA ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX      ,1 ,1 ,nullptr }, {"CLRA" ,&op::CLRA ,2 ,1 ,&op::INH},
		{"NEGB" ,&op::NEGB ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COMB" ,&op::COMB ,2 ,1 ,&op::INH}, {"LSRB"     ,&op::LSRB ,2 ,1 ,&op::INH}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"RORB" ,&op::RORB ,2 ,1 ,&op::INH}, {"ASRB" ,&op::ASRB ,2 ,1 ,&op::INH}, {"ASLB/LSLB",&op::LSLB ,2 ,1 ,&op::INH}, {"ROLB" ,&op::ROLB ,2 ,1 ,&op::INH}, {"DECB" ,&op::DECB ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INCB" ,&op::INCB ,2 ,1 ,&op::INH}, {"TSTB" ,&op::TSTB ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX      ,1 ,1 ,nullptr }, {"CLRB" ,&op::CLRB ,2 ,1 ,&op::INH},
		{"NEG"  ,&op::NEG  ,6 ,2 ,&op::IDX}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COM"  ,&op::COM  ,6 ,2 ,&op::IDX}, {"LSR"      ,&op::LSR  ,6 ,2 ,&op::IDX}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"ROR"  ,&op::ROR  ,6 ,2 ,&op::IDX}, {"ASR"  ,&op::ASR  ,6 ,2 ,&op::IDX}, {"ASL/LSL"  ,&op::LSL  ,6 ,2 ,&op::IDX}, {"ROL"  ,&op::ROL  ,6 ,2 ,&op::IDX}, {"DEC"  ,&op::DEC  ,6 ,2 ,&op::IDX}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INC"  ,&op::INC  ,6 ,2 ,&op::IDX}, {"TST"  ,&op::TST  ,6 ,2 ,&op::IDX}, {"JMP"  ,&op::JMP      ,3 ,2 ,&op::IDX}, {"CLR"  ,&op::CLR  ,6 ,2 ,&op::IDX},
		{"NEG"  ,&op::NEG  ,7 ,3 ,&op::EXT}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COM"  ,&op::COM  ,7 ,3 ,&op::EXT}, {"LSR"      ,&op::LSR  ,7 ,3 ,&op::EXT}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"ROR"  ,&op::ROR  ,7 ,3 ,&op::EXT}, {"ASR"  ,&op::ASR  ,7 ,3 ,&op::EXT}, {"ASL/LSL"  ,&op::LSL  ,7 ,3 ,&op::EXT}, {"ROL"  ,&op::ROL  ,7 ,3 ,&op::EXT}, {"DEC"  ,&op::DEC  ,7 ,3 ,&op::EXT}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INC"  ,&op::INC  ,7 ,3 ,&op::EXT}, {"TST"  ,&op::TST  ,7 ,3 ,&op::EXT}, {"JMP"  ,&op::JMP      ,4 ,3 ,&op::EXT}, {"CLR"  ,&op::CLR  ,7 ,3 ,&op::EXT},
		{"SUBA" ,&op::SUBA ,2 ,2 ,&op::IMM}, {"CMPA" ,&op::CMPA ,2 ,2 ,&op::IMM}, {"SBCA" ,&op::SBCA ,2 ,2 ,&op::IMM}, {"SUBD" ,&op::SUBD ,4 ,3 ,&op::IMM}, {"ANDA"     ,&op::ANDA ,2 ,2 ,&op::IMM}, {"BITA"     ,&op::BITA ,2 ,2 ,&op::IMM}, {"LDA"  ,&op::LDA  ,2 ,2 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"EORA"     ,&op::EORA ,2 ,2 ,&op::IMM}, {"ADCA" ,&op::ADCA ,2 ,2 ,&op::IMM}, {"ORA"  ,&op::ORA  ,2 ,2 ,&op::IMM}, {"ADDA" ,&op::ADDA ,2 ,2 ,&op::IMM}, {"CMPX" ,&op::CMPX ,4 ,3 ,&op::IMM}, {"BSR"  ,&op::BSR  ,7 ,2 ,&op::REL}, {"LDX"  ,&op::LDX      ,3 ,3 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"SUBA" ,&op::SUBA ,4 ,2 ,&op::DIR}, {"CMPA" ,&op::CMPA ,4 ,2 ,&op::DIR}, {"SBCA" ,&op::SBCA ,4 ,2 ,&op::DIR}, {"SUBD" ,&op::SUBD ,6 ,2 ,&op::DIR}, {"ANDA"     ,&op::ANDA ,4 ,2 ,&op::DIR}, {"BITA"     ,&op::BITA ,4 ,2 ,&op::DIR}, {"LDA"  ,&op::LDA  ,4 ,2 ,&op::DIR}, {"STA"  ,&op::STA  ,4 ,2 ,&op::DIR}, {"EORA"     ,&op::EORA ,4 ,2 ,&op::DIR}, {"ADCA" ,&op::ADCA ,4 ,2 ,&op::DIR}, {"ORA"  ,&op::ORA  ,4 ,2 ,&op::DIR}, {"ADDA" ,&op::ADDA ,4 ,2 ,&op::DIR}, {"CMPX" ,&op::CMPX ,6 ,2 ,&op::DIR}, {"JSR"  ,&op::JSR  ,7 ,2 ,&op::DIR}, {"LDX"  ,&op::LDX      ,5 ,2 ,&op::DIR}, {"STX"  ,&op::STX  ,5 ,2 ,&op::DIR},
		{"SUBA" ,&op::SUBA ,4 ,2 ,&op::IDX}, {"CMPA" ,&op::CMPA ,4 ,2 ,&op::IDX}, {"SBCA" ,&op::SBCA ,4 ,2 ,&op::IDX}, {"SUBD" ,&op::SUBD ,4 ,2 ,&op::IDX}, {"ANDA"     ,&op::ANDA ,4 ,2 ,&op::IDX}, {"BITA"     ,&op::BITA ,4 ,2 ,&op::IDX}, {"LDA"  ,&op::LDA  ,4 ,2 ,&op::IDX}, {"STA"  ,&op::STA  ,4 ,2 ,&op::IDX}, {"EORA"     ,&op::EORA ,4 ,2 ,&op::IDX}, {"ADCA" ,&op::ADCA ,4 ,2 ,&op::IDX}, {"ORA"  ,&op::ORA  ,4 ,2 ,&op::IDX}, {"ADDA" ,&op::ADDA ,4 ,2 ,&op::IDX}, {"CMPX" ,&op::CMPX ,6 ,2 ,&op::IDX}, {"JSR"  ,&op::JSR  ,7 ,2 ,&op::IDX}, {"LDX"  ,&op::LDX      ,5 ,2 ,&op::IDX}, {"STX"  ,&op::STX  ,5 ,2 ,&op::IDX},
		{"SUBA" ,&op::SUBA ,5 ,3 ,&op::EXT}, {"CMPA" ,&op::CMPA ,5 ,3 ,&op::EXT}, {"SBCA" ,&op::SBCA ,5 ,3 ,&op::EXT}, {"SUBD" ,&op::SUBD ,7 ,3 ,&op::EXT}, {"ANDA"     ,&op::ANDA ,5 ,3 ,&op::EXT}, {"BITA"     ,&op::BITA ,5 ,3 ,&op::EXT}, {"LDA"  ,&op::LDA  ,5 ,3 ,&op::EXT}, {"STA"  ,&op::STA  ,5 ,3 ,&op::EXT}, {"EORA"     ,&op::EORA ,5 ,3 ,&op::EXT}, {"ADCA" ,&op::ADCA ,5 ,3 ,&op::EXT}, {"ORA"  ,&op::ORA  ,5 ,3 ,&op::EXT}, {"ADDA" ,&op::ADDA ,5 ,3 ,&op::EXT}, {"CMPX" ,&op::CMPX ,7 ,3 ,&op::EXT}, {"JSR"  ,&op::JSR  ,8 ,3 ,&op::EXT}, {"LDX"  ,&op::LDX      ,6 ,3 ,&op::EXT}, {"STX"  ,&op::STX  ,6 ,3 ,&op::EXT},
		{"SUBB" ,&op::SUBB ,2 ,2 ,&op::IMM}, {"CMPB" ,&op::CMPB ,2 ,2 ,&op::IMM}, {"SBCB" ,&op::SBCB ,2 ,2 ,&op::IMM}, {"ADDD" ,&op::ADDD ,4 ,3 ,&op::IMM}, {"ANDB"     ,&op::ANDB ,2 ,2 ,&op::IMM}, {"BITB"     ,&op::BITB ,2 ,2 ,&op::IMM}, {"LDB"  ,&op::LDB  ,2 ,2 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"EORB"     ,&op::EORB ,2 ,2 ,&op::IMM}, {"ADCB" ,&op::ADCB ,2 ,2 ,&op::IMM}, {"ORB"  ,&op::ORB  ,2 ,2 ,&op::IMM}, {"ADDB" ,&op::ADDB ,2 ,2 ,&op::IMM}, {"LDD"  ,&op::LDD  ,3 ,3 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDU"  ,&op::LDU      ,3 ,3 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"SUBB" ,&op::SUBB ,4 ,2 ,&op::DIR}, {"CMPB" ,&op::CMPB ,4 ,2 ,&op::DIR}, {"SBCB" ,&op::SBCB ,4 ,2 ,&op::DIR}, {"ADDD" ,&op::ADDD ,6 ,2 ,&op::DIR}, {"ANDB"     ,&op::ANDB ,4 ,2 ,&op::DIR}, {"BITB"     ,&op::BITB ,4 ,2 ,&op::DIR}, {"LDB"  ,&op::LDB  ,4 ,2 ,&op::DIR}, {"STB"  ,&op::STB  ,4 ,2 ,&op::DIR}, {"EORB"     ,&op::EORB ,4 ,2 ,&op::DIR}, {"ADCB" ,&op::ADCB ,4 ,2 ,&op::DIR}, {"ORB"  ,&op::ORB  ,4 ,2 ,&op::DIR}, {"ADDB" ,&op::ADDB ,4 ,2 ,&op::DIR}, {"LDD"  ,&op::LDD  ,5 ,2 ,&op::DIR}, {"STD"  ,&op::STD  ,5 ,2 ,&op::DIR}, {"LDU"  ,&op::LDU      ,5 ,2 ,&op::DIR}, {"STU"  ,&op::STU  ,5 ,2 ,&op::DIR},
		{"SUBB" ,&op::SUBB ,4 ,2 ,&op::IDX}, {"CMPB" ,&op::CMPB ,4 ,2 ,&op::IDX}, {"SBCB" ,&op::SBCB ,4 ,2 ,&op::IDX}, {"ADDD" ,&op::ADDD ,6 ,2 ,&op::IDX}, {"ANDB"     ,&op::ANDB ,4 ,2 ,&op::IDX}, {"BITB"     ,&op::BITB ,4 ,2 ,&op::IDX}, {"LDB"  ,&op::LDB  ,4 ,2 ,&op::IDX}, {"STB"  ,&op::STB  ,4 ,2 ,&op::IDX}, {"EORB"     ,&op::EORB ,4 ,2 ,&op::IDX}, {"ADCB" ,&op::ADCB ,4 ,2 ,&op::IDX}, {"ORB"  ,&op::ORB  ,4 ,2 ,&op::IDX}, {"ADDB" ,&op::ADDB ,4 ,2 ,&op::IDX}, {"LDD"  ,&op::LDD  ,5 ,2 ,&op::IDX}, {"STD"  ,&op::STD  ,5 ,2 ,&op::IDX}, {"LDU"  ,&op::LDU      ,5 ,2 ,&op::IDX}, {"STU"  ,&op::STU  ,5 ,2 ,&op::IDX},
		{"SUBB" ,&op::SUBB ,5 ,3 ,&op::EXT}, {"CMPB" ,&op::CMPB ,5 ,3 ,&op::EXT}, {"SBCB" ,&op::SBCB ,5 ,3 ,&op::EXT}, {"ADDD" ,&op::ADDD ,7 ,3 ,&op::EXT}, {"ANDB"     ,&op::ANDB ,5 ,3 ,&op::EXT}, {"BITB"     ,&op::BITB ,5 ,3 ,&op::EXT}, {"LDB"  ,&op::LDB  ,5 ,3 ,&op::EXT}, {"STB"  ,&op::STB  ,5 ,3 ,&op::EXT}, {"EORB"     ,&op::EORB ,5 ,3 ,&op::EXT}, {"ADCB" ,&op::ADCB ,5 ,3 ,&op::EXT}, {"ORB"  ,&op::ORB  ,5 ,3 ,&op::EXT}, {"ADDB" ,&op::ADDB ,5 ,3 ,&op::EXT}, {"LDD"  ,&op::LDD  ,6 ,3 ,&op::EXT}, {"STD"  ,&op::STD  ,6 ,3 ,&op::EXT}, {"LDU"  ,&op::LDU      ,6 ,3 ,&op::EXT}, {"STU"  ,&op::STU  ,6 ,3 ,&op::EXT}
#else
		{"NEG"  ,&op::NEG  ,6 ,2 ,&op::DIR}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COM"  ,&op::COM  ,6 ,2 ,&op::DIR}, {"LSR"      ,&op::LSR  ,6 ,2 ,&op::DIR}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"ROR"  ,&op::ROR  ,6 ,2 ,&op::DIR}, {"ASR"  ,&op::ASR  ,6 ,2 ,&op::DIR}, {"ASL/LSL"  ,&op::LSL  ,6 ,2 ,&op::DIR}, {"ROL"  ,&op::ROL  ,6 ,2 ,&op::DIR}, {"DEC"  ,&op::DEC  ,6 ,2 ,&op::DIR}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INC"  ,&op::INC  ,6 ,2 ,&op::DIR}, {"TST"  ,&op::TST  ,6 ,2 ,&op::DIR}, {"JMP"  ,&op::JMP  ,3 ,2 ,&op::DIR}, {"CLR"  ,&op::CLR  ,6 ,2 ,&op::DIR},
		{"***"  ,nullptr   ,0 ,0 ,nullptr }, {"***"  ,nullptr   ,0 ,0 ,nullptr }, {"NOP"  ,&op::NOP  ,2 ,1 ,&op::INH}, {"SYNC" ,&op::SYNC ,4 ,1 ,&op::INH}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"LBRA" ,&op::LBRA ,5 ,3 ,&op::REL}, {"LBSR" ,&op::LBSR ,9 ,3 ,&op::REL}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"DAA"  ,&op::DAA  ,2 ,1 ,&op::INH}, {"ORCC" ,&op::ORCC ,3 ,2 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"ANDCC",&op::ANDCC,3 ,2 ,&op::IMM}, {"SEX"  ,&op::SEX  ,2 ,1 ,&op::INH}, {"EXG"  ,&op::EXG  ,8 ,2 ,&op::IMM}, {"TFR"  ,&op::TFR  ,6 ,2 ,&op::IMM},
		{"BRA"  ,&op::BRA  ,3 ,2 ,&op::REL}, {"BRN"  ,&op::BRN  ,3 ,2 ,&op::REL}, {"BHI"  ,&op::BHI  ,3 ,2 ,&op::REL}, {"BLS"  ,&op::BLS  ,3 ,2 ,&op::REL}, {"BHS/BCC"  ,&op::BCC  ,3 ,2 ,&op::REL}, {"BLO/BCS"  ,&op::BCS  ,3 ,2 ,&op::REL}, {"BNE"  ,&op::BNE  ,3 ,2 ,&op::REL}, {"BEQ"  ,&op::BEQ  ,3 ,2 ,&op::REL}, {"BVC"      ,&op::BVC  ,3 ,2 ,&op::REL}, {"BVS"  ,&op::BVS  ,3 ,2 ,&op::REL}, {"BPL"  ,&op::BPL  ,3 ,2 ,&op::REL}, {"BMI"  ,&op::BMI  ,3 ,2 ,&op::REL}, {"BGE"  ,&op::BGE  ,3 ,2 ,&op::REL}, {"BLT"  ,&op::BLT  ,3 ,2 ,&op::REL}, {"BGT"  ,&op::BGT  ,3 ,2 ,&op::REL}, {"BLE"  ,&op::BLE  ,3 ,2 ,&op::REL},
		{"LEAX" ,&op::LEAX ,4 ,2 ,&op::IDX}, {"LEAY" ,&op::LEAY ,4 ,2 ,&op::IDX}, {"LEAS" ,&op::LEAS ,4 ,2 ,&op::IDX}, {"LEAU" ,&op::LEAU ,4 ,2 ,&op::IDX}, {"PSHS"     ,&op::PSHS ,5 ,2 ,&op::IMM}, {"PULS"     ,&op::PULS ,5 ,2 ,&op::IMM}, {"PSHU" ,&op::PSHU ,5 ,2 ,&op::IMM}, {"PULU" ,&op::PULU ,5 ,2 ,&op::IMM}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"RTS"  ,&op::RTS  ,5 ,1 ,&op::INH}, {"ABX"  ,&op::ABX  ,3 ,1 ,&op::INH}, {"RTI"  ,&op::RTI  ,6 ,1 ,&op::INH}, {"CWAI" ,&op::CWAI ,20,2 ,&op::INH}, {"MUL"  ,&op::MUL  ,11,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"SWI"  ,&op::SWI  ,19,1 ,&op::INH},
		{"NEGA" ,&op::NEGA ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COMA" ,&op::COMA ,2 ,1 ,&op::INH}, {"LSRA"     ,&op::LSRA ,2 ,1 ,&op::INH}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"RORA" ,&op::RORA ,2 ,1 ,&op::INH}, {"ASRA" ,&op::ASRA ,2 ,1 ,&op::INH}, {"ASLA/LSLA",&op::LSLA ,2 ,1 ,&op::INH}, {"ROLA" ,&op::ROLA ,2 ,1 ,&op::INH}, {"DECA" ,&op::DECA ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INCA" ,&op::INCA ,2 ,1 ,&op::INH}, {"TSTA" ,&op::TSTA ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CLRA" ,&op::CLRA ,2 ,1 ,&op::INH},
		{"NEGB" ,&op::NEGB ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COMB" ,&op::COMB ,2 ,1 ,&op::INH}, {"LSRB"     ,&op::LSRB ,2 ,1 ,&op::INH}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"RORB" ,&op::RORB ,2 ,1 ,&op::INH}, {"ASRB" ,&op::ASRB ,2 ,1 ,&op::INH}, {"ASLB/LSLB",&op::LSLB ,2 ,1 ,&op::INH}, {"ROLB" ,&op::ROLB ,2 ,1 ,&op::INH}, {"DECB" ,&op::DECB ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INCB" ,&op::INCB ,2 ,1 ,&op::INH}, {"TSTB" ,&op::TSTB ,2 ,1 ,&op::INH}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CLRB" ,&op::CLRB ,2 ,1 ,&op::INH},
		{"NEG"  ,&op::NEG  ,6 ,2 ,&op::IDX}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COM"  ,&op::COM  ,6 ,2 ,&op::IDX}, {"LSR"      ,&op::LSR  ,6 ,2 ,&op::IDX}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"ROR"  ,&op::ROR  ,6 ,2 ,&op::IDX}, {"ASR"  ,&op::ASR  ,6 ,2 ,&op::IDX}, {"ASL/LSL"  ,&op::LSL  ,6 ,2 ,&op::IDX}, {"ROL"  ,&op::ROL  ,6 ,2 ,&op::IDX}, {"DEC"  ,&op::DEC  ,6 ,2 ,&op::IDX}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INC"  ,&op::INC  ,6 ,2 ,&op::IDX}, {"TST"  ,&op::TST  ,6 ,2 ,&op::IDX}, {"JMP"  ,&op::JMP  ,3 ,2 ,&op::IDX}, {"CLR"  ,&op::CLR  ,6 ,2 ,&op::IDX},
		{"NEG"  ,&op::NEG  ,7 ,3 ,&op::EXT}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"COM"  ,&op::COM  ,7 ,3 ,&op::EXT}, {"LSR"      ,&op::LSR  ,7 ,3 ,&op::EXT}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"ROR"  ,&op::ROR  ,7 ,3 ,&op::EXT}, {"ASR"  ,&op::ASR  ,7 ,3 ,&op::EXT}, {"ASL/LSL"  ,&op::LSL  ,7 ,3 ,&op::EXT}, {"ROL"  ,&op::ROL  ,7 ,3 ,&op::EXT}, {"DEC"  ,&op::DEC  ,7 ,3 ,&op::EXT}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"INC"  ,&op::INC  ,7 ,3 ,&op::EXT}, {"TST"  ,&op::TST  ,7 ,3 ,&op::EXT}, {"JMP"  ,&op::JMP  ,4 ,3 ,&op::EXT}, {"CLR"  ,&op::CLR  ,7 ,3 ,&op::EXT},
		{"SUBA" ,&op::SUBA ,2 ,2 ,&op::IMM}, {"CMPA" ,&op::CMPA ,2 ,2 ,&op::IMM}, {"SBCA" ,&op::SBCA ,2 ,2 ,&op::IMM}, {"SUBD" ,&op::SUBD ,4 ,3 ,&op::IMM}, {"ANDA"     ,&op::ANDA ,2 ,2 ,&op::IMM}, {"BITA"     ,&op::BITA ,2 ,2 ,&op::IMM}, {"LDA"  ,&op::LDA  ,2 ,2 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"EORA"     ,&op::EORA ,2 ,2 ,&op::IMM}, {"ADCA" ,&op::ADCA ,2 ,2 ,&op::IMM}, {"ORA"  ,&op::ORA  ,2 ,2 ,&op::IMM}, {"ADDA" ,&op::ADDA ,2 ,2 ,&op::IMM}, {"CMPX" ,&op::CMPX ,4 ,3 ,&op::IMM}, {"BSR"  ,&op::BSR  ,7 ,2 ,&op::REL}, {"LDX"  ,&op::LDX  ,3 ,3 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"SUBA" ,&op::SUBA ,4 ,2 ,&op::DIR}, {"CMPA" ,&op::CMPA ,4 ,2 ,&op::DIR}, {"SBCA" ,&op::SBCA ,4 ,2 ,&op::DIR}, {"SUBD" ,&op::SUBD ,6 ,2 ,&op::DIR}, {"ANDA"     ,&op::ANDA ,4 ,2 ,&op::DIR}, {"BITA"     ,&op::BITA ,4 ,2 ,&op::DIR}, {"LDA"  ,&op::LDA  ,4 ,2 ,&op::DIR}, {"STA"  ,&op::STA  ,4 ,2 ,&op::DIR}, {"EORA"     ,&op::EORA ,4 ,2 ,&op::DIR}, {"ADCA" ,&op::ADCA ,4 ,2 ,&op::DIR}, {"ORA"  ,&op::ORA  ,4 ,2 ,&op::DIR}, {"ADDA" ,&op::ADDA ,4 ,2 ,&op::DIR}, {"CMPX" ,&op::CMPX ,6 ,2 ,&op::DIR}, {"JSR"  ,&op::JSR  ,7 ,2 ,&op::DIR}, {"LDX"  ,&op::LDX  ,5 ,2 ,&op::DIR}, {"STX"  ,&op::STX  ,5 ,2 ,&op::DIR},
		{"SUBA" ,&op::SUBA ,4 ,2 ,&op::IDX}, {"CMPA" ,&op::CMPA ,4 ,2 ,&op::IDX}, {"SBCA" ,&op::SBCA ,4 ,2 ,&op::IDX}, {"SUBD" ,&op::SUBD ,4 ,2 ,&op::IDX}, {"ANDA"     ,&op::ANDA ,4 ,2 ,&op::IDX}, {"BITA"     ,&op::BITA ,4 ,2 ,&op::IDX}, {"LDA"  ,&op::LDA  ,4 ,2 ,&op::IDX}, {"STA"  ,&op::STA  ,4 ,2 ,&op::IDX}, {"EORA"     ,&op::EORA ,4 ,2 ,&op::IDX}, {"ADCA" ,&op::ADCA ,4 ,2 ,&op::IDX}, {"ORA"  ,&op::ORA  ,4 ,2 ,&op::IDX}, {"ADDA" ,&op::ADDA ,4 ,2 ,&op::IDX}, {"CMPX" ,&op::CMPX ,6 ,2 ,&op::IDX}, {"JSR"  ,&op::JSR  ,7 ,2 ,&op::IDX}, {"LDX"  ,&op::LDX  ,5 ,2 ,&op::IDX}, {"STX"  ,&op::STX  ,5 ,2 ,&op::IDX},
		{"SUBA" ,&op::SUBA ,5 ,3 ,&op::EXT}, {"CMPA" ,&op::CMPA ,5 ,3 ,&op::EXT}, {"SBCA" ,&op::SBCA ,5 ,3 ,&op::EXT}, {"SUBD" ,&op::SUBD ,7 ,3 ,&op::EXT}, {"ANDA"     ,&op::ANDA ,5 ,3 ,&op::EXT}, {"BITA"     ,&op::BITA ,5 ,3 ,&op::EXT}, {"LDA"  ,&op::LDA  ,5 ,3 ,&op::EXT}, {"STA"  ,&op::STA  ,5 ,3 ,&op::EXT}, {"EORA"     ,&op::EORA ,5 ,3 ,&op::EXT}, {"ADCA" ,&op::ADCA ,5 ,3 ,&op::EXT}, {"ORA"  ,&op::ORA  ,5 ,3 ,&op::EXT}, {"ADDA" ,&op::ADDA ,5 ,3 ,&op::EXT}, {"CMPX" ,&op::CMPX ,7 ,3 ,&op::EXT}, {"JSR"  ,&op::JSR  ,8 ,3 ,&op::EXT}, {"LDX"  ,&op::LDX  ,6 ,3 ,&op::EXT}, {"STX"  ,&op::STX  ,6 ,3 ,&op::EXT},
		{"SUBB" ,&op::SUBB ,2 ,2 ,&op::IMM}, {"CMPB" ,&op::CMPB ,2 ,2 ,&op::IMM}, {"SBCB" ,&op::SBCB ,2 ,2 ,&op::IMM}, {"ADDD" ,&op::ADDD ,4 ,3 ,&op::IMM}, {"ANDB"     ,&op::ANDB ,2 ,2 ,&op::IMM}, {"BITB"     ,&op::BITB ,2 ,2 ,&op::IMM}, {"LDB"  ,&op::LDB  ,2 ,2 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"EORB"     ,&op::EORB ,2 ,2 ,&op::IMM}, {"ADCB" ,&op::ADCB ,2 ,2 ,&op::IMM}, {"ORB"  ,&op::ORB  ,2 ,2 ,&op::IMM}, {"ADDB" ,&op::ADDB ,2 ,2 ,&op::IMM}, {"LDD"  ,&op::LDD  ,3 ,3 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDU"  ,&op::LDU  ,3 ,3 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"SUBB" ,&op::SUBB ,4 ,2 ,&op::DIR}, {"CMPB" ,&op::CMPB ,4 ,2 ,&op::DIR}, {"SBCB" ,&op::SBCB ,4 ,2 ,&op::DIR}, {"ADDD" ,&op::ADDD ,6 ,2 ,&op::DIR}, {"ANDB"     ,&op::ANDB ,4 ,2 ,&op::DIR}, {"BITB"     ,&op::BITB ,4 ,2 ,&op::DIR}, {"LDB"  ,&op::LDB  ,4 ,2 ,&op::DIR}, {"STB"  ,&op::STB  ,4 ,2 ,&op::DIR}, {"EORB"     ,&op::EORB ,4 ,2 ,&op::DIR}, {"ADCB" ,&op::ADCB ,4 ,2 ,&op::DIR}, {"ORB"  ,&op::ORB  ,4 ,2 ,&op::DIR}, {"ADDB" ,&op::ADDB ,4 ,2 ,&op::DIR}, {"LDD"  ,&op::LDD  ,5 ,2 ,&op::DIR}, {"STD"  ,&op::STD  ,5 ,2 ,&op::DIR}, {"LDU"  ,&op::LDU  ,5 ,2 ,&op::DIR}, {"STU"  ,&op::STU  ,5 ,2 ,&op::DIR},
		{"SUBB" ,&op::SUBB ,4 ,2 ,&op::IDX}, {"CMPB" ,&op::CMPB ,4 ,2 ,&op::IDX}, {"SBCB" ,&op::SBCB ,4 ,2 ,&op::IDX}, {"ADDD" ,&op::ADDD ,6 ,2 ,&op::IDX}, {"ANDB"     ,&op::ANDB ,4 ,2 ,&op::IDX}, {"BITB"     ,&op::BITB ,4 ,2 ,&op::IDX}, {"LDB"  ,&op::LDB  ,4 ,2 ,&op::IDX}, {"STB"  ,&op::STB  ,4 ,2 ,&op::IDX}, {"EORB"     ,&op::EORB ,4 ,2 ,&op::IDX}, {"ADCB" ,&op::ADCB ,4 ,2 ,&op::IDX}, {"ORB"  ,&op::ORB  ,4 ,2 ,&op::IDX}, {"ADDB" ,&op::ADDB ,4 ,2 ,&op::IDX}, {"LDD"  ,&op::LDD  ,5 ,2 ,&op::IDX}, {"STD"  ,&op::STD  ,5 ,2 ,&op::IDX}, {"LDU"  ,&op::LDU  ,5 ,2 ,&op::IDX}, {"STU"  ,&op::STU  ,5 ,2 ,&op::IDX},
		{"SUBB" ,&op::SUBB ,5 ,3 ,&op::EXT}, {"CMPB" ,&op::CMPB ,5 ,3 ,&op::EXT}, {"SBCB" ,&op::SBCB ,5 ,3 ,&op::EXT}, {"ADDD" ,&op::ADDD ,7 ,3 ,&op::EXT}, {"ANDB"     ,&op::ANDB ,5 ,3 ,&op::EXT}, {"BITB"     ,&op::BITB ,5 ,3 ,&op::EXT}, {"LDB"  ,&op::LDB  ,5 ,3 ,&op::EXT}, {"STB"  ,&op::STB  ,5 ,3 ,&op::EXT}, {"EORB"     ,&op::EORB ,5 ,3 ,&op::EXT}, {"ADCB" ,&op::ADCB ,5 ,3 ,&op::EXT}, {"ORB"  ,&op::ORB  ,5 ,3 ,&op::EXT}, {"ADDB" ,&op::ADDB ,5 ,3 ,&op::EXT}, {"LDD"  ,&op::LDD  ,6 ,3 ,&op::EXT}, {"STD"  ,&op::STD  ,6 ,3 ,&op::EXT}, {"LDU"  ,&op::LDU  ,6 ,3 ,&op::EXT}, {"STU"  ,&op::STU  ,6 ,3 ,&op::EXT}
#endif
	};
	OpCodeP2 =
	{
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"***"  ,nullptr   ,0 ,0 ,nullptr }, {"***"  ,nullptr   ,0 ,0 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LBRN" ,&op::LBRN ,5 ,4 ,&op::REL}, {"LBHI" ,&op::LBHI ,5 ,4 ,&op::REL}, {"LBLS" ,&op::LBLS ,5 ,4 ,&op::REL}, {"LBHS/LBCC",&op::LBCC ,5 ,4 ,&op::REL}, {"LBCS/LBLO",&op::LBCS ,5 ,4 ,&op::REL}, {"LBNE" ,&op::LBNE ,5 ,4 ,&op::REL}, {"LBEQ" ,&op::LBEQ ,5 ,4 ,&op::REL}, {"LBVC" ,&op::LBVC ,5 ,4 ,&op::REL}, {"LBVS" ,&op::LBVS ,5 ,4 ,&op::REL}, {"LBPL" ,&op::LBPL ,5 ,4 ,&op::REL}, {"LBMI" ,&op::LBMI ,5 ,4 ,&op::REL}, {"LBGE" ,&op::LBGE ,5 ,4 ,&op::REL}, {"LBLT" ,&op::LBLT ,5 ,4 ,&op::REL}, {"LBGT" ,&op::LBGT ,5 ,4 ,&op::REL}, {"LBLE" ,&op::LBLE ,5 ,4 ,&op::REL},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"SWI2" ,&op::SWI2 ,20,2 ,&op::INH},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPD" ,&op::CMPD ,5 ,4 ,&op::IMM}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPY" ,&op::CMPY ,5 ,4 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDY"  ,&op::LDY  ,4 ,4 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPD" ,&op::CMPD ,7 ,3 ,&op::DIR}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPY" ,&op::CMPY ,7 ,3 ,&op::DIR}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDY"  ,&op::LDY  ,6 ,3 ,&op::DIR}, {"STY"  ,&op::STY  ,6 ,3 ,&op::DIR},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPD" ,&op::CMPD ,7 ,3 ,&op::IDX}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPY" ,&op::CMPY ,7 ,3 ,&op::IDX}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDY"  ,&op::LDY  ,6 ,3 ,&op::IDX}, {"STY"  ,&op::STY  ,6 ,3 ,&op::IDX},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPD" ,&op::CMPD ,8 ,4 ,&op::EXT}, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPY" ,&op::CMPY ,8 ,4 ,&op::EXT}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDY"  ,&op::LDY  ,7 ,4 ,&op::EXT}, {"STY"  ,&op::STY  ,7 ,4 ,&op::EXT},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDS"  ,&op::LDS  ,4 ,4 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDS"  ,&op::LDS  ,6 ,4 ,&op::DIR}, {"STS"  ,&op::STS  ,6 ,3 ,&op::DIR},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDS"  ,&op::LDS  ,6 ,3 ,&op::IDX}, {"STS"  ,&op::STS  ,6 ,3 ,&op::IDX},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"      ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"LDS"  ,&op::LDS  ,7 ,4 ,&op::EXT}, {"STS"  ,&op::STS  ,7 ,4 ,&op::EXT},
	};
	OpCodeP3 =
	{
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"***"  ,nullptr   ,0 ,0 ,nullptr }, {"***"  ,nullptr   ,0 ,0 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"SWI3" ,&op::SWI3 ,20,2 ,&op::INH},
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPU" ,&op::CMPU ,5 ,4 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPS" ,&op::CMPS ,5 ,4 ,&op::IMM}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPU" ,&op::CMPU ,7 ,3 ,&op::DIR}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPS" ,&op::CMPS ,7 ,3 ,&op::DIR}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPU" ,&op::CMPU ,7 ,3 ,&op::IDX}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPS" ,&op::CMPS ,7 ,3 ,&op::IDX}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPU" ,&op::CMPU ,8 ,4 ,&op::EXT}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"CMPS" ,&op::CMPS ,8, 4 ,&op::EXT}, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr },
		{"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }, {"???"  ,&op::XXX  ,1 ,1 ,nullptr }
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
	if (exec != nullptr )
	{
		//(obj->*fp)(m, n)
		if (uint8_t result = (this->*exec)() == 255)
		{
			exec = nullptr ;
			clocksUsed = 0;
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
		// apparently does not set the E flag, this will throw a RTI from this off.
		//reg_CC |= CC::E;
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
//		  Registers: S, CC and PC.
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
	static int8_t intCount = 0;

	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Don't Care			PC+1
		++reg_PC;
		break;
	case 3:		// R	Don't Care			Z
		if ((reg_CC & CC::I) || (reg_CC & CC::F))
			clocksUsed = 255;
		else if (!nmiTriggered || !firqTriggered || !irqTriggered)
			--clocksUsed;
		break;
	case 4:		// R	Don't Care			Z
		if (nmiTriggered || firqTriggered || irqTriggered)
		{
			++intCount;
			if (intCount >= 3)
			{
				if (nmiTriggered)
				{
					clocksUsed = 1;
					intCount = 0;
					exec = &Cpu6809::NMI;
				}
				else if (firqTriggered)
				{
					clocksUsed = 1;
					intCount = 0;
					exec = &Cpu6809::FIRQ;
				}
				else if (irqTriggered)
				{
					clocksUsed = 1;
					intCount = 0;
					exec = &Cpu6809::IRQ;
				}
			}
		}
		else if (intCount < 3)
		{
			intCount = 0;
			clocksUsed = 255;
		}
		else
			--clocksUsed;
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
//		  Registers: S
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
		scratch_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			PC+2
		++reg_PC;
		break;
	case 4:		// R	Don't care			$ffff
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
		reg_CC &= scratch_lo;
		reg_CC |= CC::E;
		break;

	case 17:	// R	Don't Care			$ffff
		// wait for interrupt signal
		if (!nmiTriggered && !firqTriggered && !irqTriggered)
			--clocksUsed;
		break;

	case 18:	// R	Int Vector High		$fffx
		if (nmiTriggered)
			PC_hi = Read(0xfffc);
		else if (firqTriggered && (reg_CC & CC::F) != CC::F)
			PC_hi = Read(0xfff6);
		else if (irqTriggered && (reg_CC & CC::I) != CC::I)
			PC_hi = Read(0xfff8);
		break;
	case 19:	// R	Int Vector Low		$fffx
		if (nmiTriggered)
			PC_lo = Read(0xfffd);
		else if (firqTriggered && (reg_CC & CC::F) != CC::F)
			PC_lo = Read(0xfff7);
		else if (irqTriggered && (reg_CC & CC::I) != CC::I)
			PC_lo = Read(0xfff9);
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
//		  Registers: S
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		break;
	case 4:		// R	Don't Care			Effective Address

		break;
	case 5:		// R	Don't Care			$ffff
		break;
	case 6:		// W	Retern Address Low	SP-1
		 Write(--reg_S, PC_lo);
		 break;
	case 7:		// W	Return Address High	SP-2
		Write(--reg_S, PC_hi);
		reg_PC = offset;
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
//		  Registers: S
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 3:		// R	Offset Low			PC+2
		(this->*mode)(1);
		++reg_PC;
		break;
	case 4:		// R	Don't Care			$ffff
		break;
	case 5:		// R	Don't Care			$ffff
		break;
	case 6:		// R	Don't Care			Effective Address

		break;
	case 7:		// R	Don't Care			$ffff
		break;
	case 8:		// W	Retern Address Low	SP-1
		Write(--reg_S, PC_lo);
		break;
	case 9:		// W	Return Address High	SP-2
		Write(--reg_S, PC_hi);
		reg_PC = offset;
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
//		  Registers: S
//	Condition Codes: - - - - - - - -
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
//		  Registers: S
//	Condition Codes: - - - - - - - -
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 3:		// R	Offset Low			PC+1
		(this->*mode)(1);
		++reg_PC;
		break;
	case 4:		// R	Don't Care			$ffff
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
//*****************************************************************************
// Params:
//	None
// Returns:
//	uint8_t	- the number of clock cycles used, or 255 to signify function is
//				completed.
//*****************************************************************************
uint8_t Cpu6809::JMP()
{

	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
	case 2:		// R	Address Low, Address High, Post Byte (depends on mode)	PC+1
		
	case 3:
		break;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
	case 3:		// R	Offset High			PC+2
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C) 
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) == CC::Z)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) != CC::Z)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = reg_scratch;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ( ((reg_CC & CC::C) == CC::C) == ((reg_CC & CC::V) == CC::V) )
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) == CC::C) == ((reg_CC & CC::V) == CC::V)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ( (((reg_CC & CC::N) == CC::N) && ((reg_CC & CC::V) == CC::V)) && ((reg_CC & CC::Z) == 0))
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if (!((((reg_CC & CC::N) == CC::N) && ((reg_CC & CC::V) == CC::V)) && ((reg_CC & CC::Z) == 0)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ( ((reg_CC & CC::C) != CC::C) && ((reg_CC & CC::Z) != CC::Z))
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) != CC::C) && ((reg_CC & CC::Z) != CC::Z)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if (((reg_CC & CC::Z) == CC::Z) || 
			(((reg_CC & CC::N) == CC::N) && (reg_CC & CC::V) != CC::V) || 
			(((reg_CC & CC::N) != CC::N) && (reg_CC & CC::V) == CC::V))
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::Z) == CC::Z) ||
			(((reg_CC & CC::N) == CC::N) && (reg_CC & CC::V) != CC::V) ||
			(((reg_CC & CC::N) != CC::N) && (reg_CC & CC::V) == CC::V)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if (((reg_CC & CC::C) == CC::C) || ((reg_CC & CC::Z) == CC::Z))
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) == CC::C) || ((reg_CC & CC::Z) == CC::Z)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if (((reg_CC & CC::C) == CC::C) != ((reg_CC & CC::N) == CC::N))
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if (!(((reg_CC & CC::C) == CC::C) != ((reg_CC & CC::N) == CC::N)))
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) == CC::N)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) != CC::N)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) != CC::Z)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::Z) == CC::Z)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) != CC::N)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::N) == CC::N)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) != CC::V)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) == CC::V)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(3);
		++reg_PC;
		break;
	case 3:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) == CC::V)
			reg_PC = offset;
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
//		  Registers: -
//	Condition Codes: - - - - - - - -
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
		(this->*mode)(0);
		++reg_PC;
		break;
	case 4:		// R	Offset Low			PC+3
		(this->*mode)(1);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			$ffff
		if ((reg_CC & CC::V) != CC::V)
			clocksUsed = 255;
		break;
	case 6:
		reg_PC = offset;
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
//		  Registers: S
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
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		break;
	case 2:		// R	Opcode 2nd Byte		PC+1
		addressMode = (this->*mode)(0);
		++reg_PC;
		break;
	}
	switch (addressMode)
	{
	case ADDR_MODE::IdxZeroOffset:
		exec = this->LeaS_None;
	case ADDR_MODE::IdxConstantOffset:
		exec = this->LeaS_Const;
		break;
	case ADDR_MODE::IdxAccumulatorOffset:
		exec = this->LeaS_Accumulator;
		break;
	case ADDR_MODE::IdxAutoIncDec:
		exec = this->LeaS_AutoIncDec;
		break;
	case ADDR_MODE::IdxIndirect:
		exec = this->LeaS_Indirect;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LEAU()
//*****************************************************************************
// Load Effective Address U		(user stack)
//*****************************************************************************
// MODIFIES:
//		  Registers: U
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
//		  Registers: X
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
//		  Registers: Y
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
//		  Registers: X
//	Condition Codes:  - - - - - - - -
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
//		  Registers: A
//	Condition Codes: - - ~ - N Z V C
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
//		  Registers: B
//	Condition Codes: - - ~ - N Z V C
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
//		  Registers: A
//	Condition Codes: - - ~ - N Z V C
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
//		  Registers: B
//	Condition Codes: - - ~ - N Z V C
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
//		  Registers: A
//	Condition Codes: - - - - N Z V -
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
//		  Registers: B
//	Condition Codes: - - - - N Z V -
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
//		  Registers: A
//	Condition Codes: - - ~ - N Z V C
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
//		  Registers: B
//	Condition Codes: - - ~ - N Z V C
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
		reg_CC |= (CC::C);
		reg_CC &= ~(CC::V | CC::N | CC::Z);
		reg_CC |= (reg_A == 0) ? CC::Z : 0x00;
		reg_CC |= ((reg_A & 0x80) == 0x80) ? CC::N : 0x00;
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
		reg_CC |= (CC::C);
		reg_CC &= ~(CC::V | CC::N | CC::Z);
		reg_CC |= (reg_B == 0) ? CC::Z : 0x00;
		reg_CC |= ((reg_B & 0x80) == 0x80) ? CC::N : 0x00;
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
		reg_CC = ((reg_A & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		--reg_A;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
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
		reg_CC = ((reg_B & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		--reg_B;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
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
		reg_CC = ((reg_A & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_A;
		reg_CC = (reg_A == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
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
		reg_CC = ((reg_B & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_B;
		reg_CC = (reg_B == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
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
//		  Registers: A
//	Condition Codes: - - H - N Z V C
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
//	ANDCC()
//*****************************************************************************
//	AND the Condition Codes with the specified value
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
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Data				PC+1
		(this->*mode)(1);
		break;
	case 3:		// R	Don't Care			$ffff
		reg_CC &= offset_lo;
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
uint8_t Cpu6809::ASL()          // Arithmetic Shift Left Memory location (Logical Shift Left fill LSb with 0)
{
	static ADDR_MODE addrMode;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
	case 2:		// R	Address Low/High / Post Byte		PC+1
		addrMode = (ADDR_MODE)(this->*mode)(0);
		++reg_PC;
		break;
	}
	switch (addrMode)
	{
	case ADDR_MODE::Direct:
		switch (++clocksUsed)
		{
		case 3:		// R  Don't Care	$ffff
			break;
		case 4:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 5:		// R  Don't Care	$ffff
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
			scratch_lo = scratch_lo << 1;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			reg_CC = (((scratch_lo >> 6) & 1) != ((scratch_lo >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
			break;
		case 6:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::Extended:
		switch (++clocksUsed)
		{
		case 3:		// R  Address Low	PC+2
			(ADDR_MODE)(this->*mode)(1);
			++reg_PC;
			break;
		case 4:		// R  Don't Care	$ffff
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
			scratch_lo = scratch_lo << 1;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			reg_CC = (((scratch_lo >> 6) & 1) != ((scratch_lo >> 7) & 1)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
			break;
		case 5:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 6:		// R  Don't Care	$ffff
			break;
		case 7:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::IdxAccumulatorOffset:
	case ADDR_MODE::IdxAutoIncDec:
	case ADDR_MODE::IdxConstantOffset:
	case ADDR_MODE::IdxIndirect:
	case ADDR_MODE::IdxZeroOffset:
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
uint8_t Cpu6809::ASR()          // Arithmetic Shift Right Memory location (fill MSb with Sign bit)
{
	static ADDR_MODE addrMode;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
	case 2:		// R	Address Low/High / Post Byte		PC+1
		addrMode = (ADDR_MODE)(this->*mode)(0);
		++reg_PC;
		break;
	}
	switch (addrMode)
	{
	case ADDR_MODE::Direct:
		switch (++clocksUsed)
		{
		case 3:		// R  Don't Care	$ffff
			break;
		case 4:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 5:		// R  Don't Care	$ffff
			scratch_hi &= scratch_lo & 0x80;
			reg_CC = ((scratch_lo & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
			scratch_lo = ((scratch_lo >> 1) & 0x7f) | scratch_hi;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			break;
		case 6:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::Extended:
		switch (++clocksUsed)
		{
		case 3:		// R  Address Low	PC+2
			(ADDR_MODE)(this->*mode)(1);
			++reg_PC;
			break;
		case 4:		// R  Don't Care	$ffff
			scratch_hi &= scratch_lo & 0x80;
			reg_CC = ((scratch_lo & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
			scratch_lo = ((scratch_lo >> 1) & 0x7f) | scratch_hi;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			break;
		case 5:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 6:		// R  Don't Care	$ffff
			break;
		case 7:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::IdxAccumulatorOffset:
	case ADDR_MODE::IdxAutoIncDec:
	case ADDR_MODE::IdxConstantOffset:
	case ADDR_MODE::IdxIndirect:
	case ADDR_MODE::IdxZeroOffset:
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
uint8_t Cpu6809::CLR()          // Clear memory location
{
	static ADDR_MODE addrMode;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
	case 2:		// R	Address Low/High / Post Byte		PC+1
		addrMode = (ADDR_MODE)(this->*mode)(0);
		++reg_PC;
		break;
	}
	switch (addrMode)
	{
	case ADDR_MODE::Direct:
		switch (++clocksUsed)
		{
		case 3:		// R  Don't Care	$ffff
			break;
		case 4:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 5:		// R  Don't Care	$ffff
			reg_CC = (reg_CC & (CC::H | CC::I | CC::E | CC::F)) | CC::Z | CC::C;
			break;
		case 6:		// W  Data			EA
			Write(offset, 0x00);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::Extended:
		switch (++clocksUsed)
		{
		case 3:		// R  Address Low	PC+2
			(ADDR_MODE)(this->*mode)(1);
			++reg_PC;
			break;
		case 4:		// R  Don't Care	$ffff
			break;
		case 5:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 6:		// R  Don't Care	$ffff
			reg_CC = (reg_CC & (CC::H | CC::I | CC::E | CC::F)) | CC::Z | CC::C;
			break;
		case 7:		// W  Data			EA
			Write(offset, 0x00);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::IdxAccumulatorOffset:
	case ADDR_MODE::IdxAutoIncDec:
	case ADDR_MODE::IdxConstantOffset:
	case ADDR_MODE::IdxIndirect:
	case ADDR_MODE::IdxZeroOffset:
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
{
	static ADDR_MODE addrMode;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
	case 2:		// R	Address Low/High / Post Byte		PC+1
		addrMode = (ADDR_MODE)(this->*mode)(0);
		++reg_PC;
		break;
	}
	switch (addrMode)
	{
	case ADDR_MODE::Direct:
		switch (++clocksUsed)
		{
		case 3:		// R  Don't Care	$ffff
			break;
		case 4:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 5:		// R  Don't Care	$ffff
			scratch_lo ^= 0xff;
			reg_CC |= (CC::C);
			reg_CC &= ~(CC::V | CC::N | CC::Z);
			reg_CC |= (scratch_lo == 0) ? CC::Z : 0x00;
			reg_CC |= ((scratch_lo & 0x80) == 0x80) ? CC::N : 0x00;
			break;
		case 6:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::Extended:
		switch (++clocksUsed)
		{
		case 3:		// R  Address Low	PC+2
			(ADDR_MODE)(this->*mode)(1);
			++reg_PC;
			break;
		case 4:		// R  Don't Care	$ffff
			scratch_lo ^= 0xff;
			break;
		case 5:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 6:		// R  Don't Care	$ffff
			scratch_lo ^= 0xff;
			reg_CC |= (CC::C);
			reg_CC &= ~(CC::V | CC::N | CC::Z);
			reg_CC |= (scratch_lo == 0) ? CC::Z : 0x00;
			reg_CC |= ((scratch_lo & 0x80) == 0x80) ? CC::N : 0x00;
			break;
		case 7:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::IdxAccumulatorOffset:
	case ADDR_MODE::IdxAutoIncDec:
	case ADDR_MODE::IdxConstantOffset:
	case ADDR_MODE::IdxIndirect:
	case ADDR_MODE::IdxZeroOffset:
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
uint8_t Cpu6809::DEC()          // Decrement Memory location
{
	static ADDR_MODE addrMode;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
	case 2:		// R	Address Low/High / Post Byte		PC+1
		addrMode = (ADDR_MODE)(this->*mode)(0);
		++reg_PC;
		break;
	}
	switch (addrMode)
	{
	case ADDR_MODE::Direct:
		switch (++clocksUsed)
		{
		case 3:		// R  Don't Care	$ffff
			break;
		case 4:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 5:		// R  Don't Care	$ffff
			reg_CC = ((scratch_lo & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
			--scratch_lo;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			break;
		case 6:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::Extended:
		switch (++clocksUsed)
		{
		case 3:		// R  Address Low	PC+2
			(ADDR_MODE)(this->*mode)(1);
			++reg_PC;
			break;
		case 4:		// R  Don't Care	$ffff
			scratch_lo ^= 0xff;
			break;
		case 5:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 6:		// R  Don't Care	$ffff
			reg_CC = ((scratch_lo & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
			--scratch_lo;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			break;
		case 7:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::IdxAccumulatorOffset:
	case ADDR_MODE::IdxAutoIncDec:
	case ADDR_MODE::IdxConstantOffset:
	case ADDR_MODE::IdxIndirect:
	case ADDR_MODE::IdxZeroOffset:
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
{
	static ADDR_MODE addrMode;
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
		++reg_PC;
	case 2:		// R	Address Low/High / Post Byte		PC+1
		addrMode = (ADDR_MODE)(this->*mode)(0);
		++reg_PC;
		break;
	}
	switch (addrMode)
	{
	case ADDR_MODE::Direct:
		switch (++clocksUsed)
		{
		case 3:		// R  Don't Care	$ffff
			break;
		case 4:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 5:		// R  Don't Care	$ffff
			reg_CC = ((scratch_lo & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
			++scratch_lo;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			break;
		case 6:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::Extended:
		switch (++clocksUsed)
		{
		case 3:		// R  Address Low	PC+2
			(ADDR_MODE)(this->*mode)(1);
			++reg_PC;
			break;
		case 4:		// R  Don't Care	$ffff
			scratch_lo ^= 0xff;
			break;
		case 5:		// R  Data			EA
			scratch_lo = Read(offset);
			break;
		case 6:		// R  Don't Care	$ffff
			reg_CC = ((scratch_lo & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
			++scratch_lo;
			reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
			reg_CC = ((scratch_lo & 0x80) != 0) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
			break;
		case 7:		// W  Data			EA
			Write(offset, scratch_lo);
			clocksUsed = 0xff;
			break;
		}
		break;
	case ADDR_MODE::IdxAccumulatorOffset:
	case ADDR_MODE::IdxAutoIncDec:
	case ADDR_MODE::IdxConstantOffset:
	case ADDR_MODE::IdxIndirect:
	case ADDR_MODE::IdxZeroOffset:
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
{sd}


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
{as}


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
//	ORCC()
//*****************************************************************************
//	OR the condition code register with the given value
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
{
	switch (++clocksUsed)
	{
	case 1:		// R Opcode Fetch			PC
		break;
	case 2:		// R	Data				PC+1
		(this->*mode)(1);
		break;
	case 3:		// R	Don't Care			$ffff
		reg_CC |= offset_lo;
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
//	TST()
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
{
	switch (++clocksUsed)
	{
	case 1:		// R	Opcode Fetch		PC
	case 2:		// R	..					PC+1
		addressMode = (ADDR_MODE)(this->*mode)(1);
		++reg_PC;
		break;
		if (addressMode == ADDR_MODE::Direct)
		{
			exec = &Cpu6809::TST_DIR;
		}
		else if (addressMode == ADDR_MODE::Extended)
		{
			offset_hi = offset_lo;
			exec = &Cpu6809::TST_EXT;
		}
		else if (clocksUsed > 2 && addressMode == ADDR_MODE::Indexed)
		{
		}
	}
	return(255); 
}


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
// exceptions, as the invalid instruction exception trap does not exist.
// Instead, we usually wind up with an instruction that is close in function
// to a nearby instruction, some depend on whether the CC::?  is clear or set
// as to exactly what is carried out. AS invalid instructions are undefined,
// including the Halt And Catch Fire opcode(s), these are ignored and funneled
// through this function. (Depending on build configuration, this includes the
// RESET "interrupt" opcode $3E)
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

	return(255);
}


//*********************************************************************************************************************************
//	Address Modes (If Opcode's mnemonic is associated with only one opcode, the function may handle it internally, otherwise it
// invokes one of thse address mode functions.
//*********************************************************************************************************************************

//*****************************************************************************
//	INH()
//*****************************************************************************
//	Inherent Address mode - Opcode defines what is affected, no other bytes of
//	memory are required to perform an operation
//
// NOTE: no registers or emulator variables are effected.
//*****************************************************************************
// MODIFIES:
//		  Registers:
//	Condition Codes:
//*****************************************************************************
// Params:
//	None
// Returns:
//	ADDR_MODE - Address Mode ID  for this address mode(enum)-
//*****************************************************************************
Cpu6809::ADDR_MODE Cpu6809::INH(uint8_t adjClock)
{
	// don't have to do anything...
	return(ADDR_MODE::Inherent);
}


//*****************************************************************************
//	Immediate()
//*****************************************************************************
//	Immediate Address mode... opcode(s) followed by a single or double byte
// value to store in a register.
//
// NOTE: variable offset loads the high byte on first pass, low byte on second
//			(or just the low if second pass only) for DATA values
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	ADDR_MODE - Address Mode ID  for this address mode(enum)
//*****************************************************************************
Cpu6809::ADDR_MODE Cpu6809::IMM(uint8_t adjClock)
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
	return(ADDR_MODE::Immediate);
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
//
// NOTE: variable offset loads the high byte on first pass from reg_DP,
//			low byte on secondv (or just the low if second pass only) for a
//			MEMORY ADDRESS
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	ADDR_MODE - Address Mode ID  for this address mode(enum)
//*****************************************************************************
Cpu6809::ADDR_MODE Cpu6809::DIR(uint8_t adjClock)
{
	offset = (reg_DP << 8) | Read(reg_DP);

	return(ADDR_MODE::Direct);
}


//*****************************************************************************
//	EXT()
//*****************************************************************************
//	Extended Address mode. Uses the next two bytes following the opcode(s) to
// specifiy the exact memory address (as in what the CPU can directly access)
// the opcode functions on.
//
// NOTE: variable offset loads the high byte on first pass, low byte on second
//			for a MEMORY ADDRESS
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	ADDR_MODE - Address Mode ID  for this address mode(enum)
//*****************************************************************************
Cpu6809::ADDR_MODE Cpu6809::EXT(uint8_t adjClock)
{
	switch (adjClock)
	{
	case 0:
		offset_hi = Read(reg_DP);
		break;
	case 1:
		offset_lo = Read(reg_DP);
		break;
	}
	return(ADDR_MODE::Extended);
}


//*****************************************************************************
//	IDX()
//*****************************************************************************
//	Indexed Address mode
//
// NOTE: 
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	ADDR_MODE - Address Mode ID  for this address mode(enum)
//*****************************************************************************
Cpu6809::ADDR_MODE Cpu6809::IDX(uint8_t adjClock)
{
	offset_lo = Read(reg_PC);
	uint8_t offsetValue = offset_lo & 0x9f;

	if ((offset_lo & 0x80) != 0x80)		// 5-bit constant offset
		;
	else
	{
		switch (offsetValue)
		{
			// Non Indirect
		case 0x00:						// Auto increment Register by 1
		case 0x01:						// Auto increment Register by 2
		case 0x02:						// Auto decrement Register by 1
		case 0x03:						// Auto decrement Register by 2
			return(ADDR_MODE::IdxAutoIncDec);
		case 0x04:						// No constant offset		(2s complement offsets)
			return(ADDR_MODE::IdxZeroOffset);
		case 0x05:						// reg_B  Register offset	(2s complement offsets)
		case 0x06:						// reg_A  Register offset	(2s complement offsets)
			return(ADDR_MODE::IdxAccumulatorOffset);
		case 0x08:						// 8-bit constant offset
		case 0x09:						// 16-bit constant offset
			return(ADDR_MODE::IdxConstantOffset);
		case 0x0b:						// reg_D offset
			return(ADDR_MODE::IdxAccumulatorOffset);
		case 0x0c:						// 8-bit constant offset from PC
		case 0x0d:						// 16-bit constanrt offset from PC

			// Indirect
		case 0x11:						// Indirect increment by 2
		case 0x13:						// Indirect decrement by 2
			return(ADDR_MODE::IdxAutoIncDec);
		case 0x14:						// Indirect no offset
			return(ADDR_MODE::IdxZeroOffset);
		case 0x15:						// Indirect reg_B offset
		case 0x16:						// Indirect reg_A offset
			return(ADDR_MODE::IdxAccumulatorOffset);
		case 0x18:						// Indirect 8-bit offset (5-bit defaults to 8-bit)
		case 0x19:						// Indirect 16-bit offset
			return(ADDR_MODE::IdxConstantOffset);
		case 0x1b:						// Indirect reg_D offset
			return(ADDR_MODE::IdxAccumulatorOffset);
		case 0x1c:						// Indirect  8-bit constant offset from PC (2s complement offsets)
		case 0x1d:						// Indirect 16-bit constant offset from PC (2s complement offsets)
			return(ADDR_MODE::IdxAccumulatorOffset);
		case 0x1f:						// Extended Indirect
			return(ADDR_MODE::I);
			break;

			// Undefined or Not Allowed
		case 0x07:			// UNDEFINED
		case 0x0a:			// UNDEFINED
		case 0x0e:			// UNDEFINED
		case 0x0f:			// UNDEFINED
		case 0x17:			// UNDEFINED
		case 0x1a:			// UNDEFINED
		case 0x1e:			// UNDEFINED
		case 0x10:			// NOT ALLOWED
		case 0x12:			// NOT ALLOWED
		default:
			clocksUsed = 0xff;
			break;
		}
	}
}


//*****************************************************************************
//	REL()
//*****************************************************************************
//	. Address mode
//
// NOTE: variable offset loads the high byte on first pass, low byte on second
//			and added to reg_PC for a MEMORY ADDRES. Passes 0  & 1 are for
//			16-bit long or extended? relative addressing,
//			3 is for a single byte relative.
//*****************************************************************************
// Params:
//	uint9_t	- adjusted clock... as each opcode mnemonic may represent one or
//				two bytes of opcode, and therefore different clock cycle pulses
// Returns:
//	ADDR_MODE - Address Mode ID  for this address mode(enum)
//*****************************************************************************
Cpu6809::ADDR_MODE Cpu6809::REL(uint8_t adjClock)
{
	switch(adjClock)
	{ 
	case 0:
		offset_hi = Read(reg_PC);
		break;
	case 1:
		offset_lo = Read(reg_PC);
	case 2:
		offset = reg_PC + offset;
		break;
		// Above: two byte extended relative (long : LB??)
		// Below single byte extended relative (normal: B??)
	case 3:
		offset_lo = Read(reg_PC);
		offset_hi = (offset_lo & 0x80) ? 0xff : 0x00;
		offset = reg_PC + offset;
		break;
	}
	return(ADDR_MODE::Relative);
}


//*********************************************************************************************************************************
//	Address Modes (If Opcode's mnemonic is associated with only one opcode, the function may handle it internally, otherwise it
// invokes one of thse address mode functions.
//*********************************************************************************************************************************


uint8_t Cpu6809::LeaS_None()
{
	uint8_t eaRegister = ((offset_lo & 0x60) >> 5);

	if (((offset_lo & 0x10) == 0x10))
	{
		switch (++clocksUsed)
		{
		case 3:
			reg_scratch = (offset_lo & 0x1f) | ((offset_lo & 0x10) == 0x10 ? 0xfff0 : 0x0000);
			clocksUsed = 0xff;
			switch (eaRegister)
			{
			case 0:
				reg_S = reg_scratch + reg_X;
				break;
			case 1:
				reg_S = reg_scratch + reg_Y;
				break;
			case 2:
				reg_S = reg_scratch + reg_U;
				break;
			case 3:
				reg_S = reg_scratch + reg_S;
				break;
			}
		}
	}
	else
	{
		switch (++clocksUsed)
		{
		case 3:
			reg_scratch = (offset_lo & 0x1f) | (((offset_lo & 0x10) == 0x10) ? 0xfff0 : 0x0000);
			switch (eaRegister)
			{
			case 0:
				reg_S = reg_scratch + reg_X;
				break;
			case 1:
				reg_S = reg_scratch + reg_Y;
				break;
			case 2:
				reg_S = reg_scratch + reg_U;
				break;
			case 3:
				reg_S = reg_scratch + reg_S;
				break;
			}
		case 4:
			reg_S = Read(reg_S);
			clocksUsed = 0xff;
		}
	}
	return(clocksUsed);
}

uint8_t Cpu6809::LeaS_Const()
{}

uint8_t Cpu6809::LeaS_Accumulator()
{}

uint8_t Cpu6809::LeaS_AutoIncDec()
{

}


uint8_t Cpu6809::LeaS_Indirect()
{
	switch (++clocksUsed)
	{
	case 3:		// R	Address High		PC+2
		scratch_hi = Read(reg_PC);
		++reg_PC;
		break;
	case 4:		// R	Address Low			PC+3
		scratch_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			PC+4
		++reg_PC;
		break;
	case 6:		// R	Indirect High		IX
		S_hi = Read(reg_scratch);
		++reg_scratch;
		break;
	case 7:		// R	Indirect Low		IX+1
		S_lo = Read(reg_scratch);
		break;
	case 8:		// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


uint8_t Cpu6809::LeaU_None()
{
	uint8_t eaRegister = ((offset_lo & 0x60) >> 5);

	if (((offset_lo & 0x10) == 0x10))
	{
		switch (++clocksUsed)
		{
		case 3:
			reg_scratch = (offset_lo & 0x1f) | (((offset_lo & 0x10) == 0x10) ? 0xfff0 : 0x0000);
			clocksUsed = 0xff;
			switch (eaRegister)
			{
			case 0:
				reg_U = reg_scratch + reg_X;
				break;
			case 1:
				reg_U = reg_scratch + reg_Y;
				break;
			case 2:
				reg_U = reg_scratch + reg_U;
				break;
			case 3:
				reg_U = reg_scratch + reg_U;
				break;
			}
		}
	}
	else
	{
		switch (++clocksUsed)
		{
		case 3:
			reg_scratch = (offset_lo & 0x1f) | (((offset_lo & 0x10) == 0x10) ? 0xfff0 : 0x0000);
			switch (eaRegister)
			{
			case 0:
				reg_U = reg_scratch + reg_X;
				break;
			case 1:
				reg_U = reg_scratch + reg_Y;
				break;
			case 2:
				reg_U = reg_scratch + reg_U;
				break;
			case 3:
				reg_U = reg_scratch + reg_U;
				break;
			}
		case 4:
			reg_U = Read(reg_U);
			clocksUsed = 0xff;
		}
	}
	return(clocksUsed);
}

uint8_t Cpu6809::LeaU_Const()
{
}

uint8_t Cpu6809::LeaU_Accumulator()
{}

uint8_t Cpu6809::LeaU_AutoIncDec()
{}


uint8_t Cpu6809::LeaU_Indirect()
{
	switch (++clocksUsed)
	{
	case 3:		// R	Address High		PC+2
		scratch_hi = Read(reg_PC);
		++reg_PC;
		break;
	case 4:		// R	Address Low			PC+3
		scratch_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			PC+4
		++reg_PC;
		break;
	case 6:		// R	Indirect High		IX
		U_hi = Read(reg_scratch);
		++reg_scratch;
		break;
	case 7:		// R	Indirect Low		IX+1
		U_lo = Read(reg_scratch);
		break;
	case 8:		// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


uint8_t Cpu6809::LeaX_None()
{
	uint8_t eaRegister = ((offset_lo & 0x60) >> 5);

	if (((offset_lo & 0x10) == 0x10))
	{
		switch (++clocksUsed)
		{
		case 3:
			reg_scratch = (offset_lo & 0x1f) | (((offset_lo & 0x10) == 0x10) ? 0xfff0 : 0x0000);
			clocksUsed = 0xff;
			switch (eaRegister)
			{
			case 0:
				reg_X = reg_scratch + reg_X;
				break;
			case 1:
				reg_X = reg_scratch + reg_Y;
				break;
			case 2:
				reg_X = reg_scratch + reg_U;
				break;
			case 3:
				reg_X = reg_scratch + reg_X;
				break;
			}
		}
	}
	else
	{
		switch (++clocksUsed)
		{
		case 3:
			reg_scratch = (offset_lo & 0x1f) | (((offset_lo & 0x10) == 0x10) ? 0xfff0 : 0x0000);
			switch (eaRegister)
			{
			case 0:
				reg_X = reg_scratch + reg_X;
				break;
			case 1:
				reg_X = reg_scratch + reg_Y;
				break;
			case 2:
				reg_X = reg_scratch + reg_U;
				break;
			case 3:
				reg_X = reg_scratch + reg_X;

				break;
			}
		case 4:
			reg_X = Read(reg_X);
			clocksUsed = 0xff;
		}
	}
	return(clocksUsed);
}

uint8_t Cpu6809::LeaX_Const()
{}

uint8_t Cpu6809::LeaX_Accumulator()
{}

uint8_t Cpu6809::LeaX_AutoIncDec()
{}


uint8_t Cpu6809::LeaX_Indirect()
{
	switch (++clocksUsed)
	{
	case 3:		// R	Address High		PC+2
		scratch_hi = Read(reg_PC);
		++reg_PC;
		break;
	case 4:		// R	Address Low			PC+3
		scratch_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			PC+4
		++reg_PC;
		break;
	case 6:		// R	Indirect High		IX
		X_hi = Read(reg_scratch);
		++reg_scratch;
		break;
	case 7:		// R	Indirect Low		IX+1
		X_lo = Read(reg_scratch);
		break;
	case 8:		// R	Don't Care			$ffff
		reg_CC = (reg_X == 0x0000) ? (reg_CC & ~CC::Z) : (reg_CC | CC::Z);
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


uint8_t Cpu6809::LeaY_None()
{}

uint8_t Cpu6809::LeaY_Const()
{}

uint8_t Cpu6809::LeaY_Accumulator()
{}

uint8_t Cpu6809::LeaY_AutoIncDec()
{}


uint8_t Cpu6809::LeaY_Indirect()
{
	switch (++clocksUsed)
	{
	case 3:		// R	Address High		PC+2
		scratch_hi = Read(reg_PC);
		++reg_PC;
		break;
	case 4:		// R	Address Low			PC+3
		scratch_lo = Read(reg_PC);
		++reg_PC;
		break;
	case 5:		// R	Don't Care			PC+4
		++reg_PC;
		break;
	case 6:		// R	Indirect High		IX
		Y_hi = Read(reg_scratch);
		++reg_scratch;
		break;
	case 7:		// R	Indirect Low		IX+1
		Y_lo = Read(reg_scratch);
		break;
	case 8:		// R	Don't Care			$ffff
		reg_CC = (reg_Y == 0x0000) ? (reg_CC & ~CC::Z) : (reg_CC | CC::Z);
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TST_DIR()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset. - Direct [Page] Address Mode
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
uint8_t Cpu6809::TST_DIR()
{
	switch (++clocksUsed)
	{
	case 3:		// R	Don't Care			$ffff
		break;
	case 4:		// R	Data				EA
		scratch_lo = Read(offset);
		break;
	case 5:		// R	Don't Care			$ffff
		reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (scratch_lo == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC &= ~CC::V;
		break;
	case 6:		// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TST_EXT()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
// reset. - Extended Address Mode
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
uint8_t Cpu6809::TST_EXT()
{
	switch (++clocksUsed)
	{
	case 3:		// R	Address Low			PC+2
		(this->*mode)(1);
		++reg_PC;
		break;
	case 4:		// R	Don't Care			$ffff
		break;
	case 5:		// R	Data				EA
		scratch_lo = Read(offset);
		break;
	case 6:		// R	Don't Care			$ffff
		reg_CC = (scratch_lo == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
		reg_CC = (scratch_lo == 0x80) ? (reg_CC | CC::N) : (reg_CC & ~CC::N);
		reg_CC &= ~CC::V;
		break;
	case 7:		// R	Don't Care			$ffff
		clocksUsed = 0xff;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TST()
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
uint8_t Cpu6809::TST_IDX()
{
	switch (++clocksUsed)
	{
	case 3:
	case 4:
	case 5:
	case 6:
		break;
	}
	return(clocksUsed);
}
