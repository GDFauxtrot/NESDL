#include "NESDL.h"

void NESDL_Mapper_1::InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks)
{
    prgBanks = prgROMBanks;
    chrBanks = chrROMBanks;
    
    // Init PRG-ROM data
    if (prgBanks > 0)
    {
        prgROM = new uint8_t[prgBanks * 0x4000];
        memcpy(prgROM, prgROMData, prgBanks * 0x4000);
    }
    
    // Init CHR-ROM Data
    if (chrBanks > 0)
    {
        chrROM = new uint8_t[chrBanks * 0x2000];
        memcpy(chrROM, chrROMData, chrBanks * 0x2000);
    }
    else
    {
        // Assume 8KB of CHR-RAM
        chrROM = new uint8_t[0x2000];
        memset(chrROM, 0x00, 0x2000);
    }
    
    // Assume we start in PRG-ROM bank mode 3 (confirmed?) and 8KB CHR mode
    prgROM0Index = 0;
    prgROM1Index = (prgBanks-1);
    chrROM0Index = 0;
    chrROM1Index = 1;
    
    prgRAMEnable = true;
}

uint8_t NESDL_Mapper_1::ReadByte(uint16_t addr)
{
    if (addr < 0x1000)
    {
        return chrROM[chrROM0Index * 0x1000 + addr];
    }
    else if (addr < 0x2000)
    {
        return chrROM[chrROM1Index * 0x1000 + (addr - 0x1000)];
    }
    
    if (addr >= 0x6000 && addr < 0x8000)
    {
        if (prgRAMEnable)
        {
            return prgRAM[addr - 0x6000];
        }
    }
    else if (addr >= 0x8000 && addr < 0xC000)
    {
        return prgROM[prgROM0Index * 0x4000 + (addr - 0x8000)];
    }
    else if (addr >= 0xC000)
    {
        return prgROM[prgROM1Index * 0x4000 + (addr - 0xC000)];
    }
    
    return 0;
}

void NESDL_Mapper_1::WriteByte(uint16_t addr, uint8_t data)
{
    // If we have no CHR-ROM banks, assume CHR-RAM is on-board and make
    // this address space writable
    if (chrBanks == 0)
    {
        if (addr < 0x1000)
        {
            chrROM[chrROM0Index * 0x1000 + addr] = data;
        }
        else if (addr < 0x2000)
        {
            chrROM[chrROM1Index * 0x1000 + (addr - 0x1000)] = data;
        }
    }
    
    if (addr >= 0x6000 && addr < 0x8000)
    {
        if (prgRAMEnable)
        {
            prgRAM[addr - 0x6000] = data;
        }
    }
    else if (addr >= 0x8000)
    {
        core->cpu->DidMapperWrite();
        
        // Clear latch if bit 7 is ever written to
        if ((data & 0x80) == 0x80)
        {
            shiftRegister = 0;
            shiftIndex = 0;
            return;
        }
        
        // Ignore successive writes
        // https://www.nesdev.org/wiki/MMC1#Consecutive-cycle_writes
        if (core->cpu->IsConsecutiveMapperWrite())
        {
//            return;
        }
        
        // It takes 5 consecutive writes to fill the shift register
        // and send it to where it needs to go
        shiftRegister |= (data & 0x1) << shiftIndex++;
        if (shiftIndex == 5)
        {
            // We have 5 bits loaded to send this info to
            if (addr < 0xA000)
            {
                // Send to "control"
                WriteControl();
            }
            else if (addr < 0xC000)
            {
                // Send to CHR bank 0
                WriteCHRBank(0);
            }
            else if (addr < 0xE000)
            {
                // Send to CHR bank 1
                WriteCHRBank(1);
            }
            else
            {
                // Send to PRG bank
                WritePRGBank();
            }
            
            shiftRegister = 0;
            shiftIndex = 0;
        }
    }
}

void NESDL_Mapper_1::WriteControl()
{
    // First 2 bits of shift register are NT arrangement
    uint8_t nt = shiftRegister & 0x3;
    switch (nt)
    {
        case 0:
            mirroringMode = MirroringMode::One_LowerBank;
            break;
        case 1:
            mirroringMode = MirroringMode::One_UpperBank;
            break;
        case 2:
            mirroringMode = MirroringMode::Vertical;
            break;
        case 3:
            mirroringMode = MirroringMode::Horizontal;
            break;
    }
    
    // Next 2 bits are PRG-ROM bank mode
    prgROMMode = (shiftRegister & 0x0C) >> 2;
    // Last bit is CHR-ROM bank mode
    chrROMMode = (shiftRegister & 0x10);
}

void NESDL_Mapper_1::WriteCHRBank(uint8_t index)
{
    // We don't actually "write" CHR banks.
    // Instead, we have two indices we use as address bases into
    // the entire CHR-ROM. I prefer this, as we avoid time spent
    // memcpy'ing the data every time we bank switch
    if (!chrROMMode)
    {
        // 8KB mode - CHR addr 0x1000 proceeds data from 0x0000
        uint8_t value = shiftRegister & 0x1E; // Low bit ignored (CHR-ROM can't be 0x1000-0x2000, for instance - start address must be even)
        chrROM0Index = value;
        chrROM1Index = value + 1;
    }
    else
    {
        // 4KB mode - each bank gets treated separately
        uint8_t value = shiftRegister & 0x1F;
        if (index == 0)
        {
            chrROM0Index = value;
        }
        else if (chrROMMode)
        {
            chrROM1Index = value;
        }
    }
}

void NESDL_Mapper_1::WritePRGBank()
{
    // Same deal here - we're not "writing" PRG banks
    switch (prgROMMode)
    {
        case 0: // Full 32KB
        case 1:
            {
                uint8_t value = shiftRegister & 0x0E; // Low bit ignored
                prgROM0Index = value;
                prgROM1Index = value + 1;
            }
            break;
        case 2: // First 16KB is bank 0, second 16KB is switched
            {
                uint8_t value = shiftRegister & 0x0F;
                prgROM0Index = 0;
                prgROM1Index = value;
            }
            break;
        case 3: // First 16KB is switched, second 16KB is last bank
            {
                uint8_t value = shiftRegister & 0x0F;
                prgROM0Index = value;
                prgROM1Index = (prgBanks - 1);
            }
            break;
    }
}

