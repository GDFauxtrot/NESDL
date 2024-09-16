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
}

void NESDL_PPU::HandleProcessVisibleScanline()
{
    // Don't run logic if BG rendering is disabled
    if ((registers.mask & PPUMASK_BGENABLE) == 0x00)
    {
        return;
    }
    
    uint16_t currentScanlineCycle = elapsedCycles % 341; // TODO un-magic this number
    
    // Every 8 pixels (skipping px 0) we repeat the same process for tiles
    uint8_t pixelInFetchCycle = currentScanlineCycle % 8;
    
//    if (currentScanlineCycle == 0)
//    {
//        // THE ONLY TIME we advance the cycle count artificially - this cycle is skipped
//        // when the frame count is odd
//        if (currentFrame % 2 == 1)
//        {
//            elapsedCycles++;
//        }
//    }
//    else if (currentScanlineCycle < 256)
    if (currentScanlineCycle < 256)
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
                if (currentScanline % 8 == 0 || currentDrawX % 8 == 0)
                {
                    color += 0x00202020;
                }
                frameData[currentPixel] = color;
                currentDrawX++;
            }
//            tileBuffer[1] = tileBuffer[0];
//            tileBuffer[0] = tileFetch;
            
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
        // Take our v address and convert to an equivalent attribute table address
        /*
        uint16_t ntAddr = registers.addr;
        // Base addresses of each space
        // TODO what about PPUCTRL base nametable address?
        uint16_t ntBaseAddr = 0x2000 + ((ntAddr - 0x2000) / 0x400) * 0x400;
        uint16_t atBaseAddr = 0x23C0 + (ntAddr / 0x23C0) * 0x400;
        // Get column and row into attribute table, down to the tile quadrant
        uint8_t atRowQuad = (ntAddr - ntBaseAddr) / 0x40;
        uint8_t atColQuad = ((ntAddr - ntBaseAddr) % 0x20) / 2;
        // Finally, convert to attribute table address
        uint16_t atAddr = atBaseAddr + (atRowQuad*0x4) + (atColQuad/2);
         */
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
        uint16_t patternAddr = ((registers.ctrl & PPUCTRL_BGTILE) >> 4) * 0x1000;
        uint8_t rowIndex = currentScanline % 8;
        if (currentScanline < 48)
        {
            printf("ADDR: %#06x\tSCANLINE: %d\tINDEX: %d\tNT: %d\n", registers.v, currentScanline, rowIndex, tileFetch.nametable);
        }
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
            registers.status |= PPUSTATUS_VBLANK;
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
            }
        }
        else if (currentScanlineCycle < 341)
        {
            // Two NT bytes are fetched but is really not necessary (except for maper MMC5, we can write a special case in for that)
        }
    }
}

//void NESDL_PPU::WriteFlagToRegister(uint16_t reg, uint8_t bytePos, bool on)
//{
//    // Get current register data
//    uint8_t currentData = 0;
//    if (reg == PPU_OAMDMA)
//    {
//        currentData = registers.oamDMA;
//    }
//    else
//    {
//        // Pointer arithmetic to avoid any extra if/switch code
//        currentData = *((uint8_t*)(&registers) + (reg - PPU_PPUCTRL));
//    }
//    
//    // Bitwise operate on the register to set the given bit the way we want it
//    if (on)
//    {
//        WriteToMappedRegister(reg, currentData | (1 << (bytePos-1)));
//    }
//    else
//    {
//        WriteToMappedRegister(reg, currentData & ~(1 << (bytePos-1)));
//    }
//}

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
            // TODO fill me in
//            registers.oamAddr = data;
            break;
        case PPU_OAMDATA:
            // TODO fill me in
//            registers.oamData = data;
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
                registers.v += (registers.ctrl & PPUCTRL_INCMODE >> 2) == 0 ? 1 : 32;
            }
            break;
        case PPU_OAMDMA:
            // TODO fill me in
//            registers.oamDMA = data;
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
                    registers.status &= 0x7F;
                    // TODO emulate "PPU stale bus contents" for some flimsily copy-protected titles
                    return output;
                }
            }
            break;
        case PPU_OAMADDR:
            // Undefined behavior to read from OAMADDR -- do nothing
            break;
        case PPU_OAMDATA:
            // TODO fill me in
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
    
    // Accessing pattern tables from cartridge (0x6000-0x7FFF mapped to CPU)
    if (addr < 0x2000)
    {
        return core->ram->ReadByte(addr + 0x6000);
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
        // Read-only ROM data - ignore
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
        paletteData[addr - 0x3F00] = data;
    }
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
    // Return the 4 bytes corresponding to this index
    return paletteData[i] << 24 | paletteData[i+1] << 16 | paletteData[i+2] << 8 | paletteData[i+3];
}

uint32_t NESDL_PPU::GetColor(uint16_t pattern, uint32_t palette, uint8_t pixelIndex)
{
    // Use pixel index to select 2 bits of pattern - 0 selects 2 highest, 7 selects 2 lowest
    uint8_t index = ((7 - pixelIndex) * 2);
    uint8_t patternBits = (pattern & (0x3 << index)) >> index;
    // Take this 0-3 index and select the palette byte
    uint32_t paletteShifted = palette >> (24 - patternBits*8);
    uint8_t paletteIndex = paletteShifted & 0xFF;
    // Get color from palette index!
    return NESDL_PALETTE[paletteIndex];
}
