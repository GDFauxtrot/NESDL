#pragma once

class NESDL_Text
{
public:
    NESDL_Text(const char* text, SDL_Texture* texture, int x, int y)
    :text(text), texture(texture), textColor({0, 0, 0}), x(x), y(y), width(0), height(0), background(false), backgroundFrameColor({24, 0, 80, 128}), backgroundPadding(2), wrapLength(9999) {}
    const char* text;
    SDL_Texture* texture;
    SDL_Color textColor;
    int x;
    int y;
    int width;
    int height;
    bool background;
    SDL_Color backgroundFrameColor;
    int backgroundPadding;
    int wrapLength;
};

class NESDL_SDL
{
public:
	void SDLInit();
	void SDLQuit();
    void SetCore(NESDL_Core* coreRef);
	void UpdateScreen(double fps);
    void UpdateScreenTexture();
    void GetCloseWindowEvent(SDL_WindowEvent event);
    
    void WriteNextAPUSignal(float s);
    
    void ToggleFrameInfo();
    void Resize(int resize);
    void ToggleShowCPU();
    void ToggleShowPPU();
    void ToggleShowNT();
    
    // Functions for handling on-screen text (NESDL_Text)
    NESDL_Text* AddNewScreenText(const char* id, const char* text, int x, int y);
    void SetScreenTextPosition(const char* id, int x, int y);
    void SetScreenTextText(const char* id, const char* text);
    void SetScreenTextColor(const char* id, SDL_Color color);
    void SetScreenTextWrap(const char* id, int wrapLength);
    void RemoveScreenText(const char* id);
    void UpdateScreenTextTexture(const char* id);
    void SetScreenTextShowBackground(const char* id, bool enable);
    void SetScreenTextBackgroundColor(const char* id, SDL_Color color);
    void SetScreenTextBackgroundPadding(const char* id, int padding);
    NESDL_Text* GetScreenText(const char* id);
    int GetScreenTextWidth(const char* id);
    int GetScreenTextHeight(const char* id);
    
    SDL_AudioDeviceID audioDevice;
private:
    unordered_map<const char*, NESDL_Text*> screenText;
    TTF_Font* font;
    NESDL_Core* core;
	SDL_Window* window;
	SDL_Renderer* renderer;
    SDL_Texture* texture;
    SDL_Texture* scanlineTexture;
    
    bool showFrameInfo;
    bool showCPU;
    bool showPPU;
    bool showNTDebugWindow;
    
    SDL_Window* debugWindow;
    SDL_Renderer* debugRenderer;
    SDL_Texture* debugTexture;
};
