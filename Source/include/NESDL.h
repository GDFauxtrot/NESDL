#pragma once

//Using SDL and standard IO
#ifdef _WIN32
#include <SDL.h>
#include <SDL_ttf.h>
#else
#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>
#endif
#include <stdio.h>

// OS standard libraries
#ifdef __APPLE__
#include <unistd.h>
#endif
#ifdef _WIN32
#include <Windows.h>
#include "SDL_syswm.h"
#include "NESDL_WinMenu.h"
#endif

#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include "NESDL_Constants.h"
#include "NESDL_Input.h"

#ifdef __APPLE__
#include "NESDL_Mapper.h"
#else
#include "mappers/NESDL_Mapper.h"
#endif

#include "NESDL_CPU.h"
#include "NESDL_PPU.h"
#include "NESDL_RAM.h"
#include "NESDL_SDL.h"
#include "NESDL_APU.h"
#include "NESDL_Core.h"
