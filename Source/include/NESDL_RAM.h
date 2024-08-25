#pragma once

// Responsible for handling all memory-related tasks, for both the CPU (RAM) and PPU (VRAM)
class NESDL_RAM
{
public:
	void Init();
	uint8_t ReadByte(uint16_t addr);
	void ReadBytes(uint16_t src, uint16_t dest, size_t size);
	void WriteByte(uint16_t addr, uint8_t data);
	void WriteBytes(uint16_t addr, uint8_t* data, size_t size);
	void WriteROMData(uint8_t* addr, uint8_t bankCount);
	void WriteVROMData(uint8_t* addr, uint8_t bankCount);
private:
	uint8_t ram[0x10000]; // A whopping 64kb of memory wow
};