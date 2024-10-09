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

#define INES_NTMIRROR   0x01
#define INES_SAVEDATA   0x02
#define INES_TRAINER    0x04
#define INES_NTLAYOUT   0x08
#define INES_MAPPER_LOW 0xF0

#define INES_VSSYSTEM   0x01
#define INES_PLAYCHOICE 0x02
#define INES_NES20      0x0C
#define INES_MAPPER_HI  0xF0

class NESDL_Core
{
public:
	void Init(NESDL_SDL* sdl);
	void Update(double deltaTime);
	void LoadROM(const char* path);
    void HandleEvent(SDL_EventType eventType, SDL_KeyCode eventKeyCode);
    bool IsROMLoaded();

    // Menu bar actions (called from OS-specific areas)
    void Action_OpenROM();
    void Action_CloseROM();
    void Action_ResetSoft();
    void Action_ResetHard();
    void Action_ViewFrameInfo();
    void Action_ViewResize(int resize);
    void Action_DebugRun();
    void Action_DebugPause();
    void Action_DebugStepFrame();
    void Action_DebugStepCPU();
    void Action_DebugStepPPU();
    void Action_DebugShowCPU();
    void Action_DebugShowPPU();
    void Action_DebugShowNT();
    
	NESDL_CPU* cpu;
	NESDL_PPU* ppu;
	NESDL_RAM* ram;
	NESDL_APU* apu;
    NESDL_Input* input;
    
    bool romLoaded;
private:
    NESDL_SDL* sdlCtx;
    NESDL_Mapper* mapper;
    
    double timeSinceStartup;
    bool paused;
    bool stepFrame;
    bool stepCPU;
    bool stepPPU;
};
