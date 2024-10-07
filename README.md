# NESDL

A NES emulator written in C++ and SDL2. Just a fun project to see if I could make one, nothing more! Made for Mac and PC, but a Linux build should be possible as well.

**Note:** this is *EARLY* in development! It is missing several features, such as:

  * (most importantly) **NROM titles ONLY!** That means only NTSC (US) titles from [this list](https://nescartdb.com/search/advanced?ines=0) work.
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

***Each action item is checkmarked to show progress*** (Updated 10/7/2024)

### General/Misc

- [ ] Basic APU support (audio)
- [ ] Configuration file
  * Store Player 1/2 controls
  * Store audio level
- [ ] Save/load state support
  * Could it be cross-emulator supported - FCEUX/Nintendulator/NESTopia loads our save state? Is there an agreed-upon standard here?
- [ ] Rebindable keys menu
  * May be more difficult since I'm not using QT/IMGUI which would've possibly made this a breeze. Hopefully SDL has something
- [ ] Famicom emulation (& PAL support)

### Mappers

According to [this list](https://github.com/rynorris/nes/issues/31) (source info is currently down), the following mappers make up nearly 90% of the entire NES library, in order of popularity:

- [ ] Mapper 1 (MMC1)
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

- [ ] Mapper 9 (just (Mike Tyson's) Punch-Out!!)


## Building

Windows and Mac build information (and builds) can be provided soon. I am currently prioritizing development as the project is so early, namely getting more titles supported and controls support.

## License

This software is licensed under [GNU GPLv3](https://choosealicense.com/licenses/gpl-3.0/), and has been included in this repository (see LICENSE for a copy)