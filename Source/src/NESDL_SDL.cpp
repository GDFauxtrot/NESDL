#include "NESDL.h"

void NESDL_SDL::SDLInit()
{
    // Clear the window ptr we'll be rendering to
    window = NULL;

    // Initialize SDL
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
    }
    else
    {
        // Create window and renderer
        SDL_CreateWindowAndRenderer(NESDL_SCREEN_WIDTH, NESDL_SCREEN_HEIGHT,
            SDL_WINDOW_SHOWN, &window, &renderer);
        SDL_SetWindowTitle(window, NESDL_WINDOW_NAME.c_str());
        SDL_SetWindowResizable(window, SDL_TRUE);

        if (window == NULL)
        {
            printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        }
        else
        {
            // Create window texture to draw onto
            texture = SDL_CreateTexture(renderer,
                SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                NESDL_SCREEN_HEIGHT, NESDL_SCREEN_WIDTH);
            
            // Fill the renderer black on clear
            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
            // Clear screen to black and present
            SDL_RenderClear(renderer);
            SDL_RenderPresent(renderer);
        }
    }
}

void NESDL_SDL::SDLQuit()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void NESDL_SDL::UpdateScreen(NESDL_PPU* ppu)
{
    SDL_UpdateTexture(texture, NULL, ppu->frameData, NESDL_SCREEN_WIDTH * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
