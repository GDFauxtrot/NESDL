#pragma once

#if __APPLE__
#include "NESDL.h"

class NESDL
{
public:
    void NESDLInit();
    int NESDLMain(int argc, const char* args[]);
    NESDL_SDL* sdlCtx;
    NESDL_Core* core;
};
#endif
