#pragma once

using namespace std;

class NESDL_Core; // Decl needed for pointer refs

// Screen dimension constants
#define NESDL_SCREEN_WIDTH 256
#define NESDL_SCREEN_HEIGHT 240

const string NESDL_WINDOW_NAME = "NESDL";
const string NESDL_VERSION_STR = "0.1.0";
const string NESDL_VERSIONDATE = "April 16th, 2025";

// PPU
#define PPU_WIDTH 256
#define PPU_HEIGHT 240

// Measured in hz
#define NESDL_MASTER_CLOCK 21477273 // 236.25 MHz / 11, by definition
#define NESDL_PPU_CLOCK (NESDL_MASTER_CLOCK / 4) // 4 clocks per dot, 3x faster than CPU

// CPU clock cycles for the PPU to "warm up" (more specifically, the
// "pre-render scanline of the next frame" on NTSC)
#define NESDL_PPU_READY 29658

// SDL takes color in format ARGB
static uint32_t MakeColor(uint8_t r, uint8_t g, uint8_t b)
{
    return (r << 16) | (g << 8) | (b << 0);
};

// https://www.nesdev.org/wiki/PPU_palettes#Recommended_emulator_palette
const uint32_t NESDL_PALETTE[0x40] =
{
    MakeColor(0x62,0x62,0x62), MakeColor(0x00,0x2C,0x7C), MakeColor(0x11,0x15,0x9C), MakeColor(0x36,0x03,0x9C),
    MakeColor(0x55,0x00,0x7C), MakeColor(0x67,0x00,0x44), MakeColor(0x67,0x07,0x03), MakeColor(0x55,0x1C,0x00),
    MakeColor(0x36,0x32,0x00), MakeColor(0x11,0x44,0x00), MakeColor(0x00,0x4E,0x00), MakeColor(0x00,0x4C,0x03),
    MakeColor(0x00,0x40,0x44), MakeColor(0x00,0x00,0x00), MakeColor(0x00,0x00,0x00), MakeColor(0x00,0x00,0x00),
    
    MakeColor(0xAB,0xAB,0xAB), MakeColor(0x12,0x60,0xCE), MakeColor(0x3D,0x42,0xFA), MakeColor(0x6E,0x29,0xFA),
    MakeColor(0x99,0x1C,0xCE), MakeColor(0xB1,0x1E,0x81), MakeColor(0xB1,0x2F,0x29), MakeColor(0x99,0x4A,0x00),
    MakeColor(0x6E,0x69,0x00), MakeColor(0x3D,0x82,0x00), MakeColor(0x12,0x8F,0x00), MakeColor(0x00,0x8D,0x29),
    MakeColor(0x00,0x7C,0x81), MakeColor(0x00,0x00,0x00), MakeColor(0x00,0x00,0x00), MakeColor(0x00,0x00,0x00),
    
    MakeColor(0xFF,0xFF,0xFF), MakeColor(0x60,0xB2,0xFF), MakeColor(0x8D,0x92,0xFF), MakeColor(0xC0,0x78,0xFF),
    MakeColor(0xEC,0x6A,0xFF), MakeColor(0xFF,0x6D,0xD4), MakeColor(0xFF,0x7F,0x79), MakeColor(0xEC,0x9B,0x2A),
    MakeColor(0xC0,0xBA,0x00), MakeColor(0x8D,0xD4,0x00), MakeColor(0x60,0xE2,0x2A), MakeColor(0x47,0xE0,0x79),
    MakeColor(0x47,0xCE,0xD4), MakeColor(0x4E,0x4E,0x4E), MakeColor(0x00,0x00,0x00), MakeColor(0x00,0x00,0x00),
    
    MakeColor(0xFF,0xFF,0xFF), MakeColor(0xBF,0xE0,0xFF), MakeColor(0xD1,0xD3,0xFF), MakeColor(0xE6,0xC9,0xFF),
    MakeColor(0xF7,0xC3,0xFF), MakeColor(0xFF,0xC4,0xEE), MakeColor(0xFF,0xCB,0xC9), MakeColor(0xF7,0xD7,0xA9),
    MakeColor(0xE6,0xE3,0x97), MakeColor(0xD1,0xEE,0x97), MakeColor(0xBF,0xF3,0xA9), MakeColor(0xB5,0xF2,0xC9),
    MakeColor(0xB5,0xEB,0xEE), MakeColor(0xB8,0xB8,0xB8), MakeColor(0x00,0x00,0x00), MakeColor(0x00,0x00,0x00)
};

#define APU_SAMPLE_RATE 44100
#define APU_SAMPLE_BUF 1024

// APU square duties (four selectable "sounds" for the two square channels)
// https://www.nesdev.org/wiki/APU_Pulse
const uint8_t NESDL_SQUARE_DUTY[32] =
{
    0, 0, 0, 0, 0, 0, 0, 1, // 12.5%
    0, 0, 0, 0, 0, 0, 1, 1, // 25%
    0, 0, 0, 0, 1, 1, 1, 1, // 50%
    1, 1, 1, 1, 1, 1, 0, 0  // 75% (25% Flipped)
};

// APU triangle values (0-15)
// https://www.nesdev.org/wiki/APU_Triangle
const uint8_t NESDL_TRI_DUTY[32] =
{
    15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15
};

// APU length counter corresponding timers
// https://www.nesdev.org/wiki/APU_Length_Counter
const uint8_t NESDL_LENGTH_COUNTER[32] =
{
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

// APU noise period timers
// https://www.nesdev.org/wiki/APU_Noise
const uint16_t NESDL_NOISE_PERIOD[16] =
{
      4,   8,  16,  32,  64,   96,  128,  160,
    202, 254, 380, 508, 762, 1016, 2034, 4068
};

// DMC sample playback CPU rate
// https://www.nesdev.org/wiki/APU_DMC
const uint16_t NESDL_DMC_RATE[16]
{
    428, 380, 340, 320, 286, 254, 226, 214,
    190, 160, 142, 128, 106,  84,  72,  54
};
