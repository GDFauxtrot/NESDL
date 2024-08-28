#include "NESDL.h"

void NESDL_CPU::Init(NESDL_Core* c)
{
	elapsedCycles = 0;
    core = c;
    addrModeResult = new AddressModeResult();
}

void NESDL_CPU::Start()
{
    // CPU warms up for 7 cycles and fetches the true start address
    // from 0xFFFC. Let's do the same here

    InitializeCPURegisters();
    registers.pc = core->ram->ReadWord(registers.pc);
    elapsedCycles += 7;
}

void NESDL_CPU::Update(double newSystemTime)
{
	// Take the new system time (milliseconds since boot) and compare to our own tracked time
	// If the new system time is larger than the CPU's tracked time, then we're due for an instruction
	// Else, wait out this update - we overshot the master clock with our last instruction so we must wait
	uint64_t newCycles = MillisecondsToCPUCycles(newSystemTime);

	// Run instructions to catch up with time
    //if (newCycles > elapsedCycles)
	while (newCycles > elapsedCycles)
	{
		RunNextInstruction();
	}
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
    registers.p = PSTATUS_INTERRUPTDISABLE;
}

void NESDL_CPU::RunNextInstruction()
{
	// Get next instruction from memory according to current program counter
	uint8_t opcode = core->ram->ReadByte(registers.pc++);
    
    switch (opcode)
    {
        case 0x00:
            OP_BRK(AddrMode::IMPLICIT);
            break;
        case 0x01:
            OP_ORA(AddrMode::INDIRECTX);
            break;
        case 0x05:
            OP_ORA(AddrMode::ZEROPAGE);
            break;
        case 0x06:
            OP_ASL(AddrMode::ZEROPAGE);
            break;
        case 0x08:
            OP_PHP(AddrMode::IMPLICIT);
            break;
        case 0x09:
            OP_ORA(AddrMode::IMMEDIATE);
            break;
        case 0x0A:
            OP_ASL(AddrMode::ACCUMULATOR);
            break;
        case 0x0D:
            OP_ORA(AddrMode::ABSOLUTE);
            break;
        case 0x0E:
            OP_ASL(AddrMode::ABSOLUTE);
            break;
        case 0x10:
            OP_BPL(AddrMode::RELATIVE);
            break;
        case 0x11:
            OP_ORA(AddrMode::INDIRECTY);
            break;
        case 0x15:
            OP_ORA(AddrMode::ZEROPAGEX);
            break;
        case 0x16:
            OP_ASL(AddrMode::ZEROPAGEX);
            break;
        case 0x18:
            OP_CLC(AddrMode::IMPLICIT);
            break;
        case 0x19:
            OP_ORA(AddrMode::ABSOLUTEY);
            break;
        case 0x1D:
            OP_ORA(AddrMode::ABSOLUTEX);
            break;
        case 0x1E:
            OP_ASL(AddrMode::ABSOLUTEX);
            break;
        case 0x20:
            OP_JSR(AddrMode::ABSOLUTE);
            break;
        case 0x21:
            OP_AND(AddrMode::INDIRECTX);
            break;
        case 0x24:
            OP_BIT(AddrMode::ZEROPAGE);
            break;
        case 0x25:
            OP_AND(AddrMode::ZEROPAGE);
            break;
        case 0x26:
            OP_ROL(AddrMode::ZEROPAGE);
            break;
        case 0x28:
            OP_PLP(AddrMode::IMPLICIT);
            break;
        case 0x29:
            OP_AND(AddrMode::IMMEDIATE);
            break;
        case 0x2A:
            OP_ROL(AddrMode::ACCUMULATOR);
            break;
        case 0x2C:
            OP_BIT(AddrMode::ABSOLUTE);
            break;
        case 0x2D:
            OP_AND(AddrMode::ABSOLUTE);
            break;
        case 0x2E:
            OP_ROL(AddrMode::ABSOLUTE);
            break;
        case 0x30:
            OP_BMI(AddrMode::RELATIVE);
            break;
        case 0x31:
            OP_AND(AddrMode::INDIRECTY);
            break;
        case 0x35:
            OP_AND(AddrMode::ZEROPAGEX);
            break;
        case 0x36:
            OP_ROL(AddrMode::ZEROPAGEX);
            break;
        case 0x38:
            OP_SEC(AddrMode::IMPLICIT);
            break;
        case 0x39:
            OP_AND(AddrMode::ABSOLUTEY);
            break;
        case 0x3D:
            OP_AND(AddrMode::ABSOLUTEX);
            break;
        case 0x3E:
            OP_ROL(AddrMode::ABSOLUTEX);
            break;
        case 0x40:
            OP_RTI(AddrMode::IMPLICIT);
            break;
        case 0x41:
            OP_EOR(AddrMode::INDIRECTX);
            break;
        case 0x45:
            OP_EOR(AddrMode::ZEROPAGE);
            break;
        case 0x46:
            OP_LSR(AddrMode::ZEROPAGE);
            break;
        case 0x48:
            OP_PHA(AddrMode::IMPLICIT);
            break;
        case 0x49:
            OP_EOR(AddrMode::IMMEDIATE);
            break;
        case 0x4A:
            OP_LSR(AddrMode::ACCUMULATOR);
            break;
        case 0x4C:
            OP_JMP(false);
            break;
        case 0x4D:
            OP_EOR(AddrMode::ABSOLUTE);
            break;
        case 0x4E:
            OP_LSR(AddrMode::ABSOLUTE);
            break;
        case 0x50:
            OP_BVC(AddrMode::RELATIVE);
            break;
        case 0x51:
            OP_EOR(AddrMode::INDIRECTY);
            break;
        case 0x55:
            OP_EOR(AddrMode::ZEROPAGEX);
            break;
        case 0x56:
            OP_LSR(AddrMode::ZEROPAGEX);
            break;
        case 0x58:
            OP_CLI(AddrMode::IMPLICIT);
            break;
        case 0x59:
            OP_EOR(AddrMode::ABSOLUTEY);
            break;
        case 0x5D:
            OP_EOR(AddrMode::ABSOLUTEX);
            break;
        case 0x5E:
            OP_LSR(AddrMode::ABSOLUTEX);
            break;
        case 0x60:
            OP_RTS(AddrMode::IMPLICIT);
            break;
        case 0x61:
            OP_ADC(AddrMode::INDIRECTX);
            break;
        case 0x65:
            OP_ADC(AddrMode::ZEROPAGE);
            break;
        case 0x66:
            OP_ROR(AddrMode::ZEROPAGE);
            break;
        case 0x68:
            OP_PLA(AddrMode::IMPLICIT);
            break;
        case 0x69:
            OP_ADC(AddrMode::IMMEDIATE);
            break;
        case 0x6A:
            OP_ROR(AddrMode::ACCUMULATOR);
            break;
        case 0x6C:
            OP_JMP(true);
            break;
        case 0x6D:
            OP_ADC(AddrMode::ABSOLUTE);
            break;
        case 0x6E:
            OP_ROR(AddrMode::ABSOLUTE);
            break;
        case 0x70:
            OP_BVS(AddrMode::RELATIVE);
            break;
        case 0x71:
            OP_ADC(AddrMode::INDIRECTY);
            break;
        case 0x75:
            OP_ADC(AddrMode::ZEROPAGEX);
            break;
        case 0x76:
            OP_ROR(AddrMode::ZEROPAGEX);
            break;
        case 0x78:
            OP_SEI(AddrMode::IMPLICIT);
            break;
        case 0x79:
            OP_ADC(AddrMode::ABSOLUTEY);
            break;
        case 0x7D:
            OP_ADC(AddrMode::ABSOLUTEX);
            break;
        case 0x7E:
            OP_ROR(AddrMode::ABSOLUTEX);
            break;
        case 0x81:
            OP_STA(AddrMode::INDIRECTX);
            break;
        case 0x84:
            OP_STY(AddrMode::ZEROPAGE);
            break;
        case 0x85:
            OP_STA(AddrMode::ZEROPAGE);
            break;
        case 0x86:
            OP_STX(AddrMode::ZEROPAGE);
            break;
        case 0x88:
            OP_DEY(AddrMode::IMPLICIT);
            break;
        case 0x8A:
            OP_TXA(AddrMode::IMPLICIT);
            break;
        case 0x8C:
            OP_STY(AddrMode::ABSOLUTE);
            break;
        case 0x8D:
            OP_STA(AddrMode::ABSOLUTE);
            break;
        case 0x8E:
            OP_STX(AddrMode::ABSOLUTE);
            break;
        case 0x90:
            OP_BCC(AddrMode::RELATIVE);
            break;
        case 0x91:
            OP_STA(AddrMode::INDIRECTY);
            break;
        case 0x94:
            OP_STY(AddrMode::ZEROPAGEX);
            break;
        case 0x95:
            OP_STA(AddrMode::ZEROPAGEX);
            break;
        case 0x96:
            OP_STX(AddrMode::ZEROPAGEY);
            break;
        case 0x98:
            OP_TYA(AddrMode::IMPLICIT);
            break;
        case 0x99:
            OP_STA(AddrMode::ABSOLUTEY);
            break;
        case 0x9A:
            OP_TXS(AddrMode::IMPLICIT);
            break;
        case 0x9D:
            OP_STA(AddrMode::ABSOLUTEX);
            break;
        case 0xA0:
            OP_LDY(AddrMode::IMMEDIATE);
            break;
        case 0xA1:
            OP_LDA(AddrMode::INDIRECTX);
            break;
        case 0xA2:
            OP_LDX(AddrMode::IMMEDIATE);
            break;
        case 0xA4:
            OP_LDY(AddrMode::ZEROPAGE);
            break;
        case 0xA5:
            OP_LDA(AddrMode::ZEROPAGE);
            break;
        case 0xA6:
            OP_LDX(AddrMode::ZEROPAGE);
            break;
        case 0xA8:
            OP_TAY(AddrMode::IMPLICIT);
            break;
        case 0xA9:
            OP_LDA(AddrMode::IMMEDIATE);
            break;
        case 0xAA:
            OP_TAX(AddrMode::IMPLICIT);
            break;
        case 0xAC:
            OP_LDY(AddrMode::ABSOLUTE);
            break;
        case 0xAD:
            OP_LDA(AddrMode::ABSOLUTE);
            break;
        case 0xAE:
            OP_LDX(AddrMode::ABSOLUTE);
            break;
        case 0xB0:
            OP_BCS(AddrMode::RELATIVE);
            break;
        case 0xB1:
            OP_LDA(AddrMode::INDIRECTY);
            break;
        case 0xB4:
            OP_LDY(AddrMode::ZEROPAGEX);
            break;
        case 0xB5:
            OP_LDA(AddrMode::ZEROPAGEX);
            break;
        case 0xB6:
            OP_LDX(AddrMode::ZEROPAGEY);
            break;
        case 0xB8:
            OP_CLV(AddrMode::IMPLICIT);
            break;
        case 0xB9:
            OP_LDA(AddrMode::ABSOLUTEY);
            break;
        case 0xBA:
            OP_TSX(AddrMode::IMPLICIT);
            break;
        case 0xBC:
            OP_LDY(AddrMode::ABSOLUTEX);
            break;
        case 0xBD:
            OP_LDA(AddrMode::ABSOLUTEX);
            break;
        case 0xBE:
            OP_LDX(AddrMode::ABSOLUTEY);
            break;
        case 0xC0:
            OP_CPY(AddrMode::IMMEDIATE);
            break;
        case 0xC1:
            OP_CMP(AddrMode::INDIRECTX);
            break;
        case 0xC4:
            OP_CPY(AddrMode::ZEROPAGE);
            break;
        case 0xC5:
            OP_CMP(AddrMode::ZEROPAGE);
            break;
        case 0xC6:
            OP_DEC(AddrMode::ZEROPAGE);
            break;
        case 0xC8:
            OP_INY(AddrMode::IMPLICIT);
            break;
        case 0xC9:
            OP_CMP(AddrMode::IMMEDIATE);
            break;
        case 0xCA:
            OP_DEX(AddrMode::IMPLICIT);
            break;
        case 0xCC:
            OP_CPY(AddrMode::ABSOLUTE);
            break;
        case 0xCD:
            OP_CMP(AddrMode::ABSOLUTE);
            break;
        case 0xCE:
            OP_DEC(AddrMode::ABSOLUTE);
            break;
        case 0xD0:
            OP_BNE(AddrMode::RELATIVE);
            break;
        case 0xD1:
            OP_CMP(AddrMode::INDIRECTY);
            break;
        case 0xD5:
            OP_CMP(AddrMode::ZEROPAGEX);
            break;
        case 0xD6:
            OP_DEC(AddrMode::ZEROPAGEX);
            break;
        case 0xD8:
            OP_CLD(AddrMode::IMPLICIT);
            break;
        case 0xD9:
            OP_CMP(AddrMode::ABSOLUTEY);
            break;
        case 0xDD:
            OP_CMP(AddrMode::ABSOLUTEX);
            break;
        case 0xDE:
            OP_DEC(AddrMode::ABSOLUTEX);
            break;
        case 0xE0:
            OP_CPX(AddrMode::IMMEDIATE);
            break;
        case 0xE1:
            OP_SBC(AddrMode::INDIRECTX);
            break;
        case 0xE4:
            OP_CPX(AddrMode::ZEROPAGE);
            break;
        case 0xE5:
            OP_SBC(AddrMode::ZEROPAGE);
            break;
        case 0xE6:
            OP_INC(AddrMode::ZEROPAGE);
            break;
        case 0xE8:
            OP_INX(AddrMode::IMPLICIT);
            break;
        case 0xE9:
            OP_SBC(AddrMode::IMMEDIATE);
            break;
        case 0xEC:
            OP_CPX(AddrMode::ABSOLUTE);
            break;
        case 0xED:
            OP_SBC(AddrMode::ABSOLUTE);
            break;
        case 0xEE:
            OP_INC(AddrMode::ABSOLUTE);
            break;
        case 0xF0:
            OP_BEQ(AddrMode::RELATIVE);
            break;
        case 0xF1:
            OP_SBC(AddrMode::INDIRECTY);
            break;
        case 0xF5:
            OP_SBC(AddrMode::ZEROPAGEX);
            break;
        case 0xF6:
            OP_INC(AddrMode::ZEROPAGEX);
            break;
        case 0xF8:
            OP_SED(AddrMode::IMPLICIT);
            break;
        case 0xF9:
            OP_SBC(AddrMode::ABSOLUTEY);
            break;
        case 0xFD:
            OP_SBC(AddrMode::ABSOLUTEX);
            break;
        case 0xFE:
            OP_INC(AddrMode::ABSOLUTEX);
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
            OP_NOP(AddrMode::IMPLICIT);
            break;
        default:
            OP_KIL();
            break;
    }
}


uint64_t NESDL_CPU::MillisecondsToCPUCycles(double ms)
{
	return (NESDL_CPU_CLOCK / 1000) * ms;
}

double NESDL_CPU::CPUCyclesToMilliseconds(uint64_t cycles)
{
	double clockToSecs = 1 / (double)NESDL_CPU_CLOCK;
    return clockToSecs * cycles;
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
//    uint8_t result = 0;
    
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
            result->value = core->ram->ReadByte(addr + registers.x);
            result->address = addr;
            break;
        }
        case AddrMode::ZEROPAGEY:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            result->value = core->ram->ReadByte(addr + registers.y);
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
            result->value = core->ram->ReadByte(addr + registers.x);
            registers.pc += 2;
            result->address = addr;
            break;
        }
        case AddrMode::ABSOLUTEY:
        {
            uint16_t addr = core->ram->ReadWord(registers.pc);
            result->value = core->ram->ReadByte(addr + registers.y);
            registers.pc += 2;
            result->address = addr;
            break;
        }
        case AddrMode::INDIRECTX:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            uint16_t lsb = core->ram->ReadByte(addr + registers.x);
            uint16_t hsb = core->ram->ReadByte(addr + registers.x + 1) << 8;
            result->value = core->ram->ReadByte(hsb + lsb);
            result->address = addr;
            break;
        }
        case AddrMode::INDIRECTY:
        {
            uint8_t addr = core->ram->ReadByte(registers.pc++);
            uint16_t lsb = core->ram->ReadByte(addr);
            uint16_t hsb = (core->ram->ReadByte(addr + 1) << 8) + registers.y;
            // Trigger an "oops" if this mode crossed page bounds
            core->ram->oops = (addr % 0x100) == ((addr+1) % 0x100);
            result->value = core->ram->ReadByte(hsb + lsb);
            result->address = addr;
            break;
        }
    }
}

// Advances the CPU cycle count for most address modes with a consistent cycle count
// (With the exception of IMPLICIT as it's specific per-opcode)
void NESDL_CPU::AdvanceCyclesForAddressMode(AddrMode mode, bool pageCross, bool extraCycles, bool relSuccess)
{
    switch (mode)
    {
        case IMPLICIT:
            // Do nothing
            break;
        case ZEROPAGE:
            elapsedCycles += 3 + (extraCycles ? 2 : 0);
            break;
        case ZEROPAGEX:
        case ZEROPAGEY:
            elapsedCycles += 4 + (extraCycles ? 2 : 0);
            break;
        case ABSOLUTE:
            elapsedCycles += 4 + (extraCycles ? 2 : 0); // One exception being JMP (3/5 cycles)
            break;
        case ABSOLUTEX:
            elapsedCycles += 4 + (extraCycles ? 3 : pageCross);
            break;
        case ABSOLUTEY:
            elapsedCycles += 4 + pageCross;
            break;
        case RELATIVE:
            elapsedCycles += 2 + pageCross + relSuccess;
            break;
        case ACCUMULATOR:
        case IMMEDIATE:
            elapsedCycles += 2;
            break;
        case INDIRECTX:
            elapsedCycles += 6;
            break;
        case INDIRECTY:
            elapsedCycles += 5 + pageCross;
            break;
    }
}

void NESDL_CPU::OP_ADC(AddrMode mode)
{
    uint8_t oldAcc = registers.a;
    uint8_t carry = registers.p & PSTATUS_CARRY;
    
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    uint8_t val = addrModeResult->value;
    uint8_t result = oldAcc + val + carry;
    registers.a = result;
    
    // If an overflow or underflow was detected, set appropriate flags
    if (sign((int8_t)oldAcc) == sign((int8_t)val) && val != 0)
    {
        if (sign((int8_t)oldAcc) == -sign((int8_t)result))
        {
            SetPSFlag(PSTATUS_CARRY & PSTATUS_OVERFLOW, true);
        }
    }
    SetPSFlag(PSTATUS_ZERO, result == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)result) < 0);
}

void NESDL_CPU::OP_AND(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    uint8_t val = addrModeResult->value;
    registers.a &= val;
    
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
}

void NESDL_CPU::OP_ASL(AddrMode mode)
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
    AdvanceCyclesForAddressMode(mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x80) >> 6); // Set to old value bit 7
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_BCC(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_CARRY) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BCS(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_CARRY) == PSTATUS_CARRY)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BEQ(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_ZERO) == PSTATUS_ZERO)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BIT(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, false, false, false);
    uint8_t result = registers.a & addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, result == 0);
    SetPSFlag(PSTATUS_OVERFLOW, (result & PSTATUS_OVERFLOW) >> 5);
    SetPSFlag(PSTATUS_NEGATIVE, (result & PSTATUS_NEGATIVE) >> 6);
}

void NESDL_CPU::OP_BMI(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_NEGATIVE) == PSTATUS_NEGATIVE)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BNE(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_ZERO) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BPL(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_NEGATIVE) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BRK(AddrMode mode)
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
    elapsedCycles += 7;
}

void NESDL_CPU::OP_BVC(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_OVERFLOW) == 0x0)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BVS(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    uint16_t oldAddr = registers.pc;
    uint16_t newAddr = oldAddr + addrModeResult->value;
    bool newPage = (oldAddr / 0x100) != (newAddr / 0x100);
    bool didBranch = false;
    
    // After fetching opcode and offset, check if we branch
    if ((registers.p & PSTATUS_OVERFLOW) == PSTATUS_OVERFLOW)
    {
        registers.pc = newAddr;
        didBranch = true;
    }
    
    AdvanceCyclesForAddressMode(mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_CLC(AddrMode mode)
{
    SetPSFlag(PSTATUS_CARRY, false);
    elapsedCycles += 2; // These take two cycles!?
}

void NESDL_CPU::OP_CLD(AddrMode mode)
{
    SetPSFlag(PSTATUS_DECIMAL, false);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_CLI(AddrMode mode)
{
    SetPSFlag(PSTATUS_INTERRUPTDISABLE, false);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_CLV(AddrMode mode)
{
    SetPSFlag(PSTATUS_OVERFLOW, false);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_CMP(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.a >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.a == addrModeResult->value);
    SetPSFlag(PSTATUS_NEGATIVE, registers.a < addrModeResult->value);
}

void NESDL_CPU::OP_CPX(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.x >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.x == addrModeResult->value);
    SetPSFlag(PSTATUS_NEGATIVE, registers.x < addrModeResult->value);
}

void NESDL_CPU::OP_CPY(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.y >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.y == addrModeResult->value);
    SetPSFlag(PSTATUS_NEGATIVE, registers.y < addrModeResult->value);
}

void NESDL_CPU::OP_DEC(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, false, true, false);
    
    uint8_t val = addrModeResult->value - 1;
    core->ram->WriteByte(addrModeResult->address, val);
    
    SetPSFlag(PSTATUS_ZERO, val == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)val) < 0);
}

void NESDL_CPU::OP_DEX(AddrMode mode)
{
    registers.x--;
    elapsedCycles += 2;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
}

void NESDL_CPU::OP_DEY(AddrMode mode)
{
    registers.y--;
    elapsedCycles += 2;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
}

void NESDL_CPU::OP_EOR(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    registers.a ^= addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_INC(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, false, true, false);
    
    uint8_t val = addrModeResult->value + 1;
    core->ram->WriteByte(addrModeResult->address, val);
    
    SetPSFlag(PSTATUS_ZERO, val == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)val) < 0);
}

void NESDL_CPU::OP_INX(AddrMode mode)
{
    registers.x++;
    elapsedCycles += 2;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
}

void NESDL_CPU::OP_INY(AddrMode mode)
{
    registers.y++;
    elapsedCycles += 2;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
}

void NESDL_CPU::OP_JMP(bool indirect)
{
    // TODO implement bug where a fetch at a page bounds 0x##FF
    // grabs the LSB at FF but HSB is grabbed at 0x##00 (aka a wrap but always)
    uint16_t addr = core->ram->ReadWord(registers.pc);
    if (indirect)
    {
        addr = core->ram->ReadWord(addr);
        elapsedCycles += 5;
    }
    else
    {
        elapsedCycles += 3;
    }
    registers.pc = addr;
}

void NESDL_CPU::OP_JSR(AddrMode mode)
{
    // Retrieve the address (operand)
    uint16_t addr = core->ram->ReadWord(registers.pc);
    
    // Push this PC onto the stack to return to later
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (registers.pc >> 8));
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.pc);
    
    // Overwrite the PC with the address
    registers.pc = addr;
    
    elapsedCycles += 6;
}

void NESDL_CPU::OP_LDA(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    registers.a = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_LDX(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    registers.x = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
}

void NESDL_CPU::OP_LDY(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    registers.y = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
}

void NESDL_CPU::OP_LSR(AddrMode mode)
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
    AdvanceCyclesForAddressMode(mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x01)); // Set to old value bit 0
    SetPSFlag(PSTATUS_ZERO, value == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_NOP(AddrMode mode)
{
    elapsedCycles += 2; // nop nop
}

void NESDL_CPU::OP_ORA(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    registers.a |= addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_PHA(AddrMode mode)
{
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.a);
    elapsedCycles += 3;
}

void NESDL_CPU::OP_PHP(AddrMode mode)
{
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p);
    elapsedCycles += 3;
}

void NESDL_CPU::OP_PLA(AddrMode mode)
{
    uint8_t stackVal = core->ram->ReadByte(STACK_PTR + (registers.sp++));
    registers.a = stackVal;
    SetPSFlag(PSTATUS_ZERO, stackVal == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)stackVal) < 0);
    elapsedCycles += 4;
}

void NESDL_CPU::OP_PLP(AddrMode mode)
{
    uint8_t stackVal = core->ram->ReadByte(STACK_PTR + (registers.sp++));
    registers.p = stackVal;
    SetPSFlag(PSTATUS_ZERO, stackVal == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)stackVal) < 0);
    elapsedCycles += 4;
}

void NESDL_CPU::OP_ROL(AddrMode mode)
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
    AdvanceCyclesForAddressMode(mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x80) >> 6); // Set to old value bit 7
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_ROR(AddrMode mode)
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
    AdvanceCyclesForAddressMode(mode, false, true, false);
    
    SetPSFlag(PSTATUS_CARRY, (oldValue & 0x01)); // Set to old value bit 0
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_RTI(AddrMode mode)
{
    // Pull P from stack
    uint8_t p = core->ram->ReadByte(STACK_PTR + (registers.sp++));
    registers.p = p;
    // Pull PC from stack
    uint16_t pc = core->ram->ReadByte(STACK_PTR + (registers.sp++));
    pc += core->ram->ReadByte(STACK_PTR + (registers.sp++)) << 8;
    registers.pc = pc;
    elapsedCycles += 6;
}

void NESDL_CPU::OP_RTS(AddrMode mode)
{
    // Pull PC from stack
    uint16_t pc = core->ram->ReadByte(STACK_PTR + (registers.sp++));
    pc += core->ram->ReadByte(STACK_PTR + (registers.sp++)) << 8;
    registers.pc = pc;
    elapsedCycles += 6;
}

void NESDL_CPU::OP_SBC(AddrMode mode)
{
    uint8_t oldAcc = registers.a;
    uint8_t carry = registers.p & PSTATUS_CARRY;
    
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, core->ram->oops, false, false);
    
    uint8_t val = addrModeResult->value;
    uint8_t result = oldAcc - val - (1 - carry);
    registers.a = result;
    
    // If an overflow or underflow was detected, set appropriate flags
    if (sign((int8_t)oldAcc) == sign((int8_t)val) && val != 0)
    {
        if (sign((int8_t)oldAcc) == -sign((int8_t)result))
        {
            SetPSFlag(PSTATUS_CARRY & PSTATUS_OVERFLOW, true);
        }
    }
    SetPSFlag(PSTATUS_ZERO, result == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)result) < 0);
}

void NESDL_CPU::OP_SEC(AddrMode mode)
{
    SetPSFlag(PSTATUS_CARRY, true);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_SED(AddrMode mode)
{
    SetPSFlag(PSTATUS_DECIMAL, true);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_SEI(AddrMode mode)
{
    SetPSFlag(PSTATUS_INTERRUPTDISABLE, true);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_STA(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, true, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.a);
}

void NESDL_CPU::OP_STX(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, false, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.x);
}

void NESDL_CPU::OP_STY(AddrMode mode)
{
    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(mode, false, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.y);
}

void NESDL_CPU::OP_TAX(AddrMode mode)
{
    registers.x = registers.a;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_TAY(AddrMode mode)
{
    registers.y = registers.a;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_TSX(AddrMode mode)
{
    registers.x = registers.sp;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_TXA(AddrMode mode)
{
    registers.a = registers.x;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_TXS(AddrMode mode)
{
    registers.sp = registers.x;
    elapsedCycles += 2;
}

void NESDL_CPU::OP_TYA(AddrMode mode)
{
    registers.a = registers.y;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    elapsedCycles += 2;
}

void NESDL_CPU::OP_KIL()
{
    throw domain_error("ILLEGAL OPCODE");
}
