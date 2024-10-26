#include "NESDL.h"
#include <cmath>

// https://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    int size_s = std::snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    if( size_s <= 0 ){ throw std::runtime_error( "Error during formatting." ); }
    auto size = static_cast<size_t>( size_s );
    std::unique_ptr<char[]> buf( new char[ size ] );
    std::snprintf( buf.get(), size, format.c_str(), args ... );
    return std::string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

// https://stackoverflow.com/questions/35926722/what-is-the-format-specifier-for-binary-in-c
// (Modified)
std::string print_bin(uint8_t byte)
{
    std::string output;
    int i = CHAR_BIT; /* however many bits are in a byte on your platform */
    while(i--) {
        output += ('0' + ((byte >> i) & 1)); /* loop through and print the bits */
    }
    return output;
}

void NESDL_SDL::WriteNextAPUSignal(float signal)
{
    SDL_QueueAudio(audioDevice, &signal, 4);
}

void NESDL_SDL::SDLInit()
{
    // Clear the window ptr we'll be rendering to
    window = NULL;
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return;
    }
    
    // Initialize SDL audio
    SDL_AudioSpec spec =
    {
        .freq = APU_SAMPLE_RATE,
        .format = AUDIO_F32,
        .channels = 1,
        .samples = APU_SAMPLE_BUF,
        .callback = NULL,
        .userdata = this
    };
    audioDevice = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
    if (audioDevice <= 0)
    {
        printf("Failed to open Audio Device: %s\n", SDL_GetError());
    }
    SDL_PauseAudioDevice(audioDevice, 0);
    
    // Create window and renderer
    SDL_CreateWindowAndRenderer(NESDL_SCREEN_WIDTH, NESDL_SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN, &window, &renderer);
    SDL_SetWindowTitle(window, NESDL_WINDOW_NAME.c_str());
    SDL_SetWindowResizable(window, SDL_TRUE);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    if (window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        // Create window texture to draw onto
        texture = SDL_CreateTexture(renderer,
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
            NESDL_SCREEN_WIDTH, NESDL_SCREEN_HEIGHT);
        
        // Fill the renderer black on clear
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        // Clear screen to black and present
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
        
        // Get a pre-made "scanline" texture ready to go
        scanlineTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, NESDL_SCREEN_WIDTH, NESDL_SCREEN_HEIGHT);
        SDL_SetTextureBlendMode(scanlineTexture, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 16, 0, 48, 64);
        SDL_SetRenderTarget(renderer, scanlineTexture);
        for (int y = 0; y < NESDL_SCREEN_HEIGHT; y += 4)
        {
            SDL_Rect r = { 0, y, NESDL_SCREEN_WIDTH, 2 };
            SDL_RenderFillRect(renderer, &r);
        }
        SDL_SetRenderTarget(renderer, nullptr);
    }
    
    // Create nametable debug window and renderer
    SDL_CreateWindowAndRenderer(256, 256, // 64x64 but rendered 4x, since it's so tiny
        SDL_WINDOW_HIDDEN, &debugWindow, &debugRenderer);
    SDL_SetWindowTitle(debugWindow, "NESDL NT Debug");
    SDL_SetWindowResizable(debugWindow, SDL_TRUE);

    if (debugWindow == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        // Create window texture to draw onto
        debugTexture = SDL_CreateTexture(debugRenderer,
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
            64, 64);
        
        // Fill the renderer black on clear
        SDL_SetRenderDrawColor(debugRenderer, 0, 0, 0, 255);
        // Clear screen to black and present
        SDL_RenderClear(debugRenderer);
        SDL_RenderPresent(debugRenderer);
    }
    
    //Initialize SDL_TTF
    if(TTF_Init() == -1)
    {
        printf("SDL_TTF could not initialize! SDL_TTF Error: %s\n", TTF_GetError());
    }
    // Open and load font(s)
    font = TTF_OpenFont("font/Inconsolata-Regular.ttf", 12);
}

void NESDL_SDL::SDLQuit()
{
    // Destroy all text textures
    for(const auto& textKV: screenText) {
        SDL_DestroyTexture(textKV.second->texture);
    }
    
    SDL_CloseAudio();
    TTF_CloseFont(font);
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

void NESDL_SDL::SetCore(NESDL_Core* coreRef)
{
    core = coreRef;
}

void NESDL_SDL::UpdateScreen(double fps)
{
    // Feels a bit hacky - I want some specific NESDL_Text string values to update to specific things
    if (showFrameInfo)
    {
        const char* format = "(%.2fFPS) Frame %llu";
        string s = string_format(format, fps, core->ppu->currentFrame);
        SetScreenTextText("frameinfo", s.c_str());
    }
    if (showCPU)
    {
        const char* format = "PC: %04X\nSP: %02X\nA: %02X\nX: %02X\nY: %02X\nP: %s";
        string s = string_format(format, core->cpu->registers.pc, core->cpu->registers.sp, core->cpu->registers.a, core->cpu->registers.x, core->cpu->registers.y, print_bin(core->cpu->registers.p).c_str());
        SetScreenTextText("cpu", s.c_str());
    }
    if (showPPU)
    {
        const char* format = "CTRL: %02X\nMASK: %02X\nSTAT: %02X\nOAM: %02X\nPPU: %04X\nLine: %d\nPos: %d\nX: %d";
        string s = string_format(format, core->ppu->registers.ctrl, core->ppu->registers.mask, core->ppu->registers.status, core->ppu->registers.oamAddr, core->ppu->registers.v, core->ppu->currentScanline, core->ppu->currentScanlineCycle, core->ppu->registers.x);
        SetScreenTextText("ppu", s.c_str());
        if (showCPU)
        {
            NESDL_Text* cpuText = GetScreenText("cpu");
            NESDL_Text* ppuText = GetScreenText("ppu");
            ppuText->y = cpuText->height + 4;
        }
        else
        {
            NESDL_Text* ppuText = GetScreenText("ppu");
            ppuText->y = 0;
        }
    }
    
    // Draw game's frame data to screen
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    
    // Draw an overlay over the screen when a game isn't inserted
    if (core->romLoaded == false)
    {
        int w;
        int h;
        SDL_GetWindowSize(window, &w, &h);
        SDL_SetRenderDrawColor(renderer, 48, 0, 144, 64);
        SDL_Rect rect = { 0, 0, w, h };
        SDL_RenderFillRect(renderer, &rect);
        SDL_RenderCopy(renderer, scanlineTexture, nullptr, nullptr);
    }
    
    // Scale NESDL_Text size to window
    int winX;
    int winY;
    SDL_GetWindowSize(window, &winX, &winY);
    double winMulX = (double)winX / NESDL_SCREEN_WIDTH;
    double winMulY = (double)winY / NESDL_SCREEN_HEIGHT;
    
    // Iterate through NESDL_Text instances and draw them
    for(const auto& textKV: screenText) {
        NESDL_Text* value = textKV.second;
        if (value->texture != nullptr)
        {
            if (value->background)
            {
                SDL_Color bgColor = value->backgroundFrameColor;
                SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
                int pad = value->backgroundPadding;
                int x = (int)((value->x - pad) * winMulX);
                int y = (int)((value->y - pad) * winMulY);
                int w = (int)((value->width + pad * 2) * winMulX);
                int h = (int)((value->height + pad * 2) * winMulY);
                SDL_Rect backgroundRect = { x, y, w, h };
                SDL_RenderFillRect(renderer, &backgroundRect); // Background fill render
                SDL_RenderDrawRect(renderer, &backgroundRect); // Background border render
            }
            if (value->texture != nullptr)
            {
                int x = (int)(value->x * winMulX);
                int y = (int)(value->y * winMulY);
                int w = (int)(value->width * winMulX);
                int h = (int)(value->height * winMulY);
                SDL_Rect renderRect = { x, y, w, h };
                SDL_RenderCopy(renderer, value->texture, nullptr, &renderRect); // Text render
            }
        }
    }
    
    SDL_RenderPresent(renderer);
    
    // Debug Window
    SDL_RenderClear(debugRenderer);
    SDL_RenderCopy(debugRenderer, debugTexture, NULL, NULL);
    SDL_RenderPresent(debugRenderer);
}

void NESDL_SDL::UpdateScreenTexture()
{
    SDL_UpdateTexture(texture, NULL, core->ppu->frameData, NESDL_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_UpdateTexture(debugTexture, NULL, core->ppu->ntFrameData, 64 * sizeof(uint32_t));
}

void NESDL_SDL::GetCloseWindowEvent(SDL_WindowEvent event)
{
    uint32_t winID = SDL_GetWindowID(window);
    uint32_t debugWinID = SDL_GetWindowID(debugWindow);
    
    // Main window exit - just let SDL know we're good and done here
    if (event.windowID == winID)
    {
        SDL_Event quit { SDL_QUIT };
        SDL_PushEvent(&quit);
    }
    // Debug window exit - hide it and reset the shown flag
    if (event.windowID == debugWinID)
    {
        showNTDebugWindow = false;
        SDL_HideWindow(debugWindow);
    }
}

void NESDL_SDL::ToggleFrameInfo()
{
    showFrameInfo = !showFrameInfo;
    
    if (showFrameInfo)
    {
        NESDL_Text* text = AddNewScreenText("frameinfo", "(00.00fFPS) Frame 0", 0, 0);
        text->background = true;
        text->backgroundPadding = 0;
        text->textColor = { 255, 255, 255 };
        SetScreenTextPosition("frameinfo", 0, NESDL_SCREEN_HEIGHT - text->height);
    }
    else
    {
        RemoveScreenText("frameinfo");
    }
}

void NESDL_SDL::Resize(int resize)
{
    SDL_SetWindowSize(window, NESDL_SCREEN_WIDTH * resize, NESDL_SCREEN_HEIGHT * resize);
}

void NESDL_SDL::ToggleShowCPU()
{
    showCPU = !showCPU;
    
    if (showCPU)
    {
        NESDL_Text* text = AddNewScreenText("cpu", "PC: 0000\nSP: 00\nA: 00\nX: 00\nY: 00\nP: 00000000", 0, 0);
        text->background = true;
        text->backgroundPadding = 0;
        text->textColor = { 255, 255, 255 };
        SetScreenTextWrap("cpu", 60);
        SetScreenTextPosition("cpu", NESDL_SCREEN_WIDTH - text->width, 0);
        UpdateScreenTextTexture("cpu");
    }
    else
    {
        RemoveScreenText("cpu");
    }
}

void NESDL_SDL::ToggleShowPPU()
{
    showPPU = !showPPU;
    
    if (showPPU)
    {
        NESDL_Text* text = AddNewScreenText("ppu", "CTRL: 00\nMASK: 00\nSTAT: 00\nOAM: 00\nPPU: 0000\nLine: 000\nPos: 000\nX: 00\nY: 00", 0, 0);
        text->background = true;
        text->textColor = { 255, 255, 255 };
        SetScreenTextWrap("ppu", 60);
        text->x = NESDL_SCREEN_WIDTH - text->width;
        text->y = 0;
        if (showCPU)
        {
            NESDL_Text* cpuText = GetScreenText("cpu");
            text->y = cpuText->height + 4;
        }
    }
    else
    {
        RemoveScreenText("ppu");
    }
}

void NESDL_SDL::ToggleShowNT()
{
    showNTDebugWindow = !showNTDebugWindow;
    
    if (showNTDebugWindow)
    {
        SDL_ShowWindow(debugWindow);
    }
    else
    {
        SDL_HideWindow(debugWindow);
    }
}

/// Creates a new NESDL_Text instance with the given ID. This allows for lots of flexibility
/// to draw information to the screen, and store rendered font textures for reuse.
NESDL_Text* NESDL_SDL::AddNewScreenText(const char* id, const char* text, int x, int y)
{
    // Do nothing if we've already added this ID
    if (screenText.contains(id))
    {
        return screenText[id];
    }
    screenText[id] = new NESDL_Text("", nullptr, x, y);
    // Use the helper function already made to initialize the texture
    SetScreenTextText(id, text);
    return screenText[id];
}

void NESDL_SDL::SetScreenTextPosition(const char* id, int x, int y)
{
    if (!screenText.contains(id))
    {
        return;
    }
    screenText[id]->x = x;
    screenText[id]->y = y;
}

void NESDL_SDL::SetScreenTextText(const char* id, const char* text)
{
    if (!screenText.contains(id))
    {
        return;
    }
    screenText[id]->text = text;
    UpdateScreenTextTexture(id);
}

void NESDL_SDL::SetScreenTextColor(const char* id, SDL_Color color)
{
    if (!screenText.contains(id))
    {
        return;
    }
    screenText[id]->textColor = color;
    UpdateScreenTextTexture(id);
}

void NESDL_SDL::SetScreenTextWrap(const char* id, int wrapLength)
{
    if (!screenText.contains(id))
    {
        return;
    }
    screenText[id]->wrapLength = wrapLength;
    UpdateScreenTextTexture(id);
}

void NESDL_SDL::RemoveScreenText(const char* id)
{
    screenText.erase(id);
}

void NESDL_SDL::UpdateScreenTextTexture(const char* id)
{
    if (!screenText.contains(id))
    {
        return;
    }
    NESDL_Text* nesdlText = screenText[id];
    
    // Remove old texture (if one exists)
    if (nesdlText->texture != nullptr)
    {
        SDL_DestroyTexture(nesdlText->texture);
    }
    
    // Don't continue if empty string (causes an error)
    if (strcmp(nesdlText->text, "") == 0)
    {
        return;
    }
    
    // Draw text to an SDL_Surface first
    SDL_Surface* textSurface = TTF_RenderText_Solid_Wrapped(font, nesdlText->text, nesdlText->textColor, nesdlText->wrapLength);
    if (textSurface == nullptr)
    {
        printf("Unable to render text surface! SDL_TTF Error: %s\n", TTF_GetError());
    }
    else
    {
        // Create the texture from the drawn surface
        nesdlText->texture = SDL_CreateTextureFromSurface(renderer, textSurface);
        if (nesdlText->texture == nullptr)
        {
            printf("Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError());
        }
        else
        {
            // Store image dimensions from the result
            nesdlText->width = textSurface->w;
            nesdlText->height = textSurface->h;
        }

        // Free surface (it's served its purpose)
        SDL_FreeSurface(textSurface);
    }
}

void NESDL_SDL::SetScreenTextShowBackground(const char* id, bool enable)
{
    if (!screenText.contains(id))
    {
        return;
    }
    screenText[id]->background = enable;
}

void NESDL_SDL::SetScreenTextBackgroundColor(const char* id, SDL_Color color)
{
    if (!screenText.contains(id))
    {
        return;
    }
    screenText[id]->backgroundFrameColor = color;
}

void NESDL_SDL::SetScreenTextBackgroundPadding(const char* id, int padding)
{
    if (!screenText.contains(id))
    {
        return;
    }
    screenText[id]->backgroundPadding = padding;
}

NESDL_Text* NESDL_SDL::GetScreenText(const char* id)
{
    if (screenText.contains(id))
    {
        return screenText[id];
    }
    return nullptr;
}

int NESDL_SDL::GetScreenTextWidth(const char* id)
{
    if (!screenText.contains(id))
    {
        return 0;
    }
    return screenText[id]->width;
}

int NESDL_SDL::GetScreenTextHeight(const char* id)
{
    if (!screenText.contains(id))
    {
        return 0;
    }
    return screenText[id]->height;
}
