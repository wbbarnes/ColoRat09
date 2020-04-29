#pragma once
#include <vector>
#include <cstdint>


class MMU
{
private:
	static const uint32_t MAX_RAM;
	static const uint16_t RAM_BLOCK_SIZE;
	std::vector<uint8_t> ram;

public:
	MMU(uint32_t size = (64 * 1024));
	~MMU();
	void Init(uint32_t size = (64 * 1024));
	bool SetRamSize(uint32_t size = (64 * 1024));

	void Write(uint16_t address, uint8_t data);
	uint8_t Read(uint16_t address, bool readOnly = false);

};

