#include "NESDL.h"
#include <unistd.h>

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
    
    string romFile = "nestest.nes";
//    string romFile = "Super Mario Bros.nes";
    
    printf("%s", getcwd(NULL, 0));

    // Load ROM into core
    core.LoadRom(romFile.c_str());
    
    core.StartSystem();
    
    this_thread::sleep_for(chrono::seconds(2));
    
    // SLEEP for a second while we boot up
    SDL_GetPerformanceCounter();

    // Begin game loop!
    SDL_Event e;
    bool coreIsRunning = true;
    bool isFirstFrame = true;
    Uint64 currentPerf = SDL_GetPerformanceCounter();
    Uint64 lastPerf = currentPerf;
    double deltaTime; // in milliseconds
    Uint64 freq = SDL_GetPerformanceFrequency();
    while (coreIsRunning)
    {
        uint64_t start = SDL_GetPerformanceCounter();
        
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT)
            {
                coreIsRunning = false;
            }
        }
        if (isFirstFrame)
        {
            isFirstFrame = false;
            currentPerf = SDL_GetPerformanceCounter();
            lastPerf = currentPerf;
        }

        // Advance the core by the amount of time taken this loop
        currentPerf = SDL_GetPerformanceCounter();
        uint64_t delta = (currentPerf - lastPerf);
        deltaTime = (double)delta / (double)freq * 1000;

//        printf("\n%llu\t%llu\t%llu\t%f\n", currentPerf, lastPerf, delta, deltaTime);
        
        lastPerf = currentPerf;

        // Send the deltaTime to the system to handle
        core.Update(deltaTime);
        // Update screen with whatever's meant to be displayed from the PPU
        sdlCtx.UpdateScreen(core.ppu);
        
        printf("\n%llu %llu (%f fps)", core.ppu->currentFrame, core.cpu->elapsedCycles, (1000/deltaTime));
        
        uint64_t end = SDL_GetPerformanceCounter();
        double ms = (end - start) / (double)freq * 1000.0;
        
        // Cap frame rate to 60!
        SDL_Delay(floor(16.666 - ms));
    }

    sdlCtx.SDLQuit();
    return 0;
}
