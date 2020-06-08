/******************************************************************************
*		   File: Mc6809.cpp
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
*		 Author: William Barnes
*		Created: 2020/05/30
*	  Copyright: 2020 - under Apache 2.0 Licensing
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* Modifications: (Who, whenm, what)
*
******************************************************************************/
#include "Mc6809.h"

#define USE_RESET_3E

//*********************************************************************************************************************************
// Constructors, destructors, and any sets and/or gets for data.
//*********************************************************************************************************************************


//*****************************************************************************
//	Mc6809()
//*****************************************************************************
//	Initializes emulated CPU, but does not start it. Gets it ready for a cold-
//	Reset. Also sets the memory bus (aka MMU since the MMU handles memory
// mapping)
//*****************************************************************************
Mc6809::Mc6809(MMU* device)
{
	bus = device;
	exec = nullptr;

	clocksUsed = 0;

	// set all registers to a clear state.
	reg_CC = 0x00;			// Condition Code Register

	reg_DP = 0x00;			// Direct Page Registe

	reg_A = 0x00;			// (GP) Accumulator A
	reg_B = 0x00;			// (GP) Accumulator B
	reg_D = 0x0000;			// (GP) Accumulator D

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

	using op = Mc6809;

	OpCode[0] =
	{
#ifdef USE_RESET_3E
		{"NEG"	,&op::NEG_dir  ,6 ,6, 2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COM"	 ,&op::COM_dir	,6 ,6 ,2 }, {"LSR"		,&op::LSR_dir	  ,6 ,6 ,2 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"ROR"	,&op::ROR_dir  ,6 ,6 ,2 }, {"ASR"  ,&op::ASR_dir  ,6 ,6 ,2 }, {"ASL/LSL"  ,&op::ASL_LSL_dir	  ,6 ,6 ,2 }, {"ROL"  ,&op::ROL_dir	 ,6 ,6 ,2 }, {"DEC"	 ,&op::DEC_dir	,6 ,6 ,2 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INC"  ,&op::INC_dir  ,6 ,6 ,2 }, {"TST"  ,&op::TST_dir	 ,6 ,6 ,2 }, {"JMP"	 ,&op::JMP_dir	,3 ,3 ,2 }, {"CLR"	,&op::CLR_dir  ,6 ,6 ,2 },
		{"***"	,nullptr	   ,1 ,1 ,1 }, {"***"  ,nullptr		  ,1 ,1 ,1 }, {"NOP"  ,&op::NOP_inh	 ,2 ,2 ,1 }, {"SYNC" ,&op::SYNC_inh ,4 ,00,1 }, {"???"		,&op::XXX		  ,1 ,1 ,1 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"LBRA" ,&op::LBRA_rel ,5 ,5 ,3 }, {"LBSR" ,&op::LBSR_rel ,9 ,9 ,3 }, {"???"	  ,&op::XXX			  ,1 ,1 ,1 }, {"DAA"  ,&op::DAA_inh	 ,2 ,2 ,1 }, {"ORCC" ,&op::ORCC_imm ,3 ,3 ,2 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"ANDCC",&op::ANDCC_imm,3 ,3 ,2 }, {"SEX"  ,&op::SEX_inh	 ,2 ,2 ,1 }, {"EXG"	 ,&op::EXG_imm	,8 ,8 ,2 }, {"TFR"	,&op::TFR_imm  ,6 ,6 ,2 },
		{"BRA"	,&op::BRA_rel  ,3 ,3 ,2 }, {"BRN"  ,&op::BRN_rel  ,3 ,3 ,2 }, {"BHI"  ,&op::BHI_rel	 ,3 ,3 ,2 }, {"BLS"	 ,&op::BLS_rel	,3 ,3 ,2 }, {"BHS/BCC"	,&op::BHS_BCC_rel ,3 ,3 ,2 }, {"BLO/BCS"  ,&op::BLO_BCS_rel ,3 ,3 ,2 }, {"BNE"	,&op::BNE_rel  ,3 ,3 ,2 }, {"BEQ"  ,&op::BEQ_rel  ,3 ,3 ,2 }, {"BVC"	  ,&op::BVC_rel		  ,3 ,3 ,2 }, {"BVS"  ,&op::BVS_rel	 ,3 ,3 ,2 }, {"BPL"	 ,&op::BPL_rel	,3 ,3 ,2 }, {"BMI"	,&op::BMI_rel  ,3 ,3 ,2 }, {"BGE"  ,&op::BGE_rel  ,3 ,3 ,2 }, {"BLT"  ,&op::BLT_rel	 ,3 ,3 ,2 }, {"BGT"	 ,&op::BGT_rel	,3 ,3 ,2 }, {"BLE"	,&op::BLE_rel  ,3 ,3 ,2 },
		{"LEAX" ,&op::LEAX_idx ,4 ,99,2 }, {"LEAY" ,&op::LEAY_idx ,4 ,99,2 }, {"LEAS" ,&op::LEAS_idx ,4 ,99,2 }, {"LEAU" ,&op::LEAU_idx ,4 ,99,2 }, {"PSHS"		,&op::PSHS_imm	  ,5 ,99,2 }, {"PULS"	  ,&op::PULS_imm	,5 ,99,2 }, {"PSHU" ,&op::PSHU_imm ,5 ,99,2 }, {"PULU" ,&op::PULU_imm ,5 ,99,2 }, {"???"	  ,&op::XXX			  ,1 ,1 ,1 }, {"RTS"  ,&op::RTS_inh	 ,5 ,5 ,1 }, {"ABX"	 ,&op::ABX_inh	,3 ,3 ,1 }, {"RTI"	,&op::RTI_inh  ,6 ,15,1 }, {"CWAI" ,&op::CWAI_inh ,20,00,2 }, {"MUL"  ,&op::MUL_inh	 ,11,11,1 }, {"RESET",&op::RESET_inh,19,19,1 }, {"SWI"	,&op::SWI_inh  ,19,19,1 },
		{"NEGA" ,&op::NEGA_inh ,2 ,2 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COMA" ,&op::COMA_inh ,2 ,2 ,1 }, {"LSRA"		,&op::LSRA_inh	  ,2 ,2 ,1 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"RORA" ,&op::RORA_inh ,2 ,2 ,1 }, {"ASRA" ,&op::ASRA_inh ,2 ,2 ,1 }, {"ASLA/LSLA",&op::ASLA_LSLA_inh ,2 ,2 ,1 }, {"ROLA" ,&op::ROLA_inh ,2 ,2 ,1 }, {"DECA" ,&op::DECA_inh ,2 ,2 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INCA" ,&op::INCA_inh ,2 ,2 ,1 }, {"TSTA" ,&op::TSTA_inh ,2 ,2 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CLRA" ,&op::CLRA_inh ,2 ,2 ,1 },
		{"NEGB" ,&op::NEGB_inh ,2 ,2 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COMB" ,&op::COMB_inh ,2 ,2 ,1 }, {"LSRB"		,&op::LSRB_inh	  ,2 ,2 ,1 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"RORB" ,&op::RORB_inh ,2 ,2 ,1 }, {"ASRB" ,&op::ASRB_inh ,2 ,2 ,1 }, {"ASLB/LSLB",&op::ASLB_LSLB_inh ,2 ,2 ,1 }, {"ROLB" ,&op::ROLB_inh ,2 ,2 ,1 }, {"DECB" ,&op::DECB_inh ,2 ,2 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INCB" ,&op::INCB_inh ,2 ,2 ,1 }, {"TSTB" ,&op::TSTB_inh ,2 ,2 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CLRB" ,&op::CLRB_inh ,2 ,2 ,1 },
		{"NEG"	,&op::NEG_idx  ,6 ,99,2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COM"	 ,&op::COM_idx	,6 ,99,2 }, {"LSR"		,&op::LSR_idx	  ,6 ,99,2 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"ROR"	,&op::ROR_idx  ,6 ,99,2 }, {"ASR"  ,&op::ASR_idx  ,6 ,99,2 }, {"ASL/LSL"  ,&op::ASL_LSL_idx	  ,6 ,99,2 }, {"ROL"  ,&op::ROL_idx	 ,6 ,99,2 }, {"DEC"	 ,&op::DEC_idx	,6 ,99,2 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INC"  ,&op::INC_idx  ,6 ,99,2 }, {"TST"  ,&op::TST_idx	 ,6 ,99,2 }, {"JMP"	 ,&op::JMP_idx	,3 ,99,2 }, {"CLR"	,&op::CLR_idx  ,6 ,99,2 },
		{"NEG"	,&op::NEG_ext  ,7 ,7 ,3 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COM"	 ,&op::COM_ext	,7 ,7 ,3 }, {"LSR"		,&op::LSR_ext	  ,7 ,7 ,3 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"ROR"	,&op::ROR_ext  ,7 ,7 ,3 }, {"ASR"  ,&op::ASR_ext  ,7 ,7 ,3 }, {"ASL/LSL"  ,&op::ASL_LSL_ext	  ,7 ,7 ,3 }, {"ROL"  ,&op::ROL_ext	 ,7 ,7 ,3 }, {"DEC"	 ,&op::DEC_ext	,7 ,7 ,3 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INC"  ,&op::INC_ext  ,7 ,7 ,3 }, {"TST"  ,&op::TST_ext	 ,7 ,7 ,3 }, {"JMP"	 ,&op::JMP_ext	,4 ,4 ,3 }, {"CLR"	,&op::CLR_ext  ,7 ,7 ,3 },
		{"SUBA" ,&op::SUBA_imm ,2 ,2 ,2 }, {"CMPA" ,&op::CMPA_imm ,2 ,2 ,2 }, {"SBCA" ,&op::SBCA_imm ,2 ,2 ,2 }, {"SUBD" ,&op::SUBD_imm ,4 ,4 ,3 }, {"ANDA"		,&op::ANDA_imm	  ,2 ,2 ,2 }, {"BITA"	  ,&op::BITA_imm	,2 ,2 ,2 }, {"LDA"	,&op::LDA_imm  ,2 ,2 ,2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"EORA"	  ,&op::EORA_imm	  ,2 ,2 ,2 }, {"ADCA" ,&op::ADCA_imm ,2 ,2 ,2 }, {"ORA"	 ,&op::ORA_imm	,2 ,2 ,2 }, {"ADDA" ,&op::ADDA_imm ,2 ,2 ,2 }, {"CMPX" ,&op::CMPX_imm ,4 ,2 ,3 }, {"BSR"  ,&op::BSR_rel	 ,7 ,7 ,2 }, {"LDX"	 ,&op::LDX_imm	,3 ,3 ,3 }, {"???"	,&op::XXX	   ,1 ,1 ,1 },
		{"SUBA" ,&op::SUBA_dir ,4 ,4 ,2 }, {"CMPA" ,&op::CMPA_dir ,4 ,4 ,2 }, {"SBCA" ,&op::SBCA_dir ,4 ,4 ,2 }, {"SUBD" ,&op::SUBD_dir ,6 ,6 ,2 }, {"ANDA"		,&op::ANDA_dir	  ,4 ,4 ,2 }, {"BITA"	  ,&op::BITA_dir	,4 ,4 ,2 }, {"LDA"	,&op::LDA_dir  ,4 ,4 ,2 }, {"STA"  ,&op::STA_dir  ,4 ,4 ,2 }, {"EORA"	  ,&op::EORA_dir	  ,4 ,4 ,2 }, {"ADCA" ,&op::ADCA_dir ,4 ,4 ,2 }, {"ORA"	 ,&op::ORA_dir	,4 ,4 ,2 }, {"ADDA" ,&op::ADDA_dir ,4 ,4 ,2 }, {"CMPX" ,&op::CMPX_dir ,6 ,6 ,2 }, {"JSR"  ,&op::JSR_dir	 ,7 ,7 ,2 }, {"LDX"	 ,&op::LDX_dir	,5 ,5 ,2 }, {"STX"	,&op::STX_dir  ,5 ,5 ,2 },
		{"SUBA" ,&op::SUBA_idx ,4 ,99,2 }, {"CMPA" ,&op::CMPA_idx ,4 ,99,2 }, {"SBCA" ,&op::SBCA_idx ,4 ,99,2 }, {"SUBD" ,&op::SUBD_idx ,4 ,99,2 }, {"ANDA"		,&op::ANDA_idx	  ,4 ,99,2 }, {"BITA"	  ,&op::BITA_idx	,4 ,99,2 }, {"LDA"	,&op::LDA_idx  ,4 ,99,2 }, {"STA"  ,&op::STA_idx  ,4 ,99,2 }, {"EORA"	  ,&op::EORA_idx	  ,4 ,99,2 }, {"ADCA" ,&op::ADCA_idx ,4 ,99,2 }, {"ORA"	 ,&op::ORA_idx	,4 ,99,2 }, {"ADDA" ,&op::ADDA_idx ,4 ,99,2 }, {"CMPX" ,&op::CMPX_idx ,6 ,99,2 }, {"JSR"  ,&op::JSR_idx	 ,7 ,99,2 }, {"LDX"	 ,&op::LDX_idx	,5 ,99,2 }, {"STX"	,&op::STX_idx  ,5 ,99,2 },
		{"SUBA" ,&op::SUBA_ext ,5 ,5 ,3 }, {"CMPA" ,&op::CMPA_ext ,5 ,5 ,3 }, {"SBCA" ,&op::SBCA_ext ,5 ,5 ,3 }, {"SUBD" ,&op::SUBD_ext ,7 ,7 ,3 }, {"ANDA"		,&op::ANDA_ext	  ,5 ,5 ,3 }, {"BITA"	  ,&op::BITA_ext	,5 ,5 ,3 }, {"LDA"	,&op::LDA_ext  ,5 ,5 ,3 }, {"STA"  ,&op::STA_ext  ,5 ,5 ,3 }, {"EORA"	  ,&op::EORA_ext	  ,5 ,5 ,3 }, {"ADCA" ,&op::ADCA_ext ,5 ,5 ,3 }, {"ORA"	 ,&op::ORA_ext	,5 ,5 ,3 }, {"ADDA" ,&op::ADDA_ext ,5 ,5 ,3 }, {"CMPX" ,&op::CMPX_ext ,7 ,7 ,3 }, {"JSR"  ,&op::JSR_ext	 ,8 ,8 ,3 }, {"LDX"	 ,&op::LDX_ext	,6 ,6 ,3 }, {"STX"	,&op::STX_ext  ,6 ,6 ,3 },
		{"SUBB" ,&op::SUBB_imm ,2 ,2 ,2 }, {"CMPB" ,&op::CMPB_imm ,2 ,2 ,2 }, {"SBCB" ,&op::SBCB_imm ,2 ,2 ,2 }, {"ADDD" ,&op::ADDD_imm ,4 ,4 ,3 }, {"ANDB"		,&op::ANDB_imm	  ,2 ,2, 2 }, {"BITB"	  ,&op::BITB_imm	,2 ,2 ,2 }, {"LDB"	,&op::LDB_imm  ,2 ,2 ,2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"EORB"	  ,&op::EORB_imm	  ,2 ,2 ,2 }, {"ADCB" ,&op::ADCB_imm ,2 ,2 ,2 }, {"ORB"	 ,&op::ORB_imm	,2 ,2 ,2 }, {"ADDB" ,&op::ADDB_imm ,2 ,2 ,2 }, {"LDD"  ,&op::LDD_imm  ,3 ,3 ,3 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"LDU"	 ,&op::LDU_imm	,3 ,3 ,3 }, {"???"	,&op::XXX	   ,1 ,1 ,1 },
		{"SUBB" ,&op::SUBB_dir ,4 ,4 ,2 }, {"CMPB" ,&op::CMPB_dir ,4 ,4 ,2 }, {"SBCB" ,&op::SBCB_dir ,4 ,4 ,2 }, {"ADDD" ,&op::ADDD_dir ,6 ,6 ,2 }, {"ANDB"		,&op::ANDB_dir	  ,4 ,4 ,2 }, {"BITB"	  ,&op::BITB_dir	,4 ,4 ,2 }, {"LDB"	,&op::LDB_dir  ,4 ,4 ,2 }, {"STB"  ,&op::STB_dir  ,4 ,4 ,2 }, {"EORB"	  ,&op::EORB_dir	  ,4 ,4 ,2 }, {"ADCB" ,&op::ADCB_dir ,4 ,4 ,2 }, {"ORB"	 ,&op::ORB_dir	,4 ,4 ,2 }, {"ADDB" ,&op::ADDB_dir ,4 ,4 ,2 }, {"LDD"  ,&op::LDD_dir  ,5 ,5 ,2 }, {"STD"  ,&op::STD_dir	 ,5 ,5 ,2 }, {"LDU"	 ,&op::LDU_dir	,5 ,5 ,2 }, {"STU"	,&op::STU_dir  ,5 ,5 ,2 },
		{"SUBB" ,&op::SUBB_idx ,4 ,99,2 }, {"CMPB" ,&op::CMPB_idx ,4 ,99,2 }, {"SBCB" ,&op::SBCB_idx ,4 ,99,2 }, {"ADDD" ,&op::ADDD_idx ,6 ,99,2 }, {"ANDB"		,&op::ANDB_idx	  ,4 ,99,2 }, {"BITB"	  ,&op::BITB_idx	,4 ,99,2 }, {"LDB"	,&op::LDB_idx  ,4 ,99,2 }, {"STB"  ,&op::STB_idx  ,4 ,99,2 }, {"EORB"	  ,&op::EORB_idx	  ,4 ,99,2 }, {"ADCB" ,&op::ADCB_idx ,4 ,99,2 }, {"ORB"	 ,&op::ORB_idx	,4 ,99,2 }, {"ADDB" ,&op::ADDB_idx ,4 ,99,2 }, {"LDD"  ,&op::LDD_idx  ,5 ,99,2 }, {"STD"  ,&op::STD_idx	 ,5 ,99,2 }, {"LDU"	 ,&op::LDU_idx	,5 ,99,2 }, {"STU"	,&op::STU_idx  ,5 ,99,2 },
		{"SUBB" ,&op::SUBB_ext ,5 ,5 ,3 }, {"CMPB" ,&op::CMPB_ext ,5 ,5 ,3 }, {"SBCB" ,&op::SBCB_ext ,5 ,5 ,3 }, {"ADDD" ,&op::ADDD_ext ,7 ,7 ,3 }, {"ANDB"		,&op::ANDB_ext	  ,5 ,5 ,3 }, {"BITB"	  ,&op::BITB_ext	,5 ,5 ,3 }, {"LDB"	,&op::LDB_ext  ,5 ,5 ,3 }, {"STB"  ,&op::STB_ext  ,5 ,5 ,3 }, {"EORB"	  ,&op::EORB_ext	  ,5 ,5 ,3 }, {"ADCB" ,&op::ADCB_ext ,5 ,5 ,3 }, {"ORB"	 ,&op::ORB_ext	,5 ,5 ,3 }, {"ADDB" ,&op::ADDB_ext ,5 ,5 ,3 }, {"LDD"  ,&op::LDD_ext  ,6 ,6 ,3 }, {"STD"  ,&op::STD_ext	 ,6 ,6 ,3 }, {"LDU"	 ,&op::LDU_ext	,6 ,6 ,3 }, {"STU"	,&op::STU_ext  ,6 ,6 ,3 }
#else
		{"NEG"	,& op::NEG_dir  ,6 ,6, 2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COM"	 ,&op::COM_dir	,6 ,6 ,2 }, {"LSR"		,&op::LSR_dir	  ,6 ,6 ,2 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"ROR"	,&op::ROR_dir  ,6 ,6 ,2 }, {"ASR"  ,&op::ASR_dir  ,6 ,6 ,2 }, {"ASL/LSL"  ,&op::ASL_LSL_dir	  ,6 ,6 ,2 }, {"ROL"  ,&op::ROL_dir	 ,6 ,6 ,2 }, {"DEC"	 ,&op::DEC_dir	,6 ,6 ,2 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INC"  ,&op::INC_dir  ,6 ,6 ,2 }, {"TST"  ,&op::TST_dir	 ,6 ,6 ,2 }, {"JMP"	 ,&op::JMP_dir	,3 ,3 ,2 }, {"CLR"	,&op::CLR_dir  ,6 ,6 ,2 },
		{"***"	,nullptr	   ,1 ,1 ,1 }, {"***"  ,nullptr		  ,1 ,1 ,1 }, {"NOP"  ,&op::NOP_inh	 ,2 ,2 ,1 }, {"SYNC" ,&op::SYNC_inh ,4 ,00,1 }, {"???"		,&op::XXX		  ,1 ,1 ,1 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"LBRA" ,&op::LBRA_rel ,5 ,5 ,3 }, {"LBSR" ,&op::LBSR_rel ,9 ,9 ,3 }, {"???"	  ,&op::XXX			  ,1 ,1 ,1 }, {"DAA"  ,&op::DAA_inh	 ,2 ,2 ,1 }, {"ORCC" ,&op::ORCC_imm ,3 ,3 ,2 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"ANDCC",&op::ANDCC_imm,3 ,3 ,2 }, {"SEX"  ,&op::SEX_inh	 ,2 ,2 ,1 }, {"EXG"	 ,&op::EXG_imm	,8 ,8 ,2 }, {"TFR"	,&op::TFR_imm  ,6 ,6 ,2 },
		{"BRA"	,&op::BRA_rel  ,3 ,3 ,2 }, {"BRN"  ,&op::BRN_rel  ,3 ,3 ,2 }, {"BHI"  ,&op::BHI_rel	 ,3 ,3 ,2 }, {"BLS"	 ,&op::BLS_rel	,3 ,3 ,2 }, {"BHS/BCC"	,&op::BHS_BCC_rel ,3 ,3 ,2 }, {"BLO/BCS"  ,&op::BLO_BCS_rel ,3 ,3 ,2 }, {"BNE"	,&op::BNE_rel  ,3 ,3 ,2 }, {"BEQ"  ,&op::BEQ_rel  ,3 ,3 ,2 }, {"BVC"	  ,&op::BVC_rel		  ,3 ,3 ,2 }, {"BVS"  ,&op::BVS_rel	 ,3 ,3 ,2 }, {"BPL"	 ,&op::BPL_rel	,3 ,3 ,2 }, {"BMI"	,&op::BMI_rel  ,3 ,3 ,2 }, {"BGE"  ,&op::BGE_rel  ,3 ,3 ,2 }, {"BLT"  ,&op::BLT_rel	 ,3 ,3 ,2 }, {"BGT"	 ,&op::BGT_rel	,3 ,3 ,2 }, {"BLE"	,&op::BLE_rel  ,3 ,3 ,2 },
		{"LEAX" ,&op::LEAX_idx ,4 ,99,2 }, {"LEAY" ,&op::LEAY_idx ,4 ,99,2 }, {"LEAS" ,&op::LEAS_idx ,4 ,99,2 }, {"LEAU" ,&op::LEAU_idx ,4 ,99,2 }, {"PSHS"		,&op::PSHS_imm	  ,5 ,4 ,2 }, {"PULS"	  ,&op::PULS_imm	,5 ,5 ,2 }, {"PSHU" ,&op::PSHU_imm ,5 ,5 ,2 }, {"PULU" ,&op::PULU_imm ,5 ,5 ,2 }, {"???"	  ,&op::XXX			  ,1 ,1 ,1 }, {"RTS"  ,&op::RTS_inh	 ,5 ,5 ,1 }, {"ABX"	 ,&op::ABX_inh	,3 ,3 ,1 }, {"RTI"	,&op::RTI_inh  ,6 ,15,1 }, {"CWAI" ,&op::CWAI_inh ,20,00,2 }, {"MUL"  ,&op::MUL_inh	 ,11,11,1 }, {"RESET",&op::RESET_inh,20,20,1 }, {"SWI"	,&op::SWI_inh  ,19,19,1 },
		{"NEGA" ,&op::NEGA_inh ,2 ,2 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COMA" ,&op::COMA_inh ,2 ,2 ,1 }, {"LSRA"		,&op::LSRA_inh	  ,2 ,2 ,1 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"RORA" ,&op::RORA_inh ,2 ,2 ,1 }, {"ASRA" ,&op::ASRA_inh ,2 ,2 ,1 }, {"ASLA/LSLA",&op::ASLA_LSLA_inh ,2 ,2 ,1 }, {"ROLA" ,&op::ROLA_inh ,2 ,2 ,1 }, {"DECA" ,&op::DECA_inh ,2 ,2 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INCA" ,&op::INCA_inh ,2 ,2 ,1 }, {"TSTA" ,&op::TSTA_inh ,2 ,2 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CLRA" ,&op::CLRA_inh ,2 ,2 ,1 },
		{"NEGB" ,&op::NEGB_inh ,2 ,2 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COMB" ,&op::COMB_inh ,2 ,2 ,1 }, {"LSRB"		,&op::LSRB_inh	  ,2 ,2 ,1 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"RORB" ,&op::RORB_inh ,2 ,2 ,1 }, {"ASRB" ,&op::ASRB_inh ,2 ,2 ,1 }, {"ASLB/LSLB",&op::ASLB_LSLB_inh ,2 ,2 ,1 }, {"ROLB" ,&op::ROLB_inh ,2 ,2 ,1 }, {"DECB" ,&op::DECB_inh ,2 ,2 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INCB" ,&op::INCB_inh ,2 ,2 ,1 }, {"TSTB" ,&op::TSTB_inh ,2 ,2 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CLRB" ,&op::CLRB_inh ,2 ,2 ,1 },
		{"NEG"	,&op::NEG_idx  ,6 ,99,2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COM"	 ,&op::COM_idx	,6 ,99,2 }, {"LSR"		,&op::LSR_idx	  ,6 ,99,2 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"ROR"	,&op::ROR_idx  ,6 ,99,2 }, {"ASR"  ,&op::ASR_idx  ,6 ,99,2 }, {"ASL/LSL"  ,&op::ASL_LSL_idx	  ,6 ,99,2 }, {"ROL"  ,&op::ROL_idx	 ,6 ,99,2 }, {"DEC"	 ,&op::DEC_idx	,6 ,99,2 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INC"  ,&op::INC_idx  ,6 ,99,2 }, {"TST"  ,&op::TST_idx	 ,6 ,99,2 }, {"JMP"	 ,&op::JMP_idx	,3 ,99,2 }, {"CLR"	,&op::CLR_idx  ,6 ,99,2 },
		{"NEG"	,&op::NEG_ext  ,7 ,7 ,3 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"COM"	 ,&op::COM_ext	,7 ,7 ,3 }, {"LSR"		,&op::LSR_ext	  ,7 ,7 ,3 }, {"???"	  ,&op::XXX			,1 ,1 ,1 }, {"ROR"	,&op::ROR_ext  ,7 ,7 ,3 }, {"ASR"  ,&op::ASR_ext  ,7 ,7 ,3 }, {"ASL/LSL"  ,&op::ASL_LSL_ext	  ,7 ,7 ,3 }, {"ROL"  ,&op::ROL_ext	 ,7 ,7 ,3 }, {"DEC"	 ,&op::DEC_ext	,7 ,7 ,3 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"INC"  ,&op::INC_ext  ,7 ,7 ,3 }, {"TST"  ,&op::TST_ext	 ,7 ,7 ,3 }, {"JMP"	 ,&op::JMP_ext	,4 ,4 ,3 }, {"CLR"	,&op::CLR_ext  ,7 ,7 ,3 },
		{"SUBA" ,&op::SUBA_imm ,2 ,2 ,2 }, {"CMPA" ,&op::CMPA_imm ,2 ,2 ,2 }, {"SBCA" ,&op::SBCA_imm ,2 ,2 ,2 }, {"SUBD" ,&op::SUBD_imm ,4 ,4 ,3 }, {"ANDA"		,&op::ANDA_imm	  ,2 ,2 ,2 }, {"BITA"	  ,&op::BITA_imm	,2 ,2 ,2 }, {"LDA"	,&op::LDA_imm  ,2 ,2 ,2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"EORA"	  ,&op::EORA_imm	  ,2 ,2 ,2 }, {"ADCA" ,&op::ADCA_imm ,2 ,2 ,2 }, {"ORA"	 ,&op::ORA_imm	,2 ,2 ,2 }, {"ADDA" ,&op::ADDA_imm ,2 ,2 ,2 }, {"CMPX" ,&op::CMPX_imm ,4 ,2 ,3 }, {"BSR"  ,&op::BSR_rel	 ,7 ,7 ,2 }, {"LDX"	 ,&op::LDX_imm	,3 ,3 ,3 }, {"???"	,&op::XXX	   ,1 ,1 ,1 },
		{"SUBA" ,&op::SUBA_dir ,4 ,4 ,2 }, {"CMPA" ,&op::CMPA_dir ,4 ,4 ,2 }, {"SBCA" ,&op::SBCA_dir ,4 ,4 ,2 }, {"SUBD" ,&op::SUBD_dir ,6 ,6 ,2 }, {"ANDA"		,&op::ANDA_dir	  ,4 ,4 ,2 }, {"BITA"	  ,&op::BITA_dir	,4 ,4 ,2 }, {"LDA"	,&op::LDA_dir  ,4 ,4 ,2 }, {"STA"  ,&op::STA_dir  ,4 ,4 ,2 }, {"EORA"	  ,&op::EORA_dir	  ,4 ,4 ,2 }, {"ADCA" ,&op::ADCA_dir ,4 ,4 ,2 }, {"ORA"	 ,&op::ORA_dir	,4 ,4 ,2 }, {"ADDA" ,&op::ADDA_dir ,4 ,4 ,2 }, {"CMPX" ,&op::CMPX_dir ,6 ,6 ,2 }, {"JSR"  ,&op::JSR_dir	 ,7 ,7 ,2 }, {"LDX"	 ,&op::LDX_dir	,5 ,5 ,2 }, {"STX"	,&op::STX_dir  ,5 ,5 ,2 },
		{"SUBA" ,&op::SUBA_idx ,4 ,99,2 }, {"CMPA" ,&op::CMPA_idx ,4 ,99,2 }, {"SBCA" ,&op::SBCA_idx ,4 ,99,2 }, {"SUBD" ,&op::SUBD_idx ,4 ,99,2 }, {"ANDA"		,&op::ANDA_idx	  ,4 ,99,2 }, {"BITA"	  ,&op::BITA_idx	,4 ,99,2 }, {"LDA"	,&op::LDA_idx  ,4 ,99,2 }, {"STA"  ,&op::STA_idx  ,4 ,99,2 }, {"EORA"	  ,&op::EORA_idx	  ,4 ,99,2 }, {"ADCA" ,&op::ADCA_idx ,4 ,99,2 }, {"ORA"	 ,&op::ORA_idx	,4 ,99,2 }, {"ADDA" ,&op::ADDA_idx ,4 ,99,2 }, {"CMPX" ,&op::CMPX_idx ,6 ,99,2 }, {"JSR"  ,&op::JSR_idx	 ,7 ,99,2 }, {"LDX"	 ,&op::LDX_idx	,5 ,99,2 }, {"STX"	,&op::STX_idx  ,5 ,99,2 },
		{"SUBA" ,&op::SUBA_ext ,5 ,5 ,3 }, {"CMPA" ,&op::CMPA_ext ,5 ,5 ,3 }, {"SBCA" ,&op::SBCA_ext ,5 ,5 ,3 }, {"SUBD" ,&op::SUBD_ext ,7 ,7 ,3 }, {"ANDA"		,&op::ANDA_ext	  ,5 ,5 ,3 }, {"BITA"	  ,&op::BITA_ext	,5 ,5 ,3 }, {"LDA"	,&op::LDA_ext  ,5 ,5 ,3 }, {"STA"  ,&op::STA_ext  ,5 ,5 ,3 }, {"EORA"	  ,&op::EORA_ext	  ,5 ,5 ,3 }, {"ADCA" ,&op::ADCA_ext ,5 ,5 ,3 }, {"ORA"	 ,&op::ORA_ext	,5 ,5 ,3 }, {"ADDA" ,&op::ADDA_ext ,5 ,5 ,3 }, {"CMPX" ,&op::CMPX_ext ,7 ,7 ,3 }, {"JSR"  ,&op::JSR_ext	 ,8 ,8 ,3 }, {"LDX"	 ,&op::LDX_ext	,6 ,6 ,3 }, {"STX"	,&op::STX_ext  ,6 ,6 ,3 },
		{"SUBB" ,&op::SUBB_imm ,2 ,2 ,2 }, {"CMPB" ,&op::CMPB_imm ,2 ,2 ,2 }, {"SBCB" ,&op::SBCB_imm ,2 ,2 ,2 }, {"ADDD" ,&op::ADDD_imm ,4 ,4 ,3 }, {"ANDB"		,&op::ANDB_imm	  ,2 ,2, 2 }, {"BITB"	  ,&op::BITB_imm	,2 ,2 ,2 }, {"LDB"	,&op::LDB_imm  ,2 ,2 ,2 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"EORB"	  ,&op::EORB_imm	  ,2 ,2 ,2 }, {"ADCB" ,&op::ADCB_imm ,2 ,2 ,2 }, {"ORB"	 ,&op::ORB_imm	,2 ,2 ,2 }, {"ADDB" ,&op::ADDB_imm ,2 ,2 ,2 }, {"LDD"  ,&op::LDD_imm  ,3 ,3 ,3 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"LDU"	 ,&op::LDU_imm	,3 ,3 ,3 }, {"???"	,&op::XXX	   ,1 ,1 ,1 },
		{"SUBB" ,&op::SUBB_dir ,4 ,4 ,2 }, {"CMPB" ,&op::CMPB_dir ,4 ,4 ,2 }, {"SBCB" ,&op::SBCB_dir ,4 ,4 ,2 }, {"ADDD" ,&op::ADDD_dir ,6 ,6 ,2 }, {"ANDB"		,&op::ANDB_dir	  ,4 ,4 ,2 }, {"BITB"	  ,&op::BITB_dir	,4 ,4 ,2 }, {"LDB"	,&op::LDB_dir  ,4 ,4 ,2 }, {"STB"  ,&op::STB_dir  ,4 ,4 ,2 }, {"EORB"	  ,&op::EORB_dir	  ,4 ,4 ,2 }, {"ADCB" ,&op::ADCB_dir ,4 ,4 ,2 }, {"ORB"	 ,&op::ORB_dir	,4 ,4 ,2 }, {"ADDB" ,&op::ADDB_dir ,4 ,4 ,2 }, {"LDD"  ,&op::LDD_dir  ,5 ,5 ,2 }, {"STD"  ,&op::STD_dir	 ,5 ,5 ,2 }, {"LDU"	 ,&op::LDU_dir	,5 ,5 ,2 }, {"STU"	,&op::STU_dir  ,5 ,5 ,2 },
		{"SUBB" ,&op::SUBB_idx ,4 ,99,2 }, {"CMPB" ,&op::CMPB_idx ,4 ,99,2 }, {"SBCB" ,&op::SBCB_idx ,4 ,99,2 }, {"ADDD" ,&op::ADDD_idx ,6 ,99,2 }, {"ANDB"		,&op::ANDB_idx	  ,4 ,99,2 }, {"BITB"	  ,&op::BITB_idx	,4 ,99,2 }, {"LDB"	,&op::LDB_idx  ,4 ,99,2 }, {"STB"  ,&op::STB_idx  ,4 ,99,2 }, {"EORB"	  ,&op::EORB_idx	  ,4 ,99,2 }, {"ADCB" ,&op::ADCB_idx ,4 ,99,2 }, {"ORB"	 ,&op::ORB_idx	,4 ,99,2 }, {"ADDB" ,&op::ADDB_idx ,4 ,99,2 }, {"LDD"  ,&op::LDD_idx  ,5 ,99,2 }, {"STD"  ,&op::STD_idx	 ,5 ,99,2 }, {"LDU"	 ,&op::LDU_idx	,5 ,99,2 }, {"STU"	,&op::STU_idx  ,5 ,99,2 },
		{"SUBB" ,&op::SUBB_ext ,5 ,5 ,3 }, {"CMPB" ,&op::CMPB_ext ,5 ,5 ,3 }, {"SBCB" ,&op::SBCB_ext ,5 ,5 ,3 }, {"ADDD" ,&op::ADDD_ext ,7 ,7 ,3 }, {"ANDB"		,&op::ANDB_ext	  ,5 ,5 ,3 }, {"BITB"	  ,&op::BITB_ext	,5 ,5 ,3 }, {"LDB"	,&op::LDB_ext  ,5 ,5 ,3 }, {"STB"  ,&op::STB_ext  ,5 ,5 ,3 }, {"EORB"	  ,&op::EORB_ext	  ,5 ,5 ,3 }, {"ADCB" ,&op::ADCB_ext ,5 ,5 ,3 }, {"ORB"	 ,&op::ORB_ext	,5 ,5 ,3 }, {"ADDB" ,&op::ADDB_ext ,5 ,5 ,3 }, {"LDD"  ,&op::LDD_ext  ,6 ,6 ,3 }, {"STD"  ,&op::STD_ext	 ,6 ,6 ,3 }, {"LDU"	 ,&op::LDU_ext	,6 ,6 ,3 }, {"STU"	,&op::STU_ext  ,6 ,6 ,3 }
#endif
	};
	OpCode[1] =
	{
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"***"	,nullptr   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"LBRN" ,&op::LBRN_rel ,5 ,5 ,4 }, {"LBHI" ,&op::LBHI_rel ,5 ,6 ,4 }, {"LBLS" ,&op::LBLS_rel ,5 ,6 ,4 }, {"LBHS/LBCC",&op::LBHS_LBCC_rel,5 ,6 ,4 }, {"LBCS/LBLO",&op::LBCS_LBLO_rel,5 ,6 ,4 }, {"LBNE" ,&op::LBNE_rel ,5 ,6 ,4 }, {"LBEQ" ,&op::LBEQ_rel ,5 ,6 ,4 }, {"LBVC" ,&op::LBVC_rel ,5 ,6 ,4 }, {"LBVS" ,&op::LBVS_rel ,5 ,6 ,4 }, {"LBPL" ,&op::LBPL_rel ,5 ,6 ,4 }, {"LBMI" ,&op::LBMI_rel ,5 ,6 ,4 }, {"LBGE" ,&op::LBGE_rel ,5 ,6 ,4 }, {"LBLT" ,&op::LBLT_rel ,5 ,6 ,4 }, {"LBGT" ,&op::LBGT_rel ,5 ,6 ,4 }, {"LBLE" ,&op::LBLE_rel ,5 ,6 ,4 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"SWI2" ,&op::SWI2_inh ,20,20,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"CMPD" ,&op::CMPD_imm ,5 ,5 ,4 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CMPY" ,&op::CMPY_imm ,5 ,5 ,4 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDY"  ,&op::LDY_imm	 ,4 ,4 ,4 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"CMPD" ,&op::CMPD_dir ,7 ,7 ,3 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CMPY" ,&op::CMPY_dir ,7 ,7 ,3 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDY"  ,&op::LDY_dir	 ,6 ,6 ,3 }, {"STY"	 ,&op::STY_dir	,6 ,6 ,3 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"CMPD" ,&op::CMPD_idx ,7 ,99,3 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CMPY" ,&op::CMPY_idx ,7 ,99,3 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDY"  ,&op::LDY_idx	 ,6 ,99,3 }, {"STY"	 ,&op::STY_idx	,6 ,99,3 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"CMPD" ,&op::CMPD_ext ,8 ,8 ,4 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"CMPY" ,&op::CMPY_ext ,8 ,8 ,4 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDY"  ,&op::LDY_ext	 ,7 ,7 ,4 }, {"STY"	 ,&op::STY_ext	,7 ,7 ,4 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDS"  ,&op::LDS_imm	 ,4 ,4 ,4 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDS"  ,&op::LDS_dir	 ,6 ,6 ,4 }, {"STS"	 ,&op::STS_dir	,6 ,6, 3 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDS"  ,&op::LDS_idx	 ,6 ,99,3 }, {"STS"	 ,&op::STS_idx	,6 ,99,3 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"		,&op::XXX		   ,1 ,1 ,1 }, {"???"	   ,&op::XXX		  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"???"  ,&op::XXX		 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX	  ,1 ,1 ,1 }, {"LDS"  ,&op::LDS_ext	 ,7 ,7 ,4 }, {"STS"	 ,&op::STS_ext	,7 ,7, 4 },
	};
	OpCode[2] =
	{
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"***"	,nullptr   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"SWI3" ,&op::SWI3_inh ,20,20,2 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"CMPU" ,&op::CMPU_imm ,5 ,5 ,4 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"CMPS" ,&op::CMPS_imm ,5 ,5 ,4 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"CMPU" ,&op::CMPU_dir ,7 ,7 ,3 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"CMPS" ,&op::CMPS_dir ,7 ,7 ,3 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"CMPU" ,&op::CMPU_idx ,7 ,99,3 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"CMPS" ,&op::CMPS_idx ,7 ,99,3 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"CMPU" ,&op::CMPU_ext ,8 ,8 ,4 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"CMPS" ,&op::CMPS_ext ,8 ,8 ,4 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 },
		{"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX	,1 ,1 ,1 }, {"???"	,&op::XXX	   ,1 ,1 ,1 }, {"???"  ,&op::XXX  ,1 ,1 ,1 }, {"???"  ,&op::XXX	 ,1 ,1 ,1 }, {"???"	 ,&op::XXX		,1 ,1 ,1 }
	};
	opCodePage = 0;
}


//*****************************************************************************
//	~Mc6809()
//*****************************************************************************
//	 Cleans up memory, if owned, on exit.
//*****************************************************************************
Mc6809::~Mc6809()
{}


//*****************************************************************************
//	SetMMU()
//*****************************************************************************
//	Sets the memory bus (aka, MMU since the MMU handles memory mapping.)
//*****************************************************************************
void Mc6809::SetMMU(MMU* device)
{
	bus = device;
}


//*********************************************************************************************************************************
// Internal functionality for making this whole thing work
//*********************************************************************************************************************************


//*****************************************************************************
//	Clock()
//*****************************************************************************
//	Triggers the cycle the CPU is carrying out
//*****************************************************************************
void Mc6809::Clock()
{
	if (exec == nullptr)
	{
		if (Halt && (opCodePage == 0))
			exec = &Mc6809::HALT;
		else if (Reset && (opCodePage == 0))
			exec = &Mc6809::RESET;
		else if (Nmi && (opCodePage == 0))
			exec = &Mc6809::NMI;
		else if (Firq && (opCodePage == 0))
			exec = &Mc6809::FIRQ;
		else if (Irq && (opCodePage == 0))
			exec = &Mc6809::IRQ;
		else
			Fetch(reg_PC);
	}
	//						   (obj->*fp)(m, n)
	else if (uint8_t result = (this->*exec)() == 255)
	{
		exec = nullptr;
		clocksUsed = 0;
		opCodePage = 0;
	}
	return;
}


//*****************************************************************************
//	Read()
//*****************************************************************************
//	Reads a byte of data from the address bus.
//*****************************************************************************
uint8_t Mc6809::Read(const uint16_t address, const bool readOnly)
{
	if (address >= 0x0000 && address <= 0xffff)
		return (bus->Read(address, readOnly));
	return(0x00);
}


//*****************************************************************************
//	Write()
//*****************************************************************************
//	Writes a byte of data to the address bus.
//*****************************************************************************
void Mc6809::Write(const uint16_t address, const uint8_t byte)
{
	if (address >= 0x0000 && address <= 0xffff)
		bus->Write(address, byte);
}


//*****************************************************************************
//	Fetch()
//*****************************************************************************
//	Retrieve opcodes, and determine addressing and instruction to execute
//*****************************************************************************
uint8_t Mc6809::Fetch(const uint16_t address)
{
	static uint8_t opcode;
	opcode = Read(reg_PC);

	if (opcode == 0x10)
	{
		opCodePage = 1;
		clocksUsed = 1;
	}
	else if (opcode == 0x11)
	{
		opCodePage = 2;
		clocksUsed = 1;
	}
	else
	{
		exec = OpCode[opCodePage][opcode].opcode;
		++clocksUsed;
	}

	// process opcode and set it to execute it.
	return(clocksUsed);
}


//*****************************************************************************
//	AdjustCC_H()
//*****************************************************************************
//	Set or Clear Half-Carry Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_H(uint8_t data)
{
	reg_CC = ((data & 0x0010) == 0x0010) ? (reg_CC | CC::H) : (reg_CC & ~CC::H);
}


//*****************************************************************************
//	AdjustCC_N()
//*****************************************************************************
//	Set or Clear Negative Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_N(uint8_t data)
{
	reg_CC = ((data & 0x80) == 0x80) ? (reg_CC | CC::N) : (reg_CC | ~CC::N);
}


//*****************************************************************************
//	AdjustCC_Z()
//*****************************************************************************
//	Set or Clear Zero Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_Z(uint8_t data)
{
	reg_CC = (data == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
}


//*****************************************************************************
//	AdjustCC_V()
//*****************************************************************************
//	Set or Clear Overflow Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_V(uint8_t reg)
{
	reg_CC = (((reg >> 6) & 0x01) != ((reg >> 7) & 0x01)) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
}


//*****************************************************************************
//	AdjustCC_V()
//*****************************************************************************
//	Set or Clear Overflow Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_V(uint8_t dataA, uint8_t dataB, uint8_t result)
{
	reg_CC = (((dataA & 0x80) == 0) && ((dataB & 0x80) == 0) && ((result & 0x80) == 0x80)) ? (reg_CC | CC::V) : (reg_CC | ~CC::V);
}


//*****************************************************************************
//	AdjustCC_C()
//*****************************************************************************
//	Set or Clear Carry Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_C(uint16_t data)
{
	reg_CC = ((data & 0x0100) == 0x0100) ? (reg_CC | CC::C) : (reg_CC | ~CC::C);
}


//*****************************************************************************
//	AdjustCC_N()
//*****************************************************************************
//	Set or Clear Negative Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_N(uint16_t data)
{
	reg_CC = ((data & 0x8000) == 0x8000) ? (reg_CC | CC::N) : (reg_CC | ~CC::N);
}


//*****************************************************************************
//	AdjustCC_Z()
//*****************************************************************************
//	Set or Clear Zero Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_Z(uint16_t data)
{
	reg_CC = (data == 0) ? (reg_CC | CC::Z) : (reg_CC & ~CC::Z);
}


//*****************************************************************************
//	AdjustCC_V()
//*****************************************************************************
//	Set or Clear Overflow Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_V(uint16_t dataA, uint16_t dataB, uint16_t result)
{
	reg_CC = (((dataA & 0x8000) == 0) && ((dataB & 0x8000) == 0) && ((result & 0x8000) == 0x8000)) ? (reg_CC | CC::V) : (reg_CC | ~CC::V);
}


//*****************************************************************************
//	AdjustCC_C()
//*****************************************************************************
//	Set or Clear Carry Condition Code
//*****************************************************************************
void Mc6809::AdjustCC_C(uint32_t data)
{
	reg_CC = ((data & 0x00010000) == 0x00010000) ? (reg_CC | CC::C) : (reg_CC | ~CC::C);
}


//*********************************************************************************************************************************
// All externally triggered interrupts, halts, and resets
//*********************************************************************************************************************************


//*****************************************************************************
//	HALT\				Hardware
//*****************************************************************************
uint8_t Mc6809::HALT()
{
	clocksUsed = Halt ? 1 : 255;
	return(clocksUsed);
}


//*****************************************************************************
//	RESET\				Hardware
//*****************************************************************************
uint8_t Mc6809::RESET()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Don't care			$fffe
		if (Reset)
			--clocksUsed;
		break;
	case 2:		//	R	Don't care			$fffe
		reg_CC = (CC::I | CC::F);
		break;
	case 3:		//	R	Don't care			$fffe
		break;
	case 4:		//	R	Don't care			$fffe
		break;
	case 5:		//	R	Int Vector High		$fffe
		PC_hi = Read(0xfffe);
		break;
	case 6:		//	R	Int Vector Low		$ffff
		PC_lo = Read(0xffff);
		break;
	case 7:		//	R	Don't care			$ffff
		reg_CC = 0x00;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NMI\				Hardware
//*****************************************************************************
uint8_t Mc6809::NMI()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	?					PC
		break;
	case 2:		//	R	?					PC
		break;
	case 3:		//	R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(reg_S--, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(reg_S--, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(reg_S--, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(reg_S--, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(reg_S--, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(reg_S--, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(reg_S--, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(reg_S--, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(reg_S--, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		break;
	case 16:	//	R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	//	R	Int Vector High		$fffc
		PC_hi = Read(0xfffc);
		break;
	case 18:	//	R	Int Vector Low		$fffd
		PC_lo = Read(0xfffd);
		break;
	case 19:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	FIRQ\				Hardware
//*****************************************************************************
uint8_t Mc6809::FIRQ()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	?					PC
		break;
	case 2:		//	R	?					PC
		break;
	case 3:		//	R	Don't care			$ffff
		reg_CC &= ~CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 6:		// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		break;
	case 7:		//	R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 8:		//	R	Int Vector High		$fff6
		PC_hi = Read(0xfff6);
		break;
	case 9:		//	R	Int Vector Low		$fff7
		PC_lo = Read(0xfff7);
		break;
	case 10:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	IRQ\				Hardware
//*****************************************************************************
uint8_t Mc6809::IRQ()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	?					PC
		break;
	case 2:		//	R	?					PC
		break;
	case 3:		//	R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(reg_S--, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(reg_S--, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(reg_S--, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(reg_S--, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(reg_S--, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(reg_S--, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(reg_S--, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(reg_S--, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(reg_S--, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		break;
	case 16:	//	R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	//	R	Int Vector High		$fff8
		PC_hi = Read(0xfff8);
		break;
	case 18:	//	R	Int Vector Low		$fff9
		PC_lo = Read(0xfff9);
		break;
	case 19:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// All valid Opcodes, in mnemonic alphabetical order. (Mnemonic first, Address mode second)
//*********************************************************************************************************************************


//*****************************************************************************
//	ABX					inherent
//*****************************************************************************
uint8_t Mc6809::ABX_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	don't care			PC+1
		//reg_PC++;
		break;
	case 3:		//	R	don't care			$ffff
		reg_X += reg_B;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADCA				direct
//*****************************************************************************
uint8_t Mc6809::ADCA_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_A + scratch_lo + ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_A);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADCA				extended
//*****************************************************************************
uint8_t Mc6809::ADCA_ext()	// H N Z V C all modified. reg_A modified
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_A + scratch_lo + ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_A);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADCA				immediate
//*****************************************************************************
uint8_t Mc6809::ADCA_imm()	// H N Z V C all modified. reg_A modified
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_scratch);
		data = reg_A + scratch_lo + ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_A);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADCB				direct
//*****************************************************************************
uint8_t Mc6809::ADCB_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B + scratch_lo + ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_B = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADCB				extended
//*****************************************************************************
uint8_t Mc6809::ADCB_ext()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B + scratch_lo + ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_B = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADCB				immediate
//*****************************************************************************
uint8_t Mc6809::ADCB_imm()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		data = reg_B + scratch_lo + ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_B = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDA				direct
//*****************************************************************************
uint8_t Mc6809::ADDA_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_A + scratch_lo;
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_A);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDA				extended
//*****************************************************************************
uint8_t Mc6809::ADDA_ext()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_A + scratch_lo;
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_A);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDA				immediate
//*****************************************************************************
uint8_t Mc6809::ADDA_imm()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		data = reg_A + scratch_lo;
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_A);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDB				direct
//*****************************************************************************
uint8_t Mc6809::ADDB_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B + scratch_lo;
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_B = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDB				extended
//*****************************************************************************
uint8_t Mc6809::ADDB_ext()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B + scratch_lo;
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_B = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDB				immediate
//*****************************************************************************
uint8_t Mc6809::ADDB_imm()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		data = reg_B + scratch_lo;
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_B = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDD				direct
//*****************************************************************************
uint8_t Mc6809::ADDD_dir()
{
	static uint32_t data;
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data High			$ffff
		scratch_hi = Read(reg_scratch);
		break;
	case 5:		//	R	Data Low			EA+1
		scratch_lo = Read(++reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		data = reg_D + reg_scratch;
		AdjustCC_V(reg_scratch, reg_D, (data & 0xffff));
		reg_D = data & 0xffff;
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDD				extended
//*****************************************************************************
uint8_t Mc6809::ADDD_ext()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data High			$ffff
		data = Read(reg_scratch) << 8;
		break;
	case 6:		//	R	Data Low			EA+1
		data |= Read(++reg_scratch);
		break;
	case 7:		//	R	Don't Care			$ffff
		tempRegValue = reg_D + data;
		AdjustCC_V(data, reg_D, tempRegValue);
		reg_D = tempRegValue & 0xffff;
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ADDD				immediate
//*****************************************************************************
uint8_t Mc6809::ADDD_imm()
{
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data High			PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Data Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		tempRegValue = reg_D + reg_scratch;
		AdjustCC_V(reg_scratch, reg_D, tempRegValue);
		reg_D = tempRegValue & 0xffff;
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ANDA				direct
//*****************************************************************************
uint8_t Mc6809::ANDA_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_A &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_Z(reg_A);
		AdjustCC_N(reg_A);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ANDA				extended
//*****************************************************************************
uint8_t Mc6809::ANDA_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_A &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_Z(reg_A);
		AdjustCC_N(reg_A);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ANDA				immediate
//*****************************************************************************
uint8_t Mc6809::ANDA_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//R		Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_scratch);
		reg_A &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_Z(reg_A);
		AdjustCC_N(reg_A);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ANDB				direct
//*****************************************************************************
uint8_t Mc6809::ANDB_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//R		Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_B &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_Z(reg_B);
		AdjustCC_N(reg_B);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ANDB				extended
//*****************************************************************************
uint8_t Mc6809::ANDB_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_B &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_Z(reg_B);
		AdjustCC_N(reg_B);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ANDB				immediate
//*****************************************************************************
uint8_t Mc6809::ANDB_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_scratch);
		reg_B &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_Z(reg_B);
		AdjustCC_N(reg_B);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ANDCC				immediate
//*****************************************************************************
uint8_t Mc6809::ANDCC_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		reg_CC &= scratch_lo;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASLA				inherent
//	LSLA				inherent
//*****************************************************************************
uint8_t Mc6809::ASLA_LSLA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	don't care			PC+1
		AdjustCC_C((uint16_t)(reg_A << 1));
		AdjustCC_V(reg_A);
		reg_A = (reg_A << 1) & 0xfe;
		AdjustCC_Z(reg_A);
		AdjustCC_N(reg_A);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASLB				inherent
//	LSLB				inherent
//*****************************************************************************
uint8_t Mc6809::ASLB_LSLB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	don't care			PC+1
		AdjustCC_C((uint16_t)(reg_B << 1));
		AdjustCC_V(reg_B);
		reg_B = (reg_B << 1) & 0xfe;
		AdjustCC_Z(reg_B);
		AdjustCC_N(reg_B);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASL				direct
//	LSL				direct
//*****************************************************************************
uint8_t Mc6809::ASL_LSL_dir()
{
	static uint8_t data_lo;
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't care			$ffff
		AdjustCC_C((uint16_t)(data_lo << 1));
		AdjustCC_V(data_lo);
		data_lo = (data_lo << 1) & 0xfe;
		AdjustCC_Z(data_lo);
		AdjustCC_N(data_lo);
		break;
	case 6:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASL					extended
//	LSL					extended
//*****************************************************************************
uint8_t Mc6809::ASL_LSL_ext()
{
	static uint8_t data_lo;
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't care			$ffff
		break;
	case 5:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't care			$ffff
		AdjustCC_C((uint16_t)(data_lo << 1));
		AdjustCC_V(data_lo);
		data_lo = (data_lo << 1) & 0xfe;
		AdjustCC_Z(data_lo);
		AdjustCC_N(data_lo);
		break;
	case 7:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASRA				inherent
//*****************************************************************************
uint8_t Mc6809::ASRA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		scratch_lo = reg_A & 0x80;
		reg_CC = ((reg_A & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((reg_A >> 1) & 0x7f) | scratch_lo;
		AdjustCC_Z(reg_A);
		AdjustCC_N(reg_A);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASRB				inherent
//*****************************************************************************
uint8_t Mc6809::ASRB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		scratch_lo = reg_B & 0x80;
		reg_CC = ((reg_B & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((reg_B >> 1) & 0x7f) | scratch_lo;
		AdjustCC_Z(reg_B);
		AdjustCC_N(reg_B);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASR					direct
//*****************************************************************************
uint8_t Mc6809::ASR_dir()
{
	static uint8_t data_lo;
	uint8_t data_ghst;
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't care			$ffff
		data_ghst = data_lo & 0x80;
		reg_CC = ((data_lo & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((data_lo >> 1) & 0x7f) | scratch_lo;
		AdjustCC_Z(data_lo);
		AdjustCC_N(data_lo);
		break;
	case 6:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ASR					extended
//*****************************************************************************
uint8_t Mc6809::ASR_ext()
{
	static uint8_t data_lo;
	uint8_t data_ghst;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't care			$ffff
		break;
	case 5:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't care			$ffff
		data_ghst = data_lo & 0x80;
		reg_CC = ((data_lo & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((data_lo >> 1) & 0x7f) | scratch_lo;
		AdjustCC_Z(data_lo);
		AdjustCC_N(data_lo);
		break;
	case 7:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BEQ					relative
//*****************************************************************************
uint8_t Mc6809::BEQ_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::Z) == CC::Z)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BGE					relative
//*****************************************************************************
uint8_t Mc6809::BGE_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if (((reg_CC & CC::C) == CC::C) == ((reg_CC & CC::V) == CC::V))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BGT					relative
//*****************************************************************************
uint8_t Mc6809::BGT_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((((reg_CC & CC::N) == CC::N) && ((reg_CC & CC::V) == CC::V)) && ((reg_CC & CC::Z) == 0))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BHI					relative
//*****************************************************************************
uint8_t Mc6809::BHI_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if (((reg_CC & CC::C) != CC::C) && ((reg_CC & CC::Z) != CC::Z))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BCC					relative
//	BHS					relative
//*****************************************************************************
uint8_t Mc6809::BHS_BCC_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::C) != CC::C)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BITA				direct
//*****************************************************************************
uint8_t Mc6809::BITA_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_A &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	BITA				extended
//*****************************************************************************
uint8_t Mc6809::BITA_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_A &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	BITA				immediate
//*****************************************************************************
uint8_t Mc6809::BITA_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		reg_A &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	BITB				direct
//*****************************************************************************
uint8_t Mc6809::BITB_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_B &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	BITB				extended
//*****************************************************************************
uint8_t Mc6809::BITB_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
		scratch_hi = Read(reg_PC++);
	case 2:		//	R	Address High		PC+1
		break;
		scratch_lo = Read(reg_PC++);
	case 3:		//	R	Address Low			PC+2
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		reg_B &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	BITB				immediate
//*****************************************************************************
uint8_t Mc6809::BITB_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		reg_B &= scratch_lo;
		reg_CC &= ~CC::V;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	BLE					relative
//*****************************************************************************
uint8_t Mc6809::BLE_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
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
//	BCS					relative
//	BLO					relative
//*****************************************************************************
uint8_t Mc6809::BLO_BCS_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::C) == CC::C)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BLS					relative
//*****************************************************************************
uint8_t Mc6809::BLS_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if (((reg_CC & CC::C) == CC::C) || ((reg_CC & CC::Z) == CC::Z))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BLT					relative
//*****************************************************************************
uint8_t Mc6809::BLT_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if (((reg_CC & CC::C) == CC::C) != ((reg_CC & CC::N) == CC::N))
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BMI					relative
//*****************************************************************************
uint8_t Mc6809::BMI_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::N) == CC::N)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BNE					relative
//*****************************************************************************
uint8_t Mc6809::BNE_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::Z) != CC::Z)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BPL					relative
//*****************************************************************************
uint8_t Mc6809::BPL_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::N) != CC::N)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BRA					relative
//*****************************************************************************
uint8_t Mc6809::BRA_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BRN					relative
//*****************************************************************************
uint8_t Mc6809::BRN_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BSR					relative
//*****************************************************************************
uint8_t Mc6809::BSR_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		break;
	case 4:		//	R	Don't Care			Effective Address

		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		// W	Retern Address Low	SP-1
		Write(reg_S--, PC_lo);
		break;
	case 7:		// W	Return Address High SP-2
		Write(reg_S--, PC_hi);
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BVC					relative
//*****************************************************************************
uint8_t Mc6809::BVC_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::V) != CC::V)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	BVS					relative
//*****************************************************************************
uint8_t Mc6809::BVS_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset				PC+1
		scratch_lo = Read(reg_PC++);
		scratch_hi = (((scratch_lo & 0x80) == 0x80) ? 0xff : 0x00);
		break;
	case 3:		//	R	Don't Care			$ffff
		if ((reg_CC & CC::V) == CC::V)
			reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CLRA				inherent
//*****************************************************************************
uint8_t Mc6809::CLRA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_A = 0;
		reg_CC = (reg_CC & CC::H) | CC::Z;
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CLRB				inherent
//*****************************************************************************
uint8_t Mc6809::CLRB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_B = 0;
		reg_CC = (reg_CC & CC::H) | CC::Z;
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CLR					direct
//*****************************************************************************
uint8_t Mc6809::CLR_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care			$ffff
		reg_CC = (reg_CC & CC::H) | CC::Z;
		break;
	case 6:		// W	Data				EA
		Write(reg_scratch, 0);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CLR					extended
//*****************************************************************************
uint8_t Mc6809::CLR_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		reg_CC = (reg_CC & CC::H) | CC::Z;
		break;
	case 7:		// W	Data				EA
		Write(reg_scratch, 0);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPA				direct
//*****************************************************************************
uint8_t Mc6809::CMPA_dir()
{
	uint8_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data = Read(reg_scratch);
		reg_scratch = reg_A - data;
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		AdjustCC_V(reg_A, data, scratch_lo);
		AdjustCC_C(reg_scratch);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPA				extended
//*****************************************************************************
uint8_t Mc6809::CMPA_ext()
{
	uint8_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		data = Read(reg_scratch);
		reg_scratch = reg_A - data;
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		AdjustCC_V(reg_A, data, scratch_lo);
		AdjustCC_C(reg_scratch);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPA				immediate
//*****************************************************************************
uint8_t Mc6809::CMPA_imm()
{
	uint8_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		data = Read(reg_PC++);
		reg_scratch = reg_A - data;
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		AdjustCC_V(reg_A, data, scratch_lo);
		AdjustCC_C(reg_scratch);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPB				direct
//*****************************************************************************
uint8_t Mc6809::CMPB_dir()
{
	uint8_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data = Read(reg_scratch);
		reg_scratch = reg_B - data;
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		AdjustCC_V(reg_B, data, scratch_lo);
		AdjustCC_C(reg_scratch);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPB				extended
//*****************************************************************************
uint8_t Mc6809::CMPB_ext()
{
	uint8_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		data = Read(reg_scratch);
		reg_scratch = reg_B - data;
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		AdjustCC_V(reg_B, data, scratch_lo);
		AdjustCC_C(reg_scratch);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPB				immediate
//*****************************************************************************
uint8_t Mc6809::CMPB_imm()
{
	uint8_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		data = Read(reg_PC++);
		reg_scratch = reg_B - data;
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		AdjustCC_V(reg_B, data, scratch_lo);
		AdjustCC_C(reg_scratch);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPD				direct
//*****************************************************************************
uint8_t Mc6809::CMPD_dir()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Data High			EA
		data = Read(reg_scratch);
		break;
	case 6:		//	R	Data Low			EA+1
		data = (data << 8) | Read(++reg_scratch);
		break;
	case 7:		//	R	Don't Care			$ffff
		tempRegValue = reg_D - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_D, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPD				extended
//*****************************************************************************
uint8_t Mc6809::CMPD_ext()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Data High			EA
		data = Read(reg_scratch) << 8;
		break;
	case 7:		//	R	Data Low			EA+1
		data |= Read(++reg_scratch);
		break;
	case 8:		//	R	Don't Care			$ffff
		tempRegValue = reg_D - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_D, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPD				immediate
//*****************************************************************************
uint8_t Mc6809::CMPD_imm()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Data High			PC+2
		data = Read(reg_PC++) << 8;
		break;
	case 4:		//	R	Data Low			PC+3
		data |= Read((reg_PC++));
		break;
	case 5:		//	R	Don't Care			$ffff
		tempRegValue = reg_D - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_D, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPS				direct
//*****************************************************************************
uint8_t Mc6809::CMPS_dir()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Data High			EA
		data = Read(reg_scratch);
		break;
	case 6:		//	R	Data Low			EA+1
		data = (data << 8) | Read(++reg_scratch);
		break;
	case 7:		//	R	Don't Care			$ffff
		tempRegValue = reg_S - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_S, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPS				extended
//*****************************************************************************
uint8_t Mc6809::CMPS_ext()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Data High			EA
		data = Read(reg_scratch) << 8;
		break;
	case 7:		//	R	Data Low			EA+1
		data |= Read(++reg_scratch);
		break;
	case 8:		//	R	Don't Care			$ffff
		tempRegValue = reg_S - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_S, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPS				immediate
//*****************************************************************************
uint8_t Mc6809::CMPS_imm()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Data High			PC+2
		data = Read(reg_PC++) << 8;
		break;
	case 4:		//	R	Data Low			PC+3
		data |= Read((reg_PC++));
		break;
	case 5:		//	R	Don't Care			$ffff
		tempRegValue = reg_S - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_S, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPU				direct
//*****************************************************************************
uint8_t Mc6809::CMPU_dir()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Data High			EA
		data = Read(reg_scratch);
		break;
	case 6:		//	R	Data Low			EA+1
		data = (data << 8) | Read(++reg_scratch);
		break;
	case 7:		//	R	Don't Care			$ffff
		tempRegValue = reg_U - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_U, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPU				extended
//*****************************************************************************
uint8_t Mc6809::CMPU_ext()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+2
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Data High			EA
		data = Read(reg_scratch) << 8;
		break;
	case 7:		//	R	Data Low			EA+1
		data |= Read(++reg_scratch);
		break;
	case 8:		//	R	Don't Care			$ffff
		tempRegValue = reg_U - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_U, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPU				immediate
//*****************************************************************************
uint8_t Mc6809::CMPU_imm()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Data High			PC+2
		data = Read(reg_PC++) << 8;
		break;
	case 4:		//	R	Data Low			PC+3
		data |= Read((reg_PC++));
		break;
	case 5:		//	R	Don't Care			$ffff
		tempRegValue = reg_U - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_U, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPX				direct
//*****************************************************************************
uint8_t Mc6809::CMPX_dir()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Data High			EA
		data = Read(reg_scratch);
		break;
	case 6:		//	R	Data Low			EA+1
		data = (data << 8) | Read(++reg_scratch);
		break;
	case 7:		//	R	Don't Care			$ffff
		tempRegValue = reg_X - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_X, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPX				extended
//*****************************************************************************
uint8_t Mc6809::CMPX_ext()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data High			EA
		data = Read(reg_scratch) << 8;
		break;
	case 6:		//	R	Data Low			EA+1
		data |= Read(++reg_scratch);
		break;
	case 7:		//	R	Don't Care			$ffff
		tempRegValue = reg_X - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_X, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPX				immediate
//*****************************************************************************
uint8_t Mc6809::CMPX_imm()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data High			PC+1
		data = Read(reg_PC++) << 8;
		break;
	case 3:		//	R	Data Low			PC+2
		data |= Read((reg_PC++));
		break;
	case 4:		//	R	Don't Care			$ffff
		tempRegValue = reg_X - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_X, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPY				direct
//*****************************************************************************
uint8_t Mc6809::CMPY_dir()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Data High			EA
		data = Read(reg_scratch);
		break;
	case 6:		//	R	Data Low			EA+1
		data = (data << 8) | Read(++reg_scratch);
		break;
	case 7:		//	R	Don't Care			$ffff
		tempRegValue = reg_Y - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_Y, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPY				extended
//*****************************************************************************
uint8_t Mc6809::CMPY_ext()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+2
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Data High			EA
		data = Read(reg_scratch) << 8;
		break;
	case 7:		//	R	Data Low			EA+1
		data |= Read(++reg_scratch);
		break;
	case 8:		//	R	Don't Care			$ffff
		tempRegValue = reg_Y - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_Y, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CMPY				immediate
//*****************************************************************************
uint8_t Mc6809::CMPY_imm()
{
	static uint16_t data;
	uint32_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Data High			PC+2
		data = Read(reg_PC++) << 8;
		break;
	case 4:		//	R	Data Low			PC+3
		data |= Read((reg_PC++));
		break;
	case 5:		//	R	Don't Care			$ffff
		tempRegValue = reg_Y - data;
		AdjustCC_N((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_Z((uint16_t)(tempRegValue & 0xffff));
		AdjustCC_V(reg_Y, data, (uint16_t)(tempRegValue & 0xffff));
		AdjustCC_C(tempRegValue);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	COMA				inherent
//*****************************************************************************
uint8_t Mc6809::COMA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_A ^= 255;
		reg_CC |= (CC::C);
		reg_CC &= ~(CC::V | CC::N | CC::Z);
		reg_CC |= (reg_A == 0) ? CC::Z : 0x00;
		reg_CC |= ((reg_A & 0x80) == 0x80) ? CC::N : 0x00;
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	COMB				inherent
//*****************************************************************************
uint8_t Mc6809::COMB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_B ^= 255;
		//reg_PC++;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~(CC::V);
		reg_CC |= (CC::C);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	COM					direct
//*****************************************************************************
uint8_t Mc6809::COM_dir()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care			$ffff
		data_lo ^= 0xff;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		reg_CC &= ~(CC::V);
		reg_CC |= (CC::C);
		break;
	case 6:		// W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	COM					extended
//*****************************************************************************
uint8_t Mc6809::COM_ext()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		data_lo ^= 0xff;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		reg_CC &= ~(CC::V);
		reg_CC |= (CC::C);
		break;
	case 7:		// W  Data			EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	CWAI				inherent
//*****************************************************************************
uint8_t Mc6809::CWAI_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	CC Mask				PC+1
		scratch_lo = Read(reg_PC);
		reg_PC++;
		break;
	case 3:		//	R	Don't Care			PC+2
		reg_PC++;
		break;
	case 4:		//	R	Don't care			$ffff
		break;
	case 5:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 6:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 7:		// W	User Stack Low		SP-3	--SP
		Write(reg_S--, U_lo);
		break;
	case 8:		// W	User Stack High		SP-4	--SP
		Write(reg_S--, U_hi);
		break;
	case 9:		// W	Y  Register Low		SP-5	--SP
		Write(reg_S--, Y_lo);
		break;
	case 10:	// W	Y  Register High	SP-6	--SP
		Write(reg_S--, Y_hi);
		break;
	case 11:	// W	X  Register Low		SP-7	--SP
		Write(reg_S--, X_lo);
		break;
	case 12:	// W	X  Register High	SP-8	--SP
		Write(reg_S--, X_hi);
		break;
	case 13:	// W	DP Register			SP-9	--SP
		Write(reg_S--, reg_DP);
		break;
	case 14:	// W	B  Register			SP-10	--SP
		Write(reg_S--, reg_B);
		break;
	case 15:	// W	A  Register			SP-11	--SP
		Write(reg_S--, reg_A);
		break;
	case 16:	// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		reg_CC &= scratch_lo;
		reg_CC |= CC::E;
		break;

	case 17:	//	R	Don't Care			$ffff
		// wait for interrupt signal
		if (!Nmi && !Firq && !Irq)
			--clocksUsed;
		break;

	case 18:	//	R	Int Vector High		$fffx
		if (Nmi)
			PC_hi = Read(0xfffc);
		else if (Firq && (reg_CC & CC::F) != CC::F)
			PC_hi = Read(0xfff6);
		else if (Irq && (reg_CC & CC::I) != CC::I)
			PC_hi = Read(0xfff8);
		break;
	case 19:	//	R	Int Vector Low		$fffx
		if (Nmi)
			PC_lo = Read(0xfffd);
		else if (Firq && (reg_CC & CC::F) != CC::F)
			PC_lo = Read(0xfff7);
		else if (Irq && (reg_CC & CC::I) != CC::I)
			PC_lo = Read(0xfff9);
		break;
	case 20:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	DAA					inherent
//*****************************************************************************
uint8_t Mc6809::DAA_inh()
{
	static uint8_t carry = 0;
	static uint8_t cfLsn = 0;
	static uint8_t cfMsn = 0;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		scratch_lo = reg_A & 0x0f;
		scratch_hi = (reg_A & 0xf0) >> 4;
		carry = reg_CC & CC::C;
		if (((reg_CC & CC::H) == CC::H) || (scratch_lo > 9))
			cfLsn = 6;
		if ((carry == CC::C) || (scratch_hi > 9) || ((scratch_hi > 8) && (scratch_lo > 9)))
			cfMsn = 6;

		reg_A += cfLsn;			// fixes lsn
		reg_A += (cfMsn << 4);	// fixes msn
		reg_CC = (cfMsn == 6 || carry) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);

		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	DECA				inherent
//*****************************************************************************
uint8_t Mc6809::DECA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = ((reg_A & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		--reg_A;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	DECB				inherent
//*****************************************************************************
uint8_t Mc6809::DECB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = ((reg_B & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		--reg_B;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	DEC					direct
//*****************************************************************************
uint8_t Mc6809::DEC_dir()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care			$ffff
		reg_CC = ((data_lo & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		--data_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 6:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	DEC					extended
//*****************************************************************************
uint8_t Mc6809::DEC_ext()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		reg_CC = ((data_lo & 0x80) == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		--data_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 7:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	EORA				direct
//*****************************************************************************
uint8_t Mc6809::EORA_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		reg_A ^= Read(reg_scratch);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	EORA				extended
//*****************************************************************************
uint8_t Mc6809::EORA_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		reg_A ^= Read(reg_scratch);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	EORA				immediate
//*****************************************************************************
uint8_t Mc6809::EORA_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				EA
		reg_A ^= Read(reg_PC++);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	EORB				direct
//*****************************************************************************
uint8_t Mc6809::EORB_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		reg_B ^= Read(reg_scratch);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	EORB				extended
//*****************************************************************************
uint8_t Mc6809::EORB_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		reg_B ^= Read(reg_scratch);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	EORB				immediate
//*****************************************************************************
uint8_t Mc6809::EORB_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				EA
		reg_B ^= Read(reg_PC++);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	EXG					immediate
//*****************************************************************************
uint8_t Mc6809::EXG_imm()
{
	static uint8_t data_hi = 0;
	static uint8_t data_lo = 0;
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Post scratch_lo		PC+1
		scratch_lo = Read(reg_PC);
		reg_PC++;
		break;
	case 3:		//	R	Don't Care			$ffff
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
//	INCA				inherent
//*****************************************************************************
uint8_t Mc6809::INCA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = ((reg_A & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_A;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	INCB				inherent
//*****************************************************************************
uint8_t Mc6809::INCB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = ((reg_B & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++reg_B;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	INC					direct
//*****************************************************************************
uint8_t Mc6809::INC_dir()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care			$ffff
		reg_CC = ((data_lo & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++data_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 6:		// W  Data			EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	INC					extended
//*****************************************************************************
uint8_t Mc6809::INC_ext()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		reg_CC = ((data_lo & 0x7f) == 0x7f) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		++data_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 7:		// W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	JMP					direct
//*****************************************************************************
uint8_t Mc6809::JMP_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		PC_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		PC_hi = reg_DP;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	JMP					extended
//*****************************************************************************
uint8_t Mc6809::JMP_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		reg_PC = reg_scratch;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	JSR					directg
//*****************************************************************************
uint8_t Mc6809::JSR_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Don't Care			EA
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	W	PC Low				SP-1
		Write(reg_S--, PC_lo);
		break;
	case 7:		//	W	PC High				SP-2
		Write(reg_S--, PC_hi);
		reg_PC = reg_scratch;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	JSR					extended
//*****************************************************************************
uint8_t Mc6809::JSR_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Don't Care			EA
		break;
	case 6:		//	R	Don't Care			$ffff
		break;
	case 7:		//	W	PC Low				SP-1
		Write(reg_S--, PC_lo);
		break;
	case 8:		//	W	PC High				SP-2
		Write(reg_S--, PC_hi);
		reg_PC = reg_scratch;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	LBCS				relative
//	LBLO				relative
//*****************************************************************************
uint8_t Mc6809::LBCS_LBLO_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBEQ				relative
//*****************************************************************************
uint8_t Mc6809::LBEQ_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBGE				relative
//*****************************************************************************
uint8_t Mc6809::LBGE_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBGT				relative
//*****************************************************************************
uint8_t Mc6809::LBGT_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBHI				relative
//*****************************************************************************
uint8_t Mc6809::LBHI_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBCC				relative
//	LBHS				relative
//*****************************************************************************
uint8_t Mc6809::LBHS_LBCC_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBLE				relative
//*****************************************************************************
uint8_t Mc6809::LBLE_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBLS				relative
//*****************************************************************************
uint8_t Mc6809::LBLS_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBLT				relative
//*****************************************************************************
uint8_t Mc6809::LBLT_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBMI				relative
//*****************************************************************************
uint8_t Mc6809::LBMI_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBNE				relative
//*****************************************************************************
uint8_t Mc6809::LBNE_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBPL				relative
//*****************************************************************************
uint8_t Mc6809::LBPL_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBRA				relative
//*****************************************************************************
uint8_t Mc6809::LBRA_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		reg_PC += reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBRN				relative
//*****************************************************************************
uint8_t Mc6809::LBRN_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	case 6:
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBSR				relative
//*****************************************************************************
uint8_t Mc6809::LBSR_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Offset High			PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Offset Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Don't Care			Effective Address
		reg_scratch += reg_PC;
		break;
	case 7:		//	R	Don't Care			$ffff
		break;
	case 8:		// W	Retern Address Low	SP-1
		Write(reg_S--, PC_lo);
		break;
	case 9:		// W	Return Address High SP-2
		Write(reg_S--, PC_hi);
		reg_PC = reg_scratch;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LBVC				relative
//*****************************************************************************
uint8_t Mc6809::LBVC_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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
//	LBVS				relative
//*****************************************************************************
uint8_t Mc6809::LBVS_rel()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Offset High			PC+2
		scratch_hi = Read(reg_PC++);
		reg_PC++;
		break;
	case 4:		//	R	Offset Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
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


//*****************************************************************************
//	LDA					direct
//*****************************************************************************
uint8_t Mc6809::LDA_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
		scratch_hi = reg_DP;
	case 3:		//	R	Don't Care			$ffff
		break;
	case 4:		//	R	Data				EA
		reg_A = Read(reg_scratch);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDA					extended
//*****************************************************************************
uint8_t Mc6809::LDA_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		reg_A = Read(reg_scratch);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDA					immediate
//*****************************************************************************
uint8_t Mc6809::LDA_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				EA
		reg_A = Read(reg_PC++);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDB					direct
//*****************************************************************************
uint8_t Mc6809::LDB_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
		scratch_hi = reg_DP;
	case 3:		//	R	Don't Care			$ffff
		break;
	case 4:		//	R	Data				EA
		reg_B = Read(reg_scratch);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDB					extended
//*****************************************************************************
uint8_t Mc6809::LDB_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		reg_B = Read(reg_scratch);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDB					immediate
//*****************************************************************************
uint8_t Mc6809::LDB_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				EA
		reg_B = Read(reg_PC++);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDD					direct
//*****************************************************************************
uint8_t Mc6809::LDD_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Register High		EA
		reg_A = Read(reg_scratch++);
		break;
	case 5:		//	R	Register Low		EA+1
		reg_B = Read(reg_scratch);
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDD					extended
//*****************************************************************************
uint8_t Mc6809::LDD_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Register High		EA
		reg_A = Read(reg_scratch++);
		break;
	case 6:		//	R	Register Low		EA+1
		reg_B = Read(reg_scratch);
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDD					immediate
//*****************************************************************************
uint8_t Mc6809::LDD_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Register High		PC+1
		reg_A = Read(reg_PC++);
		break;
	case 3:		//	R	Register Low		PC+2
		reg_B = Read(reg_PC++);
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDS					direct
//*****************************************************************************
uint8_t Mc6809::LDS_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Register High		EA
		S_hi = Read(reg_scratch++);
		break;
	case 6:		//	R	Register Low		EA+1
		S_lo = Read(reg_scratch);
		AdjustCC_N(reg_S);
		AdjustCC_Z(reg_S);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDS					extended
//*****************************************************************************
uint8_t Mc6809::LDS_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Register High		EA
		S_hi = Read(reg_scratch++);
		break;
	case 7:		//	R	Register Low		EA+1
		S_lo = Read(reg_scratch);
		AdjustCC_N(reg_S);
		AdjustCC_Z(reg_S);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDS					immediate
//*****************************************************************************
uint8_t Mc6809::LDS_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Register High		PC+1
		S_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Register Low		PC+2
		S_lo = Read(reg_PC++);
		AdjustCC_N(reg_S);
		AdjustCC_Z(reg_S);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDU					direct
//*****************************************************************************
uint8_t Mc6809::LDU_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Register High		EA
		U_hi = Read(reg_scratch++);
		break;
	case 5:		//	R	Register Low		EA+1
		U_lo = Read(reg_scratch);
		AdjustCC_N(reg_U);
		AdjustCC_Z(reg_U);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDU					extended
//*****************************************************************************
uint8_t Mc6809::LDU_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Register High		EA
		U_hi = Read(reg_scratch++);
		break;
	case 6:		//	R	Register Low		EA+1
		U_lo = Read(reg_scratch);
		AdjustCC_N(reg_U);
		AdjustCC_Z(reg_U);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDU					immediate
//*****************************************************************************
uint8_t Mc6809::LDU_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Register High		PC+1
		U_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Register Low		PC+2
		U_lo = Read(reg_PC++);
		AdjustCC_N(reg_U);
		AdjustCC_Z(reg_U);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDX					direct
//*****************************************************************************
uint8_t Mc6809::LDX_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Register High		EA
		X_hi = Read(reg_scratch++);
		break;
	case 5:		//	R	Register Low		EA+1
		X_lo = Read(reg_scratch);
		AdjustCC_N(reg_X);
		AdjustCC_Z(reg_X);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDX					extended
//*****************************************************************************
uint8_t Mc6809::LDX_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Register High		EA
		X_hi = Read(reg_scratch++);
		break;
	case 6:		//	R	Register Low		EA+1
		X_lo = Read(reg_scratch);
		AdjustCC_N(reg_X);
		AdjustCC_Z(reg_X);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDX					immediate
//*****************************************************************************
uint8_t Mc6809::LDX_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Register High		PC+1
		X_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Register Low		PC+2
		X_lo = Read(reg_PC++);
		AdjustCC_N(reg_X);
		AdjustCC_Z(reg_X);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDY					direct
//*****************************************************************************
uint8_t Mc6809::LDY_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Register High		EA
		Y_hi = Read(reg_scratch++);
		break;
	case 6:		//	R	Register Low		EA+1
		Y_lo = Read(reg_scratch);
		AdjustCC_N(reg_Y);
		AdjustCC_Z(reg_Y);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDY					extended
//*****************************************************************************
uint8_t Mc6809::LDY_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Register High		EA
		Y_hi = Read(reg_scratch++);
		break;
	case 7:		//	R	Register Low		EA+1
		Y_lo = Read(reg_scratch);
		AdjustCC_N(reg_Y);
		AdjustCC_Z(reg_Y);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LDY					immediate
//*****************************************************************************
uint8_t Mc6809::LDY_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Register High		PC+1
		Y_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Register Low		PC+2
		Y_lo = Read(reg_PC++);
		AdjustCC_N(reg_Y);
		AdjustCC_Z(reg_Y);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSRA				inherent
//*****************************************************************************
uint8_t Mc6809::LSRA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = ((reg_A & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = ((reg_A >> 1) & 0x7f);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::N;
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSRB				inherent
//*****************************************************************************
uint8_t Mc6809::LSRB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = ((reg_B & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = ((reg_B >> 1) & 0x7f);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::N;
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSR					direct
//*****************************************************************************
uint8_t Mc6809::LSR_dir()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care			$ffff
		reg_CC = ((data_lo & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		data_lo = ((data_lo >> 1) & 0x7f);
		AdjustCC_Z(data_lo);
		reg_CC &= ~CC::N;
		break;
	case 6:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	LSR					extended
//*****************************************************************************
uint8_t Mc6809::LSR_ext()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		reg_CC = ((data_lo & 0x01) == 1) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		data_lo = ((data_lo >> 1) & 0x7f);
		AdjustCC_Z(data_lo);
		reg_CC &= ~CC::N;
		break;
	case 7:		//	E	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	MUL					inherent
//*****************************************************************************
uint8_t Mc6809::MUL_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		//reg_PC++;
		break;
	case 3:		//	R	Don't Care			$ffff
		reg_D = reg_A * reg_B;
		break;
	case 4:		//	R	Don't Care			$ffff
		AdjustCC_Z(reg_D);
		break;
	case 5:		//	R	Don't Care			$ffff
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		break;
	case 6:		//	R	Don't Care			$ffff
		break;
	case 7:		//	R	Don't Care			$ffff
		break;
	case 8:		//	R	Don't Care			$ffff
		break;
	case 9:		//	R	Don't Care			$ffff
		break;
	case 10:	//	R	Don't Care			$ffff
		break;
	case 11:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NEGA				inherent
//*****************************************************************************
uint8_t Mc6809::NEGA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = (reg_A == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (reg_A != 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		reg_A = 0 - reg_A;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NEGB				inherent
//*****************************************************************************
uint8_t Mc6809::NEGB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		reg_CC = (reg_B == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (reg_B != 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		reg_B = 0 - reg_B;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NEG					direct
//*****************************************************************************
uint8_t Mc6809::NEG_dir()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care			$ffff
		reg_CC = (data_lo == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (data_lo != 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		data_lo = 0 - data_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 6:		//	W	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NEG					extended
//*****************************************************************************
uint8_t Mc6809::NEG_ext()
{
	static uint8_t data_lo;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		reg_CC = (data_lo == 0x80) ? (reg_CC | CC::V) : (reg_CC & ~CC::V);
		reg_CC = (data_lo != 0) ? (reg_CC & ~CC::C) : (reg_CC | CC::C);
		data_lo = 0 - data_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 7:		//	E	Data				EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	NOP					inherent
//*****************************************************************************
uint8_t Mc6809::NOP_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care				PC+1;
		clocksUsed = 255;
		//reg_PC++;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ORA					direct
//*****************************************************************************
uint8_t Mc6809::ORA_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low				PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care				$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data					EA
		scratch_lo = Read(reg_scratch);
		reg_A |= scratch_lo;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	ORA					extended
//*****************************************************************************
uint8_t Mc6809::ORA_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address High			PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low				PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care				$ffff
		break;
	case 5:		//	R	Data					EA
		scratch_lo = Read(reg_scratch);
		reg_A |= scratch_lo;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	ORA					immediate
//*****************************************************************************
uint8_t Mc6809::ORA_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Data					PC+1
		scratch_lo = Read(reg_PC++);
		reg_A |= scratch_lo;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	ORB					direct
//*****************************************************************************
uint8_t Mc6809::ORB_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address High			PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low				PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care				$ffff
		break;
	case 5:		//	R	Data					EA
		scratch_lo = Read(reg_scratch);
		reg_B |= scratch_lo;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	ORB					extended
//*****************************************************************************
uint8_t Mc6809::ORB_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address High			PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low				PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care				$ffff
		break;
	case 5:		//	R	Data					EA
		scratch_lo = Read(reg_scratch);
		reg_B |= scratch_lo;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	ORB					immediate
//*****************************************************************************
uint8_t Mc6809::ORB_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Data					PC+1
		scratch_lo = Read(reg_PC++);
		reg_B |= scratch_lo;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		clocksUsed = 255;
		break;
	}
	return (clocksUsed);
}


//*****************************************************************************
//	ORCC				immediate
//*****************************************************************************
uint8_t Mc6809::ORCC_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		reg_CC |= scratch_lo;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	PSHS				immediate
//*****************************************************************************
uint8_t Mc6809::PSHS_imm()
{
	static int8_t bitNumber;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Post Byte			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		bitNumber = 7;
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Don't Care			SP
		break;
	case 6:		//	W	Register ?
		while (bitNumber >= 0 && !((scratch_lo >> bitNumber) & 0x01))
			--bitNumber;

		if (bitNumber > 0)
			return(255);

		switch (bitNumber)
		{
		case 7:
			Write(reg_S--, PC_lo);
			break;
		case 6:
			Write(reg_S--, U_lo);
			break;
		case 5:
			Write(reg_S--, Y_lo);
			break;
		case 4:
			Write(reg_S--, X_lo);
			break;
		case 3:
			Write(reg_S--, reg_DP);
			--clocksUsed;
			break;
		case 2:
			Write(reg_S--, reg_B);
			--clocksUsed;
			break;
		case 1:
			Write(reg_S--, reg_A);
			--clocksUsed;
			break;
		case 0:
			Write(reg_S--, reg_CC);
			--clocksUsed;
			break;
		}
		break;
	case 7:		//	W	Register ?
		switch (bitNumber)
		{
		case 7:
			Write(reg_S--, PC_hi);
			break;
		case 6:
			Write(reg_S--, U_hi);
			break;
		case 5:
			Write(reg_S--, Y_hi);
			break;
		case 4:
			Write(reg_S--, X_hi);
			break;
		}
		clocksUsed -= 2;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	PSHU				immediate
//*****************************************************************************
uint8_t Mc6809::PSHU_imm()
{
	static int8_t bitNumber;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Post Byte			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		bitNumber = 7;
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Don't Care			SP
		break;
	case 6:		//	W	Register ?
		while (bitNumber >= 0 && !((scratch_lo >> bitNumber) & 0x01))
			--bitNumber;

		if (bitNumber > 0)
			return(255);

		switch (bitNumber)
		{
		case 7:
			Write(reg_U--, PC_lo);
			break;
		case 6:
			Write(reg_U--, S_lo);
			break;
		case 5:
			Write(reg_U--, Y_lo);
			break;
		case 4:
			Write(reg_U--, X_lo);
			break;
		case 3:
			Write(reg_U--, reg_DP);
			--clocksUsed;
			break;
		case 2:
			Write(reg_U--, reg_B);
			--clocksUsed;
			break;
		case 1:
			Write(reg_U--, reg_A);
			--clocksUsed;
			break;
		case 0:
			Write(reg_U--, reg_CC);
			--clocksUsed;
			break;
		}
		break;
	case 7:		//	W	Register ?
		switch (bitNumber)
		{
		case 7:
			Write(reg_U--, PC_hi);
			break;
		case 6:
			Write(reg_U--, S_hi);
			break;
		case 5:
			Write(reg_U--, Y_hi);
			break;
		case 4:
			Write(reg_U--, X_hi);
			break;
		}
		clocksUsed -= 2;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	PULS				immediate
//*****************************************************************************
uint8_t Mc6809::PULS_imm()
{
	static int8_t bitNumber;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Post Byte			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		bitNumber = 0;
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Don't Care			SP
		break;
	case 6:		//	R	Register ?
		while (bitNumber > 8 && !((scratch_lo >> bitNumber) & 0x01))
			++bitNumber;

		if (bitNumber >= 8)
			return(255);

		switch (bitNumber)
		{
		case 7:
			Write(++reg_S, PC_hi);
			break;
		case 6:
			Write(++reg_S, U_hi);
			break;
		case 5:
			Write(++reg_S, Y_hi);
			break;
		case 4:
			Write(++reg_S, X_hi);
			break;
		case 3:
			Write(++reg_S, reg_DP);
			--clocksUsed;
			break;
		case 2:
			Write(++reg_S, reg_B);
			--clocksUsed;
			break;
		case 1:
			Write(++reg_S, reg_A);
			--clocksUsed;
			break;
		case 0:
			Write(++reg_S, reg_CC);
			--clocksUsed;
			break;
		}
		break;
	case 7:		//	R	Register ?
		switch (bitNumber)
		{
		case 7:
			Write(++reg_S, PC_lo);
			break;
		case 6:
			Write(++reg_S, U_lo);
			break;
		case 5:
			Write(++reg_S, Y_lo);
			break;
		case 4:
			Write(++reg_S, X_lo);
			break;
		}
		clocksUsed -= 2;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	PULU				immediate
//*****************************************************************************
uint8_t Mc6809::PULU_imm()
{
	static int8_t bitNumber;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Post Byte			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		bitNumber = 0;
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Don't Care			SP
		break;
	case 6:		//	W	Register ?
		while (bitNumber > 8 && !((scratch_lo >> bitNumber) & 0x01))
			++bitNumber;

		if (bitNumber >= 8)
			return(255);

		switch (bitNumber)
		{
		case 7:
			Write(++reg_U, PC_hi);
			break;
		case 6:
			Write(++reg_U, S_hi);
			break;
		case 5:
			Write(++reg_U, Y_hi);
			break;
		case 4:
			Write(++reg_U, X_hi);
			break;
		case 3:
			Write(++reg_U, reg_DP);
			--clocksUsed;
			break;
		case 2:
			Write(++reg_U, reg_B);
			--clocksUsed;
			break;
		case 1:
			Write(++reg_U, reg_A);
			--clocksUsed;
			break;
		case 0:
			Write(++reg_U, reg_CC);
			--clocksUsed;
			break;
		}
		break;
	case 7:		//	W	Register ?
		switch (bitNumber)
		{
		case 7:
			Write(++reg_U, PC_lo);
			break;
		case 6:
			Write(++reg_U, S_lo);
			break;
		case 5:
			Write(++reg_U, Y_lo);
			break;
		case 4:
			Write(++reg_U, X_lo);
			break;
		}
		clocksUsed -= 2;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROLA				inherent
//*****************************************************************************
uint8_t Mc6809::ROLA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		scratch_lo = ((reg_CC & CC::C) != 0) ? 1 : 0;
		AdjustCC_V(scratch_lo);
		reg_CC = ((reg_A & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = (reg_A << 1) | scratch_lo;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROLB				inherent
//*****************************************************************************
uint8_t Mc6809::ROLB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		scratch_lo = ((reg_CC & CC::C) != 0) ? 1 : 0;
		AdjustCC_V(scratch_lo);
		reg_CC = ((reg_B & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = (reg_B << 1) | scratch_lo;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROL					direct
//*****************************************************************************
uint8_t Mc6809::ROL_dir()
{
	static uint8_t data_lo;
	static uint8_t carry;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low				PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care				$ffff
		scratch_hi = DP;
		break;
	case 4:		//	R	Data					EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care				$ffff
		carry = ((reg_CC & CC::C) != 0) ? 1 : 0;
		AdjustCC_V(data_lo);
		reg_CC = ((data_lo & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		data_lo = (data_lo << 1) | scratch_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 6:		//	W	Data					EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROL					extended
//*****************************************************************************
uint8_t Mc6809::ROL_ext()
{
	static uint8_t data_lo;
	static uint8_t carry;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address High			PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low				PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care				$ffff
		break;
	case 5:		//	R	Data					EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care				$ffff
		carry = ((reg_CC & CC::C) != 0) ? 1 : 0;
		AdjustCC_V(data_lo);
		reg_CC = ((data_lo & 0x80) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		data_lo = (data_lo << 1) | scratch_lo;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 7:		//	W	Data					EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RORA				inherent
//*****************************************************************************
uint8_t Mc6809::RORA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		scratch_hi = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((reg_A & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_A = (reg_A >> 1) | scratch_hi;
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RORB				inherent
//*****************************************************************************
uint8_t Mc6809::RORB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		scratch_lo = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((reg_B & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		reg_B = (reg_B >> 1) | scratch_lo;
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROR					direct
//*****************************************************************************
uint8_t Mc6809::ROR_dir()
{
	static uint8_t data_lo;
	static uint8_t carry;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low				PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care				$ffff
		scratch_hi = DP;
		break;
	case 4:		//	R	Data					EA
		data_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care				$ffff
		carry = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((data_lo & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		data_lo = (data_lo >> 1) | carry;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 6:		//	W	Data					EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	ROR					extended
//*****************************************************************************
uint8_t Mc6809::ROR_ext()
{
	static uint8_t data_lo;
	static uint8_t carry;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch			PC
		reg_PC++;
		break;
	case 2:		//	R	Address High			PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low				PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care				$ffff
		break;
	case 5:		//	R	Data					EA
		data_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care				$ffff
		carry = ((reg_CC & CC::C) != 0) ? 0x80 : 0;
		reg_CC = ((data_lo & 0x01) != 0) ? (reg_CC | CC::C) : (reg_CC & ~CC::C);
		data_lo = (data_lo >> 1) | carry;
		AdjustCC_N(data_lo);
		AdjustCC_Z(data_lo);
		break;
	case 7:		//	W	Data					EA
		Write(reg_scratch, data_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RTI					inherent
//*****************************************************************************
uint8_t Mc6809::RTI_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		break;
	case 3:		//	R	CCR					SP
		reg_CC = Read(++reg_S);
		break;
	}

	// if the E flag is clear (E = 0) we do the short version
	if ((reg_CC & CC::E) != CC::E)
	{
		switch (clocksUsed)
		{
		case 4: //	R	PC High				SP+1
			PC_hi = Read(++reg_S);
			break;
		case 5: //	R	PC low				SP+2
			PC_lo = Read(++reg_S);
			break;
		case 6: //	R	Don't Care			$ffff
			clocksUsed = 255;
			break;
		}
		// Don't touch the long version below.
		return (clocksUsed);
	}

	// if the E flag is set (E = 1) we do the long version
	switch (clocksUsed)
	{
	case 4:		//	R	A Register			SP+1
		reg_A = Read(++reg_S);
		break;
	case 5:		//	R	B Register			SP+2
		reg_B = Read(++reg_S);
		break;
	case 6:		//	R	DP Register			SP+3
		reg_DP = Read(++reg_S);
		break;
	case 7:		//	R	X Register High		SP+4
		X_hi = Read(++reg_S);
		break;
	case 8:		//	R	X Register Low		SP+5
		X_lo = Read(++reg_S);
		break;
	case 9:		//	R	Y Register High		SP+6
		Y_hi = Read(++reg_S);
		break;
	case 10:	//	R	Y Register Low		SP+7
		Y_lo = Read(++reg_S);
		break;
	case 11:	//	R	User Stack High		SP+8
		U_hi = Read(++reg_S);
		break;
	case 12:	//	R	User Stack Low		SP+9
		U_lo = Read(++reg_S);
		break;
	case 13:	//	R	PC High				SP+10
		PC_hi = Read(++reg_S);
		break;
	case 14:	//	R	PC Low				SP+11
		PC_lo = Read(++reg_S);
		break;
	case 15:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	RTS					inherent
//*****************************************************************************
uint8_t Mc6809::RTS_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		//reg_PC++;
		break;
	case 3:		//	R	PC High				SP
		PC_lo = Read(++reg_S);
		break;
	case 4:		//	R	PC Low				SP+1
		PC_hi = Read(++reg_S);
		break;
	case 5:		//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SBCA				direct
//*****************************************************************************
uint8_t Mc6809::SBCA_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_A - scratch_lo - ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_A, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_A);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SBCA				extended
//*****************************************************************************
uint8_t Mc6809::SBCA_ext()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B - scratch_lo - ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SBCA				immediate
//*****************************************************************************
uint8_t Mc6809::SBCA_imm()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		data = reg_B - scratch_lo - ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SBCB				direct
//*****************************************************************************
uint8_t Mc6809::SBCB_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B - scratch_lo - ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SBCB				extended
//*****************************************************************************
uint8_t Mc6809::SBCB_ext()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B - scratch_lo - ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SBCB				immediate
//*****************************************************************************
uint8_t Mc6809::SBCB_imm()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		data = reg_B - scratch_lo - ((reg_CC & CC::C) == CC::C ? 1 : 0);
		AdjustCC_V(scratch_lo, reg_B, (data & 0xff));
		reg_A = (data & 0xff);
		AdjustCC_H(reg_B);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}

//*****************************************************************************
//	SEX					inherent
//*****************************************************************************
uint8_t Mc6809::SEX_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		if ((reg_B & 0x80) == 0x80)
		{
			reg_A = 255;
			reg_CC |= CC::N;
			reg_CC &= ~CC::Z;
		}
		else
		{
			reg_A = 0x00;
			reg_CC &= ~CC::N;
			reg_CC |= CC::Z;
		}
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STA					direct
//*****************************************************************************
uint8_t Mc6809::STA_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	W	Register			EA
		Write(reg_scratch, reg_A);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STA					extended
//*****************************************************************************
uint8_t Mc6809::STA_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High			PC+1
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	W	Register			EA
		Write(reg_scratch, reg_A);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STB					direct
//*****************************************************************************
uint8_t Mc6809::STB_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	W	Register			EA
		Write(reg_scratch, reg_B);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STB					extended
//*****************************************************************************
uint8_t Mc6809::STB_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High			PC+1
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	W	Register			EA
		Write(reg_scratch, reg_B);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STD					direct
//*****************************************************************************
uint8_t Mc6809::STD_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	W	Register High		EA
		Write(reg_scratch, reg_A);
		break;
	case 5:		//	W	Register Low		EA+1
		Write(++reg_scratch, reg_B);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STD					extended
//*****************************************************************************
uint8_t Mc6809::STD_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff

		break;
	case 5:		//	W	Register High		EA
		Write(reg_scratch, reg_A);
		break;
	case 6:		//	W	Register Low		EA+1
		Write(++reg_scratch, reg_B);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STS					direct
//*****************************************************************************
uint8_t Mc6809::STS_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	W	Register High		EA
		Write(reg_scratch, S_hi);
		break;
	case 6:		//	W	Register Low		EA+1
		Write(++reg_scratch, S_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STS					extended
//*****************************************************************************
uint8_t Mc6809::STS_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+2
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	W	Register High		EA
		Write(reg_scratch, S_hi);
		break;
	case 7:		//	W	Register Low		EA+1
		Write(++reg_scratch, S_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STU					direct
//*****************************************************************************
uint8_t Mc6809::STU_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	W	Register High		EA
		Write(reg_scratch, U_hi);
		break;
	case 5:		//	W	Register Low		EA+1
		Write(++reg_scratch, U_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STU					extended
//*****************************************************************************
uint8_t Mc6809::STU_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	W	Register High		EA
		Write(reg_scratch, U_hi);
		break;
	case 6:		//	W	Register Low		EA+1
		Write(++reg_scratch, U_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STX					direct
//*****************************************************************************
uint8_t Mc6809::STX_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	W	Register High		EA
		Write(reg_scratch, X_hi);
		break;
	case 5:		//	W	Register Low		EA+1
		Write(++reg_scratch, X_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STX					extended
//*****************************************************************************
uint8_t Mc6809::STX_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	W	Register High		EA
		Write(reg_scratch, X_hi);
		break;
	case 6:		//	W	Register Low		EA+1
		Write(++reg_scratch, X_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STY					direct
//*****************************************************************************
uint8_t Mc6809::STY_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	W	Register High		EA
		Write(reg_scratch, Y_hi);
		break;
	case 6:		//	W	Register Low		EA+1
		Write(++reg_scratch, Y_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	STY					extended
//*****************************************************************************
uint8_t Mc6809::STY_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+2
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+3
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	W	Register High		EA
		Write(reg_scratch, Y_hi);
		break;
	case 7:		//	W	Register Low		EA+1
		Write(++reg_scratch, Y_lo);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBA				direct
//*****************************************************************************
uint8_t Mc6809::SUBA_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_A - scratch_lo;
		AdjustCC_V(reg_A, scratch_lo, (uint8_t)(data & 0xff));
		reg_A = (uint8_t)(data & 0xff);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBA				extended
//*****************************************************************************
uint8_t Mc6809::SUBA_ext()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_A - scratch_lo;
		AdjustCC_V(reg_A, scratch_lo, (uint8_t)(data & 0xff));
		reg_A = (uint8_t)(data & 0xff);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBA				immediate
//*****************************************************************************
uint8_t Mc6809::SUBA_imm()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		data = reg_A - scratch_lo;
		AdjustCC_V(reg_A, scratch_lo, (uint8_t)(data & 0xff));
		reg_A = (uint8_t)(data & 0xff);
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBB				direct
//*****************************************************************************
uint8_t Mc6809::SUBB_dir()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B - scratch_lo;
		AdjustCC_V(reg_B, scratch_lo, (uint8_t)(data & 0xff));
		reg_B = (uint8_t)(data & 0xff);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBB				extended
//*****************************************************************************
uint8_t Mc6809::SUBB_ext()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		data = reg_B - scratch_lo;
		AdjustCC_V(reg_B, scratch_lo, (uint8_t)(data & 0xff));
		reg_B = (uint8_t)(data & 0xff);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBB				immediate
//*****************************************************************************
uint8_t Mc6809::SUBB_imm()
{
	uint16_t data;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Data				PC+1
		scratch_lo = Read(reg_PC++);
		data = reg_B - scratch_lo;
		AdjustCC_V(reg_B, scratch_lo, (uint8_t)(data & 0xff));
		reg_B = (uint8_t)(data & 0xff);
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBD				direct
//*****************************************************************************
uint8_t Mc6809::SUBD_dir()
{
	uint32_t data;
	static uint16_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	OpCode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 5:		//	R	Data				EA
		tempRegValue = Read(reg_scratch) << 8;
		break;
	case 6:		//	R	Data Low			EA+1
		tempRegValue |= Read(++reg_scratch);
		data = reg_D - tempRegValue;
		AdjustCC_V(reg_D, tempRegValue, (uint16_t)(data & 0xffff));
		reg_D = (uint16_t)(data & 0xffff);
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBD				extended
//*****************************************************************************
uint8_t Mc6809::SUBD_ext()
{
	uint32_t data;
	static uint16_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	OpCode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 4:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 5:		//	R	Don't Care			$ffff
		break;
	case 6:		//	R	Data				EA
		tempRegValue = Read(reg_scratch) << 8;
		break;
	case 7:		//	R	Data Low			EA+1
		tempRegValue |= Read(++reg_scratch);
		data = reg_D - tempRegValue;
		AdjustCC_V(reg_D, tempRegValue, (uint16_t)(data & 0xffff));
		reg_D = (uint16_t)(data & 0xffff);
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SUBD				immediate
//*****************************************************************************
uint8_t Mc6809::SUBD_imm()
{
	uint32_t data;
	static uint16_t tempRegValue;

	switch (++clocksUsed)
	{
	case 1:		//	R	OpCode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	OpCode 2nd Byte		PC+1
		reg_PC++;
		break;
	case 3:		//	R	Data High			PC+2
		tempRegValue = Read(reg_PC++) << 8;
	case 4:		//	R	Data Low			PC+3
		tempRegValue = Read(reg_PC++);
		data = reg_D - tempRegValue;
		AdjustCC_V(reg_D, tempRegValue, (uint16_t)(data & 0xffff));
		reg_D = (uint16_t)(data & 0xffff);
		AdjustCC_N(reg_D);
		AdjustCC_Z(reg_D);
		AdjustCC_C(data);
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SWI					inherent
//*****************************************************************************
uint8_t Mc6809::SWI_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't care			PC+1	++PC
		//reg_PC++;
		break;
	case 3:		//	R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(reg_S--, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(reg_S--, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(reg_S--, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(reg_S--, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(reg_S--, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(reg_S--, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(reg_S--, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(reg_S--, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(reg_S--, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		break;
	case 16:	//	R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	//	R	Int Vector High		$fffa
		PC_hi = Read(0xfffa);
		break;
	case 18:	//	R	Int Vector Low		$fffb
		PC_lo = Read(0xfffb);
		break;
	case 19:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SWI2				inherent
//*****************************************************************************
uint8_t Mc6809::SWI2_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd byte		PC+1	++PC
		reg_PC++;
		break;
	case 3:		//	R	Don't care			PC+2	++PC
		reg_PC++;
		break;
	case 4:		//	R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 5:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 6:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 7:		// W	User Stack Low		SP-3	--SP
		Write(reg_S--, U_lo);
		break;
	case 8:		// W	User Stack High		SP-4	--SP
		Write(reg_S--, U_hi);
		break;
	case 9:		// W	Y  Register Low		SP-5	--SP
		Write(reg_S--, Y_lo);
		break;
	case 10:	// W	Y  Register High	SP-6	--SP
		Write(reg_S--, Y_hi);
		break;
	case 11:	// W	X  Register Low		SP-7	--SP
		Write(reg_S--, X_lo);
		break;
	case 12:	// W	X  Register High	SP-8	--SP
		Write(reg_S--, X_hi);
		break;
	case 13:	// W	DP Register			SP-9	--SP
		Write(reg_S--, reg_DP);
		break;
	case 14:	// W	B  Register			SP-10	--SP
		Write(reg_S--, reg_B);
		break;
	case 15:	// W	A  Register			SP-11	--SP
		Write(reg_S--, reg_A);
		break;
	case 16:	// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		break;
	case 17:	//	R	Don't Care			$ffff
		break;
	case 18:	//	R	Int Vector High		$fff4
		PC_hi = Read(0xfff4);
		break;
	case 19:	//	R	Int Vector Low		$fff5
		PC_lo = Read(0xfff5);
		break;
	case 20:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SWI3				inherent
//*****************************************************************************
uint8_t Mc6809::SWI3_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Opcode 2nd byte		PC+1	++PC
		reg_PC++;
		break;
	case 3:		//	R	Don't care			PC+2	++PC
		reg_PC++;
		break;
	case 4:		//	R	Don't care			$ffff
		reg_CC |= CC::E;
		break;
	case 5:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 6:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 7:		// W	User Stack Low		SP-3	--SP
		Write(reg_S--, U_lo);
		break;
	case 8:		// W	User Stack High		SP-4	--SP
		Write(reg_S--, U_hi);
		break;
	case 9:		// W	Y  Register Low		SP-5	--SP
		Write(reg_S--, Y_lo);
		break;
	case 10:	// W	Y  Register High	SP-6	--SP
		Write(reg_S--, Y_hi);
		break;
	case 11:	// W	X  Register Low		SP-7	--SP
		Write(reg_S--, X_lo);
		break;
	case 12:	// W	X  Register High	SP-8	--SP
		Write(reg_S--, X_hi);
		break;
	case 13:	// W	DP Register			SP-9	--SP
		Write(reg_S--, reg_DP);
		break;
	case 14:	// W	B  Register			SP-10	--SP
		Write(reg_S--, reg_B);
		break;
	case 15:	// W	A  Register			SP-11	--SP
		Write(reg_S--, reg_A);
		break;
	case 16:	// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		break;
	case 17:	//	R	Don't Care			$ffff
		break;
	case 18:	//	R	Int Vector High		$fff2
		PC_hi = Read(0xfff2);
		break;
	case 19:	//	R	Int Vector Low		$fff3
		PC_lo = Read(0xfff3);
		break;
	case 20:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	SYNC				inherent
//*****************************************************************************
uint8_t Mc6809::SYNC_inh()
{
	static int8_t intCount = 0;

	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		//reg_PC++;
		break;
	case 3:		//	R	Don't Care			Z
		if ((reg_CC & CC::I) || (reg_CC & CC::F))
			clocksUsed = 255;
		else if (!Nmi || !Firq || !Irq)
			--clocksUsed;
		break;
	case 4:		//	R	Don't Care			Z
		if (Nmi || Firq || Irq)
		{
			++intCount;
			if (intCount >= 3)
			{
				if (Nmi)
				{
					clocksUsed = 1;
					intCount = 0;
					exec = &Mc6809::NMI;
				}
				else if (Firq)
				{
					clocksUsed = 1;
					intCount = 0;
					exec = &Mc6809::FIRQ;
				}
				else if (Irq)
				{
					clocksUsed = 1;
					intCount = 0;
					exec = &Mc6809::IRQ;
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
//	TFR					immediate
//*****************************************************************************
uint8_t Mc6809::TFR_imm()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Post Byte			PC+1
		scratch_lo = Read(reg_PC);
		reg_PC++;
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = (scratch_lo & 0xf0) >> 4;
		scratch_lo &= 0x0f;
		break;
	case 4:		//	R	Don't Care			$ffff
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
	case 5:		//	R	Don't Care			$ffff
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
	case 6:		//	R	Don't Care			$ffff
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
//	TSTA				inherent
//*****************************************************************************
uint8_t Mc6809::TSTA_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		AdjustCC_N(reg_A);
		AdjustCC_Z(reg_A);
		reg_CC &= ~CC::V;
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TSTB				inherent
//*****************************************************************************
uint8_t Mc6809::TSTB_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't Care			PC+1
		AdjustCC_N(reg_B);
		AdjustCC_Z(reg_B);
		reg_CC &= ~CC::V;
		//reg_PC++;
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TST					direct
//*****************************************************************************
uint8_t Mc6809::TST_dir()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address Low			PC+1
		scratch_lo = Read(reg_PC++);
		break;
	case 3:		//	R	Don't Care			$ffff
		scratch_hi = reg_DP;
		break;
	case 4:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		break;
	case 5:		//	R	Don't Care			$ffff
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		reg_CC &= ~CC::V;
		break;
	case 6:		//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*****************************************************************************
//	TST					extended
//*****************************************************************************
uint8_t Mc6809::TST_ext()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Address High		PC+1
		scratch_hi = Read(reg_PC++);
		break;
	case 3:		//	R	Address Low			PC+2
		scratch_lo = Read(reg_PC++);
		break;
	case 4:		//	R	Don't Care			$ffff
		break;
	case 5:		//	R	Data				EA
		scratch_lo = Read(reg_scratch);
		break;
	case 6:		//	R	Don't Care			$ffff
		AdjustCC_N(scratch_lo);
		AdjustCC_Z(scratch_lo);
		reg_CC &= ~CC::V;
		break;
	case 7:		//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Invalid opcode handler and any undocumented opcodes that are implemented
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
//	RESET "interrupt" opcode $3E)
//*****************************************************************************
//	Returns:
//	uint8_t - 255 to signify function is completed.
//				(We're treating a bad instruction as if it was a NOP)
//*****************************************************************************
uint8_t Mc6809::XXX()
{
	return(255);
}


//*****************************************************************************
//	RESET				inherent
//*****************************************************************************
uint8_t Mc6809::RESET_inh()
{
	switch (++clocksUsed)
	{
	case 1:		//	R	Opcode Fetch		PC
		reg_PC++;
		break;
	case 2:		//	R	Don't care			PC+1	++PC
		//reg_PC++;
		break;
	case 3:		//	R	Don't care			$ffff
		// apparently does not set the E flag, this will throw a RTI from this off.
		//reg_CC |= CC::E;
		break;
	case 4:		// W	PC Low				SP-1	--SP
		Write(reg_S--, PC_lo);
		break;
	case 5:		// W	PC High				SP-2	--SP
		Write(reg_S--, PC_hi);
		break;
	case 6:		// W	User Stack Low		SP-3	--SP
		Write(reg_S--, U_lo);
		break;
	case 7:		// W	User Stack High		SP-4	--SP
		Write(reg_S--, U_hi);
		break;
	case 8:		// W	Y  Register Low		SP-5	--SP
		Write(reg_S--, Y_lo);
		break;
	case 9:		// W	Y  Register High	SP-6	--SP
		Write(reg_S--, Y_hi);
		break;
	case 10:	// W	X  Register Low		SP-7	--SP
		Write(reg_S--, X_lo);
		break;
	case 11:	// W	X  Register High	SP-8	--SP
		Write(reg_S--, X_hi);
		break;
	case 12:	// W	DP Register			SP-9	--SP
		Write(reg_S--, reg_DP);
		break;
	case 13:	// W	B  Register			SP-10	--SP
		Write(reg_S--, reg_B);
		break;
	case 14:	// W	A  Register			SP-11	--SP
		Write(reg_S--, reg_A);
		break;
	case 15:	// W	CC Register			SP-12	--SP
		Write(reg_S--, reg_CC);
		break;
	case 16:	//	R	Don't Care			$ffff
		reg_CC |= (CC::I | CC::F);
		break;
	case 17:	//	R	Int Vector High		$fffe
		PC_hi = Read(0xfffe);
		break;
	case 18:	//	R	Int Vector Low		$ffff
		PC_lo = Read(0xffff);
		break;
	case 19:	//	R	Don't Care			$ffff
		clocksUsed = 255;
		break;
	}
	return(clocksUsed);
}


//*********************************************************************************************************************************
// Indexed modes for opcodes
//*********************************************************************************************************************************

uint8_t Mc6809::ADCA_idx()	// H N Z V C all modified. reg_A modified
{}
uint8_t Mc6809::ADCB_idx() {}
uint8_t Mc6809::ADDA_idx() {}
uint8_t Mc6809::ADDB_idx() {}
uint8_t Mc6809::ADDD_idx()
{
	static uint8_t data_hi;
}
uint8_t Mc6809::ANDA_idx() {}
uint8_t Mc6809::ANDB_idx() {}
uint8_t Mc6809::ASL_LSL_idx() {}
uint8_t Mc6809::ASR_idx() {}
uint8_t Mc6809::BITA_idx() {}
uint8_t Mc6809::BITB_idx() {}
uint8_t Mc6809::CLR_idx() {}
uint8_t Mc6809::CMPA_idx() {}
uint8_t Mc6809::CMPB_idx() {}
uint8_t Mc6809::CMPD_idx() {}
uint8_t Mc6809::CMPS_idx() {}
uint8_t Mc6809::CMPU_idx() {}
uint8_t Mc6809::CMPX_idx() {}
uint8_t Mc6809::CMPY_idx() {}
uint8_t Mc6809::COM_idx() {}
uint8_t Mc6809::DEC_idx() {}
uint8_t Mc6809::EORA_idx() {}
uint8_t Mc6809::EORB_idx() {}
uint8_t Mc6809::INC_idx() {}
uint8_t Mc6809::JMP_idx() {}
uint8_t Mc6809::JSR_idx() {}
uint8_t Mc6809::LDA_idx() {}
uint8_t Mc6809::LDB_idx() {}
uint8_t Mc6809::LDD_idx() {}
uint8_t Mc6809::LDS_idx() {}
uint8_t Mc6809::LDU_idx() {}
uint8_t Mc6809::LDX_idx() {}
uint8_t Mc6809::LDY_idx() {}
uint8_t Mc6809::LEAS_idx() {}
uint8_t Mc6809::LEAU_idx() {}
uint8_t Mc6809::LEAX_idx() {}
uint8_t Mc6809::LEAY_idx() {}
uint8_t Mc6809::LSR_idx() {}
uint8_t Mc6809::NEG_idx() {}
uint8_t Mc6809::ORA_idx() {}
uint8_t Mc6809::ORB_idx() {}
uint8_t Mc6809::ROL_idx() {}
uint8_t Mc6809::ROR_idx() {}
uint8_t Mc6809::SBCA_idx() {}
uint8_t Mc6809::SBCB_idx() {}
uint8_t Mc6809::STA_idx() {}
uint8_t Mc6809::STB_idx() {}
uint8_t Mc6809::STD_idx() {}
uint8_t Mc6809::STS_idx() {}
uint8_t Mc6809::STU_idx() {}
uint8_t Mc6809::STX_idx() {}
uint8_t Mc6809::STY_idx() {}
uint8_t Mc6809::SUBA_idx() {}
uint8_t Mc6809::SUBB_idx() {}
uint8_t Mc6809::SUBD_idx() {}
uint8_t Mc6809::TST_idx() {}


//.