
#include "Bus.h"
#include "MMU.h"

Bus::Bus()
{
	//mmu.SetRamSize();

}

Bus::~Bus()
{}


// Read - Write functionality
void Write(uint16_t address, uint8_t data)
{
	if (address >= 00 && address <= 0xFFFF)
		mmu.Write(address, data);
}


uint8_t Read(uint16_t address, bool readOnly = false)
{
	if (address >= 00 && address <= 0xFFFF)
		return (mmu.Read(address, readOnly));
	else
		return(0);
}
