#pragma once

class NESDL_SDL
{
public:
	void SDLInit();
	void SDLQuit();
	void UpdateScreen(NESDL_PPU* ppu);
private:
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture1;
    SDL_Texture* texture2;
    bool render1;
};
