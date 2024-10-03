#pragma once

// Responsible for handling all memory-related tasks, for both the CPU (RAM) and PPU (VRAM)
class NESDL_RAM
{
public:
    void Init(NESDL_Core* c);
	uint8_t ReadByte(uint16_t addr);
    uint16_t ReadWord(uint16_t addr);
	void WriteByte(uint16_t addr, uint8_t data);
	void WriteBytes(uint16_t addr, uint8_t* data, size_t size);
	void WriteROMData(uint8_t* data, uint8_t bankCount);
	void WriteVROMData(uint8_t* data, uint8_t bankCount);
    bool oops;
    bool ignoreChanges;
private:
    NESDL_Core* core;
	uint8_t ram[0x10000]; // 64KB address space
};
