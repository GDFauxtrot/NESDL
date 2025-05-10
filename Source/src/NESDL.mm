#import <AppKit/AppKit.h>
#include <Cocoa/Cocoa.h>

#import "../include/NESDL_OBJCBridge.h"

// Am I doing this right? I wish MacOS menu items supported a simple static function
// reference as a callback, but I'm already bending the intended workflow pretty hard
// so this will do just fine.
@interface NESDLMac : NSObject {
    @public
    NESDL nesdl;
}
// Called from main to kick everything off in Objective-C
- (int) NESDLMain:(int)argc secondValue:(const char*[])argv;
// Menu item callbacks
- (void) showAbout:(nullable id)sender;
- (void) openROM:(nullable id)sender;
- (void) closeROM:(nullable id)sender;
- (void) resetSoft:(nullable id)sender;
- (void) resetHard:(nullable id)sender;
- (void) viewFrameInfo:(nullable id)sender;
- (void) viewResize1x:(nullable id)sender;
- (void) viewResize2x:(nullable id)sender;
- (void) viewResize3x:(nullable id)sender;
- (void) viewResize4x:(nullable id)sender;
- (void) debugRun:(nullable id)sender;
- (void) debugPause:(nullable id)sender;
- (void) debugStepFrame:(nullable id)sender;
- (void) debugStepCPU:(nullable id)sender;
- (void) debugStepPPU:(nullable id)sender;
- (void) debugShowCPU:(nullable id)sender;
- (void) debugShowPPU:(nullable id)sender;
- (void) debugShowNT:(nullable id)sender;
- (void) debugAttachLog:(nullable id)sender;
- (void) debugDetachLog:(nullable id)sender;
@end

@implementation NESDLMac
// Streamlining function to make this process much less painful
// (We're not in the app's responder chain so setTarget is required)
NSMenuItem* CreateMenuItemAndAddToMenu(NSMenu* menu, NSObject* ctx, NSString* name, SEL selector, NSString* charCode)
{
    NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:name action:selector keyEquivalent:charCode];
    [item setTarget:ctx];
    [menu addItem:item];
    return item;
}

NSMenu* AddSubMenuToMenuItem(NSMenuItem* menuItem)
{
    NSMenu* menu = [[NSMenu alloc] init];
    [menuItem setSubmenu:menu];
    return menu;
}

- (int) NESDLMain:(int)argc secondValue:(const char*[])argv
{
    // On Mac -- initialize NESDL (SDL window gets initialized) first
    nesdl.NESDLInit();
 
    // With the window created (thanks SDL!) we can hijack
    // the menu bar entries and load in our own
    NSMenu* mainMenu;
    NSMenu* fileMenu;
    NSMenu* viewMenu;
    NSMenu* debugMenu;
    NSMenuItem* menuItem; // Gets reused for adding menus to the main menu bar
    
    // Get the current main menu from the window
    mainMenu = [[NSApplication sharedApplication] mainMenu];
    
    // Initialize File menu, store in main menu as the first option
    fileMenu = [[NSMenu alloc] initWithTitle:@"File"];
    CreateMenuItemAndAddToMenu(fileMenu, self, @"Open ROM...", @selector(openROM:), @"o");
    CreateMenuItemAndAddToMenu(fileMenu, self, @"Close ROM", @selector(closeROM:), @"c");
    CreateMenuItemAndAddToMenu(fileMenu, self, @"Reset System", @selector(resetSoft:), @"t");
    CreateMenuItemAndAddToMenu(fileMenu, self, @"Reset System (Hard)", @selector(resetHard:), @"r");
    CreateMenuItemAndAddToMenu(fileMenu, nullptr, @"Quit", @selector(performClose:), @"q"); // Built-in action
    menuItem = [[NSMenuItem alloc] init];
    [menuItem setSubmenu:fileMenu];
    [[NSApp mainMenu] insertItem:menuItem atIndex:1];
    
    viewMenu = [[NSMenu alloc] initWithTitle:@"View"];
    NSMenuItem* resizeItem = CreateMenuItemAndAddToMenu(viewMenu, self, @"Resize", @selector(viewFrameInfo:), @"");
    NSMenu* resize = AddSubMenuToMenuItem(resizeItem);
    CreateMenuItemAndAddToMenu(resize, self, @"1x", @selector(viewResize1x:), @"");
    CreateMenuItemAndAddToMenu(resize, self, @"2x", @selector(viewResize2x:), @"");
    CreateMenuItemAndAddToMenu(resize, self, @"3x", @selector(viewResize3x:), @"");
    CreateMenuItemAndAddToMenu(resize, self, @"4x", @selector(viewResize4x:), @"");
    CreateMenuItemAndAddToMenu(viewMenu, self, @"Show Frame Info", @selector(viewFrameInfo:), @"");
    CreateMenuItemAndAddToMenu(viewMenu, self, @"Show CPU Info", @selector(debugShowCPU:), @"");
    CreateMenuItemAndAddToMenu(viewMenu, self, @"Show PPU Info", @selector(debugShowPPU:), @"");
    CreateMenuItemAndAddToMenu(viewMenu, self, @"Show NT Viewer", @selector(debugShowNT:), @"");
    CreateMenuItemAndAddToMenu(viewMenu, self, @"About...", @selector(showAbout:), @"");
    menuItem = [[NSMenuItem alloc] init];
    [menuItem setSubmenu:viewMenu];
    [[NSApp mainMenu] insertItem:menuItem atIndex:2];
    
    debugMenu = [[NSMenu alloc] initWithTitle:@"Debug"];
    CreateMenuItemAndAddToMenu(debugMenu, self, @"Run", @selector(debugRun:), @"o");
    CreateMenuItemAndAddToMenu(debugMenu, self, @"Pause", @selector(debugPause:), @"p");
    CreateMenuItemAndAddToMenu(debugMenu, self, @"Step (Frame)", @selector(debugStepFrame:), @"1");
    CreateMenuItemAndAddToMenu(debugMenu, self, @"Step (CPU)", @selector(debugStepCPU:), @"2");
    CreateMenuItemAndAddToMenu(debugMenu, self, @"Step (PPU)", @selector(debugStepPPU:), @"3");
#ifdef _DEBUG
    CreateMenuItemAndAddToMenu(debugMenu, self, @"Attach Nintendulator Log...", @selector(debugAttachLog:), @"");
    CreateMenuItemAndAddToMenu(debugMenu, self, @"Detach Nintendulator Log", @selector(debugDetachLog:), @"");
#endif
    menuItem = [[NSMenuItem alloc] init];
    [menuItem setSubmenu:debugMenu];
    [[NSApp mainMenu] insertItem:menuItem atIndex:3];
    
    // With the main menu bar finalized, let's get into NESDL proper (and get out of Objective-C ASAP)
    int exitCode = nesdl.NESDLMain(argc, argv);
    return exitCode;
}

- (void) showAbout:(nullable id)sender {
    nesdl.core->Action_ShowAbout();
}
- (void) openROM:(nullable id)sender {
    nesdl.core->Action_OpenROM();
}
- (void) closeROM:(nullable id)sender {
    nesdl.core->Action_CloseROM();
}
- (void) resetSoft:(nullable id)sender {
    nesdl.core->Action_ResetSoft();
}
- (void) resetHard:(nullable id)sender {
    nesdl.core->Action_ResetHard();
}
- (void) viewFrameInfo:(nullable id)sender {
    nesdl.core->Action_ViewFrameInfo();
}
- (void) viewResize1x:(nullable id)sender {
    nesdl.core->Action_ViewResize(1);
}
- (void) viewResize2x:(nullable id)sender {
    nesdl.core->Action_ViewResize(2);
}
- (void) viewResize3x:(nullable id)sender {
    nesdl.core->Action_ViewResize(3);
}
- (void) viewResize4x:(nullable id)sender {
    nesdl.core->Action_ViewResize(4);
}
- (void) debugRun:(nullable id)sender {
    nesdl.core->Action_DebugRun();
}
- (void) debugPause:(nullable id)sender {
    nesdl.core->Action_DebugPause();
}
- (void) debugStepFrame:(nullable id)sender {
    nesdl.core->Action_DebugStepFrame();
}
- (void) debugStepCPU:(nullable id)sender {
    nesdl.core->Action_DebugStepCPU();
}
- (void) debugStepPPU:(nullable id)sender {
    nesdl.core->Action_DebugStepPPU();
}
- (void) debugShowCPU:(nullable id)sender {
    nesdl.core->Action_DebugShowCPU();
}
- (void) debugShowPPU:(nullable id)sender {
    nesdl.core->Action_DebugShowPPU();
}
- (void) debugShowNT:(nullable id)sender {
    nesdl.core->Action_DebugShowNT();
}
- (void) debugAttachLog:(nullable id)sender {
    nesdl.core->Action_AttachNintendulatorLog();
}
- (void) debugDetachLog:(nullable id)sender {
    nesdl.core->Action_DetachNintendulatorLog();
}

@end

int main(int argc, const char* argv[]) {
    NESDLMac *nesdl = [[NESDLMac alloc] init];
    int exitCode = [nesdl NESDLMain:argc secondValue:argv];
    return exitCode;
}
