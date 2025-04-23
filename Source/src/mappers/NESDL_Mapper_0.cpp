#include "NESDL.h"

void NESDL_Mapper_0::InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks)
{
    prgBanks = prgROMBanks;
    chrBanks = chrROMBanks;
    
    // Init PRG-ROM data
    if (prgBanks > 0)
    {
        prgROM = new uint8_t[prgBanks * 0x4000];
        memcpy(prgROM, prgROMData, prgBanks * 0x4000);
    }
    
    // Init CHR-ROM data
    if (chrBanks > 0)
    {
        chrROM = new uint8_t[chrBanks * 0x2000];
        memcpy(chrROM, chrROMData, chrBanks * 0x2000);
    }
    else
    {
        // Assume 8KB of CHR-RAM(?)
        chrROM = new uint8_t[0x2000];
        memset(chrROM, 0x00, 0x2000);
    }
}

void NESDL_Mapper_0::SetMirroringData(bool data)
{
    // We use the bit from the header to determine mirroring
    mirroringMode = data ? MirroringMode::Vertical : MirroringMode::Horizontal;
}

uint8_t NESDL_Mapper_0::ReadByte(uint16_t addr)
{
    // 0x0000 - 0x1FFFF - CHR-ROM data (accessed from PPU)
    if (addr < 0x2000)
    {
        return chrROM[addr];
    }
    
    // 0x6000-0x7FFF is currently unimplemented (Family BASIC only)
    
    // Mirror down address if we only have one PRG bank
    if (addr >= 0xC000 && prgBanks == 1)
    {
        addr -= 0x4000;
    }
    if (addr >= 0x8000)
    {
        return prgROM[addr - 0x8000];
    }
    return 0;
}
