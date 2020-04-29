#include "MMU.h"
#include "MC68F09E.h"


MC6809::MC6809(MMU* pMMU)
{
	mmu = pMMU;
	halt = 0;
	cycleCount = 0;
	Reset(true);

}

MC6809::~MC6809()
{}

// Interrupts
uint8_t MC6809::Reset(bool initCall)
{
	if (initCall)
	{
		instruction = 0;
		cycleCount = 0;
	}
	++cycleCount;
	switch (cycleCount)
	{
	case 1:
		reg_PC = 0xFFFE;
		instruction = mmu->Read(reg_PC++) << 8;
	case 2:
		mmu->Read(0xffff);
		instruction |=mmu->Read(reg_PC++);
	case 3:
		reg_PC = instruction;
	}
}

uint8_t MC6809::NMI()
{
}

uint8_t MC6809::IRQ()
{
}

uint8_t MC6809::FIRQ()
{
}

// other functions
uint8_t MC6809::Fetch()
{
	if (BA == true && BS == true);
		return (0);
	uint8_t byteRead = mmu->Read(reg_PC++);
	if(byteRead == 0x10 || byteRead == 0x11)

}

uint8_t MC6809::Halt()
{
	halt = ~halt;
}

// address modes
uint8_t MC6809::ADDR_Inherent()                                    // 01
{}

uint8_t MC6809::ADDR_Immediate()                                   // 02
{}

uint8_t MC6809::ADDR_Direct()                                      // 03
{}

uint8_t MC6809::ADDR_Extended()                                    // 04
{}

uint8_t MC6809::ADDR_Extended_Indirect()                           // 05
{}

uint8_t MC6809::ADDR_Direct()                                      // 06
{}

uint8_t MC6809::ADDR_Register()                                    // 07
{}

uint8_t MC6809::ADDR_Indexed()                                     // 08
{}

uint8_t MC6809::ADDR_Indexed_ZeroOffset()                          // 09
{}

uint8_t MC6809::ADDR_Indexed_ConstantOffset()                      // 10
{}

uint8_t MC6809::ADDR_Indexed_AccumulatorOffset()                   // 11
{}

uint8_t MC6809::ADDR_Indexed_AutoIncrementDecrement()              // 12
{}

uint8_t MC6809::ADDR_Indexed_Indirect()                            // 13
{}

uint8_t MC6809::ADDR_Relative()                                    // 14
{}

uint8_t MC6809::ADDR_Relative_ShortLong()                          // 15
{}

uint8_t MC6809::ADDR_Relative_PrgrmCounterRelativeAddressing()     // 16
{}


// instructions

//*****************************************************************************
// ABX()
//*****************************************************************************
// Add B to X
//*****************************************************************************
uint8_t MC6809::ABX()
{}

// Add to A + Carry
uint8_t MC6809::ADCA()
{}

// Add to A + Carry
uint8_t MC6809::ADCB()
{}

// Add to A
uint8_t MC6809::ADDA()
{}

// Add to B
uint8_t MC6809::ADDB()
{}

// Add to D (A << 8 | B)
uint8_t MC6809::ADDD()
{}

// And A
uint8_t MC6809::ANDA()
{}

// And B
uint8_t MC6809::ANDB()
{}

// And Condition Codes (clear one or more flags)
uint8_t MC6809::ANDCC()
{}

// Arithmetic Shift Left A (Logical Shift Left fill LSb with 0)
uint8_t MC6809::ASLA()
{}

// Arithmetic Shift Left B (Logical Shift Left fill LSb with 0)
uint8_t MC6809::ASLB()
{}

// Arithmetic Shift Left Memory location (Logical Shift Left fill LSb with 0)
uint8_t MC6809::ASL()
{}

// Arithmetic Shift Right A (fill MSb with Sign bit)
uint8_t MC6809::ASRA()
{}

// Arithmetic Shift Right B (fill MSb with Sign bit)
uint8_t MC6809::ASRB()
{}

// Arithmetic Shift Right Memory location (fill MSb with Sign bit)
uint8_t MC6809::ASR()
{}

// Branch on Carry Clear        (C clear)
uint8_t MC6809::BCC()
{}

// Long Branch on Carry Clear
uint8_t MC6809::LBCC()
{}

// Branch on Carry Set          (C set)
uint8_t MC6809::BCS()
{}

// Long Branch on Carry Set
uint8_t MC6809::LBCS()
{}

// Branch if Equal              (Z set)
uint8_t MC6809::BEQ()
{}

// Long Branch if Equal
uint8_t MC6809::LBEQ()
{}

// Branch if Greater or Equal (signed)
uint8_t MC6809::BGE()
{}

// Long Branch  if Greater or Equal
uint8_t MC6809::LBGE()
{}

// Branch if Greater than (signed)
uint8_t MC6809::BGT()
{}

// Long Branch if Greater than
uint8_t MC6809::LBGT()
{}

// Branch if Higher (unsigned)
uint8_t MC6809::BHI()
{}

// Long Branch if Higher
uint8_t MC6809::LBHI()
{}

// Branch if higher or same (unsigned)
uint8_t MC6809::BHS()
{}

// Long Branch if higher or same
uint8_t MC6809::LBHS()
{}

// Bit Test on A with a specific value (by AND)
uint8_t MC6809::BITA()
{}

// Bit Test on B with a specific value (by AND)
uint8_t MC6809::BITB()
{}

// Branch if Less than or Equal (signed)
uint8_t MC6809::BLE()
{}

// Long Branch if Less than or Equal (signed)
uint8_t MC6809::LBLE()
{}

// Branch if Lower (unsigned)
uint8_t MC6809::BLO()
{}

// Long Branch if Lower (unsigned)
uint8_t MC6809::LBLO()
{}

// Branch if Lower or Same (unsigned)
uint8_t MC6809::BLS()
{}

// Long Branch if Lower or Same (unsigned)
uint8_t MC6809::LBLS()
{}

// Branch if less than (signed)
uint8_t MC6809::BLT()
{}

// Long Branch if less than (signed)
uint8_t MC6809::LBLT()
{}

// Branch on Minus 
uint8_t MC6809::BMI()
{}

// Long Branch on Minus
uint8_t MC6809::LBMI()
{}

// Branch if Not Equal (Z = 0)
uint8_t MC6809::BNE()
{}

// Long Branch if Not Equal (Z = 0)
uint8_t MC6809::LBNE()
{}

// Branch if Plus/Positive
uint8_t MC6809::BPL()
{}

// Long Branch if Plus/Positive
uint8_t MC6809::LBPL()
{}

// Branch Always
uint8_t MC6809::BRA()
{}

// Long Branch Always
uint8_t MC6809::LBRA()
{}

// Branch Never (another NOP)
uint8_t MC6809::BRN()
{}

// Long Branch Never (another NOP)
uint8_t MC6809::LBRN()
{}

// Branch to Subroutine
uint8_t MC6809::BSR()
{}

// Long Branch to Subroutine
uint8_t MC6809::LBSR()
{}

// Branch if no overflow (V is clear)
uint8_t MC6809::BVC()
{}

// Long Branch if no overflow (V is clear)
uint8_t MC6809::LBVC()
{}

// Branch on overflow (V is set)
uint8_t MC6809::BVS()
{}

// Long Branch on overflow (V is set)
uint8_t MC6809::LBVS()
{}

// Clear register A
uint8_t MC6809::CLRA()
{}

// Clear register B
uint8_t MC6809::CLRB()
{}

// Clear memory location
uint8_t MC6809::CLR()
{}

// Compare register A to memory location or given value(CC H unaffected)
uint8_t MC6809::CMPA()
{}

// Compare register B to memory location or given value (CC H unaffected)
uint8_t MC6809::CMPB()
{}

// Compare register D ( A <<8 | B) to memory locations or given value (CC H unaffected)
uint8_t MC6809::CMPD()
{}

// Compare register S to memory locations or given value (CC H unaffected)
uint8_t MC6809::CMPS()
{}

// Compare register U to memory locations or given value (CC H unaffected)
uint8_t MC6809::CMPU()
{}

// Compare register X to memory locations or given value (CC H unaffected)
uint8_t MC6809::CMPX()
{}

// Compare register Y to memory locations or given value (CC H unaffected)
uint8_t MC6809::CMPY()
{}

// 1's Compliment A (i.e. XOR A with 0x00 or 0xFF)
uint8_t MC6809::COMA()
{}

// 1's Compliment B (i.e. XOR B with 0x00 or 0xFF) 
uint8_t MC6809::COMB()
{}

// 1's Compliment Memory location (i.e. XOR A with 0x00 or 0xFF)
uint8_t MC6809::COM()
{}

// Wait for Interrupt
uint8_t MC6809::CWAI()
{}

// Decimal Adjust A (contents of A -> BCD... BCD operation should be prior)
uint8_t MC6809::DAA()
{}

// Decrement A (A -= A      A = A - 1   --A     A--)
uint8_t MC6809::DECA()
{}

// Decrement B (B -= B      B = B - 1   --B     B--)
uint8_t MC6809::DECB()
{}

// Decrement Memory location
uint8_t MC6809::DEC()
{}

// Logical Exclusive OR register A
uint8_t MC6809::EORA()
{}

// Logical Exclusive OR register B
uint8_t MC6809::EORB()
{}

// Exchange any two registers of the same size
uint8_t MC6809::EXG()
{}

// Increment A (A += A      A = A + 1   ++A     A++)
uint8_t MC6809::INCA()
{}

// Increment B (B += B      B = B + 1   ++B     B++)
uint8_t MC6809::INCB()
{}

// Increment Memory location
uint8_t MC6809::INC()
{}

// Unconditional Jump (non-relative) to a given (direct or indirect) address
uint8_t MC6809::JMP()
{}

// Jump to a subroutine (non-relative) at a given (direct or indirect) address
uint8_t MC6809::JSR()
{}

// Load Register A
uint8_t MC6809::LDA()
{}

// Load Register A
uint8_t MC6809::LDB()
{}

// Load Register D  ( A << 8 | B)
uint8_t MC6809::LDD()
{}

// Load Register S
uint8_t MC6809::LDS()
{}

// Load Register U
uint8_t MC6809::LDU()
{}

// Load Register X
uint8_t MC6809::LDX()
{}

// Load Register Y
uint8_t MC6809::LDY()
{}

// Load Effective Address S (increment or decrement S by a given value. Use  an index register to load another)
uint8_t MC6809::LEAS()
{}

// Load Effective Address U (increment or decrement U by a given value. Use  an index register to load another)
uint8_t MC6809::LEAU()
{}

// Load Effective Address X (increment or decrement X by a given value. Use  an index register to load another)
uint8_t MC6809::LEAX()
{}

// Load Effective Address Y (increment or decrement Y by a given value. Use  an index register to load another)
uint8_t MC6809::LEAY()
{}

// Logical Shift Left register A (LSb is loaded with 0)
uint8_t MC6809::LSLA()
{}

// Logical Shift Left register B (LSb is loaded with 0)
uint8_t MC6809::LSLB()
{}

// Logical Shift Left memory location (LSb is loaded with 0)
uint8_t MC6809::LSL()
{}

// Logical Shift Right register A (MSb is loaded with 0)
uint8_t MC6809::LSRA()
{}

// Logical Shift Right register B (MSb is loaded with 0)
uint8_t MC6809::LSRB()
{}

// Logical Shift Right memory location (MSb is loaded with 0)
uint8_t MC6809::LSR()
{}

// Multiply register A * register B, store in register D (A << 8 | B)
uint8_t MC6809::MUL()
{}

// 2's Complement (negate) register A
uint8_t MC6809::NEGA()
{}

// 2's Complement (negate) register B
uint8_t MC6809::NEGB()
{}

// 2's Complement (negate) memory location
uint8_t MC6809::NEG()
{}

// No Operation, Does nothing, affects PC only.
uint8_t MC6809::NOP()
{}

// Logical OR register A
uint8_t MC6809::ORA()
{}

// Logical OR register B
uint8_t MC6809::ORB()
{}

// Logical OR Condition Codes (set one or more flags)
uint8_t MC6809::ORCC()
{}

// Push one or more registers onto the System Stack
uint8_t MC6809::PSHS()
{}

// Push one or more registers onto the User Stack
uint8_t MC6809::PSHU()
{}

// Pull one or more registers from the System Stack
uint8_t MC6809::PULS()
{}

// Pull one or more registers from the User Stack
uint8_t MC6809::PULU()
{}

// Rotate Register A Left one bit through the Carry flag (9 bit rotate)
uint8_t MC6809::ROLA()
{}

// Rotate Register B Left one bit through the Carry flag (9 bit rotate)
uint8_t MC6809::ROLB()
{}

// Rotate memory location Left one bit through the Carry flag (9 bit rotate)
uint8_t MC6809::ROL()
{}

// Rotate Register A Right one bit through the Carry flag (9 bit rotate)
uint8_t MC6809::RORA()
{}

// Rotate Register B Right one bit through the Carry flag (9 bit rotate)
uint8_t MC6809::RORB()
{}

// Rotate memory location Right one bit through the Carry flag (9 bit rotate)
uint8_t MC6809::ROR()
{}

// Return from Interrupt. Pulls return address from S
uint8_t MC6809::RTI()
{}

// Return from Subroutine. Pulls return address from S
uint8_t MC6809::RTS()
{}

// Subtract with carry (borrow) - register A
uint8_t MC6809::SBCA()
{}

// Subtract with carry (borrow) - register B
uint8_t MC6809::SBCB()
{}

// Sign Extend B into A ( A = Sign bit set on B ? 0xFF : 0x00)
uint8_t MC6809::SEX()
{}

// Store Register A
uint8_t MC6809::STA()
{}

// Store Register B 
uint8_t MC6809::STB()
{}

// Store Register D     (A << 8 | B)
uint8_t MC6809::STD()
{}

// Store Register S
uint8_t MC6809::STS()
{}

// Store Register U
uint8_t MC6809::STU()
{}

// Store Register X
uint8_t MC6809::STX()
{}

// Store Register Y
uint8_t MC6809::STY()
{}

// Subtract from register A
uint8_t MC6809::SUBA()
{}

// Subtract from register A
uint8_t MC6809::SUBB()
{}

// Subtract from register D     (A << 8 | B)
uint8_t MC6809::SUBD()
{}

// trigger Software Interrupt [1]
uint8_t MC6809::SWI()
{}

// trigger Software Interrupt 2
uint8_t MC6809::SWI2()
{}

// trigger Software Interrupt 3
uint8_t MC6809::SWI3()
{}

// Synchronize to interrupt
uint8_t MC6809::SYNC()
{}

// Transfer/Copy one register to another (of the same size)
uint8_t MC6809::TFR()
{}

// Test Register A, adjust N and Z Condition codes based on content
uint8_t MC6809::TSTA()
{}

// Test Register B, adjust N and Z Condition codes based on content
uint8_t MC6809::TSTB()
{}

// Test memory location, adjust N and Z Condition codes based on content
uint8_t MC6809::TST()
{}

// INVALID INSTRUCTION! on 6309, this would trigger the invalid operation exception vector
uint8_t MC6809::XXX()
{}
