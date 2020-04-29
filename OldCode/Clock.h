#pragma once
#include <cstdint>
#include <chrono>
#include <time.h>
class Clock
{
private:

public:
	static bool SetClockSpeed(const uint32_t frequency);
	static bool Tick();
};

