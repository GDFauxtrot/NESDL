#include "NESDL.h"

void NESDL_RAM::Init(NESDL_Core* c)
{
    core = c;
    oops = false;
    ignoreChanges = false;
}

uint8_t NESDL_RAM::ReadByte(uint16_t addr)
{
    // Filter byte according to target
    
    // 0x0000 - 0x1FFF : 2KB RAM (+ mirroring)
    if (addr < 0x2000)
    {
        return ram[addr % 0x800];
    }
    // 0x2000 - 0x3FFF, 0x4014 : 8 PPU + OAM registers (+ mirroring)
    else if (addr < 0x4000)
    {
        return core->ppu->ReadFromRegister(0x2000 + (addr % 8));
    }
    else if (addr == 0x4014)
    {
        return core->ppu->ReadFromRegister(addr);
    }
    // 0x4000 - 0x4013, 0x4015 : APU registers
    else if (addr < 0x4016)
    {
        return core->apu->ReadByte(addr);
    }
    // 0x4016, 0x4017 : Joystick 1/2 input
    else if (addr < 0x4018)
    {
        bool isPlayer2 = addr == 0x4017;
        // High 3 (or 5?) bytes are open bus, usually containing the upper byte of the controller address
        // (aka 0x40). Needed for Paperboy as it expects this open bus data, according to NESDEV
        return 0x40 | (uint8_t)core->input->GetNextPlayerInputBit(isPlayer2);
    }
    // APU test functionality + some unfinished hardware functionality (do nothing)
    else if (addr < 0x4020)
    {
        
    }
    // Unmapped (cartridge ram, handled by mapper)
    else
    {
        return mapper->ReadByte(addr);
    }
    
    return 0;
}

uint16_t NESDL_RAM::ReadWord(uint16_t addr)
{
    if (addr < 0x2000)
    {
        addr = addr % 0x800;
    }
    
    // Little-endian - swap bytes around
    uint16_t lower = (uint16_t)ReadByte(addr);
    // Check if we crossed page bounds while assembing this word
    oops = (addr / 0x100) != ((addr+1) / 0x100);
    uint16_t upper = (uint16_t)ReadByte(addr+1) << 8;
    uint16_t result = lower + upper;
    return result;
}

void NESDL_RAM::WriteByte(uint16_t addr, uint8_t data)
{
    if (ignoreChanges)
    {
        return;
    }
    // Filter byte according to target
    
    // 0x0000 - 0x1FFF : 2KB RAM (+ mirroring)
    if (addr < 0x2000)
    {
        ram[addr % 0x800] = data;
    }
    // 0x2000 - 0x3FFF, 0x4014 : 8 PPU + OAM registers (+ mirroring)
    else if (addr < 0x4000)
    {
        core->ppu->WriteToRegister(0x2000 + (addr % 8), data);
    }
    else if (addr == 0x4014)
    {
        core->ppu->WriteToRegister(addr, data);
    }
    // 0x4000 - 0x4013, 0x4015 : APU registers
    else if (addr < 0x4016)
    {
        core->apu->WriteByte(addr, data);
    }
    // 0x4016, 0x4017 : Joystick 1/2 input (fun fact: JOY2 WRITE is APU frame counter info! Weird huh
    else if (addr < 0x4018)
    {
        if (addr == 0x4016)
        {
            // Ready controller input data shift registers (we only care about the first bit)
            core->input->SetReadInputStrobe(data & 0x1);
        }
        else
        {
            // APU frame counter shenanigans
        }
    }
    // APU test functionality + some unfinished hardware functionality (do nothing)
    else if (addr < 0x4020)
    {
        
    }
    // Unmapped (cartridge ram, handled by mapper)
    else
    {
        mapper->WriteByte(addr, data);
    }
}

void NESDL_RAM::SetMapper(NESDL_Mapper* m)
{
    mapper = m;
}
