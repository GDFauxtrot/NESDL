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
    ~NESDL_Mapper() { if (prgROM != nullptr) free(prgROM); if (chrROM != nullptr) free(chrROM); }
    virtual void InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks) {}
    virtual void SetMirroringData(bool data) {}
    virtual uint8_t ReadByte(uint16_t addr) { return 0; }
    virtual void WriteByte(uint16_t addr, uint8_t data) {}
    MirroringMode GetMirroringMode() { return mirroringMode; }
protected:
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
/// As such, it is also very basic, providing no bank switching, no interrupts, no battery RAM,
/// and only 2 nametable mirroring configs (horizontal and vertical).
/// (This class declaration is functionally useless but provided for posterity)
class NESDL_Mapper_0 : public NESDL_Mapper
{
public:
    virtual void InitROMData(uint8_t* prgROMData, uint8_t prgROMBanks, uint8_t* chrROMData, uint8_t chrROMBanks);
    virtual void SetMirroringData(bool data);
    virtual uint8_t ReadByte(uint16_t addr);
};

