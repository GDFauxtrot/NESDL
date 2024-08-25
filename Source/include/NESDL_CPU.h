#pragma once

struct CPURegisters
{
	uint16_t pc; // Program Counter
	uint8_t sp; // Stack Pointer
	uint8_t a; // Accumulator
	uint8_t x; // Index X
	uint8_t y; // Index Y
	uint8_t p; // Processor (status)
};

// Bit masks for accessing the CPU processor status
#define PSTATUS_CARRY 0x1
#define PSTATUS_ZERO 0x2
#define PSTATUS_INTERRUPTDISABLE 0x4
#define PSTATUS_DECIMAL 0x8
#define PSTATUS_BREAK 0x10
#define PSTATUS_UNDEFINED 0x20
#define PSTATUS_OVERFLOW 0x40
#define PSTATUS_NEGATIVE 0x80


class NESDL_CPU
{
public:
	void Init();
private:
	CPURegisters registers;
};