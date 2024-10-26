#include "NESDL.h"

void NESDL_APU::Init(NESDL_Core* c, NESDL_SDL* s)
{
    core = c;
    sdlCtx = s;
    
    ppuElapsedCycles = 0;
    
    sequencer.steps = 4;
    counters.noiseLFSR = 1; // Load up LFSR with a bit set for XOR to work off of
}

void NESDL_APU::Update(uint32_t ppuCycles)
{
    // Update cycle counters (used to detect when to update values - APU sequencer and generators)
    ppuElapsedCycles += ppuCycles;
    counters.sequencerPPUCounter += ppuCycles*2;
    
    // APU sequencer updates happens at 240hz
    // This equates to ~22372.5 PPU cycles
    bool doSequencerUpdate = false;
    if (counters.sequencerPPUCounter >= 44745)
    {
        counters.sequencerPPUCounter -= 44745;
        doSequencerUpdate = true;
    }
    // CPU/APU update flags
    bool doCPUUpdate = ppuElapsedCycles % 3 == 0;
    bool doAPUUpdate = ppuElapsedCycles % 6 == 0;
    
    // Sequencer update - update envelopes, sweep, length counters and IRQ
    if (doCPUUpdate && counters.sequencerResetDelay > 0)
    {
        if (--counters.sequencerResetDelay == 0)
        {
            counters.sequencerIndex = 0;
            counters.sequencerPPUCounter = 0;
            if (sequencer.steps == 5)
            {
                SequencerUpdateEL();
                SequencerUpdateLS();
            }
        }
    }
    if (doSequencerUpdate)
    {
        uint8_t step = counters.sequencerIndex;
        
        // Update envelope & linear counters (except on 5-step, step 3)
        if (!(sequencer.steps == 5 && step == 3))
        {
            SequencerUpdateEL();
        }
        
        // Update length & sweeps (4-step odd steps or 5-step on 1 and 4)
        bool updateLengthSweeps = sequencer.steps == 4 && (step % 2) == 1;
        updateLengthSweeps |= sequencer.steps == 5 && (step == 1 || step == 4);
        
        if (updateLengthSweeps)
        {
            SequencerUpdateLS();
        }
        
        // Update IRQ on 4-step, step 3
        if (!sequencer.disableIRQ && sequencer.steps == 4 && step == 3)
        {
            
        }
        
        // Advance sequencer step index
        if (counters.sequencerIndex++ == sequencer.steps)
        {
            counters.sequencerIndex = 0;
        }
    }
    
    // APU update - Update Square channels
    if (doAPUUpdate)
    {
        // Square 1/2
        if (square1.timer < 8)
        {
            counters.square1WaveIndex = 0;
        }
        else
        {
            if (counters.square1WaveTimer == 0)
            {
                // Time to select the next index in our square duty, and reset timer
                counters.square1WaveIndex = (counters.square1WaveIndex - 1) & 0x7;
                counters.square1WaveTimer = counters.square1CurrentTimer;
            }
            else
            {
                counters.square1WaveTimer--;
            }
        }
        if (square2.timer < 8)
        {
            counters.square2WaveIndex = 0;
        }
        else
        {
            if (counters.square2WaveTimer == 0)
            {
                // Time to select the next index in our square duty, and reset timer
                counters.square2WaveIndex = (counters.square2WaveIndex - 1) & 0x7;
                counters.square2WaveTimer = counters.square2CurrentTimer;
            }
            else
            {
                counters.square2WaveTimer--;
            }
        }
    }
    // Triangle, Noise and DMC updates on CPU clock instead of APU)
    if (doCPUUpdate)
    {
        // On real hardware, the triangle channel has no limit.
        // However, values < 2 are ultrasonic and should just be limited tbh
        if (tri.timer < 2)
        {
            counters.triWaveIndex = 0;
        }
        else if (counters.triLinearCounter > 0)
        {
            // Triangle is clocked if length AND linear counters > 0
            if (counters.triWaveTimer == 0)
            {
                // Time to select the next index in our square duty, and reset timer
                counters.triWaveIndex = (counters.triWaveIndex + 1) % 32;
                counters.triWaveTimer = tri.timer;
            }
            else
            {
                counters.triWaveTimer--;
            }
        }
        
        // Noise
        if (counters.noiseTimer == 0)
        {
            counters.noiseTimer = counters.noiseTimerPeriod;
            // Select bit 6 if mode is set, otherwise bit 1
            // (Pre-shift left to bit 14)
            uint16_t xorBit = noise.mode ? (counters.noiseLFSR & 0x40) << 8 : (counters.noiseLFSR & 0x02) << 13;
            uint16_t feedbackBit = ((counters.noiseLFSR & 0x01) << 14) ^ xorBit;
            // Shift register right one and slot our bit into 14 (bit 15 is not used)
            counters.noiseLFSR = (counters.noiseLFSR >> 1) | (feedbackBit & 0x4000);
        }
        else
        {
            counters.noiseTimer--;
        }
        
        // DMC
        if (counters.dmcTimer == 0)
        {
            counters.dmcTimer = NESDL_DMC_RATE[dmc.rate]/4;
            
            // Clock - go through current buffer
            if (counters.dmcBufferBitsRemaining == 0)
            {
                counters.dmcBufferBitsRemaining = 8;
                counters.dmcIsSilent = (counters.dmcSampleBuffer == 0x00);
            }
            else
            {
                counters.dmcBufferBitsRemaining--;
            }
            
            if (!counters.dmcIsSilent)
            {
                int8_t value = (counters.dmcSampleBuffer & 0x01) ? 2 : -2;
                // Keep output in the [0, 127] range
                if ((value > 0 && counters.dmcOutputSample < 126) || (value < 0 && counters.dmcOutputSample > 1))
                {
                    counters.dmcOutputSample += value;
                }
                
                counters.dmcSampleBuffer = counters.dmcSampleBuffer >> 1;
            }
            
            // Buffer is empty - we should reload ASAP
            if (counters.dmcBufferBitsRemaining == 0 && counters.dmcBytesLeft > 0)
            {
                core->cpu->HaltCPUForDMC();
                
                // Get next byte sample to play!
                counters.dmcSampleBuffer = core->ram->ReadByte(counters.dmcAddr);
                counters.dmcIsSilent = false;
                
                // Increase address
                if (counters.dmcAddr == 0xFFFF)
                {
                    counters.dmcAddr = 0x8000;
                }
                else
                {
                    counters.dmcAddr++;
                }
                
                // Decrease bytes left
                if (--counters.dmcBytesLeft == 0)
                {
                    if (dmc.loop)
                    {
                        counters.dmcAddr = 0xC000 | (dmc.sampleAddr << 6);
                        counters.dmcBytesLeft = (dmc.sampleLength << 4) + 1;
                    }
                    if (dmc.irqEnabled)
                    {
                        // TODO DMC IRQ
                    }
                }
            }
        }
        else
        {
            counters.dmcTimer--;
        }
    }
    
    
    // NOTES
    // We will use a SDL sample queue approach
    // Sound gen will run on its own clock based off of the sample rate (so we always get perfect sample timings without overbuffering, perhaps sample rate over by just a bit to avoid clicks/pops)
    // Take the APU channel timer, convert that to frequency, then generate waveforms from that frequency (along with other values like sweep)
    // On a fixed clock rate relative to the sample rate timer, send samples to SDL to queue
    
    // CPU update - entire sample production is based on a specific calculation (CPU clock / sample rate)
    // So we must update only on very specific intervals derives from CPU timing
    if (doCPUUpdate)
    {
        cpuClockSampleTimer += 1.0;
    }
    if (cpuClockSampleTimer >= cpuClocksPerSample)
    {
        cpuClockSampleTimer -= cpuClocksPerSample;
        
        // Using formula from NESDEV to convert data to output values in range [0.0, 1.0]
        uint8_t s1Duty = square1.duty;
        uint8_t s2Duty = square2.duty;
        uint8_t s1Index = counters.square1WaveIndex;
        uint8_t s2Index = counters.square2WaveIndex;
        uint8_t triIndex = counters.triWaveIndex;
        float s1 = (NESDL_SQUARE_DUTY[s1Duty*8 + s1Index] ? 15.0f : 0.0f);
        float s2 = (NESDL_SQUARE_DUTY[s2Duty*8 + s2Index] ? 15.0f : 0.0f);
        float t = NESDL_TRI_DUTY[triIndex];
        float n = (counters.noiseLFSR & 0x01) ? 15.0f : 0.0f;
        float d = counters.dmcOutputSample;
        
        // Multiply channels by their volume
        if (square1.constant)
        {
            s1 *= ((float)square1.volume / 15);
        }
        else
        {
            s1 *= ((float)square1Envelope.decay / 15);
        }
        if (square2.constant)
        {
            s2 *= ((float)square2.volume / 15);
        }
        else
        {
            s2 *= ((float)square2Envelope.decay / 15);
        }
        
        // Triangle has no volume - skip
        
        if (noise.constant)
        {
            n *= ((float)noise.volume / 15);
        }
        else
        {
            n *= ((float)noiseEnvelope.decay / 15);
        }
        
        // Silence channels if length counter is 0 (or muted via status)
        s1 *= (counters.square1Length > 0) && (status.square1Enable) ? 1.0f : 0.0f;
        s2 *= (counters.square2Length > 0) && (status.square2Enable) ? 1.0f : 0.0f;
        t *= (counters.triLength > 0) && (counters.triLinearCounter > 0) && (status.triEnable) ? 1.0f : 0.0f;
        n *= (counters.noiseLength > 0) && (status.noiseEnable) ? 1.0f : 0.0f;
        
        // For Square channels - also silence if sweep goes overboard
        s1 *= IsSweepMuted(1) ? 0.0f : 1.0f;
        s2 *= IsSweepMuted(2) ? 0.0f : 1.0f;
        
        // Mix channels together!
        // https://www.nesdev.org/wiki/APU_Mixer
        float pulseOut = 0;
        if (s1 > 0 || s2 > 0)
        {
            pulseOut = 95.88f / (((float)8128 / (s1 + s2)) + 100);
        }
        float tndOut = 0;
        if (t > 0 || n > 0 || d > 0)
        {
            tndOut = 159.79f / (((float)1 / ((t/8227) + (n/12241) + (d/22638))) + 100);
        }
        float output = pulseOut + tndOut;
        
        // Send the sample off for playback
        sdlCtx->WriteNextAPUSignal(output);
    }
}

// Update envelopes (E) and Triangle linear counter (L)
void NESDL_APU::SequencerUpdateEL()
{
    // Square 1 envelope
    if (square1Envelope.start)
    {
        square1Envelope.start = false;
        square1Envelope.decay = 15;
        square1Envelope.divider = square1.volume;
    }
    else
    {
        if (square1Envelope.divider == 0)
        {
            square1Envelope.divider = square1.volume;
            
            if (square1Envelope.decay == 0)
            {
                if (square1.haltLoop) // Doubles as loop flag in envelope mode
                {
                    square1Envelope.decay = 15;
                }
            }
            else
            {
                square1Envelope.decay--;
            }
        }
        else
        {
            square1Envelope.divider--;
        }
    }
    
    // Square 2 envelope
    if (square2Envelope.start)
    {
        square2Envelope.start = false;
        square2Envelope.decay = 15;
        square2Envelope.divider = square2.volume;
    }
    else
    {
        if (square2Envelope.divider == 0)
        {
            square2Envelope.divider = square2.volume;
            
            if (square2Envelope.decay == 0)
            {
                if (square2.haltLoop) // Doubles as loop flag in envelope mode
                {
                    square2Envelope.decay = 15;
                }
            }
            else
            {
                square2Envelope.decay--;
            }
        }
        else
        {
            square2Envelope.divider--;
        }
    }
    
    // Noise envelope
    if (noiseEnvelope.start)
    {
        noiseEnvelope.start = false;
        noiseEnvelope.decay = 15;
        noiseEnvelope.divider = noise.volume;
    }
    else
    {
        if (noiseEnvelope.divider == 0)
        {
            noiseEnvelope.divider = noise.volume;
            
            if (noiseEnvelope.decay == 0)
            {
                if (noise.haltLoop) // Doubles as loop flag in envelope mode
                {
                    noiseEnvelope.decay = 15;
                }
            }
            else
            {
                noiseEnvelope.decay--;
            }
        }
        else
        {
            noiseEnvelope.divider--;
        }
    }
    
    // Triangle linear counter
    if (counters.triLinearCounterReload)
    {
        counters.triLinearCounter = tri.reload;
    }
    else
    {
        if (counters.triLinearCounter > 0)
        {
            counters.triLinearCounter--;
        }
    }
    if (tri.haltLoop == false)
    {
        counters.triLinearCounterReload = false;
    }
}

// Update length counters (L) and sweeps (S)
void NESDL_APU::SequencerUpdateLS()
{
    // Square 1 length counter
    if (status.square1Enable)
    {
        if (counters.square1Length > 0 && square1.haltLoop == false)
        {
            counters.square1Length--;
        }
    }
    else
    {
        counters.square1Length = 0;
        square1.length = 0; // Length is directly modified
    }
    
    // Square 2 length counter
    if (status.square2Enable)
    {
        if (counters.square2Length > 0 && square2.haltLoop == false)
        {
            counters.square2Length--;
        }
    }
    else
    {
        counters.square2Length = 0;
        square2.length = 0; // Length is directly modified
    }
    
    // Triangle length counter
    if (status.triEnable)
    {
        if (counters.triLength > 0 && tri.haltLoop == false)
        {
            counters.triLength--;
        }
    }
    else
    {
        counters.triLength = 0;
        tri.length = 0; // Length is directly modified
    }
    
    // Noise length counter
    if (status.noiseEnable)
    {
        if (counters.noiseLength > 0 && noise.haltLoop == false)
        {
            counters.noiseLength--;
        }
    }
    else
    {
        counters.noiseLength = 0;
        noise.length = 0; // Length is directly modified
    }
    
    // Square 1 sweep
    if (square1.sweepEnabled && counters.square1SweepTimer == 0 && square1.sweepShift > 0)
    {
        if (!IsSweepMuted(1))
        {
            counters.square1CurrentTimer = counters.square1TargetTimer;
            UpdateSweepTargetTimer(1);
        }
    }
    if (counters.square1SweepTimer == 0 || counters.square1SweepReload)
    {
        counters.square1SweepReload = false;
        counters.square1SweepTimer = square1.sweepPeriod;
    }
    else
    {
        counters.square1SweepTimer--;
    }
    
    // Square 2 sweep
    if (square2.sweepEnabled && counters.square2SweepTimer == 0 && square2.sweepShift > 0)
    {
        if (!IsSweepMuted(2))
        {
            counters.square2CurrentTimer = counters.square2TargetTimer;
            UpdateSweepTargetTimer(2);
        }
    }
    if (counters.square2SweepTimer == 0 || counters.square2SweepReload)
    {
        counters.square2SweepReload = false;
        counters.square2SweepTimer = square2.sweepPeriod;
    }
    else
    {
        counters.square2SweepTimer--;
    }
}

bool NESDL_APU::IsSweepMuted(uint8_t channel)
{
    if (channel == 1)
    {
        return counters.square1CurrentTimer < 8 || counters.square1TargetTimer >= 0x0800;
    }
    if (channel == 2)
    {
        return counters.square2CurrentTimer < 8 || counters.square2TargetTimer >= 0x0800;
    }
    return false;
}

void NESDL_APU::UpdateSweepTargetTimer(uint8_t channel)
{
    if (channel == 1)
    {
        uint16_t target = counters.square1CurrentTimer >> square1.sweepShift;
        if (square1.sweepNegate)
        {
            target = -target - 1; // 1's compliment
        }
        counters.square1TargetTimer = counters.square1CurrentTimer + target;
    }
    if (channel == 2)
    {
        uint16_t target = counters.square2CurrentTimer >> square2.sweepShift;
        if (square2.sweepNegate)
        {
            target = -target; // 2's compliment
        }
        counters.square2TargetTimer = counters.square2CurrentTimer + target;
    }
}

uint8_t NESDL_APU::ReadByte(uint16_t addr)
{
    // The only readable byte is 0x4015 (status)
    if (addr == 0x4015)
    {
        // TODO "If an interrupt flag was set at the same moment of the read, it will read back as 1 but it will not be cleared."
        
        bool interruptVal = status.frameInterrupt;
        status.frameInterrupt = false;
        return (status.dmcInterrupt << 7) | (interruptVal << 6) |
            (status.dmcEnable << 4) | (status.noiseEnable << 3) |
            (status.triEnable << 2) | (status.square2Enable << 1) | (status.square1Enable << 0);
    }
    return 0;
}

void NESDL_APU::WriteByte(uint16_t addr, uint8_t data)
{
    switch (addr)
    {
        // Square 1
        case 0x4000:
            square1.volume = data & 0x0F;
            square1.constant = data & 0x10;
            square1.haltLoop = data & 0x20;
            square1.duty = (data & 0xC0) >> 6;
            break;
        case 0x4001:
            square1.sweepShift = data & 0x07;
            square1.sweepNegate = data & 0x08;
            square1.sweepPeriod = (data & 0x70) >> 4;
            square1.sweepEnabled = data & 0x80;
            // Side-effect - Sets sweep reload
            counters.square1SweepReload = true;
            break;
        case 0x4002:
            square1.timer = (square1.timer & 0x0700) | data;
            counters.square1CurrentTimer = square1.timer;
            UpdateSweepTargetTimer(1);
            break;
        case 0x4003:
            square1.timer = ((data & 0x07) << 8) | (square1.timer & 0x00FF);
            counters.square1CurrentTimer = square1.timer;
            UpdateSweepTargetTimer(1);
            square1.length = data >> 3;
            // Side-effect - Resets timer and envelope
            counters.square1Length = NESDL_LENGTH_COUNTER[square1.length];
            counters.square1WaveTimer = 0;
            square1Envelope.start = true;
            break;
        // Square 2
        case 0x4004:
            square2.volume = data & 0x0F;
            square2.constant = data & 0x10;
            square2.haltLoop = data & 0x20;
            square2.duty = (data & 0xC0) >> 6;
            break;
        case 0x4005:
            square2.sweepShift = data & 0x07;
            square2.sweepNegate = data & 0x08;
            square2.sweepPeriod = (data & 0x70) >> 4;
            square2.sweepEnabled = data & 0x80;
            // Side-effect - Sets sweep reload
            counters.square2SweepReload = true;
            break;
        case 0x4006:
            square2.timer = (square2.timer & 0x0700) | data;
            counters.square2CurrentTimer = square2.timer;
            UpdateSweepTargetTimer(2);
            break;
        case 0x4007:
            square2.timer = ((data & 0x07) << 8) | (square2.timer & 0x00FF);
            counters.square2CurrentTimer = square2.timer;
            UpdateSweepTargetTimer(2);
            square2.length = data >> 3;
            // Side-effect - Resets timer and envelope
            counters.square2Length = NESDL_LENGTH_COUNTER[square2.length];
            counters.square2WaveTimer = 0;
            square2Envelope.start = true;
            break;
        // Triangle
        case 0x4008:
            tri.reload = data & 0x7F;
            tri.haltLoop = data & 0x80;
            break;
        case 0x400A:
            tri.timer = (tri.timer & 0x0700) | data;
            break;
        case 0x400B:
            tri.timer = ((data & 0x07) << 8) | (tri.timer & 0x00FF);
            tri.length = data >> 3;
            // Side-effect - resets linear counter reload flag
            counters.triLinearCounterReload = true;
            counters.triLength = NESDL_LENGTH_COUNTER[tri.length];
            break;
        // Noise
        case 0x400C:
            noise.volume = data & 0x0F;
            noise.constant = data & 0x10;
            noise.haltLoop = data & 0x20;
            break;
        case 0x400E:
            noise.period = data & 0x0F;
            counters.noiseTimerPeriod = NESDL_NOISE_PERIOD[noise.period];
            noise.mode = data & 0x80;
            break;
        case 0x400F:
            noise.length = data >> 3;
            counters.noiseLength = NESDL_LENGTH_COUNTER[noise.length];
            noiseEnvelope.start = true;
            break;
        // DMC
        case 0x4010:
            dmc.rate = data & 0x07;
            dmc.loop = data & 0x40;
            dmc.irqEnabled = data & 0x80;
//            counters.dmcTimer = NESDL_DMC_RATE[dmc.rate];
            break;
        case 0x4011:
            dmc.directLoad = data & 0x7F;
            counters.dmcOutputSample = (uint8_t)dmc.directLoad; // Output sample directly!
            break;
        case 0x4012:
            dmc.sampleAddr = data;
            counters.dmcAddr = 0xC000 | (data << 6);
            break;
        case 0x4013:
            dmc.sampleLength = data;
            counters.dmcBytesLeft = (data << 4) + 1;
            break;
        // Status
        case 0x4015:
            status.square1Enable = data & 0x01;
            status.square2Enable = data & 0x02;
            status.triEnable = data & 0x04;
            status.noiseEnable = data & 0x08;
            status.dmcEnable = data & 0x10;
            // Side-effect - handle DMC info, silence channels if cleared
            status.dmcInterrupt = false;
            if (!status.square1Enable)
            {
                counters.square1Length = 0;
            }
            if (!status.square2Enable)
            {
                counters.square2Length = 0;
            }
            if (!status.triEnable)
            {
                counters.triLength = 0;
            }
            if (!status.noiseEnable)
            {
                counters.noiseLength = 0;
            }
            if (!status.dmcEnable)
            {
                counters.dmcBytesLeft = 0;
            }
            else
            {
                if (dmc.sampleLength > 0)
                {
                    counters.dmcBytesLeft = (data << 4) + 1;
                }
            }
            break;
        // Frame Counter/Sequencer
        case 0x4017:
            sequencer.disableIRQ = data & 0x40;
            sequencer.steps = (data & 0x80) ? 5 : 4;
            // Side-effect - update CPU IRQ, update sequencer state (peculiar)
            counters.sequencerResetDelay = ((ppuElapsedCycles/3) % 2) == 1 ? 2 : 3;
            // TODO set CPU IRQ flag value
            break;
    }
}
