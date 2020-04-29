#pragma once
#include <cstdint>
#include "MC68F09E.h"
#include "MMU.h"

class Bus
{
private:
public:
	Bus();
	~Bus();

	// devices on the bus
	MC6809 cpu;
	MMU	mmu;

	// Read - Write functionality
	void Write(uint16_t address, uint8_t data);
	uint8_t Read(uint16_t address, bool readOnly = false);
};