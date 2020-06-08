/******************************************************************************
*		   File: Mc6809.h
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author: William Barnes
*		Created: 2020/05/30
*	  Copyright: 2020 - under Apache 2.0 Licensing
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Modifications: (Who, whenm, what)
*
******************************************************************************/
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CPU.h"
#include "MMU.h"

#define MC6809E

class Mc6809 : 	public CPU
{
	// variables
private:
	MMU* bus;
	uint8_t(Mc6809::* exec)();						// Mnemonic function from interpreted Opcode

	uint8_t clocksUsed;

protected:
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

	// Registers, Program/App accessible and internal to CPU only
	uint8_t reg_CC;				// Condition Code register		(Program Accessible)	0B
	uint8_t reg_DP;				// Direct Page register.		(Program Accessible)	0A

	union		// Registers: Accumulator A, B, and D
	{
		struct
		{
			uint8_t reg_B;		// register B, low byte of D	(Program Accessible)	09
			uint8_t reg_A;		// register A, hi byte of D		(Program Accessible)	08
		};
		uint16_t reg_D;			// register D, combo of A and B	(Program Accessible)	00
	};

	union		// Index Register X
	{
		struct
		{
			uint8_t X_lo;		// index register X low byte	(INTERNAL CPU USE ONLY)
			uint8_t X_hi;		// index register X hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_X;			// index register X				(Program Accessible)	01
	};
	union		// Index Register Y
	{
		struct
		{
			uint8_t Y_lo;		// index register Y low byte	(INTERNAL CPU USE ONLY)
			uint8_t Y_hi;		// index register Y hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_Y;			// index register Y				(Program Accessible)	02
	};
	union		// User Stack Pointer
	{
		struct
		{
			uint8_t U_lo;		// User Stack Pointer low byte		(INTERNAL CPU USE ONLY)
			uint8_t U_hi;		// User Stack Pointer hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_U;			// register U User Stack Pointer	(Program Accessible)	03
	};
	union		// System Stack Pointer
	{
		struct
		{
			uint8_t S_lo;		// System Stack Pounter low byte	(INTERNAL CPU USE ONLY)
			uint8_t S_hi;		// System Stack Pounter hi byte		(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_S;			// register S, System Stack Pounter	(Program Accessible)	04
	};
	union		// Program Counter
	{
		struct
		{
			uint8_t PC_lo;		// Program Counter low byte			(INTERNAL CPU USE ONLY)
			uint8_t PC_hi;		// Program Counter hi byte			(INTERNAL CPU USE ONLY)
		};
		uint16_t reg_PC;		// Program Counter					(Program Accessible)	05
	};
	union		// Scratch Register
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
		uint8_t(Mc6809::* opcode)();						// Mnemonic function from interpreted Opcode
		uint8_t minCycles;
		uint8_t maxCycles;
		uint8_t pgmBytes;
	};

	std::vector<OPCODE> OpCode[3];
	uint8_t opCodePage;


public:
	volatile bool Halt = false;
	volatile bool Reset = true;
	volatile bool Nmi = false;
	volatile bool Firq = false;
	volatile bool Irq = false;

	// functions
private:

protected:
	// opcodes and their address modes
	uint8_t ABX_inh();			uint8_t ADCA_dir();			uint8_t ADCA_ext();			uint8_t ADCA_idx();
	uint8_t ADCA_imm();			uint8_t ADCB_dir();			uint8_t ADCB_ext();			uint8_t ADCB_idx();
	uint8_t ADCB_imm();			uint8_t ADDA_dir();			uint8_t ADDA_ext();			uint8_t ADDA_idx();
	uint8_t ADDA_imm();			uint8_t ADDB_dir();			uint8_t ADDB_ext();			uint8_t ADDB_idx();
	uint8_t ADDB_imm();			uint8_t ADDD_dir();			uint8_t ADDD_ext();			uint8_t ADDD_idx();
	uint8_t ADDD_imm();			uint8_t ANDA_dir();			uint8_t ANDA_ext();			uint8_t ANDA_idx();
	uint8_t ANDA_imm();			uint8_t ANDB_dir();			uint8_t ANDB_ext();			uint8_t ANDB_idx();
	uint8_t ANDB_imm();			uint8_t ANDCC_imm();		uint8_t ASLA_LSLA_inh();	uint8_t ASLB_LSLB_inh();
	uint8_t ASL_LSL_dir();		uint8_t ASL_LSL_ext();		uint8_t ASL_LSL_idx();		uint8_t ASRA_inh();
	uint8_t ASRB_inh();			uint8_t ASR_dir();			uint8_t ASR_ext();			uint8_t ASR_idx();
	uint8_t BEQ_rel();			uint8_t BGE_rel();			uint8_t BGT_rel();			uint8_t BHI_rel();
	uint8_t BHS_BCC_rel();		uint8_t BITA_dir();			uint8_t BITA_ext();			uint8_t BITA_idx();
	uint8_t BITA_imm();			uint8_t BITB_dir();			uint8_t BITB_ext();			uint8_t BITB_idx();
	uint8_t BITB_imm();			uint8_t BLE_rel();			uint8_t BLO_BCS_rel();		uint8_t BLS_rel();
	uint8_t BLT_rel();			uint8_t BMI_rel();			uint8_t BNE_rel();			uint8_t BPL_rel();
	uint8_t BRA_rel();			uint8_t BRN_rel();			uint8_t BSR_rel();			uint8_t BVC_rel();
	uint8_t BVS_rel();			uint8_t CLRA_inh();			uint8_t CLRB_inh();			uint8_t CLR_dir();
	uint8_t CLR_ext();			uint8_t CLR_idx();			uint8_t CMPA_dir();			uint8_t CMPA_ext();
	uint8_t CMPA_idx();			uint8_t CMPA_imm();			uint8_t CMPB_dir();			uint8_t CMPB_ext();
	uint8_t CMPB_idx();			uint8_t CMPB_imm();			uint8_t CMPD_dir();			uint8_t CMPD_ext();
	uint8_t CMPD_idx();			uint8_t CMPD_imm();			uint8_t CMPS_dir();			uint8_t CMPS_ext();
	uint8_t CMPS_idx();			uint8_t CMPS_imm();			uint8_t CMPU_dir();			uint8_t CMPU_ext();
	uint8_t CMPU_idx();			uint8_t CMPU_imm();			uint8_t CMPX_dir();			uint8_t CMPX_ext();
	uint8_t CMPX_idx();			uint8_t CMPX_imm();			uint8_t CMPY_dir();			uint8_t CMPY_ext();
	uint8_t CMPY_idx();			uint8_t CMPY_imm();			uint8_t COMA_inh();			uint8_t COMB_inh();
	uint8_t COM_dir();			uint8_t COM_ext();			uint8_t COM_idx();			uint8_t CWAI_inh();
	uint8_t DAA_inh();			uint8_t DECA_inh();			uint8_t DECB_inh();			uint8_t DEC_dir();
	uint8_t DEC_ext();			uint8_t DEC_idx();			uint8_t EORA_dir();			uint8_t EORA_ext();
	uint8_t EORA_idx();			uint8_t EORA_imm();			uint8_t EORB_dir();			uint8_t EORB_ext();
	uint8_t EORB_idx();			uint8_t EORB_imm();			uint8_t EXG_imm();			uint8_t INCA_inh();
	uint8_t INCB_inh();			uint8_t INC_dir();			uint8_t INC_idx();			uint8_t INC_ext();
	uint8_t JMP_dir();			uint8_t JMP_ext();			uint8_t JMP_idx();			uint8_t JSR_dir();
	uint8_t JSR_ext();			uint8_t JSR_idx();			uint8_t LBCS_LBLO_rel();	uint8_t LBEQ_rel();
	uint8_t LBGE_rel();			uint8_t LBGT_rel();			uint8_t LBHI_rel();			uint8_t LBHS_LBCC_rel();
	uint8_t LBLE_rel();			uint8_t LBLS_rel();			uint8_t LBLT_rel();			uint8_t LBMI_rel();
	uint8_t LBNE_rel();			uint8_t LBPL_rel();			uint8_t LBRA_rel();			uint8_t LBRN_rel();
	uint8_t LBSR_rel();			uint8_t LBVC_rel();			uint8_t LBVS_rel();			uint8_t LDA_dir();
	uint8_t LDA_ext();			uint8_t LDA_idx();			uint8_t LDA_imm();			uint8_t LDB_dir();
	uint8_t LDB_ext();			uint8_t LDB_idx();			uint8_t LDB_imm();			uint8_t LDD_dir();
	uint8_t LDD_ext();			uint8_t LDD_idx();			uint8_t LDD_imm();			uint8_t LDS_dir();
	uint8_t LDS_ext();			uint8_t LDS_idx();			uint8_t LDS_imm();			uint8_t LDU_dir();
	uint8_t LDU_ext();			uint8_t LDU_idx();			uint8_t LDU_imm();			uint8_t LDX_dir();
	uint8_t LDX_ext();			uint8_t LDX_idx();			uint8_t LDX_imm();			uint8_t LDY_dir();
	uint8_t LDY_ext();			uint8_t LDY_idx();			uint8_t LDY_imm();			uint8_t LEAS_idx();
	uint8_t LEAU_idx();			uint8_t LEAX_idx();			uint8_t LEAY_idx();			uint8_t LSRA_inh();
	uint8_t LSRB_inh();			uint8_t LSR_dir();			uint8_t LSR_ext();			uint8_t LSR_idx();
	uint8_t MUL_inh();			uint8_t NEGA_inh();			uint8_t NEGB_inh();			uint8_t NEG_dir();
	uint8_t NEG_ext();			uint8_t NEG_idx();			uint8_t NOP_inh();			uint8_t ORA_dir();
	uint8_t ORA_ext();			uint8_t ORA_idx();			uint8_t ORA_imm();			uint8_t ORB_dir();
	uint8_t ORB_ext();			uint8_t ORB_idx();			uint8_t ORB_imm();			uint8_t ORCC_imm();
	uint8_t PSHS_imm();			uint8_t PSHU_imm();			uint8_t PULS_imm();			uint8_t PULU_imm();
	uint8_t ROLA_inh();			uint8_t ROLB_inh();			uint8_t ROL_dir();			uint8_t ROL_ext();
	uint8_t ROL_idx();			uint8_t RORA_inh();			uint8_t RORB_inh();			uint8_t ROR_dir();
	uint8_t ROR_ext();			uint8_t ROR_idx();			uint8_t RTI_inh();			uint8_t RTS_inh();
	uint8_t SBCA_dir();			uint8_t SBCA_ext();			uint8_t SBCA_idx();			uint8_t SBCA_imm();
	uint8_t SBCB_dir();			uint8_t SBCB_ext();			uint8_t SBCB_idx();			uint8_t SBCB_imm();
	uint8_t SEX_inh();			uint8_t STA_dir();			uint8_t STA_ext();			uint8_t STA_idx();
	uint8_t STB_dir();			uint8_t STB_ext();			uint8_t STB_idx();			uint8_t STD_dir();
	uint8_t STD_ext();			uint8_t STD_idx();			uint8_t STS_dir();			uint8_t STS_ext();
	uint8_t STS_idx();			uint8_t STU_dir();			uint8_t STU_ext();			uint8_t STU_idx();
	uint8_t STX_dir();			uint8_t STX_ext();			uint8_t STX_idx();			uint8_t STY_dir();
	uint8_t STY_ext();			uint8_t STY_idx();			uint8_t SUBA_dir();			uint8_t SUBA_ext();
	uint8_t SUBA_idx();			uint8_t SUBA_imm();			uint8_t SUBB_dir();			uint8_t SUBB_ext();
	uint8_t SUBB_idx();			uint8_t SUBB_imm();			uint8_t SUBD_dir();			uint8_t SUBD_ext();
	uint8_t SUBD_idx();			uint8_t SUBD_imm();			uint8_t SWI_inh();			uint8_t SWI2_inh();
	uint8_t SWI3_inh();			uint8_t SYNC_inh();			uint8_t TFR_imm();			uint8_t TSTA_inh();
	uint8_t TSTB_inh();			uint8_t TST_dir();			uint8_t TST_ext();			uint8_t TST_idx();

	// pseudo-opcode, and implemented undocumented opcodes and their address modes
	uint8_t XXX();				uint8_t RESET_inh();

	// hardware functionality
	uint8_t HALT();			// hardware halt
	uint8_t RESET();		// hardware Reset
	uint8_t NMI();			// hardware NMI
	uint8_t FIRQ();			// hardware FIRQ
	uint8_t IRQ();			// hardware IRQ

	// internal functionality
	uint8_t Read(const uint16_t address, const bool readOnly = false);
	void Write(const uint16_t address, const uint8_t byte);
	uint8_t Fetch(const uint16_t address);

	void AdjustCC_H(uint8_t reg);
	void AdjustCC_N(uint8_t reg);
	void AdjustCC_Z(uint8_t reg);
	void AdjustCC_V(uint8_t reg1, uint8_t data, uint8_t result);
	void AdjustCC_V(uint8_t reg);
	void AdjustCC_C(uint16_t word);

//	void AdjustCC_H(uint16_t reg);
	void AdjustCC_N(uint16_t reg);
	void AdjustCC_Z(uint16_t reg);
	void AdjustCC_V(uint16_t reg1, uint16_t data, uint16_t result);
	void AdjustCC_C(uint32_t word);

public:
	Mc6809(MMU* device = nullptr);
	~Mc6809();

	void SetMMU(MMU* device);
	void Clock();
};
