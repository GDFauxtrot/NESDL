#include "NESDL.h"

void NESDL_RAM::Init()
{

}

uint16_t GetMirroredAddress(uint16_t addr)
{
    // If within the range 0x0000-0x1FFF, mod the address to 0x0000-8000
    // (2KB internal RAM mirroring)
    if (addr > 0x0 && addr < 0x2000)
    {
        addr = addr % 0x800;
    }
    
    // If within the range 0x2000-0x3FFF, mod the address to 0x2000-2007
    // (PPU register bytes mirroring)
    else if (addr >= 0x2000 && addr < 0x4000)
    {
        addr = (addr % 8) + 0x2000;
    }
    
    return addr;
}

uint8_t NESDL_RAM::ReadByte(uint16_t addr)
{
    addr = GetMirroredAddress(addr);
	return ram[addr];
}

uint16_t NESDL_RAM::ReadWord(uint16_t addr)
{
    addr = GetMirroredAddress(addr);
    
    // Little-endian - swap bytes around
    uint16_t lower = (uint16_t)ram[addr];
    // Check if we crossed page bounds while assembing this word
    oops = (addr / 0x100) == ((addr+1) / 0x100);
    uint16_t upper = (uint16_t)ram[addr+1] << 8;
    uint16_t result = lower + upper;
    return result;
}

void NESDL_RAM::ReadBytes(uint16_t src, uint16_t dest, size_t size)
{
	memcpy(ram + dest, ram + src, size);
}

void NESDL_RAM::WriteByte(uint16_t addr, uint8_t data)
{
	addr = GetMirroredAddress(addr);
	ram[addr] = data;
}

void NESDL_RAM::WriteBytes(uint16_t addr, uint8_t* data, size_t size)
{
	memcpy(ram + addr, data, size);
}

void NESDL_RAM::WriteROMData(uint8_t* addr, uint8_t bankCount)
{
	// TODO assume NROM, implement bank switching and all that fun mapper stuff later!
	WriteBytes(0x8000, addr, bankCount * 0x4000);
    
    if (bankCount == 1)
    {
        // With NROM, 0xC000-FFFF are mirrors of 0x8000-BFFF
        WriteBytes(0xC000, addr, bankCount * 0x4000);
    }
}

void NESDL_RAM::WriteVROMData(uint8_t* addr, uint8_t bankCount)
{
	// TODO assume NROM, implement bank switching and all that fun mapper stuff later!
    WriteBytes(0x6000, addr, bankCount * 0x2000);
}

