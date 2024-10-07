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

## Building

Windows and Mac build information (and builds) can be provided soon. I am currently prioritizing development as the project is so early, namely getting more titles supported and controls support.

## License

This software is licensed under [GNU GPLv3](https://choosealicense.com/licenses/gpl-3.0/), and has been included in this repository (see LICENSE for a copy)