#include "NESDL.h"


void NESDL_CPU::Init(NESDL_Core* c)
{
    core = c;
    addrModeResult = new AddressModeResult();
}

void NESDL_CPU::Reset(bool hardReset)
{
    if (hardReset)
    {
        elapsedCycles = 0;
        registers.a = 0;
        registers.x = 0;
        registers.y = 0;
        registers.sp = 0x00;
    }
    registers.pc = ADDR_RESET;
    registers.sp -= 3;

    // Ready P register in a unique way
    if (hardReset)
    {
        registers.p = PSTATUS_INTERRUPTDISABLE | PSTATUS_UNDEFINED;
    }
    else
    {
        registers.p = registers.p & PSTATUS_INTERRUPTDISABLE;
    }

    nmi = false;
    delayNMI = false;
    irq = false;

    // With the CPU state reset, prime the CPU for program execution
    // CPU warms up for 7 cycles and fetches the start address to PC
    ppuCycleCounter = 0;
    registers.pc = core->ram->ReadWord(registers.pc);
    elapsedCycles += 7;

    // Get how long our next (first) instruction will take
    // (Also primes addrModeResult for debugging)
    nextInstructionPPUCycles = GetCyclesForNextInstruction() * 3;

    nintendulatorLogIndex = 0;
}

void NESDL_CPU::Update(uint32_t ppuCycles)
{
    while (ppuCycleCounter >= 0)
    {
        if (delayedDMA)
        {
            delayedDMA = false;
            HaltCPUForDMAWrite();
        }
        else
        {
            uint64_t prevElapsedCycles = elapsedCycles;
            if (dma)
            {
                dma = false;
                delayedDMA = true;
            }
            
            // Hack - we can predict a VBL occuring during this instruction, since CPU runs instructions as a whole
            // but PPU timing for VBL/NMI is more granular than this
            // GetCyclesForNextInstruction prepped our addrModeResult address, we just need to check it
            if (addrModeResult->address >= 0x2000)
            {
                uint16_t addr = 0x2000 + (addrModeResult->address % 0x8);
                if (addr == PPU_PPUSTATUS)
                {
                    core->ppu->PreprocessPPUForReadInstructionTiming(nextInstructionPPUCycles);
                }
            }
            
            EvaluateNintendulatorDebug();

            // Debug Nintendulator format
            // printf("\n%s", DebugMakeCurrentStateLine().c_str());

            bool wasNMIFiredInTime = core->ppu->elapsedCycles - 3 >= core->ppu->nmiFiredAt;
            if (nmi && !delayNMI && wasNMIFiredInTime)
            {
                irq = false;
                nmi = false;
                nmiFired = true;
            }
            delayNMI = false;

            // HACK for IRQ timing - PPU IRQ goes low JUST before CPU opcode fetch happens, this simulates that
            bool wasIRQFiredExactly = core->ppu->elapsedCycles - 3 == core->ppu->irqFiredAt;
            if (irq && wasIRQFiredExactly && (registers.p & PSTATUS_INTERRUPTDISABLE) == 0)
            {
                irq = false;
                irqFired = true;
            }

            uint8_t ppuCyclesElapsed = nextInstructionPPUCycles;
            if (nmiFired)
            {
                nmiFired = false;
                NMI();
            }
            else if (irqFired)
            {
                irqFired = false;
                IRQ();
            }
            else
            {
                ppuCycleCounter -= nextInstructionPPUCycles;
                RunNextInstruction();
            }

            // Handle IRQ line pulled low after instruction (next instruction may be interrupted)
            if (irq)
            {
                bool wasIRQFiredInTime = core->ppu->elapsedCycles + ppuCyclesElapsed - 3 >= core->ppu->irqFiredAt;
                bool irqReadyToFire = wasIRQFiredInTime && (registers.p & PSTATUS_INTERRUPTDISABLE) == 0;

                if (irqReadyToFire)
                {
                    irq = false;
                    irqFired = true;
                }
            }

            // Set P register after IRQ check (this timing is important for IRQ accuracy)
            if (iFlagReady)
            {
                iFlagReady = false;
                registers.p = iFlagNextSetState;
            }

            // Figure out how long our new next instruction will take
            nextInstructionPPUCycles = GetCyclesForNextInstruction() * 3;

            wasLastInstructionAMapperWrite = didMapperWrite;
            didMapperWrite = false;
        }
    }
    ppuCycleCounter += ppuCycles;
    
    // Used for step debugging
    nextInstructionReady = (ppuCycleCounter >= 0);
}

uint8_t NESDL_CPU::GetCyclesForNextInstruction()
{
    // Run next instruction to see how long it'll take, then reset the CPU state
    CPURegisters regState = registers;
    uint64_t prevElapsedCycles = elapsedCycles;
    // TODO
    // Well... a better solution to this would be nice. It helps to simulate the
    // next instruction to figure out timing, but WOW what a mess this is.
    ignoreChanges = true;
    RunNextInstruction();
    ignoreChanges = false;
    registers = regState;
    uint8_t result = (uint8_t)(elapsedCycles - prevElapsedCycles); // No cycle counting ever goes above 255
    elapsedCycles = prevElapsedCycles;
    return result;
}

void NESDL_CPU::RunNextInstruction()
{
    // Get next instruction from memory according to current program counter
    uint8_t opcode = core->ram->ReadByte(registers.pc++);

    switch (CPU_OPCODES[opcode])
    {
        case ADC:
            OP_ADC(opcode);
            break;
        case AND:
            OP_AND(opcode);
            break;
        case ASL:
            OP_ASL(opcode);
            break;
        case BCC:
            OP_BCC(opcode);
            break;
        case BCS:
            OP_BCS(opcode);
            break;
        case BEQ:
            OP_BEQ(opcode);
            break;
        case BIT:
            OP_BIT(opcode);
            break;
        case BMI:
            OP_BMI(opcode);
            break;
        case BNE:
            OP_BNE(opcode);
            break;
        case BPL:
            OP_BPL(opcode);
            break;
        case BRK:
            OP_BRK(opcode);
            break;
        case BVC:
            OP_BVC(opcode);
            break;
        case BVS:
            OP_BVS(opcode);
            break;
        case CLC:
            OP_CLC(opcode);
            break;
        case CLD:
            OP_CLD(opcode);
            break;
        case CLI:
            OP_CLI(opcode);
            break;
        case CLV:
            OP_CLV(opcode);
            break;
        case CMP:
            OP_CMP(opcode);
            break;
        case CPX:
            OP_CPX(opcode);
            break;
        case CPY:
            OP_CPY(opcode);
            break;
        case DEC:
            OP_DEC(opcode);
            break;
        case DEX:
            OP_DEX(opcode);
            break;
        case DEY:
            OP_DEY(opcode);
            break;
        case EOR:
            OP_EOR(opcode);
            break;
        case INC:
            OP_INC(opcode);
            break;
        case INX:
            OP_INX(opcode);
            break;
        case INY:
            OP_INY(opcode);
            break;
        case JMP:
            OP_JMP(opcode, opcode == 0x6C);
            break;
        case JSR:
            OP_JSR(opcode);
            break;
        case LDA:
            OP_LDA(opcode);
            break;
        case LDX:
            OP_LDX(opcode);
            break;
        case LDY:
            OP_LDY(opcode);
            break;
        case LSR:
            OP_LSR(opcode);
            break;
        case NOP:
            OP_NOP(opcode);
            break;
        case ORA:
            OP_ORA(opcode);
            break;
        case PHA:
            OP_PHA(opcode);
            break;
        case PHP:
            OP_PHP(opcode);
            break;
        case PLA:
            OP_PLA(opcode);
            break;
        case PLP:
            OP_PLP(opcode);
            break;
        case ROL:
            OP_ROL(opcode);
            break;
        case ROR:
            OP_ROR(opcode);
            break;
        case RTI:
            OP_RTI(opcode);
            break;
        case RTS:
            OP_RTS(opcode);
            break;
        case SBC:
            OP_SBC(opcode);
            break;
        case SEC:
            OP_SEC(opcode);
            break;
        case SED:
            OP_SED(opcode);
            break;
        case SEI:
            OP_SEI(opcode);
            break;
        case STA:
            OP_STA(opcode);
            break;
        case STX:
            OP_STX(opcode);
            break;
        case STY:
            OP_STY(opcode);
            break;
        case TAX:
            OP_TAX(opcode);
            break;
        case TAY:
            OP_TAY(opcode);
            break;
        case TSX:
            OP_TSX(opcode);
            break;
        case TXA:
            OP_TXA(opcode);
            break;
        case TXS:
            OP_TXS(opcode);
            break;
        case TYA:
            OP_TYA(opcode);
            break;
        case KIL:
            OP_KIL();
            break;
    }
}

// Returns 1 if the value > 0, -1 if the value is < 0, and 0 if the value is 0.
template <typename T> int8_t sign(T val) {
    return (T(0) < val) - (val < T(0));
}


void NESDL_CPU::SetPSFlag(uint8_t flag, bool on)
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
        case AddrMode::RELATIVEADDR:
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
        case AddrMode::ABSOLUTEADDR:
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
            // Trigger an "oops" if this mode crossed page bounds
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
            // Trigger an "oops" if this mode crossed page bounds
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

void NESDL_CPU::AdvanceCyclesForAddressMode(uint8_t opcode, AddrMode mode, bool pageCross, bool extraCycles, bool relSuccess)
{
    if (opcode == 0x4C)
    {
        elapsedCycles += 3;
        return;
    }
    if (opcode == 0x6C)
    {
        elapsedCycles += 5;
        return;
    }
    switch (mode)
    {
        case IMPLICIT:
            // Handle it on a per-instruction basis
            switch (opcode)
            {
                case 0x08:
                case 0x48:
                    elapsedCycles += 3;
                    break;
                case 0x28:
                case 0x68:
                    elapsedCycles += 4;
                    break;
                case 0x40:
                case 0x60:
                    elapsedCycles += 6;
                    break;
                case 0x00:
                    elapsedCycles += 7;
                    break;
                default:
                    elapsedCycles += 2;
                    break;
            }
            break;
        case ZEROPAGE:
            elapsedCycles += 3 + (extraCycles ? 2 : 0);
            break;
        case ZEROPAGEX:
        case ZEROPAGEY:
            elapsedCycles += 4 + (extraCycles ? 2 : 0);
            break;
        case ABSOLUTEADDR:
            elapsedCycles += 4 + (extraCycles ? 2 : 0); // One exception being JMP (3/5 cycles)
            break;
        case ABSOLUTEX:
            elapsedCycles += 4 + (extraCycles ? 3 : pageCross);
            break;
        case ABSOLUTEY:
            elapsedCycles += 4 + pageCross;
            break;
        case RELATIVEADDR:
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

void NESDL_CPU::OP_ADC(uint8_t opcode, bool sbc)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    // Add A, M and C together to get the result of addition with carry
    uint8_t oldAcc = registers.a;
    uint8_t val = addrModeResult->value;
    if (sbc)
    {
        val = ~val;
    }
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

void NESDL_CPU::OP_AND(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    uint8_t val = addrModeResult->value;
    registers.a &= val;
    
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
}

void NESDL_CPU::OP_ASL(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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

void NESDL_CPU::OP_BCC(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BCS(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BEQ(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BIT(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    uint8_t result = registers.a & addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, result == 0);
    SetPSFlag(PSTATUS_OVERFLOW, (addrModeResult->value & PSTATUS_OVERFLOW) >> 5);
    SetPSFlag(PSTATUS_NEGATIVE, (addrModeResult->value & PSTATUS_NEGATIVE) >> 6);
}

void NESDL_CPU::OP_BMI(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BNE(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BPL(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BRK(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    // Push PC onto stack in two bytes (high then low since we're going backwards)
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (registers.pc >> 8));
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (uint8_t)registers.pc);
    // Push P onto stack
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p);
    // Read interrupt vector from FFFE/F into PC
    uint16_t lsb = core->ram->ReadByte(ADDR_IRQ);
    uint16_t hsb = core->ram->ReadByte(ADDR_IRQ+1) << 8;
    registers.pc = hsb + lsb;
    // Set break flag
    SetPSFlag(PSTATUS_BREAK, true);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_BVC(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_BVS(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    
    AdvanceCyclesForAddressMode(opcode, mode, newPage, false, didBranch);
}

void NESDL_CPU::OP_CLC(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    SetPSFlag(PSTATUS_CARRY, false);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_CLD(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    SetPSFlag(PSTATUS_DECIMAL, false);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_CLI(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    //SetPSFlag(PSTATUS_INTERRUPTDISABLE, false);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    if (ignoreChanges == false)
    {
        iFlagReady = true;
        iFlagNextSetState = registers.p & (~PSTATUS_INTERRUPTDISABLE);
    }
}

void NESDL_CPU::OP_CLV(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    SetPSFlag(PSTATUS_OVERFLOW, false);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_CMP(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.a >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.a == addrModeResult->value);
    int8_t result = registers.a - addrModeResult->value;
    SetPSFlag(PSTATUS_NEGATIVE, result < 0);
}

void NESDL_CPU::OP_CPX(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.x >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.x == addrModeResult->value);
    int8_t result = registers.x - addrModeResult->value;
    SetPSFlag(PSTATUS_NEGATIVE, result < 0);
}

void NESDL_CPU::OP_CPY(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    SetPSFlag(PSTATUS_CARRY, registers.y >= addrModeResult->value);
    SetPSFlag(PSTATUS_ZERO, registers.y == addrModeResult->value);
    int8_t result = registers.y - addrModeResult->value;
    SetPSFlag(PSTATUS_NEGATIVE, result < 0);
}

void NESDL_CPU::OP_DEC(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    uint8_t val = addrModeResult->value - 1;
    core->ram->WriteByte(addrModeResult->address, val);
    
    SetPSFlag(PSTATUS_ZERO, val == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)val) < 0);
}

void NESDL_CPU::OP_DEX(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.x--;

    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_DEY(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.y--;
    
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_EOR(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.a ^= addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_INC(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
    
    uint8_t val = addrModeResult->value + 1;
    core->ram->WriteByte(addrModeResult->address, val);
    
    SetPSFlag(PSTATUS_ZERO, val == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)val) < 0);
}

void NESDL_CPU::OP_INX(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.x++;
    
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_INY(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.y++;
    
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_JMP(uint8_t opcode, bool indirect)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
        AdvanceCyclesForAddressMode(opcode, AddrMode::ABSOLUTEADDR, false, false, false);
    }
    else
    {
        AdvanceCyclesForAddressMode(opcode, AddrMode::IMPLICIT, false, false, false);
    }
    registers.pc = addr;
}

void NESDL_CPU::OP_JSR(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    // Retrieve the address (operand) and advance PC
    uint16_t addr = core->ram->ReadWord(registers.pc++);
    
    // Push this PC onto the stack to return to later
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (registers.pc >> 8));
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (uint8_t)registers.pc);
    
    // Overwrite the PC with the address
    registers.pc = addr;
    
    AdvanceCyclesForAddressMode(opcode, mode, false, true, false);
}

void NESDL_CPU::OP_LDA(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.a = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_LDX(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.x = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
}

void NESDL_CPU::OP_LDY(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.y = addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
}

void NESDL_CPU::OP_LSR(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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

void NESDL_CPU::OP_NOP(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    // Special case for nop - need to clear AddrModeResult values
    addrModeResult->address = 0;
    addrModeResult->value = 0;

    // nop nop
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_ORA(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    GetByteForAddressMode(mode, addrModeResult);
    AdvanceCyclesForAddressMode(opcode, mode, core->ram->oops, false, false);
    
    registers.a |= addrModeResult->value;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
}

void NESDL_CPU::OP_PHA(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.a);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_PHP(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    // Break flag pushes as 1 but physically isn't a stored bit (status is a 6 bit register)
    // Additionally, mysterious bit 6 ("undefined") always pushes to stack as 1 too
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p | PSTATUS_BREAK | PSTATUS_UNDEFINED);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_PLA(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    uint8_t stackVal = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    registers.a = stackVal;
    SetPSFlag(PSTATUS_ZERO, stackVal == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)stackVal) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_PLP(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    uint8_t stackVal = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    //registers.p = stackVal & 0xEF; // "B flag and extra bit are ignored" - NESDEV (but Nintendulator only ignores B)
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    if (ignoreChanges == false)
    {
        iFlagReady = true;
        iFlagNextSetState = stackVal & 0xEF; // "B flag and extra bit are ignored" - NESDEV (but Nintendulator only ignores B)
    }
}

void NESDL_CPU::OP_ROL(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    SetPSFlag(PSTATUS_ZERO, value == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_ROR(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    SetPSFlag(PSTATUS_ZERO, value == 0);
    SetPSFlag(PSTATUS_NEGATIVE, (value & 0x80) >> 6); // Set to new value bit 7
}

void NESDL_CPU::OP_RTI(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    // Pull P from stack
    uint8_t p = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    registers.p = p;
    // Pull PC from stack
    uint16_t pc = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    pc += core->ram->ReadByte(STACK_PTR + (++registers.sp)) << 8;
    registers.pc = pc;
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_RTS(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    // Pull PC from stack
    uint16_t pc = core->ram->ReadByte(STACK_PTR + (++registers.sp));
    pc += core->ram->ReadByte(STACK_PTR + (++registers.sp)) << 8;
    registers.pc = pc+1; // Not sure why we advance PC once but it seems to work out that way
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_SBC(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    // SBC is the exact same as ADC except the value for M is bit-flipped
    OP_ADC(opcode, true);
}

void NESDL_CPU::OP_SEC(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    SetPSFlag(PSTATUS_CARRY, true);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_SED(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    SetPSFlag(PSTATUS_DECIMAL, true);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_SEI(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    //SetPSFlag(PSTATUS_INTERRUPTDISABLE, true);
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    if (ignoreChanges == false)
    {
        iFlagReady = true;
        iFlagNextSetState = registers.p | PSTATUS_INTERRUPTDISABLE;
    }
}

void NESDL_CPU::OP_STA(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    core->ppu->incrementV = false;
    core->ppu->isWriting = true;
    GetByteForAddressMode(mode, addrModeResult);
    core->ppu->incrementV = true;
    core->ppu->isWriting = false;
    AdvanceCyclesForAddressMode(opcode, mode, true, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.a);
}

void NESDL_CPU::OP_STX(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    core->ppu->incrementV = false;
    core->ppu->isWriting = true;
    GetByteForAddressMode(mode, addrModeResult);
    core->ppu->incrementV = true;
    core->ppu->isWriting = false;
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.x);
}

void NESDL_CPU::OP_STY(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    core->ppu->incrementV = false;
    core->ppu->isWriting = true;
    GetByteForAddressMode(mode, addrModeResult);
    core->ppu->incrementV = true;
    core->ppu->isWriting = false;
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
    
    core->ram->WriteByte(addrModeResult->address, registers.y);
}

void NESDL_CPU::OP_TAX(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.x = registers.a;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TAY(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.y = registers.a;
    SetPSFlag(PSTATUS_ZERO, registers.y == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.y) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TSX(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.x = registers.sp;
    SetPSFlag(PSTATUS_ZERO, registers.x == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.x) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TXA(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.a = registers.x;
    SetPSFlag(PSTATUS_ZERO, registers.a == 0);
    SetPSFlag(PSTATUS_NEGATIVE, sign((int8_t)registers.a) < 0);
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TXS(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

    registers.sp = registers.x;
    
    AdvanceCyclesForAddressMode(opcode, mode, false, false, false);
}

void NESDL_CPU::OP_TYA(uint8_t opcode)
{
    AddrMode mode = CPU_ADDRMODES[opcode];

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
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (uint8_t)registers.pc);
    
    // Set break flag (before pushing P to stack)
    SetPSFlag(PSTATUS_BREAK, false);
    
    // Push P to stack
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p);
    
    // Set interrupt flag
    SetPSFlag(PSTATUS_INTERRUPTDISABLE, true);
    
    // Advance to address specified at NMI location (0xFFFA)
    registers.pc = core->ram->ReadWord(ADDR_NMI);
    
    // Advance 7 cycles
    elapsedCycles += 7;
    ppuCycleCounter = -21; // 7 cycles * 3
}

void NESDL_CPU::IRQ()
{
    // Push PC to stack
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (registers.pc >> 8));
    core->ram->WriteByte(STACK_PTR + (registers.sp--), (uint8_t)registers.pc);

    // Set break flag (before pushing P to stack)
    SetPSFlag(PSTATUS_BREAK, false);

    // Push P to stack
    core->ram->WriteByte(STACK_PTR + (registers.sp--), registers.p);

    // Set interrupt flag
    SetPSFlag(PSTATUS_INTERRUPTDISABLE, true);

    // Advance to address specified at IRQ location (0xFFFE)
    registers.pc = core->ram->ReadWord(ADDR_IRQ);

    // Advance 7 cycles
    elapsedCycles += 7;
    ppuCycleCounter = -21; // 7 cycles * 3
}

void NESDL_CPU::HaltCPUForDMAWrite()
{
    uint16_t delay = elapsedCycles % 2 == 0 ? 514 : 513;
    elapsedCycles += delay;
    ppuCycleCounter -= delay*3;
}

void NESDL_CPU::HaltCPUForDMC()
{
    uint8_t delay = elapsedCycles % 2 == 0 ? 3 : 4;
    elapsedCycles += delay;
    ppuCycleCounter -= delay*3;
}

void NESDL_CPU::DidMapperWrite()
{
    didMapperWrite = true;
}

bool NESDL_CPU::IsConsecutiveMapperWrite()
{
    return wasLastInstructionAMapperWrite;
}

void NESDL_CPU::DebugBindNintendulator(const char* path)
{
    if (nintendulatorDebugging)
    {
        nintendulatorLog.clear();
    }

    printf("Attaching Nintendulator log: %s\n", path);

    ifstream logStream;
    logStream.open(path);
    
    string line;
    uint64_t lineCount = 0;
    while (getline(logStream, line))
    {
        nintendulatorLog.push_back(line);
        lineCount++;
    }

    logStream.close();

    nintendulatorDebugging = true;
    nintendulatorLogIndex = 0;
    nintendulatorLogCount = lineCount;

    printf("Attachment done! (%llu lines)", lineCount);
}

void NESDL_CPU::DebugUnbindNintendulator()
{
    if (nintendulatorDebugging)
    {
        nintendulatorLog.clear();
        nintendulatorDebugging = false;
        printf("Detached Nintendulator log.");
    }
}

string NESDL_CPU::DebugMakeCurrentStateLine()
{
    static stringstream returnStr;
    returnStr.str("");

    // Memory reads cause side effects, we need to flag that we don't want those
    ignoreChanges = true;

    // Something to help us is the cached addrModeResult from simulating the next instruction.
    // Our memory fetches were already done for us!

    returnStr << string_format("%04X  ", registers.pc);
    uint8_t opcode = core->ram->ReadByte(registers.pc);
    uint8_t param0 = core->ram->ReadByte(registers.pc+1);
    uint8_t param1 = core->ram->ReadByte(registers.pc+2);
    const char* opcodeStr = CPU_OPCODE_STR[CPU_OPCODES[opcode]];
    uint16_t readAddr = addrModeResult->address;
    uint8_t readValue = addrModeResult->value;

    AddrMode nextInstructionMode = CPU_ADDRMODES[opcode];

    // Nintendulator is particular about reads to some addresses like PPU, controller & some cartridge
    // the "actual" value isn't retrieved, but some underlying bytes in RAM. I've never seen it not be 0xFF
    if (nextInstructionMode == AddrMode::ABSOLUTEADDR ||
        nextInstructionMode == AddrMode::ABSOLUTEX ||
        nextInstructionMode == AddrMode::ABSOLUTEY)
    {
        uint16_t addr = ((uint16_t)param1 << 8) | param0;
        if ((addr >= 0x2000 && addr < 0x8000))
        {
            readValue = 0xFF;
        }
    }
    switch (nextInstructionMode)
    {
        case AddrMode::ACCUMULATOR:
        case AddrMode::IMPLICIT:
        {
            returnStr << string_format("%02X        ", opcode) << opcodeStr;
            if (opcode == 0x0A || opcode == 0x2A || opcode == 0x4A || opcode == 0x6A)
            {
                // Treat these no-arg mode (Accumulator) as unique cases, eg. "LSR A"
                returnStr << " A                           ";
            }
            else
            {
                returnStr << "                             ";
            }
            break;
        }
        case AddrMode::RELATIVEADDR:
        {
            // Add 2 since it's relative to PC after advancing 2 bytes (aka this instruction)
            returnStr << string_format("%02X %02X     ", opcode, param0) << opcodeStr;
            returnStr << string_format(" $%04X                       ", (registers.pc + (int8_t)readValue + 2));
            break;
        }
        case AddrMode::IMMEDIATE:
        {
            returnStr << string_format("%02X %02X     ", opcode, param0) << opcodeStr;
            returnStr << string_format(" #$%02X                        ", readValue);
            break;
        }
        case AddrMode::ZEROPAGE:
        {
            returnStr << string_format("%02X %02X     ", opcode, param0) << opcodeStr;
            returnStr << string_format(" $%02X = %02X                    ", param0, readValue);
            break;
        }
        case AddrMode::ZEROPAGEX:
        {
            returnStr << string_format("%02X %02X     ", opcode, param0) << opcodeStr;
            returnStr << string_format(" $%02X,X @ %02X = %02X             ", param0, readAddr, readValue);
            break;
        }
        case AddrMode::ZEROPAGEY:
        {
            returnStr << string_format("%02X %02X     ", opcode, param0) << opcodeStr;
            returnStr << string_format(" $%02X,Y @ %02X = %02X             ", param0, readAddr, readValue);
            break;
        }
        case AddrMode::ABSOLUTEADDR:
        {
            returnStr << string_format("%02X %02X %02X  ", opcode, param0, param1) << opcodeStr;
            if (opcode == 0x20 || opcode == 0x4C || opcode == 0x6C)
            {
                returnStr << string_format(" $%02X%02X                       ", param1, param0);
            }
            else
            {
                returnStr << string_format(" $%02X%02X = %02X                  ", param1, param0, readValue);
            }
            break;
        }
        case AddrMode::ABSOLUTEX:
        {
            returnStr << string_format("%02X %02X %02X  ", opcode, param0, param1) << opcodeStr;
            returnStr << string_format(" $%02X%02X,X @ %04X = %02X         ", param1, param0, readAddr, readValue);
            break;
        }
        case AddrMode::ABSOLUTEY:
        {
            returnStr << string_format("%02X %02X %02X  ", opcode, param0, param1) << opcodeStr;
            returnStr << string_format(" $%02X%02X,Y @ %04X = %02X         ", param1, param0, readAddr, readValue);
            break;
        }
        case AddrMode::INDIRECTX:
        {
            returnStr << string_format("%02X %02X     ", opcode, param0) << opcodeStr;
            returnStr << string_format(" ($%02X,X) = %04X @ %04X = %02X  ", param0, (readAddr - registers.x), readAddr, readValue);
            break;
        }
        case AddrMode::INDIRECTY:
        {
            returnStr << string_format("%02X %02X     ", opcode, param0) << opcodeStr;
            returnStr << string_format(" ($%02X),Y = %04X @ %04X = %02X  ", param0, (readAddr - registers.y), readAddr, readValue);
            break;
        }
    }

    returnStr << string_format("A:%02X X:%02X Y:%02X P:%02X SP:%02X PPU:%3d,%3d CYC:%llu", registers.a, registers.x, registers.y, registers.p, registers.sp, core->ppu->currentScanline, core->ppu->currentScanlineCycle, elapsedCycles);

    ignoreChanges = false;

    return returnStr.str();
}

void NESDL_CPU::EvaluateNintendulatorDebug()
{
    if (!nintendulatorDebugging)
    {
        return;
    }

    string ourCurrentLine = DebugMakeCurrentStateLine();
    //printf("\n%s", ourCurrentLine.c_str());
    if (nintendulatorLogIndex < nintendulatorLogCount)
    {
        // Construct and store CPU state before running next instruction

        // Compare state to Nintendulator line - if it's off, we can print
        // a line comparison and try to debug
        string nintendulatorLine = nintendulatorLog[nintendulatorLogIndex];

        // The "quickest" way I could think of validating against Nintendulator is
        // comparing string-to-string outright
        if (ourCurrentLine != nintendulatorLine)
        {
            printf("\nLines differ!\n  Ours:%s\nTheirs:%s\n", ourCurrentLine.c_str(), nintendulatorLine.c_str());
        }
        nintendulatorLogIndex++;
    }
    else if (nintendulatorLogIndex == nintendulatorLogCount)
    {
        printf("Nintendulator out of lines!\n");
    }
}