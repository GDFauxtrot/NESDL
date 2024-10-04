#include "NESDL.h"
#include <memory>
#include <fstream>

void NESDL_Core::Init()
{
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

	timeSinceStartup = 0;
}

void NESDL_Core::Update(double deltaTime)
{
	// Convert deltaTime to the amount of cycles we need to advance on this frame
    uint64_t ppuTiming1 = (NESDL_PPU_CLOCK / 1000) * timeSinceStartup;
	timeSinceStartup += deltaTime;
    uint64_t ppuTiming2 = (NESDL_PPU_CLOCK / 1000) * timeSinceStartup;
    uint32_t ppuCycles = (uint32_t)(ppuTiming2 - ppuTiming1);
    
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
            sdlCtx->UpdateScreen(ppu);
        }
    }
}

void NESDL_Core::StartSystem(NESDL_SDL* sdl)
{
    cpu->Start();
    sdlCtx = sdl;
}

void NESDL_Core::LoadRom(const char* path)
{
	ifstream file;
	file.open(path, ifstream::in | ifstream::binary);

	// Read header
	FileHeader_INES header;
	file.read((char*)&header, sizeof(header));

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
}

void NESDL_Core::HandleEvent(SDL_EventType eventType, SDL_KeyCode eventKeyCode)
{
    input->RegisterKey(eventKeyCode, eventType == SDL_KEYDOWN);
}
