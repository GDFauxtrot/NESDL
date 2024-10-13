#pragma once

// Unlike other components, I opted to spread out register values
// since there are so many interworking components to how the APU works,
// timers depending on certain flags, etc.
// (Also registers don't need reconstructing for reads except for 0x4015)

struct APUSquare
{
    // 0x4000-0x4007 - Square 1/2
    uint8_t     volume;         // 0x4000/4
    bool        constant;
    bool        haltLoop;
    uint8_t     duty;
    uint8_t     sweepShift;     // 0x4001/5
    bool        sweepNegate;
    uint8_t     sweepPeriod;
    bool        sweepEnabled;
    uint16_t    timer;          // 0x4002/3/6/7
    uint8_t     length;
};

struct APUTriangle
{
    // 0x4008-0x400B - Triangle
    uint8_t     reload;         // 0x4008
    bool        haltLoop;
    uint16_t    timer;          // 0x400A/B
    uint8_t     length;
};

struct APUNoise
{
    // 0x400C-0x400F - Noise
    uint8_t     volume;         // 0x400C
    bool        constant;
    bool        haltLoop;
    uint8_t     period;         // 0x400E
    bool        mode;
    uint8_t     length;         // 0x400F
};

struct APUDMC
{
    // 0x4010-0x4013 - DMC
    uint16_t    rate;           // 0x4010
    bool        loop;
    bool        irqEnabled;
    uint16_t    directLoad;     // 0x4011
    uint16_t    sampleAddr;     // 0x4012
    uint16_t    sampleLength;   // 0x4013
};

struct APUStatus
{
    // 0x4015 - Status
    bool        square1Enable;
    bool        square2Enable;
    bool        triEnable;
    bool        noiseEnable;
    bool        dmcEnable;
    bool        frameInterrupt;
    bool        dmcInterrupt;
};

struct APUEnvelope
{
    bool start;
    uint8_t decay;
    uint8_t divider;
};

// struct APUGenerator
// {
// };

struct APUSequencer
{
    // 0x4017 - Frame Counter/Sequencer
    bool        disableIRQ;
    uint8_t     steps;
};

struct APUCounters
{
    // Square 1
    uint8_t     square1Length;
    uint16_t    square1CurrentTimer;
    uint16_t    square1TargetTimer;
    uint16_t    square1WaveTimer;
    uint8_t     square1WaveIndex;
    uint8_t     square1SweepTimer;
    bool        square1SweepReload;
    
    // Square 2
    uint8_t     square2Length;
    uint16_t    square2CurrentTimer;
    uint16_t    square2TargetTimer;
    uint16_t    square2WaveTimer;
    uint8_t     square2WaveIndex;
    uint8_t     square2SweepTimer;
    bool        square2SweepReload;
    
    // Triangle
    uint8_t     triLength;
    uint16_t    triWaveTimer;
    uint8_t     triWaveIndex;
    uint8_t     triLinearCounter;
    bool        triLinearCounterReload;
    
    // Noise
    uint8_t     noiseLength;
    uint16_t    noiseTimer;
    uint16_t    noiseTimerPeriod;
    uint16_t    noiseLFSR;
    
    // DMC
    uint16_t    dmcTimer;
    uint8_t     dmcSampleBuffer;
    uint8_t     dmcBufferBitsRemaining;
    uint16_t    dmcAddr;
    uint16_t    dmcBytesLeft;
    bool        dmcIsSilent;
    uint8_t     dmcOutputSample; // 7-bit
    
    // Sequencer
    uint16_t    sequencerPPUCounter;
    uint8_t     sequencerIndex;
    uint8_t     sequencerResetDelay;
};

class NESDL_APU
{
public:
    void Init(NESDL_Core* c, NESDL_SDL* s);
    void Update(uint32_t ppuCycles);
    uint8_t ReadByte(uint16_t addr);
    void WriteByte(uint16_t addr, uint8_t data);
private:
    void SequencerUpdateEL();
    void SequencerUpdateLS();
    
    bool IsSweepMuted(uint8_t channel);
    void UpdateSweepTargetTimer(uint8_t channel);
    
    NESDL_Core* core;
    NESDL_SDL* sdlCtx;
    
    // Frame counters, audio timers
    uint64_t ppuElapsedCycles;
    double cpuClocksPerSample = (double)1789773 / (double)(APU_SAMPLE_RATE - 1); // -1 to slightly overrun, avoid pops
    double cpuClockSampleTimer;
    
    // APU internal counter/timer info
    APUCounters counters;
    APUEnvelope square1Envelope;
    APUEnvelope square2Envelope;
    APUEnvelope noiseEnvelope;
    
    // Various APU register data (organized into structs)
    APUSquare square1;
    APUSquare square2;
    APUTriangle tri;
    APUNoise noise;
    APUDMC dmc;
    APUStatus status;
    APUSequencer sequencer;
};
