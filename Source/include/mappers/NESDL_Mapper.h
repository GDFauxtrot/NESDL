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
    virtual void SetMirroringData(bool data);
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
    uint8_t prgROM0Index; // PRG 16KB bank 1
    uint8_t prgROM1Index; // PRG 16KB bank 2
    uint8_t chrROM0Index; // CHR  4KB bank 1
    uint8_t chrROM1Index; // CHR  4KB bank 2
    uint8_t prgROMMode;
    bool chrROMMode;
};
