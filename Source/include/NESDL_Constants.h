#pragma once

//Screen dimension constants
const int NESDL_SCREEN_WIDTH = 256;
const int NESDL_SCREEN_HEIGHT = 240;
const string NESDL_WINDOW_NAME = "NESDL";

// Measured in hz
const long NESDL_CPU_CLOCK = 21477272; // 236.25s MHz / 11
const long NESDL_PPU_CLOCK = NESDL_CPU_CLOCK * 3; // 3x faster than CPU