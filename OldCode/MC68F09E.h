#pragma once
#include <cstdint>
#include "MMU.h"

class MC6809
{
public:
    enum ConditionCodes
    {
        C = (1 << 0),   // Carry Overflow
        V = (1 << 1),   // Overflow
        Z = (1 << 2),   // Zero
        N = (1 << 3),   // Nagative
        I = (1 << 4),   // /IRQ mask
        H = (1 << 5),   // Half-Carry
        F = (1 << 6),   // /FIRQ mask
        E = (1 << 7),   // Entire flag  (if set, ALL registers are stored on stack, if clear only the PC and CC registers are stored on the stack)
    };
    uint8_t reg_A;      // accumulator A    (Accumulator D: reg_A << 8 | reg_B)
    uint8_t reg_B;      // accumulator B
    uint8_t reg_DP;     // Direct Page Register (index - upper 8 bits of 16-bit address bus)
    uint8_t reg_CC;     // Condition Codes
    uint16_t reg_X;     // index register X
    uint16_t reg_Y;     // inted register Y
    uint16_t reg_U;     // User stack pointer
    uint16_t reg_S;     // System stack pointer
    uint16_t reg_PC;    // Program counter

    MMU *mmu;
    uint8_t cycleCount;
    uint16_t instruction;
    uint8_t byteRead;
    bool BA;
    bool BS;
public:
    MC6809(MMU *pMMU);
    ~MC6809();

    // Interrupts
    uint8_t Reset(bool initCall = false);
    uint8_t NMI();
    uint8_t IRQ();
    uint8_t FIRQ();

    // other functions
    uint8_t Fetch();

    uint8_t Halt();
private:
    // address modes
    uint8_t ADDR_Direct();
    uint8_t ADDR_Extended();
    uint8_t ADDR_Immediate();
    uint8_t ADDR_Indexed();
    uint8_t ADDR_Inherent();
    uint8_t ADDR_Relative();

    uint8_t ADDR_Extended_Indirect();                           // 05
    uint8_t ADDR_Indexed_AccumulatorOffset();                   // 11
    uint8_t ADDR_Indexed_AutoIncrementDecrement();              // 12
    uint8_t ADDR_Indexed_ConstantOffset();                      // 10
    uint8_t ADDR_Indexed_Indirect();                            // 13
    uint8_t ADDR_Indexed_ZeroOffset();                          // 09
    uint8_t ADDR_Register();                                    // 07
    uint8_t ADDR_Relative_ShortLong();                          // 15
    uint8_t ADDR_Relative_PrgrmCounterRelativeAddressing();     // 16

                                                                // instructions
    uint8_t ABX();          // Add B to X
    uint8_t ADCA();         // Add to A + Carry
    uint8_t ADCB();         // Add to A + Carry
    uint8_t ADDA();         // Add to A
    uint8_t ADDB();         // Add to B
    uint8_t ADDD();         // Add to D (A << 8 | B)
    uint8_t ANDA();         // And A
    uint8_t ANDB();         // And B
    uint8_t ANDCC();        // And Condition Codes (clear one or more flags)
    uint8_t ASLA();         // Arithmetic Shift Left A (Logical Shift Left fill LSb with 0)
    uint8_t ASLB();         // Arithmetic Shift Left B (Logical Shift Left fill LSb with 0)
    uint8_t ASL();          // Arithmetic Shift Left Memory location (Logical Shift Left fill LSb with 0)
    uint8_t ASRA();         // Arithmetic Shift Right A (fill MSb with Sign bit)
    uint8_t ASRB();         // Arithmetic Shift Right B (fill MSb with Sign bit)
    uint8_t ASR();          // Arithmetic Shift Right Memory location (fill MSb with Sign bit)
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
    uint8_t BITA();         // Bit Test on A with a specific value (by AND)
    uint8_t BITB();         // Bit Test on B with a specific value (by AND)
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
    uint8_t BRA();          // Branch Always
    uint8_t LBRA();         // Long Branch Always
    uint8_t BRN();          // Branch Never (another NOP)
    uint8_t LBRN();         // Long Branch Never (another NOP)
    uint8_t BSR();          // Branch to Subroutine
    uint8_t LBSR();         // Long Branch to Subroutine
    uint8_t BVC();          // Branch if no overflow (V is clear)
    uint8_t LBVC();         // Long Branch if no overflow (V is clear)
    uint8_t BVS();          // Branch on overflow (V is set)
    uint8_t LBVS();         // Long Branch on overflow (V is set)
    uint8_t CLRA();         // Clear register A
    uint8_t CLRB();         // Clear register B
    uint8_t CLR();          // Clear memory location
    uint8_t CMPA();         // Compare register A to memory location or given value(CC H unaffected)
    uint8_t CMPB();         // Compare register B to memory location or given value (CC H unaffected)
    uint8_t CMPD();         // Compare register D ( A <<8 | B) to memory locations or given value (CC H unaffected)
    uint8_t CMPS();         // Compare register S to memory locations or given value (CC H unaffected)
    uint8_t CMPU();         // Compare register U to memory locations or given value (CC H unaffected)
    uint8_t CMPX();         // Compare register X to memory locations or given value (CC H unaffected)
    uint8_t CMPY();         // Compare register Y to memory locations or given value (CC H unaffected)
    uint8_t COMA();         // 1's Compliment A (i.e. XOR A with 0x00 or 0xFF)
    uint8_t COMB();         // 1's Compliment B (i.e. XOR B with 0x00 or 0xFF) 
    uint8_t COM();          // 1's Compliment Memory location (i.e. XOR A with 0x00 or 0xFF)
    uint8_t CWAI();         // Wait for Interrupt
    uint8_t DAA();          // Decimal Adjust A (contents of A -> BCD... BCD operation should be prior)
    uint8_t DECA();         // Decrement A (A -= A      A = A - 1   --A     A--)
    uint8_t DECB();         // Decrement B (B -= B      B = B - 1   --B     B--) 
    uint8_t DEC();          // Decrement Memory location
    uint8_t EORA();         // Logical Exclusive OR register A
    uint8_t EORB();         // Logical Exclusive OR register B
    uint8_t EXG();          // Exchange any two registers of the same size
    uint8_t INCA();         // Increment A (A += A      A = A + 1   ++A     A++)
    uint8_t INCB();         // Increment B (B += B      B = B + 1   ++B     B++)
    uint8_t INC();          // Increment Memory location
    uint8_t JMP();          // Unconditional Jump (non-relative) to a given (direct or indirect) address
    uint8_t JSR();          // Jump to a subroutine (non-relative) at a given (direct or indirect) address
    uint8_t LDA();          // Load Register A
    uint8_t LDB();          // Load Register A
    uint8_t LDD();          // Load Register D  ( A << 8 | B)
    uint8_t LDS();          // Load Register S
    uint8_t LDU();          // Load Register U
    uint8_t LDX();          // Load Register X
    uint8_t LDY();          // Load Register Y
    uint8_t LEAS();         // Load Effective Address S (increment or decrement S by a given value. Use  an index register to load another)
    uint8_t LEAU();         // Load Effective Address U (increment or decrement U by a given value. Use  an index register to load another)
    uint8_t LEAX();         // Load Effective Address X (increment or decrement X by a given value. Use  an index register to load another)
    uint8_t LEAY();         // Load Effective Address Y (increment or decrement Y by a given value. Use  an index register to load another)
    uint8_t LSLA();         // Logical Shift Left register A (LSb is loaded with 0)
    uint8_t LSLB();         // Logical Shift Left register B (LSb is loaded with 0)
    uint8_t LSL();          // Logical Shift Left memory location (LSb is loaded with 0)
    uint8_t LSRA();         // Logical Shift Right register A (MSb is loaded with 0)
    uint8_t LSRB();         // Logical Shift Right register B (MSb is loaded with 0)
    uint8_t LSR();          // Logical Shift Right memory location (MSb is loaded with 0)
    uint8_t MUL();          // Multiply register A * register B, store in register D (A << 8 | B)
    uint8_t NEGA();         // 2's Complement (negate) register A
    uint8_t NEGB();         // 2's Complement (negate) register B
    uint8_t NEG();          // 2's Complement (negate) memory location
    uint8_t NOP();          // No Operation, Does nothing, affects PC only.
    uint8_t ORA();          // Logical OR register A
    uint8_t ORB();          // Logical OR register B
    uint8_t ORCC();         // Logical OR Condition Codes (set one or more flags)
    uint8_t PSHS();         // Push one or more registers onto the System Stack
    uint8_t PSHU();         // Push one or more registers onto the User Stack
    uint8_t PULS();         // Pull one or more registers from the System Stack
    uint8_t PULU();         // Pull one or more registers from the User Stack
    uint8_t ROLA();         // Rotate Register A Left one bit through the Carry flag (9 bit rotate)
    uint8_t ROLB();         // Rotate Register B Left one bit through the Carry flag (9 bit rotate)
    uint8_t ROL();          // Rotate memory location Left one bit through the Carry flag (9 bit rotate)
    uint8_t RORA();         // Rotate Register A Right one bit through the Carry flag (9 bit rotate)
    uint8_t RORB();         // Rotate Register B Right one bit through the Carry flag (9 bit rotate)
    uint8_t ROR();          // Rotate memory location Right one bit through the Carry flag (9 bit rotate)
    uint8_t RTI();          // Return from Interrupt. Pulls return address from S
    uint8_t RTS();          // Return from Subroutine. Pulls return address from S
    uint8_t SBCA();         // Subtract with carry (borrow) - register A
    uint8_t SBCB();         // Subtract with carry (borrow) - register B
    uint8_t SEX();          // Sign Extend B into A ( A = Sign bit set on B ? 0xFF : 0x00)
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
    uint8_t SWI();          // trigger Software Interrupt [1]
    uint8_t SWI2();         // trigger Software Interrupt 2
    uint8_t SWI3();         // trigger Software Interrupt 3
    uint8_t SYNC();         // Synchronize to interrupt
    uint8_t TFR();          // Transfer/Copy one register to another (of the same size)
    uint8_t TSTA();         // Test Register A, adjust N and Z Condition codes based on content
    uint8_t TSTB();         // Test Register B, adjust N and Z Condition codes based on content
    uint8_t TST();          // Test memory location, adjust N and Z Condition codes based on content

    uint8_t XXX();          // INVALID INSTRUCTION! on 6309, this would trigger the invalid operation exception vector
};

