# NESDL

A NES emulator written in C++ and SDL2. Just a fun project to see if I could make one, nothing more! Made for Mac and PC, but a Linux build should be possible as well.

**Note:** this is *EARLY* in development! It is missing several features, such as:

  * Some popular ROM mappers (like MMC3, for now)
  * Player 2 (the logic is in place, but no keybinds yet)
  * Non-standard controller support (eg. Zapper, Power Pad, and so on)
  * Rebindable controls
  * Save/load states
  * Cartridge save data (eg. Excitebike stages, Kirby's Adventure progress, etc.)
  * PAL region support

![](img/title.png)


## Controls

Currently, controls are hard-bound to the keyboard (mainly for me to use while development is under-way):

  * **D-Pad** - WASD
  * **A** - N
  * **B** - M
  * **SELECT** - Comma (,)
  * **START** - Period (.)


## To-Do

***Each action item is checkmarked to show progress*** (Updated 5/14/2025)

### General/Misc

- [x] Basic APU support (audio)
  - Not fully working but mostly capable, some issues or inconsistencies may occur
- [ ] Configuration file
  * Store Player 1/2 controls
  * Store audio level
- [ ] Save/load state support
  * Could it be cross-emulator supported - FCEUX/Nintendulator/NESTopia loads our save state? Is there an agreed-upon standard here?
- [ ] Rebindable keys menu
  * May be more difficult since I'm not using QT/IMGUI which would've possibly made this a breeze. Hopefully SDL has something
- [ ] NES 2.0 header support
- [ ] Famicom emulation (& PAL support)
- [ ] Ubuntu & Arch package builds?


### Mappers

According to [this list](https://github.com/rynorris/nes/issues/31) (source info is currently down), the following mappers make up nearly 90% of the entire NES library, in order of popularity:

- [X] Mapper 1 (MMC1)
- [ ] Mapper 4 (MMC3)
- [ ] Mapper 2 (UxROM)
- [x] Mapper 0 (NROM)
- [ ] Mapper 3 (CNROM)
- [ ] Mapper 7 (AxROM)

Low priority:

- [ ] Mapper 206 (DxROM/earlier MMC3)
- [ ] Mapper 11 (Color Dreams)
- [ ] Mapper 5 (MMC5) (notably Castlevania III)
- [ ] Mapper 19 (Namco 129/163)

Some notable mappers I want implemented as well:

- [x] Mapper 9 (just (Mike Tyson's) Punch-Out!!)


## Building

Development instructions are heavy work-in-progress, due to lack of time and a bit of personal inexperience making open source software that's easy to download and compile.

### Dependencies

This project is built using **[SDL2](https://github.com/libsdl-org/SDL/releases)** (2.30.6) and **[SDL2_ttf](https://github.com/libsdl-org/SDL_ttf/releases)** (2.22.0), you will want to download the development (or "devel") versions of each on their official GitHub pages.

### Windows

This project was heavily developed on Mac, but started and is fully capable on Windows as well (thanks SDL!)

NESDL has been tested on Visual Studio 2017+ and Windows 10+, though I see no issues with running NESDL on a Windows OS as old as SDL2 will allow (it may be fun to test this).

Make sure your dev environment has SDL2 properly configured! I need to make this process easier but what you'll need to do is [outlined in this guide by Lazy Foo'](https://lazyfoo.net/tutorials/SDL/01_hello_SDL/windows/msvc2019/index.php). (Or, just unzip the SDL devel libraries in the same folder as mine, "C:\\vclibs\\...", and then either make sure SDL2 and SDL2_ttf is in your system PATH or DLL's included in your build directory.)

A final step to ensure fonts are working in your dev builds - include the "font" folder where your NESDL build executables are.

### Mac

As of writing this (4/14/2025), it has been some time since I have opened the project on a Mac device. The same dependencies are in place for Mac as they are for Windows (a development copy of SDL2 and SDL2_ttf configured for your local Xcode development environment, ideally matching versions). At some point I will ensure the project can be easily cloned and ran with proper instructions!


## License

This software is licensed under [GNU GPLv3](https://choosealicense.com/licenses/gpl-3.0/), and has been included in this repository (see LICENSE for a copy)

This software uses the library [nativefiledialog](https://github.com/mlabbe/nativefiledialog). Its license [can be found here](https://github.com/mlabbe/nativefiledialog/blob/master/LICENSE). Some modification of the source code has been performed, including removing the removal of platform-specific files; any further modifications have been denoted in the source code itself.
