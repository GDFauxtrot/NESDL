#include "NESDL.h"

void NESDL_CPU::Init()
{
	elapsedTime = 0;
}

void NESDL_CPU::Update(double newSystemTime)
{
	// Take the new system time (milliseconds since boot) and compare to our own tracked time
	// If the new system time is larger than the CPU's tracked time, then we're due for an instruction
	// Else, wait out this update - we overshot the master clock with our last instruction so we must wait
	uint64_t newCycles = MillisecondsToCPUCycles(newSystemTime);
	uint64_t elapsedCycles = MillisecondsToCPUCycles(elapsedTime);

	// Run instructions to catch up with time
	while (newCycles > elapsedCycles)
	{
		RunNextInstruction();
		elapsedCycles = MillisecondsToCPUCycles(elapsedTime);
	}
}

void NESDL_CPU::RunNextInstruction()
{
	// Get next instruction from memory according to current program counter
	uint8_t opcode = core->ram->ReadByte(registers.pc);

}


uint64_t MillisecondsToCPUCycles(double ms)
{
	// Convert the given amount of time in milliseconds to the amount of CPU cycles it represents
	return 0;
}

double CPUCyclesToMilliseconds(uint64_t c)
{
	// Convert the given amount of CPU cycles to the amount of time in milliseconds it represents
	return 0;
}