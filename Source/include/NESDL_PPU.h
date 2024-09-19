#pragma once

// The documented registers of the PPU provided in one struct for organization
//struct PPURegisters
//{
//    uint8_t ctrl;       // PPU control register
//    uint8_t mask;       // PPU mask register
//    uint8_t status;     // PPU status register
//    uint8_t oamAddr;    // OAM address port
//    uint8_t oamData;    // OAM data port
//    uint8_t scroll;     // PPU scroll position register
//    uint16_t addr;       // PPU address register
//    uint8_t data;       // PPU data port
//    uint8_t oamDMA;     // OAM DMA register
//};

struct PPURegisters
{
    // Internal registers
    uint16_t v; // 15-bit register - current VRAM address
    uint16_t t; // 15-bit register - temp VRAM address (usually top-left screen tile)
    uint8_t x; // 3-bit register - fine X scroll
    uint8_t y; // Technically not an internal register? Fine Y scroll
    bool w; // 1-bit register - write toggle

    // Memory-mapped registers
    uint8_t ctrl; // PPU control register
    uint8_t mask; // PPU mask register
    uint8_t status;     // PPU status register
};

// The bytes collected during PPU tile fetching
struct PPUTileFetch
{
    uint8_t nametable;
    uint8_t attribute;
    uint16_t pattern;
    uint8_t paletteIndex;
};

// 4-byte struct for Object Attribute Memory
struct OAMEntry
{
    uint8_t yPos;
    uint8_t tileIndex;
    uint8_t attributes;
    uint8_t xLeftPos;
};

// PPU registers mapped in RAM
#define PPU_PPUCTRL     0x2000
#define PPU_PPUMASK     0x2001
#define PPU_PPUSTATUS   0x2002
#define PPU_OAMADDR     0x2003
#define PPU_OAMDATA     0x2004
#define PPU_PPUSCROLL   0x2005
#define PPU_PPUADDR     0x2006
#define PPU_PPUDATA     0x2007
#define PPU_OAMDMA      0x4014

// PPUCTRL flags
#define PPUCTRL_NAMETABLE_L 0x01
#define PPUCTRL_NAMETABLE_H 0x02
#define PPUCTRL_INCMODE     0x04
#define PPUCTRL_SPRTILE     0x08
#define PPUCTRL_BGTILE      0x10
#define PPUCTRL_SPRHEIGHT   0x20
#define PPUCTRL_PPUSELECT   0x40
#define PPUCTRL_NMIENABLE   0x80

// PPUMASK flags
#define PPUMASK_GREYSCALE       0x01
#define PPUMASK_BG_LCOL_ENABLE  0x02
#define PPUMASK_SPR_LCOL_ENABLE 0x04
#define PPUMASK_BGENABLE        0x08
#define PPUMASK_SPRENABLE       0x10
#define PPUMASK_TINT_R          0x20
#define PPUMASK_TINT_G          0x40
#define PPUMASK_TINT_B          0x80

// PPUSTATUS flags (0-4 are unused/stale values)
#define PPUSTATUS_SPROVERFLOW   0x20
#define PPUSTATUS_SPR0HIT       0x40
#define PPUSTATUS_VBLANK        0x80

// OAM attributes flags
#define OAMATTR_PALETTE_L   0x1
#define OAMATTR_PALETTE_H   0x2
#define OAMATTR_PRIORITY    0x20
#define OAMATTR_FLIPX       0x40
#define OAMATTR_FLIPY       0x80

class NESDL_PPU
{
public:
	void Init(NESDL_Core* c);
    void Start();
	void Update(uint32_t ppuCycles);
    uint64_t MillisecondsToPPUCycles(double ms);
    bool IsPPUReady();
    void RunNextCycle();
    void HandleProcessVisibleScanline();
    void HandleProcessVBlankScanline();
    void WriteToRegister(uint16_t registerAddr, uint8_t data);
    uint8_t ReadFromRegister(uint16_t registerAddr);
    uint8_t ReadFromVRAM(uint16_t addr);
    void WriteToVRAM(uint16_t addr, uint8_t data);
    void FetchAndStoreTile();
    uint16_t WeavePatternBits(uint8_t low, uint8_t high);
    uint32_t GetPalette(uint8_t index, bool spriteLayer);
    uint32_t GetColor(uint16_t pattern, uint32_t palette, uint8_t pixelIndex);
    PPURegisters registers;
    uint32_t frameData[NESDL_SCREEN_WIDTH * NESDL_SCREEN_HEIGHT]; // Frame buffer
    bool ignoreChanges;
    bool incrementV;
    uint64_t currentFrame;
private:
    NESDL_Core* core;
    uint64_t elapsedCycles;
    uint16_t currentDrawX;
    uint16_t currentScanline;
    uint8_t vram[0x1000];  // 4kb VRAM for the PPU (physically 2kb on stock but supports 4 nametables)
    uint8_t paletteData[0x20]; // Bit of space at the end of VRAM address space for palette data
    PPUTileFetch tileFetch;
    PPUTileFetch tileBuffer[2]; // Representation of the tile data sitting in shift registers
    uint8_t ppuDataReadBuffer; // Special internal buffer for PPUDATA reads
    OAMEntry oam[64]; // 64 slots (256 bytes) for PPU "Object Attribute Memory"
};
