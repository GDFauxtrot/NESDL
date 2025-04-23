#include "NESDL.h"

void NESDL_PPU::Init(NESDL_Core* c)
{
    core = c;
}

void NESDL_PPU::Reset(bool hardReset)
{
    elapsedCycles = 21;
    currentFrame = 0;
    currentScanline = 0;
    currentScanlineCycle = 0;
    currentDrawX = 0;
	ppuOpenBus = 0x1F; // Not always on IRL hardware, but most of the time
    registers.ctrl = 0;
    registers.mask = 0;
    if (hardReset)
    {
        registers.oamAddr = 0;
        registers.v = 0;
    }
    else
    {
        registers.status &= PPUSTATUS_VBLANK; // Keep VBL, the rest go to 0
    }
    registers.t = 0;
    registers.x = 0;
    registers.w = false;
    
    tileFetch.nametable = 0;
    tileFetch.attribute = 0;
    tileFetch.pattern = 0;
    tileFetch.paletteIndex = 0;
    
    incrementV = true; // NESDL flag - a bit hacky though
    ignoreChanges = false;
    oamN = 0;
    oamM = 0;
    secondaryOAMNextSlot = 0;
    disregardVBL = false;
    disregardNMI = false;
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

void NESDL_PPU::SetMapper(NESDL_Mapper* m)
{
    mapper = m;
}

bool NESDL_PPU::IsPPUReady()
{
    return elapsedCycles >= NESDL_PPU_READY * 3;
}

void NESDL_PPU::RunNextCycle()
{
    // Each PPU cycle is one pixel being rendered. There are 341 cycles per scanline,
    // 262 scanlines per frame. (Although only 256x240 is rendered)
    
    if (currentScanline < 240) // Visible scanlines (0-239)
    {
        HandleProcessVisibleScanline();
    }
    else if (currentScanline == 240)
    {
        // Signal to core that we've finished drawing to our frame data
        if (currentScanlineCycle == 0)
        {
            frameDataReady = true;
        }
    }
    else if (currentScanline > 240 && currentScanline < 262) // VBLANK scanlines (241-261)
    {
        HandleProcessVBlankScanline();
    }
    
    elapsedCycles++;
    currentScanlineCycle = elapsedCycles % 341;
    
    // Wrap counters around
    if (currentScanlineCycle == 0)
    {
        currentScanline++;
    }
    if (currentScanline > 0 && currentScanline % 262 == 0)
    {
        currentScanline = 0;
        currentFrame++;
    }

    // MMC3 only: clock IRQ at certain times
    if (mapper->mapperNumber == 4)
    {
        // 8x16 mode
        if (registers.ctrl & PPUCTRL_SPRHEIGHT)
        {
            if (registers.ctrl & PPUCTRL_BGTILE && !(registers.ctrl & PPUCTRL_SPRTILE))
            {
                // If BG=0x1000 (& Spr = 0x0000), clock IRQ at PPU=324
                if (currentScanlineCycle == 324)
                {
                    //printf("\nIRQ Clock (Scanline 324)");
                    ((NESDL_Mapper_4*)mapper)->ClockIRQ();
                }
            }
            else if (!(registers.ctrl & PPUCTRL_BGTILE) && registers.ctrl & PPUCTRL_SPRTILE)
            {
                // If BG=0x0000 (& Spr = 0x1000), clock IRQ at PPU=260
                if (currentScanlineCycle == 260)
                {
                    //printf("\nIRQ Clock (Scanline 260)");
                    ((NESDL_Mapper_4*)mapper)->ClockIRQ();
                }
            }
        }
        // 8x8 mode
        else
        {
            if (registers.ctrl & PPUCTRL_BGTILE && !(registers.ctrl & PPUCTRL_SPRTILE))
            {
                // If BG=0x1000 (& Spr = 0x0000), clock IRQ at PPU=324
                if (currentScanlineCycle == 324)
                {
                    //printf("\nIRQ Clock (Scanline 324)");
                    ((NESDL_Mapper_4*)mapper)->ClockIRQ();
                }
            }
            else if (!(registers.ctrl & PPUCTRL_BGTILE) && registers.ctrl & PPUCTRL_SPRTILE)
            {
                // If BG=0x0000 (& Spr = 0x1000), clock IRQ at PPU=260
                if (currentScanlineCycle == 260)
                {
                    //printf("\nIRQ Clock (Scanline 260)");
                    ((NESDL_Mapper_4*)mapper)->ClockIRQ();
                }
            }
        }
    }
}

void NESDL_PPU::HandleProcessVisibleScanline()
{
    if (currentScanline == 0 && currentScanlineCycle == 0)
    {
        if (currentFrame > 0)
        {
            frameFinished = true;
        }
        
        // Clear frameDataSprite table for next frame
        memset(frameDataSprite, 0x3F, sizeof(frameDataSprite));
        
        if ((registers.mask & PPUMASK_BGENABLE) != 0x00)
        {
            // THE ONLY TIME we advance the cycle count artificially - this cycle is skipped
            // when the frame count is odd (counting by 0)
            if (currentFrame % 2 == 0)
            {
                elapsedCycles++;
                currentScanlineCycle = elapsedCycles % 341;
            }
        }
    }
    
    // TILE LOGIC - don't run if BG rendering is disabled
    if ((registers.mask & PPUMASK_BGENABLE) != 0x00)
    {
        // Every 8 pixels (skipping px 0) we repeat the same process for tiles
        uint8_t pixelInFetchCycle = currentScanlineCycle % 8;
        
        if (currentScanlineCycle > 0 && currentScanlineCycle < 256)
        {
            FetchAndStoreTile(pixelInFetchCycle); // Runs across multiple cycles in order to achieve this
            
            // On the last cycle, render the tile sitting in the shift register and store the loaded tile data in
            if (pixelInFetchCycle == 7) // Cycle 7-8 - Fetch pattern table high bytes (finishes on next cycle 0)
            {
                // Grab the tile to be rendered (from tileBuffer) and render it, advancing our
                // currentDrawX index
                PPUTileFetch toRender = tileBuffer[1];
                tileBuffer[1] = tileBuffer[0];
                tileBuffer[0] = tileFetch;
                
                uint8_t start = 0;
                uint8_t end = 8;
                
                uint8_t tileIndex = (currentScanlineCycle - 7) / 8;
                if (tileIndex == 0)
                {
                    start = (registers.x & 0x7);
                }
                
                for (int i = start; i < end; ++i)
                {
                    uint16_t currentPixel = (currentScanline * PPU_WIDTH) + currentDrawX;
                    
                    // Don't render if we wrote a sprite pixel here, unless the BG tile has priority
                    uint8_t patternBits = GetPatternBits(toRender.pattern, i);
                    bool bgPriority = (frameDataSprite[currentPixel] & 0x80) != 0x00;
                    bool bgOverSprite = bgPriority && patternBits != 0x00;
                    bool sprDrawn = (frameDataSprite[currentPixel] & 0x40) != 0x00;
                    if (bgOverSprite || !sprDrawn)
                    {
                        uint32_t palette = GetPalette(toRender.paletteIndex, false);
                        uint32_t color = GetColor(toRender.pattern, palette, i);
                        // Tile debugging lines
//                        if (currentScanline % 8 == 0 || currentDrawX % 8 == 0)
//                        {
//                            color += 0x00202020;
//                        }
                        frameData[currentPixel] = color;
                        if (patternBits != 0x00)
                        {
                            frameDataSprite[currentPixel] = 0x80 | (frameDataSprite[currentPixel] & 0x7F);
                        }
                    }
                    currentDrawX++;
                }
                
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
                
                // Not sure how else to pull this off? We want to render the last tile at 0x#400/0x#C00
                if (tileIndex == 31 && (registers.x & 0x7) != 0x00)
                {
                    FetchAndStoreTile(1);
                    FetchAndStoreTile(3);
                    FetchAndStoreTile(5);
                    
                    toRender = tileBuffer[1];
                    
                    uint8_t i = 0;
                    while (currentDrawX < 256)
                    {
                        uint16_t currentPixel = (currentScanline * PPU_WIDTH) + currentDrawX;
                        
                        // Don't render if we wrote a sprite pixel here, unless the BG tile has priority
                        uint8_t patternBits = GetPatternBits(toRender.pattern, i);
                        bool bgPriority = (frameDataSprite[currentPixel] & 0x80) != 0x00;
                        bool bgOverSprite = bgPriority && patternBits != 0x00;
                        bool sprDrawn = (frameDataSprite[currentPixel] & 0x40) != 0x00;
                        if (bgOverSprite || !sprDrawn)
                        {
                            uint32_t palette = GetPalette(toRender.paletteIndex, false);
                            uint32_t color = GetColor(toRender.pattern, palette, i);
                            frameData[currentPixel] = color;
                            if (patternBits != 0x00)
                            {
                                frameDataSprite[currentPixel] = 0x80 | (frameDataSprite[currentPixel] & 0x7F);
                            }
                        }
                        currentDrawX++;
                        i++;
                    }
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
            // "Tile data for the sprites on the next scanline are fetched here"
        }
        else if (currentScanlineCycle < 337)
        {
            // Run tile fetch for the next two tiles
            FetchAndStoreTile(pixelInFetchCycle);
            
            if (pixelInFetchCycle == 7)
            {
//                PPUTileFetch toRender = tileBuffer[1];
                tileBuffer[1] = tileBuffer[0];
                tileBuffer[0] = tileFetch;
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
        else if (currentScanlineCycle < 341)
        {
            // Two NT bytes are fetched, replicated for mapper timing purposes (eg. MMC2, MMC5)
            if (currentScanlineCycle % 2 == 1)
            {
                FetchAndStoreTile(1);
            }
        }
    }
    else
    {
        // BG rendering is disabled = we should be rendering backdrop instead
        uint16_t currentPixel = (currentScanline * PPU_WIDTH) + currentScanlineCycle;
        frameData[currentPixel] = NESDL_PALETTE[paletteData[0]];
    }
    
    // SPRITE LOGIC - don't run if sprite rendering is disabled (or we're on line 0, we don't run)
    if ((registers.mask & PPUMASK_SPRENABLE) != 0x00)
    {
        // Draw this line's sprite data
        if (currentScanlineCycle < 256 && currentScanline > 0)
        {
            for (int i = 0; i < sprDataToDrawCount; ++i)
            {
                PPUSprFetch sprData = sprDataToDraw[i];
                
                // Exit early - we're not rendering (yet)
                if (currentScanlineCycle < sprData.startX || currentScanlineCycle >= sprData.startX + 8)
                {
                    continue;
                }
                
                uint16_t currentPixel = (currentScanline * PPU_WIDTH) + currentScanlineCycle;
                uint8_t index = (currentScanlineCycle - sprData.startX) % 8;
                
                // We're going out of bounds on screen - don't continue rendering
                if (currentPixel >= (PPU_WIDTH * PPU_HEIGHT))
                {
                    continue;
                }

                // We only go further IFF the pixel to render isn't the backdrop index!
                if (GetPatternBits(sprData.pattern, index) != 0x0)
                {
                    // Sprite 0 Hit
                    bool leftSide = currentScanlineCycle < 8 && (registers.mask & (PPUMASK_BG_LCOL_ENABLE | PPUMASK_SPR_LCOL_ENABLE));
                    bool lastRow = currentScanline == 255;
                    bool spr0HitAlready = (registers.status & PPUSTATUS_SPR0HIT) != 0;
                    // If this is sprite zero, we haven't hit sprite 0 this frame yet, then mark that we rendered it at least one pixel!
                    if (sprData.oamIndex == 0 && !(leftSide || lastRow || spr0HitAlready))
                    {
                        registers.status |= PPUSTATUS_SPR0HIT;
                    }
                    
                    // We only keep going if this sprite has a lower OAM index than any written before it here
                    // (AKA this sprite has priority)
                    uint8_t pixelOAM = frameDataSprite[currentPixel] & 0x3F;
                    if (sprData.oamIndex > pixelOAM)
                    {
                        continue;
                    }
                    
                    // This sprite pixel is significant - mark the OAM index, regardless of if we've written to it or not yet or BG tile priority (used for the "sprite priority quirk")
                    frameDataSprite[currentPixel] = (frameDataSprite[currentPixel] & 0xC0) | (sprData.oamIndex & 0x3F);
                    
                    if (sprData.bgPriority)
                    {
                        // Skip if we also drew a tile on this cycle, before we set the priority data,
                        // but we know the tile sprite wasn't backdrop (aka the tile takes priority)
                        if ((frameDataSprite[currentPixel] & 0x80) > 0)
                        {
                            continue;
                        }
                        
                        frameDataSprite[currentPixel] = 0x80 | (frameDataSprite[currentPixel] & 0x7F);
                    }
                    
                    // We finally get to draw the pixel!
                    
                    // Write a bit signifying that we wrote a SPR pixel here
                    frameDataSprite[currentPixel] = 0x40 | (frameDataSprite[currentPixel] & 0xBF);
                    
                    uint32_t palette = GetPalette(sprData.paletteIndex, true);
                    uint32_t color = GetColor(sprData.pattern, palette, index);
                    frameData[currentPixel] = color;
                }
            }
        }
        
        // Secondary OAM is cleared to 0xFF over 64 cycles, but we can get away with just doing it all at once
        if (currentScanlineCycle < 64)
        {
            if (currentScanlineCycle == 63)
            {
                memset(secondaryOAM, 0xFF, sizeof(secondaryOAM));
                // Reset counters and flags before sprite evaluation
                oamN = 0;
                oamM = 0;
                secondaryOAMNextSlot = 0;
                sprFetchIndex = 0;
                registers.status &= ~PPUSTATUS_SPROVERFLOW;
            }
        }
        else if (currentScanlineCycle < 256)
        {
            // Fetch next line's sprite data
            if (currentScanlineCycle % 2 == 0)
            {
                // Odd cycle - read from OAM
                if (secondaryOAMNextSlot < 8)
                {
                    secondaryOAM[secondaryOAMNextSlot*5] = oam[oamN*4 + oamM];
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
                    uint8_t sprY = secondaryOAM[secondaryOAMNextSlot*5];
                    uint8_t sprHeight = (registers.ctrl & PPUCTRL_SPRHEIGHT) != 0x00 ? 16 : 8;
                    if (currentScanline >= sprY && currentScanline < sprY + sprHeight)
                    {
                        // Sprite in range - copy the rest of the data to secondary OAM
                        for (int i = 1; i < 4; ++i)
                        {
                            secondaryOAM[secondaryOAMNextSlot*5 + i] = oam[oamN*4 + (++oamM)];
                            if (oamM == 3)
                            {
                                secondaryOAM[secondaryOAMNextSlot*5 + 4] = oamN;
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
        else if (currentScanlineCycle < 320)
        {
            if (currentScanlineCycle == 257)
            {
                sprDataToDrawCount = 0;
            }
            // Sprite fetching! First 4 cycles are for reading, last 4 are for writing
            uint8_t pixelInFetchCycle = currentScanlineCycle % 8;
            
            if (pixelInFetchCycle == 4)
            {
                // The hardware doesn't technically "render" during this time, we just got next scanline's
                // sprite data ready to go. But we're an emulator, and the wiki isn't clear to me on what
                // the PPU is doing with these read cycles, so... let's just render!
                if (sprFetchIndex < secondaryOAMNextSlot)
                {
                    // Get sprite info
                    uint8_t sprY        = secondaryOAM[sprFetchIndex*5];
                    uint8_t sprTile     = secondaryOAM[sprFetchIndex*5 + 1];
                    uint8_t sprAttr     = secondaryOAM[sprFetchIndex*5 + 2];
                    uint8_t sprX        = secondaryOAM[sprFetchIndex*5 + 3];
                    uint8_t sprOAMIndex = secondaryOAM[sprFetchIndex*5 + 4];
                    
                    // Get sprite attributes
                    uint8_t sprPaletteIndex = sprAttr & SPR_PALETTE;
                    bool sprTilePriority    = (sprAttr & SPR_PRIORITY) != 0x00;
                    bool sprFlipHorizontal  = (sprAttr & SPR_FLIPX) != 0x00;
                    bool sprFlipVertical    = (sprAttr & SPR_FLIPY) != 0x00;
                    bool sprIs8By16 = (registers.ctrl & PPUCTRL_SPRHEIGHT) != 0x00;
                    
                    // Find out where we'll be starting to render on this line
                    uint16_t sprRow = (currentScanline - sprY) % (sprIs8By16 ? 16 : 8);
                    
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
                    uint16_t sprPattern = WeavePatternBits(ptrnLow, ptrnHi, sprFlipHorizontal);
                    
                    sprDataToDraw[sprDataToDrawCount].oamIndex = sprOAMIndex;
                    sprDataToDraw[sprDataToDrawCount].pattern = sprPattern;
                    sprDataToDraw[sprDataToDrawCount].startX = sprX;
                    sprDataToDraw[sprDataToDrawCount].paletteIndex = sprPaletteIndex;
                    sprDataToDraw[sprDataToDrawCount].bgPriority = sprTilePriority;
                    sprDataToDrawCount++;
                    
                    // Advance sprite fetch index since we successfully drew this one
                    sprFetchIndex++;
                }
            }
        }
        // The rest of the cycles are unused by the sprite evaluation/rendering loop
    }
}

void NESDL_PPU::FetchAndStoreTile(uint8_t pixelInFetchCycle)
{
    // Every 8 pixels (skipping cycle 0 I believe) we repeat the same process for tiles
    
    if (pixelInFetchCycle == 1)  // Cycle 1-2 - Fetch nametable
    {
        // Read the byte from memory at PPUADDR and store it in our NT register
        uint16_t addr = registers.v;
        addr = 0x2000 | (addr & 0xFFF); // Only keep 12 bits and combine into NT address space
        tileFetch.nametable = ReadFromVRAM(addr);
    }
    if (pixelInFetchCycle == 3) // Cycle 3-4 - Fetch attribute table
    {
        uint16_t ntSelect = (registers.v & 0xC00); // NT select occupies same bits at AT address, no shifting needed
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
        uint8_t fineY = ((registers.v & 0x7000) >> 12);
        uint8_t rowIndex = fineY % 8;
        patternAddr = ((patternAddr + tileFetch.nametable) << 4) + rowIndex;
        
        uint8_t ptrnLow = ReadFromVRAM(patternAddr);
        uint8_t ptrnHi = ReadFromVRAM(patternAddr + 8);
        
        // Weave bits together and store
        tileFetch.pattern = WeavePatternBits(ptrnLow, ptrnHi, false);
    }
    if (pixelInFetchCycle == 7) // Cycle 7-8 - Fetch pattern table high bytes (finishes on next cycle 0)
    {
        // tileFetch has been fully constructed - no need to do anything here
    }
}

void NESDL_PPU::HandleProcessVBlankScanline()
{
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
            disregardNMI = false;
        }
    }
    else if (currentScanline == 261)
    {
        // Pre-render scanline
        bool isRenderingEnabled = (registers.mask & PPUMASK_BGENABLE) != 0x00 || (registers.mask & PPUMASK_SPRENABLE) != 0x00;
        
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
            
            FetchAndStoreTile(pixelInFetchCycle); // Runs across multiple cycles in order to achieve this
        }
        else if (currentScanlineCycle == 256)
        {
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
                // "tile data for the sprites on the next scanline are fetched here"
                // (we don't need to do this, as far as I'm aware)
                
                // Special case for pre-render scanline: reset vertical scroll (repeatedly)
                if (currentScanlineCycle >= 280 && currentScanlineCycle <= 304)
                {
                    registers.v = (registers.v & 0x041F) | (registers.t & 0xFBE0);
                }
            }
        }
        else if (currentScanlineCycle < 337)
        {
        }
        else if (currentScanlineCycle < 341)
        {
            // Two NT bytes are fetched but is really not necessary (except for maper MMC5, we can write a special case in for that)
        }
    }
}

void NESDL_PPU::PreprocessPPUForReadInstructionTiming(uint8_t instructionPPUTime)
{
    uint32_t cyclesThisFrame = (currentScanline * 341) + currentScanlineCycle;
    uint32_t cyclesAfterInstruction = cyclesThisFrame + instructionPPUTime;
    uint32_t setTarget = ((241 * 341) + 1);
    uint32_t clearTarget = ((261 * 341) + 1);
    
    // This VBL preprocessor is mainly to handle the case where we LDA 0x2002 at the end of scanline 240,
    // when the timing is most crucial (VBL AND NMI), as well as clearing on scanline 261.
    
    if (!disregardVBL)
    {
        // Extra-specific timing thing - if we're reading ON (241,1), the PPU hasn't run a step yet. So let's just... VBL
        if (cyclesThisFrame == setTarget)
        {
            registers.status |= PPUSTATUS_VBLANK;
            disregardVBL = true;
        }
        if (cyclesThisFrame < setTarget && cyclesAfterInstruction >= (setTarget-1))
        {
            // Some weird VBL race conditions (thank you NESDEV):
            
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

void NESDL_PPU::UpdateNTFrameData()
{
    memset(ntFrameData, 0x00, sizeof(ntFrameData));
    for (uint16_t i = 0x2000; i < 0x3000; ++i)
    {
        uint16_t a = GetMirroredAddress(i);
        uint32_t c = 0xFF000000 | (vram[a - 0x2000] << 16) | (vram[a - 0x2000] << 8) | vram[a - 0x2000];
        
        uint16_t p = i - (0x2000 + ((i - 0x2000) / 0x400) * 0x400);
        uint8_t pRow = p / 32;
        uint8_t pInRow = p % 32;
        if (i >= 0x2800)
        {
            pRow += 32;
        }
        if (i % 0x800 >= 0x400)
        {
            pInRow += 32;
        }
        ntFrameData[(pRow * 64) + pInRow] = c;
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

    // Writes to any register load the open bus
    // https://www.nesdev.org/wiki/Open_bus_behavior#PPU_open_bus
    ppuOpenBus = data;

    switch (registerAddr)
    {
        case PPU_PPUCTRL:
            // If NMI flag is flipped from 0 to 1 and PPUSTATUS VBL flag is set, we trigger an NMI immediately
            if ((registers.status & PPUSTATUS_VBLANK) != 0x00 && (registers.ctrl & PPUCTRL_NMIENABLE) == 0x00 && (data & PPUCTRL_NMIENABLE) != 0x00)
            {
                core->cpu->nmi = true;
                core->cpu->delayNMI = true;
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
                oam[registers.oamAddr++] = data;
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
                registers.t = (registers.t & 0x00FF) | (uint16_t(data & 0x3F) << 8);
            }
            else
            {
                uint8_t lastHighV = registers.v >> 8;
                // Low byte second
                registers.t = (registers.t & 0xFF00) | data;
                // Store completed 15-bit register into v
                registers.v = registers.t;
                uint8_t newHighV = registers.v >> 8;

                // MMC3 only: if A12 (bit 12) goes 0->1 on write, trigger IRQ clock
                if (mapper->mapperNumber == 4)
                {
                    if ((lastHighV & 0x10) == 0 && (newHighV & 0x10) != 0)
                    {
                        //printf("\nIRQ Clock (2006)");
                        ((NESDL_Mapper_4*)mapper)->ClockIRQ();
                    }
                }
            }
            registers.w = !registers.w;
            break;
        case PPU_PPUDATA:
            {
                uint8_t lastHighV = registers.v >> 8;
                WriteToVRAM(registers.v, data);

                // Increment v by the amount specified from PPUCTRL bit 2
                if (incrementV)
                {
                    registers.v += (registers.ctrl & PPUCTRL_INCMODE) != 0 ? 32 : 1;
                }

                uint8_t newHighV = registers.v >> 8;

                // MMC3 only: if A12 (bit 12) goes 0->1 on write, trigger IRQ clock
                if (mapper->mapperNumber == 4)
                {
                    if ((lastHighV & 0x10) == 0 && (newHighV & 0x10) != 0)
                    {
                        //printf("\nIRQ Clock (2007)");
                        ((NESDL_Mapper_4*)mapper)->ClockIRQ();
                    }
                }
            }
            break;
        case PPU_OAMDMA:
            // DMA write is a very involved process - the CPU is halted while RAM is transferred to OAM.
            // Since we're an emulator and the STA/X instruction is still running, if we alter
            // the CPU's elapsed cycle count now it will effectively halt the very next instruction
            core->cpu->dma = true;
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
                // Ignore VBL reset if operation is writing
                if (ignoreChanges || isWriting)
                {
                    ppuOpenBus = (ppuOpenBus & 0x1F) + (registers.status & 0xE0);
                }
                else
                {
                    // On PPUSTATUS read, always set latch w to 0
                    registers.w = false;
                    uint8_t output = registers.status;
                    // Also reset VBLANK on read
                    registers.status &= ~PPUSTATUS_VBLANK;
                    // Put data onto open bus for return
                    ppuOpenBus = (ppuOpenBus & 0x1F) + (output & 0xE0);
                }
            }
            break;
        case PPU_OAMADDR:
            // Undefined behavior to read from OAMADDR -- do nothing
            break;
        case PPU_OAMDATA:
            // Some models don't have read functionality. We can assume developers knew this at the time, and don't bother truly implementing this (famous last words)
            ppuOpenBus = 0xFF;
            //return 0xFF;
            break;
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
                    // Return palette data ("unreliable"), otherwise return VRAM data
                    if (registers.v >= 0x3F00)
                    {
                        // Put data onto open bus for return
                        ppuOpenBus = (ppuOpenBus & 0xC0) + (ReadFromVRAM(registers.v) & 0x3F);
                    }
                    else
                    {
                        uint8_t lastHighV = registers.v >> 8;

                        uint8_t vramData = ppuDataReadBuffer;
                        ppuDataReadBuffer = ReadFromVRAM(registers.v);
                        // Increment v by the amount specified from PPUCTRL bit 2
                        if (incrementV)
                        {
                            registers.v += (registers.ctrl & PPUCTRL_INCMODE >> 2) == 0 ? 1 : 32;
                        }
                        // Put data onto open bus for return
                        ppuOpenBus = vramData;

                        uint8_t newHighV = registers.v >> 8;

                        // MMC3 only: if A12 (bit 12) goes 0->1 on write, trigger IRQ clock
                        if (mapper->mapperNumber == 4)
                        {
                            if ((lastHighV & 0x10) == 0 && (newHighV & 0x10) != 0)
                            {
                                //printf("\nIRQ Clock (2007)");
                                ((NESDL_Mapper_4*)mapper)->ClockIRQ();
                            }
                        }
                    }
                }
            }
            break;
        case PPU_OAMDMA:
            // Undefined behavior to read from OAMDMA -- do nothing
            break;
    }
    
    return ppuOpenBus;
}

uint8_t NESDL_PPU::ReadFromVRAM(uint16_t addr)
{
    // Mirrored address space(s) 0x3000-0x3EFF -> 0x2000-0x2EFF
    if (addr >= 0x3000 && addr < 0x3F00)
    {
        addr -= 0x1000;
    }
    if (addr >= 0x3F20)
    {
        addr = 0x3F00 | (addr % 0x20);
    }
    
    // Accessing CHR ROM/RAM data from cartridge
    if (addr < 0x2000)
    {
        return mapper->ReadByte(addr);
    }
    else if (addr >= 0x2000 && addr < 0x3000)
    {
        addr = GetMirroredAddress(addr);
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
    // Mirror address space(s)
    if (addr >= 0x3000 && addr < 0x3F00)
    {
        addr -= 0x1000;
    }
    if (addr >= 0x3F20)
    {
        addr = 0x3F00 | (addr % 0x20);
    }
    
    if (addr < 0x2000)
    {
        mapper->WriteByte(addr, data);
        return;
    }
    else if (addr >= 0x2000 && addr < 0x3000)
    {
        addr = GetMirroredAddress(addr);
        vram[addr - 0x2000] = data;
    }
    else if (addr >= 0x3F00)
    {
        uint16_t index = (addr - 0x3F00) % 0x20;
        paletteData[index] = data & 0x3F; // 6-bit only
        // Palette data for color index 0 is mirrored across both BG and sprite palettes
        if (index % 4 == 0)
        {
            paletteData[index >= 0x10 ? (index-0x10) : (index+0x10)] = data & 0x3F; // 6-bit only
        }
    }
}

uint16_t NESDL_PPU::GetMirroredAddress(uint16_t addr)
{
    // Nametable mirroring: [0, 1]
    //                      [2, 3]
    MirroringMode mode = mapper->GetMirroringMode();
    switch (mode)
    {
        case MirroringMode::Horizontal:
            // Horizontal mirror - NT 0 maps to 1, NT 2 maps to 3
            if (addr >= 0x2C00 || (addr >= 0x2400 && addr < 0x2800))
            {
                addr -= 0x400;
            }
            if (addr >= 0x2800) // Map addr NT 2 to physical NT 1
            {
                addr -= 0x400;
            }
            break;
        case MirroringMode::Vertical:
            // Vertical mirror - NT 0 maps to 2, NT 1 maps to 3
            if (addr >= 0x2800)
            {
                addr -= 0x800;
            }
            break;
        case MirroringMode::One_LowerBank:
        case MirroringMode::One_UpperBank:
            // Both versions map to NT 0
            addr = 0x2000 | (addr % 0x400);
            break;
        case MirroringMode::Four:
            // 4-way means NO mirroring!
            break;
    }
    return addr;
}

uint16_t NESDL_PPU::WeavePatternBits(uint8_t low, uint8_t high, bool flip)
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
        if (flip)
        {
            result |= (low16 << (14 - i*2)) | (high16 << ((14 - i*2) + 1));
        }
        else
        {
            result |= (low16 << (i*2)) | (high16 << ((i*2) + 1));
        }
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
    uint8_t patternBits = GetPatternBits(pattern, pixelIndex);
    // Take this 0-3 index and select the palette byte
    uint32_t paletteShifted = palette >> (24 - patternBits*8);
    uint8_t paletteIndex = paletteShifted & 0xFF;
    if ((registers.mask & PPUMASK_GREYSCALE) != 0x00) // Only select from four greyscale palette values
    {
        paletteIndex &= 0x30;
    }
    // Get color from palette index!
    uint32_t resultColor = NESDL_PALETTE[paletteIndex];
    // TODO implement tinting via PPUMASK. I'm not keen on perfectly recreating
    // tinting via accurate video signal manipulation, a fast approx will do
    
    return resultColor;
}

uint8_t NESDL_PPU::GetPatternBits(uint16_t pattern, uint8_t pixelIndex)
{
    uint8_t index = ((7 - pixelIndex) * 2);
    return (pattern & (0x3 << index)) >> index;
}
