#include <string>
#include <vector>
#include "CPU.h"
#include "MMU.h"

class Cpu6809 //: public CPU
{
protected:

	uint8_t JSR();			// Jump to Subrourine												***	To-Do, Needs Work
	uint8_t JMP();			// Jump to memory location											***	To-Do, Needs Work

	uint8_t LEAS();         // Load Effective Address S											***	To-Do, Needs Work
	uint8_t LEAU();         // Load Effective Address U											***	To-Do, Needs Work
	uint8_t LEAX();         // Load Effective Address X											***	To-Do, Needs Work
	uint8_t LEAY();         // Load Effective Address Y											***	To-Do, Needs Work

	uint8_t BITA();         // Bit Test on A with a specific value (by AND)						***	To-Do, Needs Work
	uint8_t BITB();         // Bit Test on B with a specific value (by AND)						***	To-Do, Needs Work

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
	uint8_t LSR();          // Logical Shift Right memory location (MSb is loaded with 0)		***	To-Do, Needs Work
	uint8_t NEG();          // 2's Complement (negate) memory location							***	To-Do, Needs Work
	uint8_t ORA();          // Logical OR register A											***	To-Do, Needs Work
	uint8_t ORB();          // Logical OR register B											***	To-Do, Needs Work
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
};

