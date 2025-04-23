#include "NESDL.h"

void NESDL_Mapper_9::InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks)
{
    prgBanks = prgROMBanks * 2; // iNES is PRG-ROM in 16KB blocks, MMC2 works in 8KB
    chrBanks = chrROMBanks;
    
    // Init PRG-ROM data
    if (prgBanks > 0)
    {
        prgROM = new uint8_t[prgBanks * 0x2000];
        memcpy(prgROM, prgROMData, prgBanks * 0x2000);
    }
    
    // Init CHR-ROM data
    if (chrBanks > 0)
    {
        chrROM = new uint8_t[chrBanks * 0x2000];
        memcpy(chrROM, chrROMData, chrBanks * 0x2000);
    }
    
    // Vertical by default
    mirroringMode = MirroringMode::Vertical;
}

uint8_t NESDL_Mapper_9::ReadByte(uint16_t addr)
{
    // Reads from PPU work normally, but also set latches
    // Corresponding to the lower byte
    if (addr < 0x1000)
    {
        uint8_t data = 0x00;
        
        // Read data from latch
        if (chrROM0Latch == 0xFD || chrROM0Latch == 0xFE)
        {
            uint8_t index = (chrROM0Latch == 0xFD) ? chrROM0Index0 : chrROM0Index1;
            data = chrROM[index * 0x1000 + addr];
        }
        
        if (!core->cpu->ignoreChanges)
        {
            uint8_t tile = (addr >> 4);
            // Update latch (for latch 0 - ONLY when address ends in 0x8!)
            if ((addr & 0xF) == 0x8 && (tile == 0xFD || tile == 0xFE))
            {
                chrROM0Latch = tile;
            }
        }
        
        // Return data (before latch updated)
        return data;
    }
    else if (addr < 0x2000)
    {
        uint8_t data = 0x00;
        
        // Read data from latch
        if (chrROM1Latch == 0xFD || chrROM1Latch == 0xFE)
        {
            uint8_t index = (chrROM1Latch == 0xFD) ? chrROM1Index0 : chrROM1Index1;
            data = chrROM[index * 0x1000 + (addr - 0x1000)];
        }
        
        if (!core->cpu->ignoreChanges)
        {
            uint8_t tile = (addr >> 4);
            // Update latch (for latch 1 - ONLY when address ends in value >= 0x8!)
            if ((addr & 0x8) == 0x8 && (tile == 0xFD || tile == 0xFE))
            {
                chrROM1Latch = tile;
            }
        }
        
        // Return data (before latch updated)
        return data;
    }
    
    if (addr >= 0x6000 && addr < 0x8000)
    {
        // 8KB PRG-RAM
        // TODO is this enabled by default? Punch-Out!! doesn't seem to care
        return prgRAM[addr - 0x6000];
    }
    else if (addr >= 0x8000 && addr < 0xA000)
    {
        // 8KB switchable PRG-ROM
        return prgROM[prgROMIndex * 0x2000 + (addr - 0x8000)];
    }
    else if (addr >= 0xA000 && addr <= 0xFFFF)
    {
        // 3 8KB PRG-ROM banks (fixed to last 3 banks of ROM)
        uint8_t bankIndex = ((addr / 0x2000) - 5) + (prgBanks - 3);
        return prgROM[bankIndex * 0x2000 + (addr % 0x2000)];
    }
    
    return 0;
}

void NESDL_Mapper_9::WriteByte(uint16_t addr, uint8_t data)
{
    if (core->cpu->ignoreChanges)
    {
        return;
    }
    
    if (addr >= 0x6000 && addr < 0x8000)
    {
        // PRG-RAM
        prgRAM[addr - 0x6000] = data;
    }
    else if (addr >= 0x8000 && addr < 0xA000)
    {
        // PRG-ROM -- do nothing
    }
    else if (addr >= 0xA000 && addr < 0xB000)
    {
        // PRG-ROM bank select
        prgROMIndex = data & 0x0F;
    }
    else if (addr >= 0xB000 && addr < 0xC000)
    {
        // CHR-ROM bank 1 select (during FD latch)
        chrROM0Index0 = data & 0x1F;
    }
    else if (addr >= 0xC000 && addr < 0xD000)
    {
        // CHR-ROM bank 1 select (during FE latch)
        chrROM0Index1 = data & 0x1F;
    }
    else if (addr >= 0xD000 && addr < 0xE000)
    {
        // CHR-ROM bank 2 select (during FD latch)
        chrROM1Index0 = data & 0x1F;
    }
    else if (addr >= 0xE000 && addr < 0xF000)
    {
        // CHR-ROM bank 2 select (during FE latch)
        chrROM1Index1 = data & 0x1F;
    }
    else if (addr >= 0xF000 && addr <= 0xFFFF)
    {
        // Mirroring mode
        mirroringMode = (data & 0x1) == 0 ? MirroringMode::Vertical : MirroringMode::Horizontal;
    }
}
