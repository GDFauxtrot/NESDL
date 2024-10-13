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

// Bit masks for instruction addressing modes
enum AddrMode { IMPLICIT, RELATIVE, ACCUMULATOR, IMMEDIATE,
    ZEROPAGE, ZEROPAGEX, ZEROPAGEY, ABSOLUTE, ABSOLUTEX, ABSOLUTEY,
    INDIRECTX, INDIRECTY };

struct AddressModeResult
{
    uint16_t address;
    uint8_t value;
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
    
    uint64_t elapsedCycles;
    CPURegisters registers;
    bool nmi;
    bool delayNMI;
    bool dma;
    bool nextInstructionReady;
private:
    void RunNextInstruction();
    void SetPSFlag(uint8_t flag, bool on);
    void GetByteForAddressMode(AddrMode mode, AddressModeResult* result);
    void AdvanceCyclesForAddressMode(uint8_t opcode, AddrMode mode, bool pageCross, bool extraCycles, bool relSuccess);
    uint8_t GetCyclesForNextInstruction();
    void HaltCPUForDMAWrite();
    
    // All opcode declarations
    void OP_ADC(uint8_t opcode, AddrMode mode, bool sbc);
    void OP_AND(uint8_t opcode, AddrMode mode);
    void OP_ASL(uint8_t opcode, AddrMode mode);
    void OP_BCC(uint8_t opcode, AddrMode mode);
    void OP_BCS(uint8_t opcode, AddrMode mode);
    void OP_BEQ(uint8_t opcode, AddrMode mode);
    void OP_BIT(uint8_t opcode, AddrMode mode);
    void OP_BMI(uint8_t opcode, AddrMode mode);
    void OP_BNE(uint8_t opcode, AddrMode mode);
    void OP_BPL(uint8_t opcode, AddrMode mode);
    void OP_BRK(uint8_t opcode, AddrMode mode);
    void OP_BVC(uint8_t opcode, AddrMode mode);
    void OP_BVS(uint8_t opcode, AddrMode mode);
    void OP_CLC(uint8_t opcode, AddrMode mode);
    void OP_CLD(uint8_t opcode, AddrMode mode);
    void OP_CLI(uint8_t opcode, AddrMode mode);
    void OP_CLV(uint8_t opcode, AddrMode mode);
    void OP_CMP(uint8_t opcode, AddrMode mode);
    void OP_CPX(uint8_t opcode, AddrMode mode);
    void OP_CPY(uint8_t opcode, AddrMode mode);
    void OP_DEC(uint8_t opcode, AddrMode mode);
    void OP_DEX(uint8_t opcode, AddrMode mode);
    void OP_DEY(uint8_t opcode, AddrMode mode);
    void OP_EOR(uint8_t opcode, AddrMode mode);
    void OP_INC(uint8_t opcode, AddrMode mode);
    void OP_INX(uint8_t opcode, AddrMode mode);
    void OP_INY(uint8_t opcode, AddrMode mode);
    void OP_JMP(uint8_t opcode, bool indirect);
    void OP_JSR(uint8_t opcode, AddrMode mode);
    void OP_LDA(uint8_t opcode, AddrMode mode);
    void OP_LDX(uint8_t opcode, AddrMode mode);
    void OP_LDY(uint8_t opcode, AddrMode mode);
    void OP_LSR(uint8_t opcode, AddrMode mode);
    void OP_NOP(uint8_t opcode, AddrMode mode);
    void OP_ORA(uint8_t opcode, AddrMode mode);
    void OP_PHA(uint8_t opcode, AddrMode mode);
    void OP_PHP(uint8_t opcode, AddrMode mode);
    void OP_PLA(uint8_t opcode, AddrMode mode);
    void OP_PLP(uint8_t opcode, AddrMode mode);
    void OP_ROL(uint8_t opcode, AddrMode mode);
    void OP_ROR(uint8_t opcode, AddrMode mode);
    void OP_RTI(uint8_t opcode, AddrMode mode);
    void OP_RTS(uint8_t opcode, AddrMode mode);
    void OP_SBC(uint8_t opcode, AddrMode mode);
    void OP_SEC(uint8_t opcode, AddrMode mode);
    void OP_SED(uint8_t opcode, AddrMode mode);
    void OP_SEI(uint8_t opcode, AddrMode mode);
    void OP_STA(uint8_t opcode, AddrMode mode);
    void OP_STX(uint8_t opcode, AddrMode mode);
    void OP_STY(uint8_t opcode, AddrMode mode);
    void OP_TAX(uint8_t opcode, AddrMode mode);
    void OP_TAY(uint8_t opcode, AddrMode mode);
    void OP_TSX(uint8_t opcode, AddrMode mode);
    void OP_TXA(uint8_t opcode, AddrMode mode);
    void OP_TXS(uint8_t opcode, AddrMode mode);
    void OP_TYA(uint8_t opcode, AddrMode mode);
    void OP_KIL();
    void NMI(); // NMI interrupt (special case)
    
    bool delayedDMA;
	NESDL_Core* core;
    AddressModeResult* addrModeResult;
    int16_t ppuCycleCounter;
    
    bool didMapperWrite;
    bool wasLastInstructionAMapperWrite; // Not happy with this, but needed
};
