#pragma once

enum MirroringMode
{
    Horizontal,
    Vertical,
    One_LowerBank,
    One_UpperBank,
    Four
};

class NESDL_Mapper
{
public:
    NESDL_Mapper(NESDL_Core* c) : core(c) {}
    ~NESDL_Mapper() { if (prgROM != nullptr) free(prgROM); if (chrROM != nullptr) free(chrROM); }
    virtual void InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks) {}
    virtual void SetMirroringData(bool data) {}
    virtual MirroringMode GetMirroringMode() { return MirroringMode::Horizontal; }
    virtual uint8_t ReadByte(uint16_t addr) { return 0; }
    virtual void WriteByte(uint16_t addr, uint8_t data) {}
    
    bool ignoreChanges;
protected:
    NESDL_Core* core;
    MirroringMode mirroringMode;
    uint8_t* prgROM;
    uint8_t* chrROM;
    uint8_t prgBanks;
    uint8_t chrBanks;
};

/// iNES Header 000 - "NROM" (Released July 1983, "Donkey Kong" JP)
/// https://www.nesdev.org/wiki/NROM
///
/// The first mapping scheme designated by the iNES standard, and also the first board available.
/// As such, it is also very basic, providing no bank switching, no interrupts, no battery RAM
/// (Excitebike US lied!), and only 2 nametable mirroring configs (horizontal and vertical).
class NESDL_Mapper_0 : public NESDL_Mapper
{
public:
    using NESDL_Mapper::NESDL_Mapper; // Inherit constructor(s)
    virtual void InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks);
    virtual void SetMirroringData(bool data);
    MirroringMode GetMirroringMode() { return mirroringMode; }
    virtual uint8_t ReadByte(uint16_t addr);
    // No WriteByte - at least, not until Family BASIC gets supported
};

/// iNES Header 001 - "MMC1" (Released April 1987, "Morita Shougi" JP)
/// https://www.nesdev.org/wiki/MMC1
///
/// The most popular mapper across the entire library of games, covering over 680
/// individual (known) releases! It is a simple but effective mapper, lacking IRQ or
/// any fancy conveniences, but providing great bank-switching and NT-switching capabilities.
class NESDL_Mapper_1 : public NESDL_Mapper
{
public:
    using NESDL_Mapper::NESDL_Mapper; // Inherit constructor(s)
    virtual void InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks);
    MirroringMode GetMirroringMode() { return mirroringMode; }
    virtual uint8_t ReadByte(uint16_t addr);
    virtual void WriteByte(uint16_t addr, uint8_t data);
private:
    void WriteControl();
    void WriteCHRBank(uint8_t index);
    void WritePRGBank();
    MirroringMode mirroringMode;
    uint8_t shiftRegister;
    uint8_t shiftIndex;
    uint8_t prgROM0Index;   // PRG 16KB bank 1
    uint8_t prgROM1Index;   // PRG 16KB bank 2
    uint8_t chrROM0Index;   // CHR  4KB bank 1
    uint8_t chrROM1Index;   // CHR  4KB bank 2
    uint8_t prgROMMode;
    uint8_t prgRAM[0x2000];
    bool chrROMMode;
    bool prgRAMEnable;
};

/// iNES Header 09  - "MMC2" (Released October 1987, "Mike Tyson's Punch-Out!!" US)
/// https://www.nesdev.org/wiki/MMC2
///
/// A mapper in which only one game was made on it - Mike Tyson's Punch-Out!! (and later Punch-Out!!)
/// Very similar to MMC1 except it doesn't use a shift register to handle bank switching -
/// simple address writes do the job. It's almost like a simpler version of MMC1, which is great for us
class NESDL_Mapper_9 : public NESDL_Mapper
{
public:
    using NESDL_Mapper::NESDL_Mapper; // Inherit constructor(s)
    virtual void InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks);
    MirroringMode GetMirroringMode() { return mirroringMode; }
    virtual uint8_t ReadByte(uint16_t addr);
    virtual void WriteByte(uint16_t addr, uint8_t data);
private:
    MirroringMode mirroringMode;
    uint8_t prgROMIndex;    // PRG 8KB bank   (0x8000 - 0x9FFF)
    uint8_t chrROM0Index0;  // CHR 4KB bank 1 - Latch 0 0xFD (0x0000 - 0x0FFF)
    uint8_t chrROM0Index1;  // CHR 4KB bank 1 - Latch 0 0xFE (0x0000 - 0x0FFF)
    uint8_t chrROM1Index0;  // CHR 4KB bank 2 - Latch 1 0xFD (0x1000 - 0x1FFF)
    uint8_t chrROM1Index1;  // CHR 4KB bank 2 - Latch 1 0xFE (0x1000 - 0x1FFF)
    uint8_t chrROM0Latch;   // Can be either 0xFD or 0xFE
    uint8_t chrROM1Latch;
    uint8_t prgRAM[0x2000];
};
