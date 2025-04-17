#ifdef _WIN32
#include "NESDL.h"

#define ID_FILE_OPEN	101
#define ID_FILE_CLOSE	102
#define ID_FILE_RESET	103
#define ID_FILE_RESETH	104
#define ID_FILE_QUIT	105

#define ID_VIEW_RESIZE1	201
#define ID_VIEW_RESIZE2	202
#define ID_VIEW_RESIZE3	203
#define ID_VIEW_RESIZE4	204
#define ID_VIEW_FRAME	205
#define ID_VIEW_SHOWCPU	206
#define ID_VIEW_SHOWPPU	207
#define ID_VIEW_SHOWNT	208
#define ID_VIEW_ABOUT	209

#define ID_DBUG_RUN		301
#define ID_DBUG_PAUSE	302
#define ID_DBUG_STEPFRM	303
#define ID_DBUG_STEPCPU	304
#define ID_DBUG_STEPPPU	305

void NESDL_WinMenu::Initialize(SDL_Window* window)
{
    // Get HWND first
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window, &wmInfo);
    HWND hwnd = wmInfo.info.win.window;

    // Create menu bar items
    HMENU mainMenu = CreateMenu();
    HMENU fileMenu = CreateMenu();
    HMENU resizeMenu = CreateMenu();
    HMENU viewMenu = CreateMenu();
    HMENU debugMenu = CreateMenu();

    // Initialize File menu, store in main menu as the first option
    AppendMenu(mainMenu, MF_POPUP, (UINT_PTR)fileMenu, L"File");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_OPEN, L"Open ROM...");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_CLOSE, L"Close ROM");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_RESET, L"Reset System");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_RESETH, L"Reset System (Hard)");
    AppendMenu(fileMenu, MF_STRING, ID_FILE_QUIT, L"Quit");
    
    // View menu is next
    AppendMenu(mainMenu, MF_POPUP, (UINT_PTR)viewMenu, L"View");
    AppendMenu(viewMenu, MF_POPUP, (UINT_PTR)resizeMenu, L"Resize");
    AppendMenu(resizeMenu, MF_STRING, ID_VIEW_RESIZE1, L"1x");
    AppendMenu(resizeMenu, MF_STRING, ID_VIEW_RESIZE2, L"2x");
    AppendMenu(resizeMenu, MF_STRING, ID_VIEW_RESIZE3, L"3x");
    AppendMenu(resizeMenu, MF_STRING, ID_VIEW_RESIZE4, L"4x");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_FRAME, L"Show Frame Info");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_SHOWCPU, L"Show CPU Info");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_SHOWPPU, L"Show PPU Info");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_SHOWNT, L"Show NT Viewer");
    AppendMenu(viewMenu, MF_STRING, ID_VIEW_ABOUT, L"About...");

    // Debug menu is last
    AppendMenu(mainMenu, MF_POPUP, (UINT_PTR)debugMenu, L"Debug");
    AppendMenu(debugMenu, MF_STRING, ID_DBUG_RUN, L"Run");
    AppendMenu(debugMenu, MF_STRING, ID_DBUG_PAUSE, L"Pause");
    AppendMenu(debugMenu, MF_STRING, ID_DBUG_STEPFRM, L"Step (Frame)");
    AppendMenu(debugMenu, MF_STRING, ID_DBUG_STEPCPU, L"Step (CPU)");
    AppendMenu(debugMenu, MF_STRING, ID_DBUG_STEPPPU, L"Step (PPU)");

    // attach menu bar to the window
    SetMenu(hwnd, mainMenu);
}

void NESDL_WinMenu::HandleWindowEvent(int eventId, NESDL_Core* core)
{
    switch (eventId)
    {
        case ID_FILE_OPEN:
            core->Action_OpenROM();
            break;
        case ID_FILE_CLOSE:
            core->Action_CloseROM();
            break;
        case ID_FILE_RESET:
            core->Action_ResetSoft();
            break;
        case ID_FILE_RESETH:
            core->Action_ResetHard();
            break;
        case ID_FILE_QUIT:
            core->Action_Quit();
            break;
        case ID_VIEW_RESIZE1:
            core->Action_ViewResize(1);
            break;
        case ID_VIEW_RESIZE2:
            core->Action_ViewResize(2);
            break;
        case ID_VIEW_RESIZE3:
            core->Action_ViewResize(3);
            break;
        case ID_VIEW_RESIZE4:
            core->Action_ViewResize(4);
            break;
        case ID_VIEW_FRAME:
            core->Action_ViewFrameInfo();
            break;
        case ID_VIEW_SHOWCPU:
            core->Action_DebugShowCPU();
            break;
        case ID_VIEW_SHOWPPU:
            core->Action_DebugShowPPU();
            break;
        case ID_VIEW_SHOWNT:
            core->Action_DebugShowNT();
            break;
        case ID_VIEW_ABOUT:
            core->Action_ShowAbout();
            break;
        case ID_DBUG_RUN:
            core->Action_DebugRun();
            break;
        case ID_DBUG_PAUSE:
            core->Action_DebugPause();
            break;
        case ID_DBUG_STEPFRM:
            core->Action_DebugStepFrame();
            break;
        case ID_DBUG_STEPCPU:
            core->Action_DebugStepCPU();
            break;
        case ID_DBUG_STEPPPU:
            core->Action_DebugStepPPU();
            break;
    }
}
#endif
