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
typedef uint8_t PStatusFlag;

#define STACK_PTR 0x0100

// Bit masks for instruction addressing modes
enum AddrMode { IMPLICIT, RELATIVE, ACCUMULATOR, IMMEDIATE,
    ZEROPAGE, ZEROPAGEX, ZEROPAGEY, ABSOLUTE, ABSOLUTEX, ABSOLUTEY,
    INDIRECTX, INDIRECTY };

class NESDL_Core; // Declaration needed for a pointer ref

struct AddressModeResult
{
    uint16_t address;
    uint8_t value;
};

class NESDL_CPU
{
public:
	void Init(NESDL_Core* c);
	void Update(double newSystemTime);
	void RunNextInstruction();
	uint64_t MillisecondsToCPUCycles(double ms);
	double CPUCyclesToMilliseconds(uint64_t c);
    void SetPSFlag(PStatusFlag flag, bool on);
    void GetByteForAddressMode(AddrMode mode, AddressModeResult* result);
    void AdvanceCyclesForAddressMode(AddrMode mode, bool pageCross, bool extraCycles, bool relSuccess);
    bool HasAddressPageBeenCrossed(uint16_t addr1, uint16_t addr2);
    
    // All opcode declarations
    void OP_ADC(AddrMode mode);
    void OP_AND(AddrMode mode);
    void OP_ASL(AddrMode mode);
    void OP_BCC(AddrMode mode);
    void OP_BCS(AddrMode mode);
    void OP_BEQ(AddrMode mode);
    void OP_BIT(AddrMode mode);
    void OP_BMI(AddrMode mode);
    void OP_BNE(AddrMode mode);
    void OP_BPL(AddrMode mode);
    void OP_BRK(AddrMode mode);
    void OP_BVC(AddrMode mode);
    void OP_BVS(AddrMode mode);
    void OP_CLC(AddrMode mode);
    void OP_CLD(AddrMode mode);
    void OP_CLI(AddrMode mode);
    void OP_CLV(AddrMode mode);
    void OP_CMP(AddrMode mode);
    void OP_CPX(AddrMode mode);
    void OP_CPY(AddrMode mode);
    void OP_DEC(AddrMode mode);
    void OP_DEX(AddrMode mode);
    void OP_DEY(AddrMode mode);
    void OP_EOR(AddrMode mode);
    void OP_INC(AddrMode mode);
    void OP_INX(AddrMode mode);
    void OP_INY(AddrMode mode);
    void OP_JMP(bool indirect);
    void OP_JSR(AddrMode mode);
    void OP_LDA(AddrMode mode);
    void OP_LDX(AddrMode mode);
    void OP_LDY(AddrMode mode);
    void OP_LSR(AddrMode mode);
    void OP_NOP(AddrMode mode);
    void OP_ORA(AddrMode mode);
    void OP_PHA(AddrMode mode);
    void OP_PHP(AddrMode mode);
    void OP_PLA(AddrMode mode);
    void OP_PLP(AddrMode mode);
    void OP_ROL(AddrMode mode);
    void OP_ROR(AddrMode mode);
    void OP_RTI(AddrMode mode);
    void OP_RTS(AddrMode mode);
    void OP_SBC(AddrMode mode);
    void OP_SEC(AddrMode mode);
    void OP_SED(AddrMode mode);
    void OP_SEI(AddrMode mode);
    void OP_STA(AddrMode mode);
    void OP_STX(AddrMode mode);
    void OP_STY(AddrMode mode);
    void OP_TAX(AddrMode mode);
    void OP_TAY(AddrMode mode);
    void OP_TSX(AddrMode mode);
    void OP_TXA(AddrMode mode);
    void OP_TXS(AddrMode mode);
    void OP_TYA(AddrMode mode);
    void OP_KIL();
    
    CPURegisters registers;
private:
	NESDL_Core* core;
    AddressModeResult* addrModeResult;
	uint64_t elapsedCycles;
};
