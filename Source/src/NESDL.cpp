#include "NESDL.h"

int main(int argc, char* args[])
{
    // Initialize SDL first
    NESDL_SDL sdlCtx;
    sdlCtx.SDLInit();

    // Initialize player input

    // Initialize system
    NESDL_Core core;
    core.Init();

    // Get ROM file (we hard coding this for now! Let's build a menu bar to load stuff later)
    string romFile = "D:\\Projects\\NESDL\\x64\\Debug\\nestest.nes";

    // Load ROM into core
    core.LoadRom(romFile.c_str());

    // Begin game loop!
    SDL_Event e;
    bool coreIsRunning = true;
    Uint64 currentPerf = SDL_GetPerformanceCounter();
    Uint64 lastPerf;
    double deltaTime; // in milliseconds
    Uint64 freq = SDL_GetPerformanceFrequency();
    while (coreIsRunning)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                coreIsRunning = false;
            }
        }

        // Advance the core by the amount of time taken this loop
        lastPerf = currentPerf;
        currentPerf = SDL_GetPerformanceCounter();
        deltaTime = (double)((currentPerf - lastPerf) * 1000) / (double) freq;

        // Send the deltaTime to the system to handle
        core.Update(deltaTime);

        // Update screen with whatever's meant to be displayed from the PPU
        //sdlCtx.UpdateScreen(core.ppu->pixels);
    }

    sdlCtx.SDLQuit();
    return 0;
}