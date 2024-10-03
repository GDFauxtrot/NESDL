#include "NESDL.h"

void NESDL_CPU::Init(NESDL_Core* c)
{
	elapsedCycles = 0;
    core = c;
    addrModeResult = new AddressModeResult();
    ppuCycleCounter = 0;
}

void NESDL_CPU::Start()
{
    // CPU warms up for 7 cycles and fetches the true start address
    // from 0xFFFC. Let's do the same here

    InitializeCPURegisters();
    registers.pc = core->ram->ReadWord(registers.pc);
    elapsedCycles += 7;
}

void NESDL_CPU::Update(uint32_t ppuCycles)
{
    // Get the next instruction to be run and wait that many cycles to complete it.
    // We want to execute the instruction fully AFTER the alotted time for that instruction
    // in order to leave room for the PPU to execute what it needs to properly
    // (If we wanted perfect accurate emulation, we'd simulate waits on opcode reads and various
    // actions on the hardware, but this method also seems to work well enough)
    
    // PPU cycle counter has set amount of cycles (normally 1) added to it every update.
    // If the count >= 3, we're due for at least one CPU cycle update
    
    // Find out how long the next instruction will take.
    // We delay execution until we've counted up to that value, reset the counter
    // then run the instruction and evaluate the next instruction's time.
    uint8_t nextInstructionPPUTime = GetCyclesForNextInstruction() * 3;
//    ppuCycleCounter += ppuCycles;
//    while (ppuCycleCounter > nextInstructionPPUTime)
//    {
//        // Handle NMI
//        if (nmi)
//        {
//            nmi = false;
//            NMI();
//        }
//        else
//        {
//            ppuCycleCounter -= nextInstructionPPUTime;
//            RunNextInstruction();
//            nextInstructionPPUTime = GetCyclesForNextInstruction() * 3;
//        }
//    }
    while (ppuCycleCounter >= 0)
    {
        // Handle NMI
        if (nmi && !delayNMI)
        {
            nmi = false;
            NMI();
        }
        else if (dmaDelay)
        {
            dmaDelay = false;
            HaltCPUForDMAWrite();
        }
        else
        {
            if (dmaDelayNextFrame)
            {
                dmaDelayNextFrame = false;
                dmaDelay = true;
            }
            delayNMI = false;
            
            // Hack - we can predict a VBL occuring during this instruction, just... flip it on,
            // for timing accuracy purposes pls
            // GetCyclesForNextInstruction prepped our addrModeResult address, we just need to check it
            if (addrModeResult->address >= 0x2000)
            {
                uint16_t addr = 0x2000 + (addrModeResult->address % 0x8);
                if (addr == PPU_PPUSTATUS)
                {
                    core->ppu->PreprocessPPUForReadInstructionTiming(nextInstructionPPUTime);
                }
            }
            
//            if (elapsedCycles >= 831487)
//            {
//                // Debug Nintendulator format
//                printf("\n%04X\t\t\t\t\t\t\t\t\t\t\tA:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%llu", registers.pc, registers.a, registers.x, registers.y, registers.p, registers.sp, core->ppu->posInScanline, core->ppu->currentScanline, elapsedCycles);
//            }
            
            ppuCycleCounter -= nextInstructionPPUTime;
            RunNextInstruction();
//            nextInstructionPPUTime = GetCyclesForNextInstruction() * 3;
        }
    }
    ppuCycleCounter += ppuCycles;
    
//    if (ppuCycleCounter == 0)
//    {
////        printf("\n%04X\t\t\t\t\t\t\t\t\t\t\tA:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%llu", registers.pc, registers.a, registers.x, registers.y, registers.p, registers.sp, core->ppu->posInScanline, core->ppu->currentScanline, elapsedCycles);
//        nextInstruction = GetNextInstructionByte();
//        nextInstructionSet = true;
//    }
//    
//    if (nextInstructionSet && ppuCycleCounter >= 5)
//    {
//        nextInstructionSet = false;
//        ppuCycleCounter -= GetCyclesForNextInstruction() * 3;
//        RunNextInstruction();
//    }
//    ppuCycleCounter += ppuCycles;
}

void NESDL_CPU::InitializeCPURegisters()
{
    // Initialize registers with values measured from real-world hardware,
    // assuming a hard reset.
    registers.a = 0;
    registers.x = 0;
    registers.y = 0;
    registers.pc = 0xFFFC;
    registers.sp = 0xFD;
    registers.p = PSTATUS_INTERRUPTDISABLE | PSTATUS_UNDEFINED;
}

uint8_t NESDL_CPU::GetCyclesForNextInstruction()
{
    // Run next instruction to see how long it'll take, then reset the CPU state
    CPURegisters regState = registers;
    uint64_t prevElapsedCycles = elapsedCycles;
    core->ppu->ignoreChanges = true;
    core->ram->ignoreChanges = true;
    core->input->ignoreChanges = true;
    RunNextInstruction();
    core->ppu->ignoreChanges = false;
    core->ram->ignoreChanges = false;
    core->input->ignoreChanges = false;
    registers = regState;
    uint8_t result = elapsedCycles - prevElapsedCycles;
    elapsedCycles = prevElapsedCycles;
    return result;
}

uint8_t NESDL_CPU::GetNextInstructionByte()
{
    return core->ram->ReadByte(registers.pc++);
}

void NESDL_CPU::RunNextInstruction()
{
	// Get next instruction from memory according to current program counter
	uint8_t opcode = core->ram->ReadByte(registers.pc++);
//    uint8_t opcode = nextInstruction;
    
    switch (opcode)
    {
        case 0x00:
            OP_BRK(opcode, AddrMode::IMPLICIT);
            break;
        case 0x01:
            OP_ORA(opcode, AddrMode::INDIRECTX);
            break;
        case 0x05:
            OP_ORA(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x06:
            OP_ASL(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x08:
            OP_PHP(opcode, AddrMode::IMPLICIT);
            break;
        case 0x09:
            OP_ORA(opcode, AddrMode::IMMEDIATE);
            break;
        case 0x0A:
            OP_ASL(opcode, AddrMode::ACCUMULATOR);
            break;
        case 0x0D:
            OP_ORA(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x0E:
            OP_ASL(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x10:
            OP_BPL(opcode, AddrMode::RELATIVE);
            break;
        case 0x11:
            OP_ORA(opcode, AddrMode::INDIRECTY);
            break;
        case 0x15:
            OP_ORA(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x16:
            OP_ASL(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x18:
            OP_CLC(opcode, AddrMode::IMPLICIT);
            break;
        case 0x19:
            OP_ORA(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0x1D:
            OP_ORA(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x1E:
            OP_ASL(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x20:
            OP_JSR(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x21:
            OP_AND(opcode, AddrMode::INDIRECTX);
            break;
        case 0x24:
            OP_BIT(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x25:
            OP_AND(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x26:
            OP_ROL(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x28:
            OP_PLP(opcode, AddrMode::IMPLICIT);
            break;
        case 0x29:
            OP_AND(opcode, AddrMode::IMMEDIATE);
            break;
        case 0x2A:
            OP_ROL(opcode, AddrMode::ACCUMULATOR);
            break;
        case 0x2C:
            OP_BIT(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x2D:
            OP_AND(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x2E:
            OP_ROL(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x30:
            OP_BMI(opcode, AddrMode::RELATIVE);
            break;
        case 0x31:
            OP_AND(opcode, AddrMode::INDIRECTY);
            break;
        case 0x35:
            OP_AND(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x36:
            OP_ROL(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x38:
            OP_SEC(opcode, AddrMode::IMPLICIT);
            break;
        case 0x39:
            OP_AND(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0x3D:
            OP_AND(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x3E:
            OP_ROL(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x40:
            OP_RTI(opcode, AddrMode::IMPLICIT);
            break;
        case 0x41:
            OP_EOR(opcode, AddrMode::INDIRECTX);
            break;
        case 0x45:
            OP_EOR(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x46:
            OP_LSR(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x48:
            OP_PHA(opcode, AddrMode::IMPLICIT);
            break;
        case 0x49:
            OP_EOR(opcode, AddrMode::IMMEDIATE);
            break;
        case 0x4A:
            OP_LSR(opcode, AddrMode::ACCUMULATOR);
            break;
        case 0x4C:
            OP_JMP(opcode, false);
            break;
        case 0x4D:
            OP_EOR(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x4E:
            OP_LSR(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x50:
            OP_BVC(opcode, AddrMode::RELATIVE);
            break;
        case 0x51:
            OP_EOR(opcode, AddrMode::INDIRECTY);
            break;
        case 0x55:
            OP_EOR(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x56:
            OP_LSR(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x58:
            OP_CLI(opcode, AddrMode::IMPLICIT);
            break;
        case 0x59:
            OP_EOR(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0x5D:
            OP_EOR(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x5E:
            OP_LSR(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x60:
            OP_RTS(opcode, AddrMode::IMPLICIT);
            break;
        case 0x61:
            OP_ADC(opcode, AddrMode::INDIRECTX);
            break;
        case 0x65:
            OP_ADC(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x66:
            OP_ROR(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x68:
            OP_PLA(opcode, AddrMode::IMPLICIT);
            break;
        case 0x69:
            OP_ADC(opcode, AddrMode::IMMEDIATE);
            break;
        case 0x6A:
            OP_ROR(opcode, AddrMode::ACCUMULATOR);
            break;
        case 0x6C:
            OP_JMP(opcode, true);
            break;
        case 0x6D:
            OP_ADC(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x6E:
            OP_ROR(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x70:
            OP_BVS(opcode, AddrMode::RELATIVE);
            break;
        case 0x71:
            OP_ADC(opcode, AddrMode::INDIRECTY);
            break;
        case 0x75:
            OP_ADC(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x76:
            OP_ROR(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x78:
            OP_SEI(opcode, AddrMode::IMPLICIT);
            break;
        case 0x79:
            OP_ADC(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0x7D:
            OP_ADC(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x7E:
            OP_ROR(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x81:
            OP_STA(opcode, AddrMode::INDIRECTX);
            break;
        case 0x84:
            OP_STY(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x85:
            OP_STA(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x86:
            OP_STX(opcode, AddrMode::ZEROPAGE);
            break;
        case 0x88:
            OP_DEY(opcode, AddrMode::IMPLICIT);
            break;
        case 0x8A:
            OP_TXA(opcode, AddrMode::IMPLICIT);
            break;
        case 0x8C:
            OP_STY(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x8D:
            OP_STA(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x8E:
            OP_STX(opcode, AddrMode::ABSOLUTE);
            break;
        case 0x90:
            OP_BCC(opcode, AddrMode::RELATIVE);
            break;
        case 0x91:
            OP_STA(opcode, AddrMode::INDIRECTY);
            break;
        case 0x94:
            OP_STY(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x95:
            OP_STA(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0x96:
            OP_STX(opcode, AddrMode::ZEROPAGEY);
            break;
        case 0x98:
            OP_TYA(opcode, AddrMode::IMPLICIT);
            break;
        case 0x99:
            OP_STA(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0x9A:
            OP_TXS(opcode, AddrMode::IMPLICIT);
            break;
        case 0x9D:
            OP_STA(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0xA0:
            OP_LDY(opcode, AddrMode::IMMEDIATE);
            break;
        case 0xA1:
            OP_LDA(opcode, AddrMode::INDIRECTX);
            break;
        case 0xA2:
            OP_LDX(opcode, AddrMode::IMMEDIATE);
            break;
        case 0xA4:
            OP_LDY(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xA5:
            OP_LDA(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xA6:
            OP_LDX(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xA8:
            OP_TAY(opcode, AddrMode::IMPLICIT);
            break;
        case 0xA9:
            OP_LDA(opcode, AddrMode::IMMEDIATE);
            break;
        case 0xAA:
            OP_TAX(opcode, AddrMode::IMPLICIT);
            break;
        case 0xAC:
            OP_LDY(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xAD:
            OP_LDA(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xAE:
            OP_LDX(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xB0:
            OP_BCS(opcode, AddrMode::RELATIVE);
            break;
        case 0xB1:
            OP_LDA(opcode, AddrMode::INDIRECTY);
            break;
        case 0xB4:
            OP_LDY(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0xB5:
            OP_LDA(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0xB6:
            OP_LDX(opcode, AddrMode::ZEROPAGEY);
            break;
        case 0xB8:
            OP_CLV(opcode, AddrMode::IMPLICIT);
            break;
        case 0xB9:
            OP_LDA(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0xBA:
            OP_TSX(opcode, AddrMode::IMPLICIT);
            break;
        case 0xBC:
            OP_LDY(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0xBD:
            OP_LDA(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0xBE:
            OP_LDX(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0xC0:
            OP_CPY(opcode, AddrMode::IMMEDIATE);
            break;
        case 0xC1:
            OP_CMP(opcode, AddrMode::INDIRECTX);
            break;
        case 0xC4:
            OP_CPY(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xC5:
            OP_CMP(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xC6:
            OP_DEC(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xC8:
            OP_INY(opcode, AddrMode::IMPLICIT);
            break;
        case 0xC9:
            OP_CMP(opcode, AddrMode::IMMEDIATE);
            break;
        case 0xCA:
            OP_DEX(opcode, AddrMode::IMPLICIT);
            break;
        case 0xCC:
            OP_CPY(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xCD:
            OP_CMP(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xCE:
            OP_DEC(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xD0:
            OP_BNE(opcode, AddrMode::RELATIVE);
            break;
        case 0xD1:
            OP_CMP(opcode, AddrMode::INDIRECTY);
            break;
        case 0xD5:
            OP_CMP(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0xD6:
            OP_DEC(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0xD8:
            OP_CLD(opcode, AddrMode::IMPLICIT);
            break;
        case 0xD9:
            OP_CMP(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0xDD:
            OP_CMP(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0xDE:
            OP_DEC(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0xE0:
            OP_CPX(opcode, AddrMode::IMMEDIATE);
            break;
        case 0xE1:
            OP_SBC(opcode, AddrMode::INDIRECTX);
            break;
        case 0xE4:
            OP_CPX(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xE5:
            OP_SBC(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xE6:
            OP_INC(opcode, AddrMode::ZEROPAGE);
            break;
        case 0xE8:
            OP_INX(opcode, AddrMode::IMPLICIT);
            break;
        case 0xE9:
            OP_SBC(opcode, AddrMode::IMMEDIATE);
            break;
        case 0xEC:
            OP_CPX(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xED:
            OP_SBC(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xEE:
            OP_INC(opcode, AddrMode::ABSOLUTE);
            break;
        case 0xF0:
            OP_BEQ(opcode, AddrMode::RELATIVE);
            break;
        case 0xF1:
            OP_SBC(opcode, AddrMode::INDIRECTY);
            break;
        case 0xF5:
            OP_SBC(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0xF6:
            OP_INC(opcode, AddrMode::ZEROPAGEX);
            break;
        case 0xF8:
            OP_SED(opcode, AddrMode::IMPLICIT);
            break;
        case 0xF9:
            OP_SBC(opcode, AddrMode::ABSOLUTEY);
            break;
        case 0xFD:
            OP_SBC(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0xFE:
            OP_INC(opcode, AddrMode::ABSOLUTEX);
            break;
        case 0x80:
        case 0x82:
        case 0xC2:
        case 0xE2:
        case 0x04:
        case 0x44:
        case 0x64:
        case 0x89:
        case 0xEA:
        case 0x0C:
        case 0x14:
        case 0x34:
        case 0x54:
        case 0x74:
        case 0xD4:
        case 0xF4:
        case 0x1A:
        case 0x3A:
        case 0x5A:
        case 0x7A:
        case 0xDA:
        case 0xFA:
        case 0x1C:
        case 0x3C:
        case 0x5C:
        case 0x7C:
        case 0xDC:
        case 0xFC:
            OP_NOP(opcode, AddrMode::IMPLICIT);
            break;
        default:
            OP_KIL();
            break;
    }
}

// Returns 1 if the value > 0, -1 if the value is < 0, and 0 if the value is 0.
template <typename T> int8_t sign(T val) {
    return (T(0) < val) - (val < T(0));
}


void NESDL_CPU::SetPSFlag(PStatusFlag flag, bool on)
{
    if (on)
    {
        registers.p |= flag;
    }
    else
    {
        registers.p &= ~flag;
    }
}

// Retrieves the byte at the current PC address according to the given address mode.
// Will automatically advance the PC register but not advance the cycle count.
void NESDL_CPU::GetByteForAddressMode(AddrMode mode, AddressModeResult* result)
{
    // Reset the "oops" flag
    core->ram->oops = false;
    
    switch (mode)
    {
        case AddrMode::IMPLICIT:
        case AddrMode::ACCUMULATOR:
            // Do nothing
            break;
        case AddrMode::RELATIVE:
        case AddrMode::IMMEDIATE:
            result->value = core->ram->ReadByte(registers.pc++);
            result->address = 0;
            break;
        case AddrMode::ZEROPAGE:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            result->value = core->ram->ReadByte(addr);
            result->address = addr;
            break;
        }
        case AddrMode::ZEROPAGEX:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            addr = (addr + registers.x) % 256;
            result->value = core->ram->ReadByte(addr);
            result->address = addr;
            break;
        }
        case AddrMode::ZEROPAGEY:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            addr = (addr + registers.y) % 256;
            result->value = core->ram->ReadByte(addr);
            result->address = addr;
            break;
        }
        case AddrMode::ABSOLUTE:
        {
            uint16_t addr = core->ram->ReadWord(registers.pc);
            result->value = core->ram->ReadByte(addr);
            registers.pc += 2;
            result->address = addr;
            break;
        }
        case AddrMode::ABSOLUTEX:
        {
            uint16_t addr = core->ram->ReadWord(registers.pc);
            core->ram->oops = (addr / 0x100) != ((addr + registers.x) / 0x100);
            addr += registers.x;
            result->value = core->ram->ReadByte(addr);
            registers.pc += 2;
            result->address = addr;
            break;
        }
        case AddrMode::ABSOLUTEY:
        {
            uint16_t addr = core->ram->ReadWord(registers.pc);
            core->ram->oops = (addr / 0x100) != ((addr + registers.y) / 0x100);
            addr += registers.y;
            result->value = core->ram->ReadByte(addr);
            registers.pc += 2;
            result->address = addr;
            break;
        }
        case AddrMode::INDIRECTX:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            uint16_t lsb = core->ram->ReadByte((addr + registers.x) % 256);
            uint16_t hsb = core->ram->ReadByte((addr + registers.x + 1) % 256) << 8;
            result->value = core->ram->ReadByte(hsb + lsb);
            result->address = hsb + lsb;
            break;
        }
        case AddrMode::INDIRECTY:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            uint16_t lsb = core->ram->ReadByte(addr);
            uint16_t hsb = (core->ram->ReadByte((addr + 1) % 256) << 8);
            uint16_t resultAddr = hsb + lsb;
            uint16_t targetAddr = resultAddr + registers.y;
            // Trigger an "oops" if this mode crossed page bounds
            core->ram->oops = (resultAddr / 0x100) != (targetAddr / 0x100);
            result->value = core->ram->ReadByte(targetAddr);
            result->address = targetAddr;
            break;
        }
    }
}

// Advances the CPU cycle count for most address modes with a consistent cycle count
// (With the exception of IMPLICIT as it's specific per-opcode)
void NESDL_CPU::AdvanceCyclesForAddressMode(uint8_t opcode, AddrMode mode, bool pageCross, bool extraCycles, bool relSuccess)
{
    elapsedCycles += GetCyclesForAddressMode(opcode, mode, pageCross, extraCycles, relSuccess);
}

uint8_t NESDL_CPU::GetCyclesForAddressMode(uint8_t opcode, AddrMode mode, bool pageCross, bool extraCycles, bool relSuccess)
{
    if (opcode == 0x4C)
    {
        return 3;
    }
    if (opcode == 0x6C)
    {
        return 5;
    }
    switch (mode)
    {
        case IMPLICIT:
            // Handle it on a per-instruction basis
            switch (opcode)
            {
                case 0x08:
                case 0x48:
                    return 3;
                    break;
                case 0x28:
                case 0x68:
                    return 4;
                    break;
                case 0x40:
                case 0x60:
                    return 6;
                    break;
                case 0x00:
                    return 7;
                    break;
            }
            return 2;
            break;
        case ZEROPAGE:
            return 3 + (extraCycles ? 2 : 0);
            break;
        case ZEROPAGEX:
        case ZEROPAGEY:
            return 4 + (extraCycles ? 2 : 0);
            break;
        case ABSOLUTE:
            return 4 + (extraCycles ? 2 : 0); // One exception being JMP (3/5 cycles)
            break;
        case ABSOLUTEX:
            return 4 + (extraCycles ? 3 : pageCross);
            break;
        case ABSOLUTEY:
            return 4 + pageCross;
            break;
        case RELATIVE:
            return 2 + pageCross + relSuccess;
            break;
        case ACCUMULATOR:
        case IMMEDIATE:
            return 2;
            break;
        case INDIRECTX:
            return 6;
            break;
        case INDIRECTY:
            return 5 + pageCross;
            break;
    }
}

void NESDL_CPU::OP_ADC(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    // Add A, M and C together to get the result of addition with carry
    uint8_t oldAcc = registers.a;
    uint8_t val = addrModeResult->value;
    uint8_t carry = registers.p & PSTATUS_CARRY;
    uint8_t result = oldAcc + val + carry; // A+M+C
    registers.a = result;
    
    // Set the flags as a result of this operation
    
    // If the result of A+M < A, or (A+M)+C < (A+M), then we have a carry (C)
    SetPSFlag(PSTATUS_CARRY, (oldAcc + val) < oldAcc || result < (oldAcc + val));

    // Gonna cheat on this one and just check if the value goes beyond the range of an int8
    // (I'm not terribly clever but it is terribly late and I'm terribly tired)
    int16_t result16 = (int8_t)oldAcc + (int8_t)val + carry;
    SetPSFlag(PSTATUS_OVERFLOW, result16 < -128 || result16 > 127);
    
    // Set zero and negative flags as usual
    SetPSFlag(PSTATUS_ZERO, result == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)result) < 0);
}

void NESDL_CPU::OP_AND(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    uint8_t val = addrModeResult->value;
    registers.a &= val;
    
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
}

void NESDL_CPU::OP_ASL(uint8_t opcode, AddrMode mode)
{
    uint8_t value = 0;
    uint8_t oldValue = 0;
    if (mode == AddrMode::ACCUMULATOR)
    {
        oldValue = registers.a;
        registers.a *= 2;
        value = registers.a;
    }
    else
    {
        GetByteForAddressMode(mode, addrModeResult);
        uint8_t val = addrModeResult->value;
        oldValue = val;
        val *= 2;
        value = val;
        core->ram->WriteByte(addrModeResult->address, val);
    }
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x80) >> 6); // Set to old value bit 7
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_BCC(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_CARRY) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BCS(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_CARRY) == PSTATUS_CARRY)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BEQ(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_ZERO) == PSTATUS_ZERO)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BIT(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    uint8_t result = registers.a & addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, result == 0);
    SetPSFlag(PSTATUS_OVERFLOW, (addrModeResult->value & PSTATUS_OVERFLOW) >> 5);
    SetPSFlag(PSTATUS_NEGATIVE, (addrModeResult->value & PSTATUS_NEGATIVE) >> 6);
}

void NESDL_CPU::OP_BMI(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_NEGATIVE) == PSTATUS_NEGATIVE)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BNE(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_ZERO) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BPL(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_NEGATIVE) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BRK(uint8_t opcode, AddrMode mode)
{
    // Push PC onto stack in two bytes (high then low since we're going backwards)
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (registers.pc >> 8));
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.pc);
    // Push P onto stack
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p);
    // Read interrupt vector from FFFE/F into PC
    uint16_t lsb = core->ram->ReadByte(0xFFFE);
    uint16_t hsb = core->ram->ReadByte(0xFFFF) << 8;
    registers.pc = hsb + lsb;
    // Set break flag
    SetPSFlag(PSTATUS_BREAK, true);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_BVC(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_OVERFLOW) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BVS(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + (int8_t)addrModeResult->value;
    bool newPage = false;
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_OVERFLOW) == PSTATUS_OVERFLOW)
    {
        registers.pc = newAddr;
        didBranch = true;
        
        newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    }
    
//    EvaluateNMIPendingForBranch();
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_CLC(uint8_t opcode, AddrMode mode)
{
    SetPSFlag(PSTATUS_CARRY, false);
    
    // These take two cycles!?
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_CLD(uint8_t opcode, AddrMode mode)
{
    SetPSFlag(PSTATUS_DECIMAL, false);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_CLI(uint8_t opcode, AddrMode mode)
{
    SetPSFlag(PSTATUS_INTERRUPTDISABLE, false);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_CLV(uint8_t opcode, AddrMode mode)
{
    SetPSFlag(PSTATUS_OVERFLOW, false);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_CMP(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.a >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.a == addrModeResult->value);
    int8_t result = registers.a - addrModeResult->value;
    SetPSFlag(PSTATUS_NEGATIVE, result < 0);
}

void NESDL_CPU::OP_CPX(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.x >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.x == addrModeResult->value);
    int8_t result = registers.x - addrModeResult->value;
    SetPSFlag(PSTATUS_NEGATIVE, result < 0);
}

void NESDL_CPU::OP_CPY(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.y >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.y == addrModeResult->value);
    int8_t result = registers.y - addrModeResult->value;
    SetPSFlag(PSTATUS_NEGATIVE, result < 0);
}

void NESDL_CPU::OP_DEC(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    uint8_t val = addrModeResult->value - 1;
    core->ram->WriteByte(addrModeResult->address, val);
    
    SetPSFlag(PSTATUS_ZERO, val == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)val) < 0);
}

void NESDL_CPU::OP_DEX(uint8_t opcode, AddrMode mode)
{
    registers.x--;

    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_DEY(uint8_t opcode, AddrMode mode)
{
    registers.y--;
    
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_EOR(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.a ^= addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_INC(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    uint8_t val = addrModeResult->value + 1;
    core->ram->WriteByte(addrModeResult->address, val);
    
    SetPSFlag(PSTATUS_ZERO, val == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)val) < 0);
}

void NESDL_CPU::OP_INX(uint8_t opcode, AddrMode mode)
{
    registers.x++;
    
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_INY(uint8_t opcode, AddrMode mode)
{
    registers.y++;
    
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_JMP(uint8_t opcode, bool indirect)
{
    uint16_t addr = core->ram->ReadWord(registers.pc);
    if (indirect)
    {
        // Indirect jump has a bug on addresses at the end of pages (eg. 0x##FF) where
        // the LSB comes from 0x##FF but the HSB comes from 0x##00 (basically a page wrap)
        if ((addr & 0x00FF) == 0xFF)
        {
            uint16_t lsb = core->ram->ReadByte(addr);
            uint16_t hsb = core->ram->ReadByte(addr & 0xFF00) << 8;
            addr = hsb + lsb;
        }
        else
        {
            addr = core->ram->ReadWord(addr);
        }
        AdvanceCyclesForAddressMode(opcode, AddrMode::ABSOLUTE, false, false, false);
    }
    else
    {
        AdvanceCyclesForAddressMode(opcode, AddrMode::IMPLICIT, false, false, false);
    }
    registers.pc = addr;
    
    // HACK - in Nintendulator, I've seen an NMI occur AFTER a JMP coinciding at (241,2) on the PPU.
    // I've been led to assume that the NMI trips on (241,1) and waits for the current instruction to finish,
    // so I'm not sure why JMP runs first in this very specific instance. But for accuracy I'd like to match this
    if (core->ppu->posInScanline == 334)
    {
        delayNMI = true;
    }
}

void NESDL_CPU::OP_JSR(uint8_t opcode, AddrMode mode)
{
    // Retrieve the address (operand) and advance PC
    uint16_t addr = core->ram->ReadWord(registers.pc++);
    
    // Push this PC onto the stack to return to later
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (registers.pc >> 8));
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.pc);
    
    // Overwrite the PC with the address
    registers.pc = addr;
    
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
}

void NESDL_CPU::OP_LDA(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.a = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_LDX(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.x = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
}

void NESDL_CPU::OP_LDY(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.y = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
}

void NESDL_CPU::OP_LSR(uint8_t opcode, AddrMode mode)
{
    uint8_t value = 0;
    uint8_t oldValue = 0;
    if (mode == AddrMode::ACCUMULATOR)
    {
        oldValue = registers.a;
        registers.a /= 2;
        value = registers.a;
    }
    else
    {
        GetByteForAddressMode(mode, addrModeResult);
        uint8_t val = addrModeResult->value;
        oldValue = val;
        val /= 2;
        value = val;
        core->ram->WriteByte(addrModeResult->address, val);
    }
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x01)); // Set to old value bit 0
    SetPSFlag(PSTATUS_ZERO, value == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_NOP(uint8_t opcode, AddrMode mode)
{
    // nop nop
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_ORA(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.a |= addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_PHA(uint8_t opcode, AddrMode mode)
{
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.a);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_PHP(uint8_t opcode, AddrMode mode)
{
    // Break flag pushes as 1 but physically isn't a stored bit (status is a 6 bit register)
    // Additionally, mysterious bit 6 ("undefined") always pushes to stack as 1 too
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p | PSTATUS_BREAK | PSTATUS_UNDEFINED);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_PLA(uint8_t opcode, AddrMode mode)
{
    uint8_t stackVal = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    registers.a = stackVal;
    SetPSFlag(PSTATUS_ZERO, stackVal == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)stackVal) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_PLP(uint8_t opcode, AddrMode mode)
{
    uint8_t stackVal = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    registers.p = stackVal;
//    SetPSFlag(PSTATUS_ZERO, stackVal == 0);
//    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)stackVal) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_ROL(uint8_t opcode, AddrMode mode)
{
    // Similar to ASL except bit 0 becomes the carry flag
    uint8_t value = 0;
    uint8_t oldValue = 0;
    if (mode == AddrMode::ACCUMULATOR)
    {
        oldValue = registers.a;
        registers.a *= 2;
        registers.a += (registers.p & PSTATUS_CARRY);
        value = registers.a;
    }
    else
    {
        GetByteForAddressMode(mode, addrModeResult);
        uint8_t val = addrModeResult->value;
        oldValue = val;
        val *= 2;
        val += (registers.p & PSTATUS_CARRY);
        value = val;
        core->ram->WriteByte(addrModeResult->address, val);
    }
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x80) >> 6); // Set to old value bit 7
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_ROR(uint8_t opcode, AddrMode mode)
{
    // Similar to ASL except bit 0 becomes the carry flag
    uint8_t value = 0;
    uint8_t oldValue = 0;
    if (mode == AddrMode::ACCUMULATOR)
    {
        oldValue = registers.a;
        registers.a /= 2;
        registers.a += (registers.p & PSTATUS_CARRY) << 7;
        value = registers.a;
    }
    else
    {
        GetByteForAddressMode(mode, addrModeResult);
        uint8_t val = addrModeResult->value;
        oldValue = val;
        val /= 2;
        val += (registers.p & PSTATUS_CARRY) << 7;
        value = val;
        core->ram->WriteByte(addrModeResult->address, val);
    }
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x01)); // Set to old value bit 0
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_RTI(uint8_t opcode, AddrMode mode)
{
    // Pull P from stack
    uint8_t p = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    registers.p = p;
    // Pull PC from stack
    uint16_t pc = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    pc += core->ram->ReadByte(STACK_PTR + (++registers.sp)) << 8;
    registers.pc = pc;
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_RTS(uint8_t opcode, AddrMode mode)
{
    // Pull PC from stack
    uint16_t pc = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    pc += core->ram->ReadByte(STACK_PTR + (++registers.sp)) << 8;
    registers.pc = pc+1; // Not sure why we advance PC once but it seems to work out that way
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_SBC(uint8_t opcode, AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    // Subtract A, M and (1-C) together to get the result of subtraction with carry
    uint8_t oldAcc = registers.a;
    uint8_t val = addrModeResult->value;
    uint8_t carry = registers.p & PSTATUS_CARRY;
    uint8_t result = oldAcc - val - (1-carry); // A-M-(1-C)
    registers.a = result;
    
    // Set the flags as a result of this operation
    
    // If the result of A+M < A, or (A+M)+C < (A+M), then we have a carry (C)
    SetPSFlag(PSTATUS_CARRY, (oldAcc + val) < oldAcc || result < (oldAcc + val) || (oldAcc == 0 && val == 0 && carry == 1));

    // Gonna cheat on this one and just check if the value goes beyond the range of an int8
    // (I'm not terribly clever but it is terribly late and I'm terribly tired)
    int16_t result16 = (int8_t)oldAcc - (int8_t)val - (1-carry);
    SetPSFlag(PSTATUS_OVERFLOW, result16 < -128 || result16 > 127);
    
    // Set zero and negative flags as usual
    SetPSFlag(PSTATUS_ZERO, result == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)result) < 0);
}

void NESDL_CPU::OP_SEC(uint8_t opcode, AddrMode mode)
{
    SetPSFlag(PSTATUS_CARRY, true);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_SED(uint8_t opcode, AddrMode mode)
{
    SetPSFlag(PSTATUS_DECIMAL, true);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_SEI(uint8_t opcode, AddrMode mode)
{
    SetPSFlag(PSTATUS_INTERRUPTDISABLE, true);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_STA(uint8_t opcode, AddrMode mode)
{
    core->ppu->incrementV = false;
    GetByteForAddressMode(mode, addrModeResult);
    core->ppu->incrementV = true;
    AdvanceCyclesForAddressMode(opcode, mode, true, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.a);
}

void NESDL_CPU::OP_STX(uint8_t opcode, AddrMode mode)
{
    core->ppu->incrementV = false;
    GetByteForAddressMode(mode, addrModeResult);
    core->ppu->incrementV = true;
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.x);
}

void NESDL_CPU::OP_STY(uint8_t opcode, AddrMode mode)
{
    core->ppu->incrementV = false;
    GetByteForAddressMode(mode, addrModeResult);
    core->ppu->incrementV = true;
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.y);
}

void NESDL_CPU::OP_TAX(uint8_t opcode, AddrMode mode)
{
    registers.x = registers.a;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TAY(uint8_t opcode, AddrMode mode)
{
    registers.y = registers.a;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TSX(uint8_t opcode, AddrMode mode)
{
    registers.x = registers.sp;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TXA(uint8_t opcode, AddrMode mode)
{
    registers.a = registers.x;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TXS(uint8_t opcode, AddrMode mode)
{
    registers.sp = registers.x;
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TYA(uint8_t opcode, AddrMode mode)
{
    registers.a = registers.y;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_KIL()
{
    throw domain_error("ILLEGAL OPCODE");
}

void NESDL_CPU::NMI()
{
    // Push PC to stack
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (registers.pc >> 8));
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.pc);
    
    // Set break flag (before pushing P to stack)
    SetPSFlag(PSTATUS_BREAK, false);
    
    // Push P to stack
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p);
    
    // Set interrupt flag
    SetPSFlag(PSTATUS_INTERRUPTDISABLE, true);
    
    // Advance to address specified at NMI location (0xFFFA)
    registers.pc = core->ram->ReadWord(0xFFFA);
    
    // Advance 7 cycles
    elapsedCycles += 7;
    ppuCycleCounter -= 21;
}

void NESDL_CPU::HaltCPUForDMAWrite()
{
    uint16_t delay = elapsedCycles % 2 == 0 ? 514 : 513;
    elapsedCycles += delay;
    ppuCycleCounter -= delay*3;
}
