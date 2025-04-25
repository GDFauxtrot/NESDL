#include "NESDL.h"

// https://stackoverflow.com/questions/427477/fastest-way-to-clamp-a-real-fixed-floating-point-value
double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

// For MacOS specifically, we must utilize NESDL.mm as our entry point in order to
// gain the ability to insert menu bar items (since SDL doesn't expose this capability).
// NESDL thus becomes an object for the Objective-C++ side to instantiate and manage,
// including sending menu bar selection callbacks.
// For all other platforms, NESDL.cpp is simply a file to write our main function in.
#ifdef __APPLE__
#import "../include/NESDL_OBJCBridge.h"

void NESDL::NESDLInit()
{
    // Initialize SDL first
    sdlCtx = new NESDL_SDL();
    sdlCtx->SDLInit();
    
    // Initialize system (incl. input)
    core = new NESDL_Core();
    core->Init(sdlCtx);
    
    sdlCtx->SetCore(core);
}

int NESDL::NESDLMain(int argc, const char* args[])
#else
int main(int argc, char* args[])
#endif
{
#ifndef __APPLE__
    // Initialize SDL first
    NESDL_SDL* sdlCtx = new NESDL_SDL();
    sdlCtx->SDLInit();
    
    // Initialize system (incl. input)
    NESDL_Core* core = new NESDL_Core();
    core->Init(sdlCtx);
    
    sdlCtx->SetCore(core);
#endif
    
//    printf("%s", getcwd(NULL, 0));

    // SLEEP for a second while we boot up
    SDL_GetPerformanceCounter();

    // Begin game loop!
    SDL_Event e;
    bool isRunning = true;
    Uint64 currentPerf = SDL_GetPerformanceCounter();
    Uint64 lastPerf = currentPerf;
    double deltaTime; // in milliseconds
    Uint64 freq = SDL_GetPerformanceFrequency();
    while (isRunning)
    {
        // Counter check for entire program loop (used for framerate capping, not emulation time)
        uint64_t start = SDL_GetPerformanceCounter();
        
        while (SDL_PollEvent(&e))
        {
            core->HandleEvent((SDL_EventType)e.type, (SDL_KeyCode)e.key.keysym.sym);
#ifdef _WIN32
            if (e.type == SDL_SYSWMEVENT)
            {
                if (e.syswm.msg->msg.win.msg == WM_COMMAND)
                {
                    NESDL_WinMenu::HandleWindowEvent((int)e.syswm.msg->msg.win.wParam, core);
                }
            }
#endif
            if (e.type == SDL_QUIT)
            {
                isRunning = false;
            }
            if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)
            {
                sdlCtx->GetCloseWindowEvent(e.window);
            }
        }
        
        // Advance the core by the amount of time taken this loop
        // (NOTE: Don't simulate a delay longer than 50ms!
        // We can't detect when SDL stalls due to menu bar interaction or debugging, and
        // we don't want to play catch-up for potentially several seconds of simulation)
        double longestMSDelay = 50;
        currentPerf = SDL_GetPerformanceCounter();
        uint64_t delta = (currentPerf - lastPerf);
        deltaTime = clamp((double)delta / (double)freq * 1000, 0, longestMSDelay);
        lastPerf = currentPerf;

        if (core->IsROMLoaded())
        {
            // Send the deltaTime to the system to handle
            core->Update(deltaTime);
        }
        
        // NES screen texture is updated from the core (PPU reports frame readiness directly to SDL!)
        // But we per-loop update the screen anyways for the use of NESDL_Text
        sdlCtx->UpdateScreen((1000/deltaTime));
        
//        printf("\n%llu %llu (%f fps)", core->ppu->currentFrame, core->cpu->elapsedCycles, (1000/deltaTime));
        
        // Cap frame rate to 60 for the application at all times (not fully necessary, but helps with CPU usage)
        uint64_t end = SDL_GetPerformanceCounter();
        double ms = (end - start) / (double)freq * 1000.0;
#ifdef _WIN32
        // Spinlock solution (more accurate, less fluctual but worse perf)
        /*
        int64_t remainder = (1/60.0 * freq) - (end - start);
        uint64_t goal = end + remainder;
        if (remainder > 0)
        {
            // Busy wait, but provides most accurate results
            while (SDL_GetPerformanceCounter() < goal) {}
        }
        */
        SDL_Delay((uint32_t) max(floor(16.6666 - ms), 0)); // Takes whole ms, we'd rather wait less MS than more
#else
        SDL_Delay((uint32_t) max(floor(16.6666 - ms), (double)0)); // Takes whole ms, we'd rather wait less MS than more
        // Can we get a more granular timer?
#endif
    }

    core->Exit();
    sdlCtx->SDLQuit();
    return 0;
}
