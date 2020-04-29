#include "MMU.h"

const uint32_t MMU::MAX_RAM = (16 * 1024 * 1024);
const uint16_t MMU::RAM_BLOCK_SIZE = (4 * 1024);

MMU::MMU(uint32_t size)
{
	SetRamSize(size);
}

MMU::~MMU()
{}

bool MMU::SetRamSize(uint32_t size)
{
	ram.resize(size,0x00);

	return(true);
}

void MMU::Write(uint16_t address, uint8_t data)
{
	uint32_t physicalRam;

	ram.at(physicalRam) = data;
}

uint8_t MMU::Read(uint16_t address, bool readOnly)
{
	uint32_t physicalRam;

	return (ram.at(physicalRam));

}
