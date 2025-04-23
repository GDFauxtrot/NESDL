#pragma once

// Responsible for handling all memory-related tasks, for both the CPU (RAM) and PPU (VRAM)
class NESDL_RAM
{
public:
    void Init(NESDL_Core* c);
	uint8_t ReadByte(uint16_t addr);
    uint16_t ReadWord(uint16_t addr);
	void WriteByte(uint16_t addr, uint8_t data);
    void SetMapper(NESDL_Mapper* m);
    
    bool oops;
private:
    NESDL_Core* core;
    NESDL_Mapper* mapper;
	uint8_t ram[0x800]; // 2KB internal RAM (the rest of address space is mirrored/rerouted)
};
