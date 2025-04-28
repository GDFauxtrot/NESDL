#pragma once

struct CPURegisters
{
	uint16_t pc; // Program Counter
	uint8_t sp; // Stack Pointer
	uint8_t a; // Accumulator
	uint8_t x; // Index X
	uint8_t y; // Index Y
	uint8_t p; // Processor (status)
};

// Bit masks for accessing the CPU processor status
#define PSTATUS_CARRY 0x1
#define PSTATUS_ZERO 0x2
#define PSTATUS_INTERRUPTDISABLE 0x4
#define PSTATUS_DECIMAL 0x8
#define PSTATUS_BREAK 0x10
#define PSTATUS_UNDEFINED 0x20
#define PSTATUS_OVERFLOW 0x40
#define PSTATUS_NEGATIVE 0x80

#define STACK_PTR 0x0100

#define ADDR_NMI 0xFFFA
#define ADDR_RESET 0xFFFC
#define ADDR_IRQ 0xFFFE

// 6502 instruction addressing modes
enum AddrMode { IMPLICIT, RELATIVEADDR, ACCUMULATOR, IMMEDIATE,
    ZEROPAGE, ZEROPAGEX, ZEROPAGEY, ABSOLUTEADDR, ABSOLUTEX, ABSOLUTEY,
    INDIRECTX, INDIRECTY };

// 6502 instructions
enum Opcode {
    // Official
    ADC, AND, ASL, BCC, BCS, BEQ, BIT, BMI, BNE, BPL, BRK, BVC, BVS, CLC,
    CLD, CLI, CLV, CMP, CPX, CPY, DEC, DEX, DEY, EOR, INC, INX, INY, JMP,
    JSR, LDA, LDX, LDY, LSR, NOP, ORA, PHA, PHP, PLA, PLP, ROL, ROR, RTI,
    RTS, SBC, SEC, SED, SEI, STA, STX, STY, TAX, TAY, TSX, TXA, TXS, TYA,
    // Unofficial/Kill
    KIL
};

// List of 6502 opcode strings, organized by Opcode enum value.
// (Opcode enum and this table must be aligned!)
static const char* CPU_OPCODE_STR[57] =
{
    // Official
    "ADC", "AND", "ASL", "BCC", "BCS", "BEQ", "BIT", "BMI", "BNE", "BPL", "BRK", "BVC", "BVS", "CLC",
    "CLD", "CLI", "CLV", "CMP", "CPX", "CPY", "DEC", "DEX", "DEY", "EOR", "INC", "INX", "INY", "JMP",
    "JSR", "LDA", "LDX", "LDY", "LSR", "NOP", "ORA", "PHA", "PHP", "PLA", "PLP", "ROL", "ROR", "RTI",
    "RTS", "SBC", "SEC", "SED", "SEI", "STA", "STX", "STY", "TAX", "TAY", "TSX", "TXA", "TXS", "TYA",
    // Unofficial/Kill
    "KIL"
};

struct AddressModeResult
{
    uint16_t address;
    uint8_t value;
};

struct DebugCPUState
{
    uint64_t elapsedCycles;
    CPURegisters registers;
    uint16_t ppuScanline;
    uint16_t ppuScanlineCycle;
    uint8_t nextOpcode;
    uint8_t nextParam0;
    uint8_t nextParam1;
    AddrMode nextAddrMode;
};

// List of 6502 instruction address modes, organized by byte value. Corresponds bytes with an AddrMode enum
const AddrMode CPU_ADDRMODES[0x100] =
{
    // 0x00
    AddrMode::IMPLICIT,     AddrMode::INDIRECTX,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,    AddrMode::ACCUMULATOR,  AddrMode::IMMEDIATE,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0x10
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,
    // 0x20
    AddrMode::ABSOLUTEADDR, AddrMode::INDIRECTX,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,
    AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,    AddrMode::ACCUMULATOR,  AddrMode::IMMEDIATE,
    AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0x30
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,
    // 0x40
    AddrMode::IMPLICIT,     AddrMode::INDIRECTX,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,    AddrMode::ACCUMULATOR,  AddrMode::IMMEDIATE,
    AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0x50
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,
    // 0x60
    AddrMode::IMPLICIT,     AddrMode::INDIRECTX,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,    AddrMode::ACCUMULATOR,  AddrMode::IMMEDIATE,
    AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0x70
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,
    // 0x80
    AddrMode::IMPLICIT,     AddrMode::INDIRECTX,    AddrMode::IMPLICIT,     AddrMode::INDIRECTX,
    AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMPLICIT,     AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,
    AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0x90
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEY,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,
    // 0xA0
    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,
    AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,
    AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0xB0
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEY,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEY,    AddrMode::ABSOLUTEX,
    // 0xC0
    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,    AddrMode::IMPLICIT,     AddrMode::INDIRECTX,
    AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,
    AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0xD0
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,
    // 0xE0
    AddrMode::IMMEDIATE,    AddrMode::INDIRECTX,    AddrMode::IMPLICIT,     AddrMode::INDIRECTX,
    AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,     AddrMode::ZEROPAGE,
    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,    AddrMode::IMPLICIT,     AddrMode::IMMEDIATE,
    AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR, AddrMode::ABSOLUTEADDR,
    // 0xF0
    AddrMode::RELATIVEADDR, AddrMode::INDIRECTY,    AddrMode::IMMEDIATE,    AddrMode::INDIRECTY,
    AddrMode::IMPLICIT,     AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,    AddrMode::ZEROPAGEX,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEY,
    AddrMode::IMPLICIT,     AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX,    AddrMode::ABSOLUTEX
};

// List of 6502 instructions, organized by byte value. Corresponds bytes with an Opcode enum
const Opcode CPU_OPCODES[0x100] =
{
    // 0x00
    Opcode::BRK, Opcode::ORA, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::ORA, Opcode::ASL, Opcode::KIL,
    Opcode::PHP, Opcode::ORA, Opcode::ASL, Opcode::KIL, Opcode::NOP, Opcode::ORA, Opcode::ASL, Opcode::KIL,
    // 0x10
    Opcode::BPL, Opcode::ORA, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::ORA, Opcode::ASL, Opcode::KIL,
    Opcode::CLC, Opcode::ORA, Opcode::NOP, Opcode::KIL, Opcode::NOP, Opcode::ORA, Opcode::ASL, Opcode::KIL,
    // 0x20
    Opcode::JSR, Opcode::AND, Opcode::KIL, Opcode::KIL, Opcode::BIT, Opcode::AND, Opcode::ROL, Opcode::KIL,
    Opcode::PLP, Opcode::AND, Opcode::ROL, Opcode::KIL, Opcode::BIT, Opcode::AND, Opcode::ROL, Opcode::KIL,
    // 0x30
    Opcode::BMI, Opcode::AND, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::AND, Opcode::ROL, Opcode::KIL,
    Opcode::SEC, Opcode::AND, Opcode::NOP, Opcode::KIL, Opcode::NOP, Opcode::AND, Opcode::ROL, Opcode::KIL,
    // 0x40
    Opcode::RTI, Opcode::EOR, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::EOR, Opcode::LSR, Opcode::KIL,
    Opcode::PHA, Opcode::EOR, Opcode::LSR, Opcode::KIL, Opcode::JMP, Opcode::EOR, Opcode::LSR, Opcode::KIL,
    // 0x50
    Opcode::BVC, Opcode::EOR, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::EOR, Opcode::LSR, Opcode::KIL,
    Opcode::CLI, Opcode::EOR, Opcode::NOP, Opcode::KIL, Opcode::NOP, Opcode::EOR, Opcode::LSR, Opcode::KIL,
    // 0x60
    Opcode::RTS, Opcode::ADC, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::ADC, Opcode::ROR, Opcode::KIL,
    Opcode::PLA, Opcode::ADC, Opcode::ROR, Opcode::KIL, Opcode::JMP, Opcode::ADC, Opcode::ROR, Opcode::KIL,
    // 0x70
    Opcode::BVS, Opcode::ADC, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::ADC, Opcode::ROR, Opcode::KIL,
    Opcode::SEI, Opcode::ADC, Opcode::NOP, Opcode::KIL, Opcode::NOP, Opcode::ADC, Opcode::ROR, Opcode::KIL,
    // 0x80
    Opcode::NOP, Opcode::STA, Opcode::NOP, Opcode::KIL, Opcode::STY, Opcode::STA, Opcode::STX, Opcode::KIL,
    Opcode::DEY, Opcode::NOP, Opcode::TXA, Opcode::KIL, Opcode::STY, Opcode::STA, Opcode::STX, Opcode::KIL,
    // 0x90
    Opcode::BCC, Opcode::STA, Opcode::KIL, Opcode::KIL, Opcode::STY, Opcode::STA, Opcode::STX, Opcode::KIL,
    Opcode::TYA, Opcode::STA, Opcode::TXS, Opcode::KIL, Opcode::KIL, Opcode::STA, Opcode::KIL, Opcode::KIL,
    // 0xA0
    Opcode::LDY, Opcode::LDA, Opcode::LDX, Opcode::KIL, Opcode::LDY, Opcode::LDA, Opcode::LDX, Opcode::KIL,
    Opcode::TAY, Opcode::LDA, Opcode::TAX, Opcode::KIL, Opcode::LDY, Opcode::LDA, Opcode::LDX, Opcode::KIL,
    // 0xB0
    Opcode::BCS, Opcode::LDA, Opcode::KIL, Opcode::KIL, Opcode::LDY, Opcode::LDA, Opcode::LDX, Opcode::KIL,
    Opcode::CLV, Opcode::LDA, Opcode::TSX, Opcode::KIL, Opcode::LDY, Opcode::LDA, Opcode::LDX, Opcode::KIL,
    // 0xC0
    Opcode::CPY, Opcode::CMP, Opcode::NOP, Opcode::KIL, Opcode::CPY, Opcode::CMP, Opcode::DEC, Opcode::KIL,
    Opcode::INY, Opcode::CMP, Opcode::DEX, Opcode::KIL, Opcode::CPY, Opcode::CMP, Opcode::DEC, Opcode::KIL,
    // 0xD0
    Opcode::BNE, Opcode::CMP, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::CMP, Opcode::DEC, Opcode::KIL,
    Opcode::CLD, Opcode::CMP, Opcode::NOP, Opcode::KIL, Opcode::NOP, Opcode::CMP, Opcode::DEC, Opcode::KIL,
    // 0xE0
    Opcode::CPX, Opcode::SBC, Opcode::NOP, Opcode::KIL, Opcode::CPX, Opcode::SBC, Opcode::INC, Opcode::KIL,
    Opcode::INX, Opcode::SBC, Opcode::NOP, Opcode::SBC, Opcode::CPX, Opcode::SBC, Opcode::INC, Opcode::KIL,
    // 0xF0
    Opcode::BEQ, Opcode::SBC, Opcode::KIL, Opcode::KIL, Opcode::NOP, Opcode::SBC, Opcode::INC, Opcode::KIL,
    Opcode::SED, Opcode::SBC, Opcode::NOP, Opcode::KIL, Opcode::NOP, Opcode::SBC, Opcode::INC, Opcode::KIL
};

class NESDL_CPU
{
public:
    void Init(NESDL_Core* c);
    void Reset(bool hardReset);
    void Update(uint32_t ppuCycles);
    void DidMapperWrite();
    bool IsConsecutiveMapperWrite();
    void HaltCPUForDMC();

    void DebugBindNintendulator(const char* path);
    void DebugUnbindNintendulator();

    uint64_t elapsedCycles;
    CPURegisters registers;
    bool nmi;
    bool delayNMI;
    bool dma;
    bool irq;
    bool iFlagReady;
    uint8_t iFlagNextSetState;
    bool nextInstructionReady;

    bool ignoreChanges;
private:
    void RunNextInstruction();
    void SetPSFlag(uint8_t flag, bool on);
    void GetByteForAddressMode(AddrMode mode, AddressModeResult* result);
    void AdvanceCyclesForAddressMode(uint8_t opcode, AddrMode mode, bool pageCross, bool extraCycles, bool relSuccess);
    uint8_t GetCyclesForNextInstruction();
    void HaltCPUForDMAWrite();
    
    // All opcode declarations
    void OP_ADC(uint8_t opcode, bool sbc = false);
    void OP_AND(uint8_t opcode);
    void OP_ASL(uint8_t opcode);
    void OP_BCC(uint8_t opcode);
    void OP_BCS(uint8_t opcode);
    void OP_BEQ(uint8_t opcode);
    void OP_BIT(uint8_t opcode);
    void OP_BMI(uint8_t opcode);
    void OP_BNE(uint8_t opcode);
    void OP_BPL(uint8_t opcode);
    void OP_BRK(uint8_t opcode);
    void OP_BVC(uint8_t opcode);
    void OP_BVS(uint8_t opcode);
    void OP_CLC(uint8_t opcode);
    void OP_CLD(uint8_t opcode);
    void OP_CLI(uint8_t opcode);
    void OP_CLV(uint8_t opcode);
    void OP_CMP(uint8_t opcode);
    void OP_CPX(uint8_t opcode);
    void OP_CPY(uint8_t opcode);
    void OP_DEC(uint8_t opcode);
    void OP_DEX(uint8_t opcode);
    void OP_DEY(uint8_t opcode);
    void OP_EOR(uint8_t opcode);
    void OP_INC(uint8_t opcode);
    void OP_INX(uint8_t opcode);
    void OP_INY(uint8_t opcode);
    void OP_JMP(uint8_t opcode, bool indirect);
    void OP_JSR(uint8_t opcode);
    void OP_LDA(uint8_t opcode);
    void OP_LDX(uint8_t opcode);
    void OP_LDY(uint8_t opcode);
    void OP_LSR(uint8_t opcode);
    void OP_NOP(uint8_t opcode);
    void OP_ORA(uint8_t opcode);
    void OP_PHA(uint8_t opcode);
    void OP_PHP(uint8_t opcode);
    void OP_PLA(uint8_t opcode);
    void OP_PLP(uint8_t opcode);
    void OP_ROL(uint8_t opcode);
    void OP_ROR(uint8_t opcode);
    void OP_RTI(uint8_t opcode);
    void OP_RTS(uint8_t opcode);
    void OP_SBC(uint8_t opcode);
    void OP_SEC(uint8_t opcode);
    void OP_SED(uint8_t opcode);
    void OP_SEI(uint8_t opcode);
    void OP_STA(uint8_t opcode);
    void OP_STX(uint8_t opcode);
    void OP_STY(uint8_t opcode);
    void OP_TAX(uint8_t opcode);
    void OP_TAY(uint8_t opcode);
    void OP_TSX(uint8_t opcode);
    void OP_TXA(uint8_t opcode);
    void OP_TXS(uint8_t opcode);
    void OP_TYA(uint8_t opcode);
    void OP_KIL();
    void NMI(); // NMI interrupt
    void IRQ(); // IRQ interrupt

    string DebugMakeCurrentStateLine();
    void EvaluateNintendulatorDebug();

    bool delayedDMA;
    NESDL_Core* core;
    AddressModeResult* addrModeResult;
    int16_t ppuCycleCounter;
    uint8_t nextInstructionPPUCycles;
    bool irqFired;
    
    bool didMapperWrite;
    bool wasLastInstructionAMapperWrite; // Not happy with this, but needed

    DebugCPUState debugCPUState;
    bool nintendulatorDebugging;
    vector<string> nintendulatorLog;
    uint64_t nintendulatorLogIndex;
    uint64_t nintendulatorLogCount;
};
