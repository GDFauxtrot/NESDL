#include "NESDL.h"

void NESDL_RAM::Init()
{

}

uint8_t NESDL_RAM::ReadByte(uint16_t addr)
{
	// Mod address to 0x0000-0x0800 range if within the range 0x0000-0x2000 (address mirroring)
	//uint16_t realAddr = (addr < 0x2000 ? addr = addr % 0x800 : addr);

	return ram[addr];
}

void NESDL_RAM::ReadBytes(uint16_t src, uint16_t dest, size_t size)
{
	memcpy_s(ram + dest, size, ram + src, size);
}

void NESDL_RAM::WriteByte(uint16_t addr, uint8_t data)
{
	// Mod address to 0x0000-0x0800 range if within the range 0x0000-0x2000 (address mirroring)
	//uint16_t realAddr = (addr < 0x2000 ? addr = addr % 0x800 : addr);

	ram[addr] = data;
}

void NESDL_RAM::WriteBytes(uint16_t addr, uint8_t* data, size_t size)
{
	memcpy_s(ram + addr, size, data, size);
}

void NESDL_RAM::WriteROMData(uint8_t* addr, uint8_t bankCount)
{
	// TODO assume NROM, implement bank switching and all that fun mapper stuff later!
	WriteBytes(0x8000, addr, bankCount * 0x4000);
}

void NESDL_RAM::WriteVROMData(uint8_t* addr, uint8_t bankCount)
{
	// TODO assume NROM, implement bank switching and all that fun mapper stuff later!
}

