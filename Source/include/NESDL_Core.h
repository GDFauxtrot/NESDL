#pragma once

struct FileHeader_INES
{
	uint32_t id; // "NES" followed by byte 1A
	uint8_t banks; // Number of ROM banks
	uint8_t vbanks; // Number of VROM banks
	uint8_t ctrl1; // ROM control byte 1
	uint8_t ctrl2; // ROM control byte 2
	uint8_t rbanks; // Number of RAM banks (if 0, assume 1)
	uint8_t tvsys; // TV system (?? rarely used)
	uint8_t tvsys2; // TV system ROM presence (?? rarely used)
	uint8_t unused[5]; // Unused (future) flags
};

class NESDL_Core
{
public:
	void Init();
	void Update(double deltaTime);
	void LoadRom(const char* path);

	NESDL_CPU *cpu;
	NESDL_PPU *ppu;
	NESDL_RAM *ram;
	NESDL_APU *apu;
private:
	
};