#include "NESDL.h"

void NESDL_Mapper_4::InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks)
{
    prgBanks = prgROMBanks * 2; // iNES is PRG-ROM in 16KB blocks, MMC3 works in 8KB
	chrBanks = chrROMBanks * 8; // iNES is CHR-ROM in 8KB blocks, MMC3 works in 1KB

    // Init PRG-ROM data
    if (prgBanks > 0)
    {
        prgROM = new uint8_t[prgBanks * 0x2000];
        memcpy(prgROM, prgROMData, prgBanks * 0x2000);
    }

    // Init CHR-ROM data
    if (chrBanks > 0)
    {
        chrROM = new uint8_t[chrBanks * 0x400];
        memcpy(chrROM, chrROMData, chrBanks * 0x400);
    }
    else
    {
        // Assume 8KB of CHR-RAM
        chrROM = new uint8_t[0x2000];
        memset(chrROM, 0x00, 0x2000);
    }
}

uint8_t NESDL_Mapper_4::ReadByte(uint16_t addr)
{
    if (chrA12Inversion == false)
    {
        if (addr < 0x800)
        {
            return chrROM[(chrROM0Index * 0x400) + addr];
        }
        else if (addr < 0x1000)
        {
            return chrROM[(chrROM1Index * 0x400) + (addr - 0x800)];
        }
        else if (addr < 0x1400)
        {
            return chrROM[(chrROM2Index * 0x400) + (addr - 0x1000)];
        }
        else if (addr < 0x1800)
        {
            return chrROM[(chrROM3Index * 0x400) + (addr - 0x1400)];
        }
        else if (addr < 0x1C00)
        {
            return chrROM[(chrROM4Index * 0x400) + (addr - 0x1800)];
        }
        else if (addr < 0x2000)
        {
            return chrROM[(chrROM5Index * 0x400) + (addr - 0x1C00)];
        }
    }
    else
    {
        if (addr < 0x400)
        {
            return chrROM[(chrROM2Index * 0x400) + addr];
        }
        else if (addr < 0x800)
        {
            return chrROM[(chrROM3Index * 0x400) + (addr - 0x400)];
        }
        else if (addr < 0xC00)
        {
            return chrROM[(chrROM4Index * 0x400) + (addr - 0x800)];
        }
        else if (addr < 0x1000)
        {
            return chrROM[(chrROM5Index * 0x400) + (addr - 0xC00)];
        }
        else if (addr < 0x1800)
        {
            return chrROM[(chrROM0Index * 0x400) + (addr - 0x1000)];
        }
        else if (addr < 0x2000)
        {
            return chrROM[(chrROM1Index * 0x400) + (addr - 0x1800)];
        }
    }

    if (addr >= 0x6000 && addr < 0x8000)
    {
        return prgRAM[addr - 0x6000];
    }
    else if (addr >= 0x8000)
    {
        // CPU Bank 0x8000-0x9FFF returns PRG-ROM bank 0 or second-to-last bank
        if (addr < 0xA000)
        {
            if (prgROMBankMode == 0)
            {
                return prgROM[(prgROM0Index * 0x2000) + (addr - 0x8000)];
            }
            else
            {
                return prgROM[((prgBanks - 2) * 0x2000) + (addr - 0x8000)];
            }
        }
        // CPU Bank 0xA000-0xBFFF returns PRG-ROM bank 1
        else if (addr < 0xC000)
        {
            return prgROM[(prgROM1Index * 0x2000) + (addr - 0xA000)];
        }
        // CPU Bank 0xC000-0xDFFF returns second-to-last bank or PRG-ROM bank 0
        else if (addr < 0xE000)
        {
            if (prgROMBankMode == 0)
            {
                return prgROM[((prgBanks - 2) * 0x2000) + (addr - 0xC000)];
            }
            else
            {
                return prgROM[(prgROM0Index * 0x2000) + (addr - 0xC000)];
            }
        }
        // CPU Bank 0xE000-0xFFFF returns last bank
        else
        {
            return prgROM[((prgBanks - 1) * 0x2000) + (addr - 0xE000)];
        }
    }

    return 0;
}

void NESDL_Mapper_4::WriteByte(uint16_t addr, uint8_t data)
{
    // If we have no CHR-ROM banks, assume CHR-RAM is on-board and make
    // this address space writable
    if (chrBanks == 0)
    {
        if (chrA12Inversion == false)
        {
            if (addr < 0x800)
            {
                uint16_t a = (chrROM0Index * 0x400) + addr;
                chrROM[a] = data;
            }
            else if (addr < 0x1000)
            {
                uint16_t a = (chrROM1Index * 0x400) + (addr - 0x800);
                chrROM[a] = data;
            }
            else if (addr < 0x1400)
            {
                uint16_t a = (chrROM2Index * 0x400) + (addr - 0x1000);
                chrROM[a] = data;
            }
            else if (addr < 0x1800)
            {
                uint16_t a = (chrROM3Index * 0x400) + (addr - 0x1400);
                chrROM[a] = data;
            }
            else if (addr < 0x1C00)
            {
                uint16_t a = (chrROM4Index * 0x400) + (addr - 0x1800);
                chrROM[a] = data;
            }
            else if (addr < 0x2000)
            {
                uint16_t a = (chrROM5Index * 0x400) + (addr - 0x1C00);
                chrROM[a] = data;
            }
        }
        else
        {
            if (addr < 0x400)
            {
                uint16_t a = (chrROM2Index * 0x400) + addr;
                chrROM[a] = data;
            }
            else if (addr < 0x800)
            {
                uint16_t a = (chrROM3Index * 0x400) + (addr - 0x400);
                chrROM[a] = data;
            }
            else if (addr < 0xC00)
            {
                uint16_t a = (chrROM4Index * 0x400) + (addr - 0x800);
                chrROM[a] = data;
            }
            else if (addr < 0x1000)
            {
                uint16_t a = (chrROM5Index * 0x400) + (addr - 0xC00);
                chrROM[a] = data;
            }
            else if (addr < 0x1800)
            {
                uint16_t a = (chrROM0Index * 0x400) + (addr - 0x1000);
                chrROM[a] = data;
            }
            else if (addr < 0x2000)
            {
                uint16_t a = (chrROM1Index * 0x400) + (addr - 0x1800);
                chrROM[a] = data;
            }
        }
    }

    // PRG-RAM (MMC3 has write protection flags but we can ignore them?)
    // TODO investigate this and also work on CPU open bus behavior
    if (addr >= 0x6000 && addr < 0x8000)
    {
        prgRAM[addr - 0x6000] = data;
    }
    // Bank Select / Bank Data writes
    if (addr >= 0x8000 && addr <= 0x9FFF)
    {
        // Bank Select logic on even
        if (addr % 2 == 0)
        {
            bankRegisterMode = data & 0x07; // First 3 bits selects bank data
            prgROMBankMode = (data & 0x40) >> 6; // Bit 7 selects PRG-ROM bank mode
            chrA12Inversion = (data & 0x80) >> 7; // Bit 8 selects "CHR A12 inversion" (bank addr swaps)
        }
        // Bank Data logic on odd
        else
        {
            // Note: R6/R7 only writes first 6 bits to PRG-ROM, the rest write whole byte as normal
            // (there's a bit of nuance here regarding an "8-bit extension"? we don't need it though)
            // Additionally, R0/R1 ignores first bit to CHR-ROM, the rest write whole byte as normal

            switch (bankRegisterMode)
            {
                case 0:
                    chrROM0Index = (data & 0xFE);
                    break;
                case 1:
                    chrROM1Index = (data & 0xFE);
                    break;
                case 2:
                    chrROM2Index = data;
                    break;
                case 3:
                    chrROM3Index = data;
                    break;
                case 4:
                    chrROM4Index = data;
                    break;
                case 5:
                    chrROM5Index = data;
                    break;
                case 6:
                    prgROM0Index = (data & 0x3F);
                    break;
                case 7:
                    prgROM1Index = (data & 0x3F);
                    break;
            }
        }
    }
    // NT Arrangement / PRG-RAM Protect
    if (addr >= 0xA000 && addr <= 0xBFFF)
    {
        // Nametable arrangement logic on even
        if (addr % 2 == 0)
        {
            // Can only change if mirroring isn't hard-wired already (aka 4-way mirroring)
            if (mirroringModeHardwired == false)
            {
                mirroringMode = (MirroringMode) (data & 0x01);
            }
        }
        // PRG-RAM protect logic on odd
        else
        {
            // Do nothing... for now?
            // 1) Bits 5-6 used for MMC6 mapper, which shares same mapper number in iNES sadly
            // 2) Bit 7 used for RAM write protection (can totally safely ignore)
            // 3) Bit 8 used for ROM enable/disable (need to send CPU open bus data on disable for proper emulation)
            // Very low priority but will need to return to this eventually!
        }
    }
    // IRQ Latch / IRQ Reload
    if (addr >= 0xC000 && addr <= 0xDFFF)
    {
        // IRQ latch on even
        if (addr % 2 == 0)
        {
            irqCounterReload = data;
        }
        // IRQ reload on odd
        else
        {
            irqCounter = 0;
        }
    }
    // IRQ Disable / IRQ Enable
    if (addr >= 0xE000 && addr <= 0xFFFF)
    {
        // IRQ disable on even
        if (addr % 2 == 0)
        {
            irqEnabled = false;
            // Side-effect of clearing the IRQ flag
            core->cpu->irq = false;
        }
        // IRQ enable on odd
        else
        {
            irqEnabled = true;
        }
    }
}

void NESDL_Mapper_4::ClockIRQ()
{
    //printf("%d/%d|%d", irqCounter, irqCounterReload, irqEnabled);
    if (irqCounter == 0)
    {
        irqCounter = irqCounterReload;
    }
    else
    {
        irqCounter -= 1;
    }

    if (irqCounter == 0 && irqEnabled)
    {
        //printf("\nIRQ Trigger");
        core->cpu->irq = true;
        core->cpu->delayIRQ = true;
    }
}
