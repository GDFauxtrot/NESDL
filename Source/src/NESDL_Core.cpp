#include "NESDL.h"
#include <memory>
#ifdef _WIN32
#include "../src/nfd/nfd.h"
#else
#include "nfd.h"
#endif

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
    config = new NESDL_Config();

    cpu->Init(this);
    ppu->Init(this);
    ram->Init(this);
    apu->Init(this, sdlCtx);
    input->Init(this);
    config->Init(this);
    
    // Connect player 1 controller from the start
    input->SetControllerConnected(true, false);
}

void NESDL_Core::Exit()
{
    delete cpu;
    delete ppu;
    delete ram;
    delete apu;
    delete input;
    delete config;
}

void NESDL_Core::Update(double deltaTime)
{
    // Convert deltaTime to the amount of cycles we need to advance on this frame
    uint64_t ppuTiming1 = (uint64_t)((NESDL_PPU_CLOCK / 1000) * timeSinceStartup);
    timeSinceStartup += deltaTime;
    uint64_t ppuTiming2 = (uint64_t)((NESDL_PPU_CLOCK / 1000) * timeSinceStartup);
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
        // Send PPU cycles (finest clock cycle in the hardware) to handle their own cycle updates
        cpu->Update(1);
        ppu->Update(1);
        apu->Update(1);
        
        // Only update SDL screen texture IF the visible screen has finished being drawn to
        // Prevents visible screen tearing from mid-frame drawing
        if (ppu->frameDataReady)
        {
            ppu->frameDataReady = false;
            ppu->UpdateNTFrameData();
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

    // If header doesn't say "NES" with an EOF char, surely this isn't a valid ROM file
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
    
    // Set up mapper
    uint8_t mapperNum = ((header.ctrl1 & INES_MAPPER_LOW) >> 4) | (header.ctrl2 & INES_MAPPER_HI);
    switch (mapperNum)
    {
        case 0:
            mapper = new NESDL_Mapper_0(this);
            mapper->SetMirroringData(header.ctrl1 & INES_NTMIRROR);
            break;
        case 1:
            mapper = new NESDL_Mapper_1(this);
            break;
        case 4:
            mapper = new NESDL_Mapper_4(this);
            break;
        case 9:
            mapper = new NESDL_Mapper_9(this);
            break;
        default:
            stringstream ss;
            ss << "ROM uses an unsupported mapper! #" << to_string(mapperNum);
            sdlCtx->ShowTextNotice(ss.str().c_str());
            ss << "\n";
            printf(ss.str().c_str());
            file.close();
            return;
    }
    mapper->mapperNumber = mapperNum;

    // Read off the 16KB allocated PRG-ROM banks
    shared_ptr<vector<uint8_t>> prgROM;
    shared_ptr<vector<uint8_t>> chrROM;
    if (romBankCount > 0)
    {
        prgROM = make_shared<vector<uint8_t>>(romBankCount * 0x4000);
        file.read((char*)prgROM->data(), romBankCount * 0x4000);
    }

    // Read off the 8KB allocated CHR-ROM banks
    if (vromBankCount > 0)
    {
        chrROM = make_shared<vector<uint8_t>>(vromBankCount * 0x2000);
        file.read((char*)chrROM->data(), vromBankCount * 0x2000);
    }
    
    // Done with file -- we can close it now
    file.close();
    
    // Initialize mapper with all of its pertinent data
    uint8_t* prgPtr = nullptr;
    if (romBankCount > 0)
    {
        prgPtr = prgROM->data();
    }
    uint8_t* chrPtr = nullptr;
    if (vromBankCount > 0)
    {
        chrPtr = chrROM->data();
    }
    mapper->InitROMData(prgPtr, romBankCount, chrPtr, vromBankCount);
    
    // Let components know we exist
    ram->SetMapper(mapper);
    ppu->SetMapper(mapper);
    
    romLoaded = true;
}

void NESDL_Core::HandleEvent(SDL_EventType eventType, SDL_KeyCode eventKeyCode)
{
    if (eventType == SDL_KEYUP || eventType == SDL_KEYDOWN)
    {
        input->RegisterKey(eventKeyCode, eventType == SDL_KEYDOWN);
    }
}

bool NESDL_Core::IsROMLoaded()
{
    return romLoaded;
}

void NESDL_Core::Action_Quit()
{
    // Used primarily for Windows - send a SDL_QUIT event
    SDL_Event ev;
    ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}

void NESDL_Core::Action_ShowAbout()
{
    sdlCtx->ShowAbout();
}

void NESDL_Core::Action_OpenROM()
{
    nfdchar_t* romFilePath = NULL;
    string defFilePath = GetDirectoryOf(config->ReadString(ConfigKey::LASTROM, ""));
    nfdresult_t result = NFD_OpenDialog("nes", defFilePath.c_str(), &romFilePath);
    // Don't continue if we didn't successfully choose a file, or (somehow) it's empty
    if (result != NFD_OKAY || strcmp(romFilePath, "") == 0)
    {
        return;
    }
    else
    {
        Action_CloseROM();
    }
    LoadROM(romFilePath);
    if (romLoaded)
    {
        config->WriteValue("NESDL_LastROM", string(romFilePath));
        Action_ResetHard();
    }
}
void NESDL_Core::Action_CloseROM()
{
    if (romLoaded)
    {
        // Leave all states intact but "unplug" all cartridge data
        free(mapper);
        ram->SetMapper(nullptr);
        ppu->SetMapper(nullptr);
        romLoaded = false;
        
        // Clear screen on ROM close (better signifier of ROM no longer running than not)
        memset(ppu->frameData, 0x00, sizeof(ppu->frameData));
        sdlCtx->UpdateScreenTexture();
    }
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
void NESDL_Core::Action_DebugShowNT()
{
    sdlCtx->ToggleShowNT();
}
void NESDL_Core::Action_AttachNintendulatorLog()
{
    nfdchar_t* logFilePath = NULL;
    string defFilePath = GetDirectoryOf(config->ReadString(ConfigKey::LASTLOG, ""));
    nfdresult_t result = NFD_OpenDialog("debug", defFilePath.c_str(), &logFilePath);
    // Don't continue if we didn't successfully choose a file, or (somehow) it's empty
    if (result != NFD_OKAY || strcmp(logFilePath, "") == 0)
    {
        return;
    }
    else
    {
        config->WriteValue("NESDL_LastLog", string(logFilePath));
        cpu->DebugBindNintendulator(logFilePath);
    }
}
void NESDL_Core::Action_DetachNintendulatorLog()
{
    cpu->DebugUnbindNintendulator();
}

// https://stackoverflow.com/questions/8518743/get-directory-from-file-path-c
string NESDL_Core::GetDirectoryOf(const string& filePath)
{
    size_t pos = filePath.find_last_of("\\/");
    return (string::npos == pos) ? "" : filePath.substr(0, pos);
}