#include "NESDL.h"

// TODO okay so as I understand it, we need to do the following to get the PPU running:
// 1) Boot up PPU along with CPU, initialize with values figured out from NESDEV
// 2) Up to a certain amount of frames from start, we disable writes on some registers
//   along with other behaviors
// 3) Once the PPU is "warmed up", we enable register writes
// 4) From the start of the system, we go through the drawing process, drawing lines horizontally
//   and firing HBLANK IRQ's at the end of each line draw. There is a delay between each line
// 5) Once all lines are drawn, we fire a VBLANK and return to line 0, delaying a certain amount
//   of lines as we do so

// Going to need to write logic for handling tile and sprite reads, palette conversion,
// background and foreground drawing, all of that fun stuff.
// All the info we need will be on NESDEV!

// (In our case, we do all of this work on an SDL texture bytewise, and on the VBLANK,
// we tell SDL to display the new texture since it's finished drawing)

// https://www.nesdev.org/wiki/PPU_rendering

void NESDL_PPU::Init(NESDL_Core* c)
{
    elapsedCycles = 0;
    core = c;
    
    tileFetch.nametable = 0;
    tileFetch.attribute = 0;
    tileFetch.pattern = 0;
    incrementV = true;
    oamN = 0;
    oamM = 0;
    secondaryOAMNextSlot = 0;
}

void NESDL_PPU::Update(uint32_t ppuCycles)
{
    // Run instructions to catch up with time
    uint64_t targetCycles = elapsedCycles + ppuCycles;
    while (targetCycles > elapsedCycles)
    {
        RunNextCycle();
    }
}

uint64_t NESDL_PPU::MillisecondsToPPUCycles(double ms)
{
    return (NESDL_PPU_CLOCK / 1000) * ms;
}

bool NESDL_PPU::IsPPUReady()
{
    return elapsedCycles >= NESDL_PPU_CYCLE_READY * 3;
}

// TODO misnomer - we're not running clock cycles one-at-a-time exactly, but
// we're not processing "instructions" like the CPU either. Need a better name!
void NESDL_PPU::RunNextCycle()
{
    // Each PPU cycle is one pixel being rendered. There are 341 cycles per scanline,
    // 262 scanlines per frame. (Although only 256x240 is rendered)
    
    if (currentScanline < 240) // Visible scanlines (0-239)
    {
        HandleProcessVisibleScanline();
    }
    else if (currentScanline > 240 && currentScanline < 262) // VBLANK scanlines (241-261)
    {
        HandleProcessVBlankScanline();
    }
    
    elapsedCycles++;
    
    // Wrap counters around
    if (elapsedCycles % 341 == 0)
    {
        currentScanline++;
    }
    if (currentScanline > 0 && currentScanline % 262 == 0)
    {
        currentScanline = 0;
        currentFrame++;
    }
    
    posInScanline = elapsedCycles % 341;
}

void memset32(void* dest, uint32_t value, uintptr_t size)
{
    uintptr_t i;
    for(i = 0; i < (size & (~3)); i += 4)
    {
        memcpy(((char*)dest) + i, &value, 4);
    }
}

void NESDL_PPU::HandleProcessVisibleScanline()
{
    uint16_t currentScanlineCycle = elapsedCycles % 341; // TODO un-magic this number
    
    if (currentScanline == 0 && currentScanlineCycle == 0)
    {
        
        // Set background color to palette color at 0x3F00
//        uint32_t p = GetPalette(0,false);
//        uint32_t c = GetColor(0,p,0);
//        memset32(frameData, c, sizeof(frameData));
        
        if ((registers.mask & PPUMASK_BGENABLE) != 0x00)
        {
            // THE ONLY TIME we advance the cycle count artificially - this cycle is skipped
            // when the frame count is odd (counting by 0)
            if (currentFrame % 2 == 0)
            {
                elapsedCycles++;
            }
        }
    }
    
    // TILE LOGIC - don't run if BG rendering is disabled
    if ((registers.mask & PPUMASK_BGENABLE) != 0x00)
    {
        // Every 8 pixels (skipping px 0) we repeat the same process for tiles
        uint8_t pixelInFetchCycle = currentScanlineCycle % 8;
        
//        if (currentScanlineCycle == 0 && (registers.mask & PPUMASK_SPRENABLE) != 0x00)
//        {
//            // THE ONLY TIME we advance the cycle count artificially - this cycle is skipped
//            // when the frame count is odd (counting by 0)
//            if (currentFrame % 2 == 0)
//            {
//                elapsedCycles++;
//            }
//        }
//        else if (currentScanlineCycle < 256)
        if (currentScanlineCycle > 0 && currentScanlineCycle < 256)
        {
            FetchAndStoreTile(); // Runs across multiple cycles in order to achieve this
            
            // On the last cycle, render the tile sitting in the shift register and store the loaded tile data in
            if (pixelInFetchCycle == 7) // Cycle 7-8 - Fetch pattern table high bytes (finishes on next cycle 0)
            {
                // Grab the tile to be rendered (from tileBuffer) and render it, advancing our
                // currentDrawX index
                PPUTileFetch toRender = tileFetch;
                //            PPUTileFetch toRender = tileBuffer[1];
                for (int i = 0; i < 8; ++i)
                {
                    uint16_t currentPixel = (currentScanline * PPU_WIDTH) + currentDrawX;
                    uint32_t palette = GetPalette(toRender.paletteIndex, false);
                    uint32_t color = GetColor(toRender.pattern, palette, i);
                    // Tile debugging lines
//                    if (currentScanline % 8 == 0 || currentDrawX % 8 == 0)
//                    {
//                        color += 0x00202020;
//                    }
                    frameData[currentPixel] = color;
                    currentDrawX++;
                }
//                tileBuffer[1] = tileBuffer[0];
//                tileBuffer[0] = tileFetch;
                
                // Coarse X increment (wraps horizontal scroll, technically runs 1 cycle sooner?)
                // https://www.nesdev.org/wiki/PPU_scrolling#Wrapping_around
                if ((registers.v & 0x001F) == 31)   // if coarse X == 31
                {
                    registers.v &= ~0x001F;         // coarse X = 0
                    registers.v ^= 0x0400;          // switch horizontal nametable
                }
                else
                {
                    registers.v += 1;               // increment coarse X
                }
            }
        }
        else if (currentScanlineCycle == 256)
        {
            // Coarse Y increment
            if ((registers.v & 0x7000) != 0x7000)       // if fine Y < 7
            {
                registers.v += 0x1000;                  // increment fine Y
            }
            else
            {
                registers.v &= ~0x7000;                 // fine Y = 0
                int y = (registers.v & 0x03E0) >> 5;    // let y = coarse Y
                if (y == 29)
                {
                    y = 0;                              // coarse Y = 0
                    registers.v ^= 0x0800;              // switch vertical nametable
                }
                else if (y == 31)
                {
                    y = 0;                              // coarse Y = 0, nametable not switched
                }
                else
                {
                    y += 1;                             // increment coarse Y
                }
                registers.v = (registers.v & ~0x03E0) | (y << 5); // put coarse Y back into v
            }
        }
        else if (currentScanlineCycle == 257)
        {
            // Reset horizontal scroll
            registers.v = (registers.v & 0xFBE0) | (registers.t & 0x41F);
            currentDrawX = 0;
        }
        else if (currentScanlineCycle < 321)
        {
            // TODO "tile data for the sprites on the next scanline are fetched here"
        }
        else if (currentScanlineCycle < 337)
        {
            FetchAndStoreTile(); // Run tile fetch for the next two tiles (takes exactly 16 cycles which is what we want)
            
            if (pixelInFetchCycle == 7)
            {
                //            tileBuffer[1] = tileBuffer[0];
                //            tileBuffer[0] = tileFetch;
            }
        }
        else if (currentScanlineCycle < 341)
        {
            // Two NT bytes are fetched but is really not necessary (except for maper MMC5, we can write a special case in for that)
        }
    }
    
    // SPRITE LOGIC - don't run if sprite rendering is disabled (or we're on line 0, we don't run)
    if ((registers.mask & PPUMASK_SPRENABLE) != 0x00 && currentScanline > 0)
    {
        // Secondary OAM is cleared to 0xFF over 64 cycles, but we can get away with just doing it all at once
        if (currentScanlineCycle < 64)
        {
        }
        else if (currentScanlineCycle == 64)
        {
            memset(secondaryOAM, 0xFF, sizeof(secondaryOAM));
            // Reset counters and flags before sprite evaluation
            oamN = 0;
            oamM = 0;
            secondaryOAMNextSlot = 0;
            sprFetchIndex = 0;
            registers.status &= ~(PPUSTATUS_SPROVERFLOW | PPUSTATUS_SPR0HIT);
            
        }
        else if (currentScanlineCycle <= 256)
        {
            if (currentScanlineCycle % 2 == 1)
            {
                // Odd cycle - read from OAM
                if (secondaryOAMNextSlot < 8)
                {
                    secondaryOAM[secondaryOAMNextSlot*4] = oam[oamN*4 + oamM];
                }
            }
            else
            {
                // Even cycle - write to secondary OAM (if not full yet, otherwise flag overflow)
                if (secondaryOAMNextSlot != 8 && oamN < 64)
                {
                    // Check the sprite Y we evaluated last. If the sprite falls on this scanline,
                    // then it's "in range" and we fill secondary OAM with its info before moving on
                    // to the next slot
                    uint8_t sprY = secondaryOAM[secondaryOAMNextSlot*4];
                    uint8_t sprHeight = (registers.ctrl & PPUCTRL_SPRHEIGHT) != 0x00 ? 16 : 8;
                    if (currentScanline >= sprY+1 && currentScanline < (sprY+1) + sprHeight)
                    {
                        // Sprite in range - copy the rest of the data to secondary OAM
                        for (int i = 1; i < 4; ++i)
                        {
                            secondaryOAM[secondaryOAMNextSlot*4 + i] = oam[oamN*4 + (++oamM)];
                            if (oamM == 3)
                            {
                                oamM = 0;
                                oamN++;
                            }
                        }
                        if (++secondaryOAMNextSlot == 8)
                        {
                            registers.status |= PPUSTATUS_SPROVERFLOW;
                        }
                    }
                    else
                    {
                        // Sprite NOT in range - just increment N
                        oamN++;
                    }
                }
                else
                {
                    oamN++;
                }
            }
        }
        else if (currentScanlineCycle <= 320)
        {
            // Sprite fetching/drawing! First 4 cycles are for reading, last 4 are for writing
            uint8_t pixelInFetchCycle = currentScanlineCycle % 8;
            
            if (pixelInFetchCycle == 4)
            {
                // The hardware doesn't technically "render" during this time, we just got next scanline's
                // sprite data ready to go. But we're an emulator, and the wiki isn't clear to me on what
                // the PPU is doing with these read cycles, so... let's just render!
                if (sprFetchIndex < secondaryOAMNextSlot)
                {
                    // Get sprite info
                    uint8_t sprY    = secondaryOAM[sprFetchIndex*4];
                    uint8_t sprTile = secondaryOAM[sprFetchIndex*4 + 1];
                    uint8_t sprAttr = secondaryOAM[sprFetchIndex*4 + 2];
                    uint8_t sprX    = secondaryOAM[sprFetchIndex*4 + 3];
                    
                    // Get sprite attributes
                    uint8_t sprPaletteIndex = sprAttr & SPR_PALETTE;
                    bool sprTilePriority    = (sprAttr & SPR_PRIORITY) != 0x00;
                    bool sprFlipHorizontal  = (sprAttr & SPR_FLIPX) != 0x00;
                    bool sprFlipVertical    = (sprAttr & SPR_FLIPY) != 0x00;
                    bool sprIs8By16 = (registers.ctrl & PPUCTRL_SPRHEIGHT) != 0x00;
                    
                    // Find out where we'll be starting to render on this line
                    uint16_t sprStartX = (currentScanline * PPU_WIDTH) + sprX;
                    uint16_t sprRow = (currentScanline - sprY - 1) % (sprIs8By16 ? 16 : 8);
                    
                    // Flip tile read if the sprite's vertical flip bit is set
                    if (sprFlipVertical)
                    {
                        sprRow = (sprIs8By16 ? 15 : 7) - sprRow;
                    }
                    
                    // Get pattern for this sprite, decode to a version that can be palettized
                    uint16_t patternAddr = 0;
                    if (sprIs8By16)
                    {
                        // 8x16 sprite works a bit differently - bit 0 selects nametable, the rest
                        // selects the top tile (bottom sprite tile is the next one over)
                        patternAddr = (sprTile & 0x1) * 0x100;
                        // We should be selecting the second (bottom) tile instead
                        if (sprRow >= 8)
                        {
                            patternAddr += 1;
                            sprRow -= 8;
                        }
                        patternAddr = ((patternAddr + (sprTile & 0xFE)) << 4) + sprRow;
                    }
                    else
                    {
                        // 8x8 sprite
                        patternAddr = ((registers.ctrl & PPUCTRL_SPRTILE) >> 3) * 0x100;
                        patternAddr = ((patternAddr + sprTile) << 4) + sprRow;
                    }
                    uint8_t ptrnLow = ReadFromVRAM(patternAddr);
                    uint8_t ptrnHi = ReadFromVRAM(patternAddr + 8);
                    uint16_t sprPattern = WeavePatternBits(ptrnLow, ptrnHi);
                    
                    // Go through all 8 pixels for this sprite and draw them according to the sprite's attributes
                    for (uint8_t i = 0; i < 8; ++i)
                    {
                        uint8_t index = sprFlipHorizontal ? (7-i) : i;
                        
                        // Get pixel position on the scanline + loop X offset
                        uint16_t currentPixel = sprStartX + index;
                        
                        // We're going out of bounds on screen - return early
                        if (currentPixel >= (PPU_WIDTH * PPU_HEIGHT))
                        {
                            break;
                        }
                        
                        uint32_t palette = GetPalette(sprPaletteIndex, true);
                        uint32_t color = GetColor(sprPattern, palette, index);
                        
                        if (sprTilePriority)
                        {
                            // We may discard this pixel if the tile takes priority
                            if (frameData[currentPixel] != NESDL_PALETTE[paletteData[0]])
                            {
                                continue;
                            }
                            // TODO need a better way to detect this? Some tiles may use the same color
                            // as 0x3F00 but not be transparent. Curious how the NES does this w/o
                            // multiple screen buffers
                            
                            // If this is sprite zero, mark that we rendered it at least one pixel!
                            if (sprFetchIndex == 0)
                            {
                                registers.status |= PPUSTATUS_SPR0HIT;
                            }
                        }
                        
                        frameData[currentPixel] = color;
                    }
                    
                    // Advance sprite fetch index since we successfully drew this one
                    sprFetchIndex++;
                }
            }
        }
        // The rest of the cycles are unused by the sprite evaluation/rendering loop
    }
}

void NESDL_PPU::FetchAndStoreTile()
{
    uint16_t currentScanlineCycle = elapsedCycles % 341; // TODO un-magic this number
    // Every 8 pixels (skipping px 0) we repeat the same process for tiles
    uint8_t pixelInFetchCycle = currentScanlineCycle % 8;
    
    /*
     FROM: https://www.nesdev.org/wiki/PPU_programmer_reference#Internal_registers
     
     Conceptually, the PPU does this 33 times for each scanline:

         Fetch a nametable entry from $2000-$2FFF.
         Fetch the corresponding attribute table entry from $23C0-$2FFF and increment the current VRAM address within the same row.
         Fetch the low-order byte of an 8x1 pixel sliver of pattern table from $0000-$0FF7 or $1000-$1FF7.
         Fetch the high-order byte of this sliver from an address 8 bytes higher.
         Turn the attribute data and the pattern table data into palette indices, and combine them with data from sprite data using priority.

     It also does a fetch of a 34th (nametable, attribute, pattern) tuple that is never used, but some mappers rely on this fetch for timing purposes.
     */
    
    if (pixelInFetchCycle == 1)  // Cycle 1-2 - Fetch nametable
    {
        // Read the byte from memory at PPUADDR and store it in our NT register
        uint16_t addr = registers.v;
        addr = 0x2000 | (addr & 0xFFF); // Only keep 12 bits and combine into NT address space
        tileFetch.nametable = ReadFromVRAM(addr);
    }
    if (pixelInFetchCycle == 3) // Cycle 3-4 - Fetch attribute table
    {
        uint8_t ntSelect = (registers.v & 0xC00); // NT select occupies same bits at AT address, no shifting needed
        uint8_t colSelect = (registers.v >> 2) & 0x7;
        uint8_t rowSelect = (registers.v >> 7) & 0x7;
        uint16_t atAddr = 0x23C0 | ntSelect | (rowSelect << 3) | colSelect;
        tileFetch.attribute = ReadFromVRAM(atAddr);
        
        // While we're here, also get this tile's palette index from attribute
        // Count the attribute quadrants as indices: [0, 1]
        //                                           [2, 3]
        uint8_t colQuadSelect = registers.v & 0x2; // Bit 2 of coarse X
        uint8_t rowQuadSelect = (registers.v >> 5) & 0x2; // Bit 2 of coarse Y
        // Select quad for this tile (2 tiles per quad per row/column)
        uint8_t quadIndex = (colQuadSelect >> 1) + rowQuadSelect;
        
        // Take this index and fetch the two bits for our attribute tile (palette index)
        tileFetch.paletteIndex = (tileFetch.attribute >> (quadIndex * 2)) & 0x3;
    }
    if (pixelInFetchCycle == 5) // Cycle 5-6 - Fetch pattern table low bytes
    {
        // Since we don't need to emulate the two fetches separately (as far as I'm aware),
        // we can simply put it all here in one pass for sake of simplicity
        
        // Start with which pattern bank to read from
        uint16_t patternAddr = ((registers.ctrl & PPUCTRL_BGTILE) >> 4) * 0x100;
        uint8_t rowIndex = currentScanline % 8;
//        if (currentScanline < 48)
//        {
//            printf("ADDR: %#06x\tSCANLINE: %d\tINDEX: %d\tNT: %d\n", registers.v, currentScanline, rowIndex, tileFetch.nametable);
//        }
        patternAddr = ((patternAddr + tileFetch.nametable) << 4) + rowIndex;
        
        uint8_t ptrnLow = ReadFromVRAM(patternAddr);
        uint8_t ptrnHi = ReadFromVRAM(patternAddr + 8);
        
        // Weave bits together and store
        tileFetch.pattern = WeavePatternBits(ptrnLow, ptrnHi);
    }
    if (pixelInFetchCycle == 7) // Cycle 7-8 - Fetch pattern table high bytes (finishes on next cycle 0)
    {
        // Since we performed both pattern table fetches together already, there's nothing to do here.
        // tileFetch has been fully constructed
    }
}

void NESDL_PPU::HandleProcessVBlankScanline()
{
    uint16_t currentScanlineCycle = elapsedCycles % 341; // TODO un-magic this number
    
    // Every 8 pixels (skipping px 0) we repeat the same process for tiles
    uint8_t pixelInFetchCycle = currentScanlineCycle % 8;
    
    if (currentScanline == 241)
    {
        // Set VBLANK FLAG at this exact moment (241, 1)
        if (currentScanlineCycle == 1)
        {
            if (!disregardVBL)
            {
                registers.status |= PPUSTATUS_VBLANK;
            }
            if (!disregardNMI && (registers.ctrl & PPUCTRL_NMIENABLE) != 0x00)
            {
                // Also triggers an NMI (VBlank NMI)
                core->cpu->nmi = true;
            }
            disregardVBL = false;
        }
        if (currentScanlineCycle == 7)
        {
            // Ideally NMI fires at VBL+9 (since it begins vectoring around VBL+2 or 4?)
            // Since we're an emulator and can run this all instantaneously*, we wait to
            // fire NMI until it's officially ready on hardware to do so
//            if ((registers.ctrl & PPUCTRL_NMIENABLE) != 0x00)
//            {
//                // Also triggers an NMI (VBlank NMI)
//                core->cpu->nmi = true;
//            }
        }
    }
    else if (currentScanline == 261)
    {
        // Pre-render scanline
        bool isRenderingEnabled = (registers.mask & PPUMASK_BGENABLE) != 0x00 | (registers.mask & PPUMASK_SPRENABLE) != 0x00;
        
        // Clear ALL status flags
        if (currentScanlineCycle == 1)
        {
            registers.status &= 0x1F;
        }
        
        if (currentScanlineCycle > 0 && currentScanlineCycle < 256)
        {
            // Don't run logic if BG rendering is disabled
            if ((registers.mask & PPUMASK_BGENABLE) == 0x00)
            {
                return;
            }
            
            FetchAndStoreTile(); // Runs across multiple cycles in order to achieve this
            
            // On the last cycle, render the tile sitting in the shift register and store the loaded tile data in
            if (pixelInFetchCycle == 7) // Cycle 7-8 - Fetch pattern table high bytes (finishes on next cycle 0)
            {
                // Grab the tile to be rendered but don't render (dummy scanline)
//                tileBuffer[1] = tileBuffer[0];
//                tileBuffer[0] = tileFetch;
                
                // Coarse X increment (wraps horizontal scroll, technically runs 1 cycle sooner?)
                // https://www.nesdev.org/wiki/PPU_scrolling#Wrapping_around
//                if ((registers.v & 0x001F) == 31)   // if coarse X == 31
//                {
//                    registers.v &= ~0x001F;         // coarse X = 0
//                    registers.v ^= 0x0400;          // switch horizontal nametable
//                }
//                else
//                {
//                    registers.v += 1;               // increment coarse X
//                }
            }
        }
        else if (currentScanlineCycle == 256)
        {
            // Coarse Y increment
//            if ((registers.v & 0x7000) != 0x7000)       // if fine Y < 7
//            {
//                registers.v += 0x1000;                  // increment fine Y
//            }
//            else
//            {
//                registers.v &= ~0x7000;                 // fine Y = 0
//                int y = (registers.v & 0x03E0) >> 5;    // let y = coarse Y
//                if (y == 29)
//                {
//                    y = 0;                              // coarse Y = 0
//                    registers.v ^= 0x0800;              // switch vertical nametable
//                }
//                else if (y == 31)
//                {
//                    y = 0;                              // coarse Y = 0, nametable not switched
//                }
//                else
//                {
//                    y += 1;                             // increment coarse Y
//                }
//                registers.v = (registers.v & ~0x03E0) | (y << 5); // put coarse Y back into v
//            }
        }
        else if (currentScanlineCycle == 257)
        {
            // Reset horizontal scroll
            if (isRenderingEnabled)
            {
                registers.v = (registers.v & 0xFBE0) | (registers.t & 0x41F);
            }
            currentDrawX = 0;
        }
        else if (currentScanlineCycle < 321)
        {
            if (isRenderingEnabled)
            {
                // TODO "tile data for the sprites on the next scanline are fetched here"
                
                // Special case for pre-render scanline: reset vertical scroll (repeatedly)
                if (currentScanlineCycle >= 280 && currentScanlineCycle <= 304)
                {
                    registers.v = (registers.v & 0x041F) | (registers.t & 0xFBE0);
                }
            }
        }
        else if (currentScanlineCycle < 337)
        {
            // Don't run logic if BG rendering is disabled
            if ((registers.mask & PPUMASK_BGENABLE) == 0x00)
            {
                return;
            }
            
            FetchAndStoreTile(); // Run tile fetch for the next two tiles (takes exactly 16 cycles which is what we want)
            
            if (pixelInFetchCycle == 7)
            {
//                tileBuffer[1] = tileBuffer[0];
//                tileBuffer[0] = tileFetch;
                registers.v += 1;               // increment coarse X
            }
        }
        else if (currentScanlineCycle < 341)
        {
            // Two NT bytes are fetched but is really not necessary (except for maper MMC5, we can write a special case in for that)
        }
    }
}

void NESDL_PPU::PreprocessPPUForReadInstructionTiming(uint8_t instructionPPUTime)
{
    uint32_t cyclesThisFrame = (currentScanline * 341) + posInScanline;
    uint32_t cyclesAfterInstruction = cyclesThisFrame + instructionPPUTime;
    uint32_t setTarget = ((241 * 341) + 1);
    uint32_t clearTarget = ((261 * 341) + 1);
    
    // If we're before PPU cycle (241,1) and the next CPU instruction occurs when we pass over this cycle,
    // just set the VBL flag right away
//    if (cyclesThisFrame < target && cyclesThisFrame + instructionPPUTime >= target)
//    {
//        // Race condition - "Reading PPUSTATUS within two cycles of the start of VBL will return 0" (ouch)
//        if (cyclesThisFrame < (target-1))
//        {
//            registers.status |= PPUSTATUS_VBLANK;
//            if ((registers.ctrl & PPUCTRL_NMIENABLE) != 0x00)
//            {
//                // Also triggers an NMI (VBlank NMI)
//                core->cpu->nmi = true;
//            }
//            vblAlreadyHappened = true;
//        }
//        else
//        {
//            disregardVBL = true;
//        }
//    }
    if (!disregardVBL)
    {
        // Extra-specific timing thing - if we're reading ON (241,1), the PPU hasn't run a step yet. So let's just... VBL
        if (cyclesThisFrame == setTarget)
        {
            registers.status |= PPUSTATUS_VBLANK;
        }
        if (cyclesThisFrame < setTarget && cyclesAfterInstruction >= (setTarget-1))
        {
            // Some weird VBL race conditions:
            
            // 1) Reading target-1 results in no VBL, no NMI
            if (cyclesAfterInstruction == setTarget-1)
            {
                registers.status &= ~PPUSTATUS_VBLANK;
                disregardVBL = true;
                disregardNMI = true;
            }
            // 2) Reading target or target+1 results in a VBL but no NMI
            else if (cyclesAfterInstruction == setTarget || cyclesAfterInstruction == setTarget+1)
            {
                registers.status |= PPUSTATUS_VBLANK;
                disregardVBL = true;
                disregardNMI = true;
            }
            // Reading 2+ frames around VBL results in normal behavior
            else
            {
                registers.status |= PPUSTATUS_VBLANK;
                disregardVBL = true;
                disregardNMI = false;
            }
        }
        // Another edge case - we crossed over (261,1) on read, so by this point, the VBL flag should be cleared (among others)
        else if (cyclesThisFrame < clearTarget && cyclesAfterInstruction >= clearTarget)
        {
            registers.status &= 0x1F;
        }
    }
    
}

void NESDL_PPU::WriteToRegister(uint16_t registerAddr, uint8_t data)
{
    // Do not allow writes when the PPU is still warming up
    if ((!IsPPUReady() && (registerAddr == PPU_PPUCTRL ||
                         registerAddr == PPU_PPUMASK ||
                         registerAddr == PPU_PPUSCROLL ||
                         registerAddr == PPU_PPUADDR)) || ignoreChanges)
    {
        return;
    }
    
    switch (registerAddr)
    {
        case PPU_PPUCTRL:
            // If NMI flag is flipped from 0 to 1 and PPUSTATUS VBL flag is set, we trigger an NMI immediately
            if ((registers.status & PPUSTATUS_VBLANK) != 0x00 && (registers.ctrl & PPUCTRL_NMIENABLE) == 0x00 && (data & PPUCTRL_NMIENABLE) != 0x00)
            {
                core->cpu->nmi = true;
            }
            registers.ctrl = data;
            // Write nametable bits to register t
            registers.t = (registers.t & 0xF3FF) | ((uint16_t(data) & 0x3) << 10);
            
            break;
        case PPU_PPUMASK:
            registers.mask = data;
            break;
        case PPU_PPUSTATUS:
            // Undefined behavior to write to PPUSTATUS -- do nothing
            break;
        case PPU_OAMADDR:
            registers.oamAddr = data;
            break;
        case PPU_OAMDATA:
            // "Probably best to completely ignore writes during rendering" - NESDEV
//            if (currentScanline >= 240)
            {
                vram[registers.oamAddr++] = data;
            }
            break;
        case PPU_PPUSCROLL:
            // Writes scroll position data to registers t and x depending on latch w
            if (!registers.w)
            {
                // High 5 bits of data become low 5 bits of t (coarse X), low 3 bits become x (fine X)
                registers.t = (registers.t & 0xFFE0) | (uint16_t(data) >> 3);
                registers.x = data & 0x7;
            }
            else
            {
                // Setting y value is trickier
                registers.t = (registers.t & 0xC1F) |
                    (uint16_t(data) << 12) |
                    ((uint16_t(data) & 0xC0) << 2) |
                    ((uint16_t(data) & 0x38) << 2);
                registers.y = data;
                // t is not assigned to v as we cannot change scroll position mid-frame via PPUSCROLL
            }
            registers.w = !registers.w;
            break;
        case PPU_PPUADDR:
            // Depending on register w, write to either the low or high byte of t.
            // If we wrote both, store t into v
            if (!registers.w)
            {
                // High byte first (t bit 14 is set to 0, bit 15 is nonexistent on hardware)
                registers.t = (registers.t & 0x00FF) | ((uint16_t(data) & 0x3F) << 8);
            }
            else
            {
                // Low byte second
                registers.t = (registers.t & 0xFF00) | data;
                // Store completed 15-bit register into v
                registers.v = registers.t;
            }
            registers.w = !registers.w;
            break;
        case PPU_PPUDATA:
            WriteToVRAM(registers.v, data);
            // Increment v by the amount specified from PPUCTRL bit 2
            if (incrementV)
            {
                registers.v += (registers.ctrl & PPUCTRL_INCMODE) != 0 ? 32 : 1;
            }
            break;
        case PPU_OAMDMA:
            // DMA write is a very involved process - the CPU is halted while RAM is transferred to OAM.
            // Since we're an emulator and the STA/X instruction is still running, if we alter
            // the CPU's elapsed cycle count now it will effectively halt the very next instruction
//            core->cpu->HaltCPUForDMAWrite();
            core->cpu->dmaDelayNextFrame = true;
            // Transfer data from 0x##00 - 0x##FF
            uint16_t targetAddr = data << 8;
            for (int i = 0; i < 256; ++i)
            {
                oam[i] = core->ram->ReadByte(targetAddr + i);
            }
            break;
    }
}

uint8_t NESDL_PPU::ReadFromRegister(uint16_t registerAddr)
{
    // In case address is mirrored
    if (registerAddr >= 0x2000 && registerAddr < 0x4000)
    {
        registerAddr = 0x2000 + (registerAddr % 0x8);
    }
    else if (registerAddr != PPU_OAMDMA)
    {
        // Uh-oh where did we get this address from
        return 0;
    }
    
    switch (registerAddr)
    {
        case PPU_PPUCTRL:
            // Undefined behavior to read from PPUCTRL -- do nothing
            break;
        case PPU_PPUMASK:
            // Undefined behavior to read from PPUMASK -- do nothing
            break;
        case PPU_PPUSTATUS:
            {
                // On PPUSTATUS read, always set latch w to 0
                if (ignoreChanges)
                {
                    return registers.status;
                }
                else
                {
                    registers.w = false;
                    uint8_t output = registers.status;
                    // Also reset VBLANK on read
                    registers.status &= ~PPUSTATUS_VBLANK;
                    // TODO emulate "PPU stale bus contents" for some flimsily copy-protected titles
                    return output;
                }
            }
            break;
        case PPU_OAMADDR:
            // Undefined behavior to read from OAMADDR -- do nothing
            break;
        case PPU_OAMDATA:
            // Some models don't have read functionality. We can assume developers knew this at the time, and don't bother implementing this (right?)
            return 0xFF;
        case PPU_PPUSCROLL:
            // Undefined behavior to read from PPUSCROLL -- do nothing
            break;
        case PPU_PPUADDR:
            // Undefined behavior to read from PPUADDR -- do nothing
            break;
        case PPU_PPUDATA:
            // We return data stored in the PPUDATA buffer rather than VRAM directly
            {
                if (ignoreChanges)
                {
                    return ppuDataReadBuffer;
                }
                else
                {
                    if (registers.v >= 0x3F00)
                    {
                        return ReadFromVRAM(registers.v);
                    }
                    else
                    {
                        uint8_t vramData = ppuDataReadBuffer;
                        ppuDataReadBuffer = ReadFromVRAM(registers.v);
                        // TODO support palette RAM reading rules ( it's complicated, see: https://www.nesdev.org/wiki/PPU_registers#Reading_palette_RAM )
                        // Increment v by the amount specified from PPUCTRL bit 2
                        if (incrementV)
                        {
                            registers.v += (registers.ctrl & PPUCTRL_INCMODE >> 2) == 0 ? 1 : 32;
                        }
                        return vramData;
                    }
                }
            }
            break;
        case PPU_OAMDMA:
            // Undefined behavior to read from OAMDMA -- do nothing
            break;
    }
    
    return 0;
}

uint8_t NESDL_PPU::ReadFromVRAM(uint16_t addr)
{
    // Depending on mirroring, we return values different
    // TODO assuming horizontal mirroring for now!
    
    // Accessing CHR ROM/RAM data from cartridge
    if (addr < 0x2000)
    {
        return chrData[addr];
    }
    // Mirrored address space(s) 0x3000-0x3EFF -> 0x2000-0x2EFF
    if (addr >= 0x3000 && addr < 0x3F00)
    {
        addr -= 0x1000;
    }
    if (addr >= 0x3F20)
    {
        addr = 0x3F00 | (addr % 0x20);
    }
    
    if (addr >= 0x2000 && addr < 0x3000)
    {
        // TODO nametable mirroring! Assuming horizontal
        if (addr >= 0x2800)
        {
            addr = 0x2000 | (addr % 0x800);
        }
        return vram[addr - 0x2000];
    }
    else if (addr >= 0x3F00)
    {
        return paletteData[addr - 0x3F00];
    }
    return 0;
}

void NESDL_PPU::WriteToVRAM(uint16_t addr, uint8_t data)
{
    if (addr < 0x2000)
    {
        // Read-only ROM data - ignore (for now - TODO implement CHR-RAM)
        chrData[addr] = data;
        return;
    }
    // Mirrored address space(s)
    if (addr >= 0x3000 && addr < 0x3F00)
    {
        addr -= 0x1000;
    }
    if (addr >= 0x3F20)
    {
        addr = 0x3F00 | (addr % 0x20);
    }
    
    if (addr >= 0x2000 && addr < 0x3000)
    {
        // TODO nametable mirroring! Assuming horizontal
        if (addr >= 0x2800)
        {
            addr = 0x2000 | (addr % 0x800);
        }
        vram[addr - 0x2000] = data;
    }
    else if (addr >= 0x3F00)
    {
        uint16_t index = (addr - 0x3F00) % 0x20;
        paletteData[index] = data;
        // Palette data for color index 0 is mirrored across both BG and sprite palettes
        if (index % 4 == 0)
        {
            paletteData[index >= 0x10 ? (index-0x10) : (index+0x10)] = data;
        }
    }
}

void NESDL_PPU::WriteCHRROM(uint8_t* addr)
{
    memcpy(chrData, addr, sizeof(chrData));
}

uint16_t NESDL_PPU::WeavePatternBits(uint8_t low, uint8_t high)
{
    // For a given set of 8 low and 8 high bits, "weave" them together
    // Example:
    // low = abcdefgh | high = ABCDEFGH
    // result = AaBbCcDdEeFfGgHh
    uint16_t result = 0;
    for (uint8_t i = 0; i < 8; i++)
    {
        uint8_t low16 = (low & (1 << i)) >> i;
        uint8_t high16 = (high & (1 << i)) >> i;
        result |= (low16 << (i*2)) | (high16 << ((i*2) + 1));
    }
    return result;
}

uint32_t NESDL_PPU::GetPalette(uint8_t index, bool spriteLayer)
{
    uint8_t i = spriteLayer ? (index * 4) + 0x10 : (index * 4);
    // Return the 4 bytes corresponding to this index (first color is backdrop unless under certain circumstances)
    return paletteData[spriteLayer ? 0x10 : 0] << 24 | paletteData[i+1] << 16 | paletteData[i+2] << 8 | paletteData[i+3];
}

uint32_t NESDL_PPU::GetColor(uint16_t pattern, uint32_t palette, uint8_t pixelIndex)
{
    // Use pixel index to select 2 bits of pattern - 0 selects 2 highest, 7 selects 2 lowest
    uint8_t index = ((7 - pixelIndex) * 2);
    uint8_t patternBits = (pattern & (0x3 << index)) >> index;
    // Take this 0-3 index and select the palette byte
    uint32_t paletteShifted = palette >> (24 - patternBits*8);
    uint8_t paletteIndex = paletteShifted & 0xFF;
    if ((registers.mask & PPUMASK_GREYSCALE) != 0x00) // Only select from four greyscale palette values
    {
        paletteIndex &= 0x30;
    }
    // Get color from palette index!
    uint32_t resultColor = NESDL_PALETTE[paletteIndex];
//    if ((registers.mask & (PPUMASK_TINT_R | PPUMASK_TINT_G | PPUMASK_TINT_B)) != 0x00)
//    {
//        // Tint result colors if needed
//        uint8_t r = (resultColor & 0xFF0000) >> 16;
//        uint8_t g = (resultColor & 0x00FF00) >> 8;
//        uint8_t b = (resultColor & 0x0000FF);
//        if ((registers.mask & PPUMASK_TINT_R) != 0x00)
//        {
//            r /= 0.816328;
//            g *= 0.816328;
//            b *= 0.816328;
//        }
//        if ((registers.mask & PPUMASK_TINT_G) != 0x00)
//        {
//            r *= 0.816328;
//            g /= 0.816328;
//            b *= 0.816328;
//        }
//        if ((registers.mask & PPUMASK_TINT_B) != 0x00)
//        {
//            r *= 0.816328;
//            g *= 0.816328;
//            b /= 0.816328;
//        }
//        resultColor = MakeColor(r, g, b);
//    }
    
    
    return resultColor;
}
