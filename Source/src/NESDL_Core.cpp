#include "NESDL.h"
#include <memory>
#include <fstream>
#include "nfd.h"

void NESDL_Core::Init(NESDL_SDL* sdl)
{
    // Hold onto the program's SDL context
    sdlCtx = sdl;
    
	// Initialize the system - CPU, PPU, RAM and APU
	cpu = new NESDL_CPU();
	ppu = new NESDL_PPU();
	ram = new NESDL_RAM();
	apu = new NESDL_APU();
    input = new NESDL_Input();

	cpu->Init(this);
    ppu->Init(this);
    ram->Init(this);
//    apu->Init(this);
    input->Init(this);
    
    // Connect player 1 controller from the start
    input->SetControllerConnected(true, false);
}

void NESDL_Core::Update(double deltaTime)
{
	// Convert deltaTime to the amount of cycles we need to advance on this frame
    uint64_t ppuTiming1 = (NESDL_PPU_CLOCK / 1000) * timeSinceStartup;
	timeSinceStartup += deltaTime;
    uint64_t ppuTiming2 = (NESDL_PPU_CLOCK / 1000) * timeSinceStartup;
    uint32_t ppuCycles = (uint32_t)(ppuTiming2 - ppuTiming1);
    
    // Force 0 cycles if we're paused, or just 1 if we want to step the PPU.
    // CPU stepping gets a bit more involved
    bool stepped = false;
    if (paused && !stepCPU && !stepFrame)
    {
        ppuCycles = 0;
    }
    if (stepPPU)
    {
        stepPPU = false;
        ppuCycles = 1;
        stepped = true;
    }
    for (uint32_t i = 0; i < ppuCycles; ++i)
    {
        // Send current timeSinceStartup to CPU and PPU to handle their own cycle updates
        cpu->Update(1);
        ppu->Update(1);
        
        // Only update SDL screen texture IF the visible screen has finished being drawn to
        // Prevents visible screen tearing from mid-frame drawing
        if (ppu->frameDataReady)
        {
            ppu->frameDataReady = false;
            sdlCtx->UpdateScreenTexture();
        }
        // We ignore the paused flag if we're CPU stepping,
        // UNTIL the next instruction is ready.
        if (stepCPU && cpu->nextInstructionReady)
        {
            stepCPU = false;
            stepped = true;
            break;
        }
        // Same goes for frame stepping
        if (stepFrame && ppu->frameFinished)
        {
            ppu->frameFinished = false;
            stepFrame = false;
            stepped = true;
            break;
        }
    }
    if (stepped)
    {
        sdlCtx->UpdateScreen(0);
    }
}

void NESDL_Core::LoadROM(const char* path)
{
	ifstream file;
	file.open(path, ifstream::in | ifstream::binary);

	// Attempt to read file as a ROM with a valid 16-byte header
    // (TODO what if the file is < 16 bytes, or empty? This would surely fail)
	FileHeader_INES header;
	file.read((char*)&header, sizeof(header));

    // If header doesn't say "NES" with an EOF char, surely this isn't a ROM file
    if (header.id != 0x1A53454E)
    {
        return;
    }
    
	// Get the program bank count and VROM bank count. We'll be reading this data soon
	uint8_t romBankCount = header.banks;
	uint8_t vromBankCount = header.vbanks;

	// TODO read header for any trainer or mapping data! Then skip ahead if needed
	// Check for trainer flag in bit 2, skip it if detected
	if ((header.ctrl1 & 0x4) == 0x4) 
	{
		file.seekg(512, ios_base::cur);
	}
    
    // Relay mirroring info to PPU
    ppu->SetMirroringMode(header.ctrl1 & 0x1);

	// Read off the 16KB allocated PRG-ROM banks
    if (romBankCount > 0)
    {
        prgROM = make_shared<vector<uint8_t>>(romBankCount * 0x4000);
        file.read((char*)prgROM->data(), romBankCount * 0x4000);
        ram->WriteROMData(prgROM->data(), romBankCount); // TODO abstract this to a mapper! Assuming NROM
    }

	// Read off the 8KB allocated CHR-ROM banks
    if (vromBankCount > 0)
    {
        chrROM = make_shared<vector<uint8_t>>(vromBankCount * 0x2000);
        file.read((char*)chrROM->data(), vromBankCount * 0x2000);
        ppu->WriteCHRROM(chrROM->data()); // TODO abstract this to a mapper! Assuming NROM (no bankswitches, 1 bank only)
    }
	
	file.close();
    
    romLoaded = true;
}

void NESDL_Core::HandleEvent(SDL_EventType eventType, SDL_KeyCode eventKeyCode)
{
    input->RegisterKey(eventKeyCode, eventType == SDL_KEYDOWN);
}

bool NESDL_Core::IsROMLoaded()
{
    return romLoaded;
}

void NESDL_Core::Action_OpenROM()
{
    nfdchar_t *romFilePath = NULL;
    nfdresult_t result = NFD_OpenDialog("nes", NULL, &romFilePath);
    // Don't continue if we didn't successfully choose a file, or (somehow) it's empty
    if (result != NFD_OKAY || strcmp(romFilePath, "") == 0)
    {
        return;
    }
    LoadROM(romFilePath);
    Action_ResetHard();
}
void NESDL_Core::Action_CloseROM()
{
    // Leave all states intact but "unplug" all cartridge data
    ram->ClearROMData();
    ppu->ClearROMData();
    prgROM.reset();
    chrROM.reset();
    romLoaded = false;
}
void NESDL_Core::Action_ResetSoft()
{
    cpu->Reset(false);
    ppu->Reset(false);
    sdlCtx->UpdateScreenTexture();
}
void NESDL_Core::Action_ResetHard()
{
    timeSinceStartup = 0;
    cpu->Reset(true);
    ppu->Reset(true);
    sdlCtx->UpdateScreenTexture();
}
void NESDL_Core::Action_ViewFrameInfo()
{
    sdlCtx->ToggleFrameInfo();
}
void NESDL_Core::Action_ViewResize(int resize)
{
    sdlCtx->Resize(resize);
}
void NESDL_Core::Action_DebugRun()
{
    paused = false;
}
void NESDL_Core::Action_DebugPause()
{
    paused = true;
}
void NESDL_Core::Action_DebugStepFrame()
{
    paused = true;
    stepFrame = true;
    stepCPU = false;
    stepPPU = false;
}
void NESDL_Core::Action_DebugStepCPU()
{
    paused = true;
    stepFrame = false;
    stepCPU = true;
    stepPPU = false;
}
void NESDL_Core::Action_DebugStepPPU()
{
    paused = true;
    stepFrame = false;
    stepCPU = false;
    stepPPU = true;
}
void NESDL_Core::Action_DebugShowCPU()
{
    sdlCtx->ToggleShowCPU();
}
void NESDL_Core::Action_DebugShowPPU()
{
    sdlCtx->ToggleShowPPU();
}
