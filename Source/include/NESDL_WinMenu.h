#pragma once

struct SDL_Window; // Decl needed for pointer refs
class NESDL_Core;

class NESDL_WinMenu
{
public:
	static void Initialize(SDL_Window* window);
	static void HandleWindowEvent(int eventId, NESDL_Core* core);
};