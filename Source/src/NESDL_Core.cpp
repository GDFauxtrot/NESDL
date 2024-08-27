#include "NESDL.h"
#include <memory>
#include <fstream>

void NESDL_Core::Init()
{
	// Initialize the system - CPU, PPU, RAM and APU
	cpu = new NESDL_CPU();
	ppu = new NESDL_PPU();
	ram = new NESDL_RAM();
	apu = new NESDL_APU();

	cpu->Init(this);

	timeSinceStartup = 0;
}

void NESDL_Core::Update(double deltaTime)
{
	// Convert deltaTime to the amount of cycles we need to advance on this frame
	timeSinceStartup += deltaTime;

	// Send current timeSinceStartup to CPU and PPU to handle their own cycle updates
	cpu->Update(timeSinceStartup);
	ppu->Update(timeSinceStartup);
}

void NESDL_Core::LoadRom(const char* path)
{
	ifstream file;
	file.open(path, ifstream::in | ifstream::binary);

	// Read header
	FileHeader_INES header;
	file.read((char*)&header, sizeof(header));

	// Get the program bank count and VROM bank count. We'll be reading this data soon
	uint8_t romBankCount = header.banks;
	uint8_t vromBankCount = header.vbanks;

	// TODO read header for any trainer or mapping data! Then skip ahead if needed
	// Check for trainer flag in bit 2, skip it if detected
	if ((header.ctrl1 & 0x4) == 0x4) 
	{
		file.seekg(512, ios_base::cur);
	}

	// Read off the 16KB allocated ROM banks
	unique_ptr<vector<uint8_t>> romData = make_unique<vector<uint8_t>>(romBankCount * 0x4000);
	file.read((char*)romData->data(), romBankCount * 0x4000);
	ram->WriteROMData(romData->data(), romBankCount); // TODO abstract this to a mapper! Assuming NROM

	// Read off the 8KB allocated VROM banks
	unique_ptr<vector<uint8_t>> vromData = make_unique<vector<uint8_t>>(vromBankCount * 0x2000);
	file.read((char*)vromData->data(), vromBankCount * 0x2000);
	ram->WriteVROMData(vromData->data(), vromBankCount); // TODO abstract this to a mapper! Assuming NROM

	file.close();
    
    // AGAIN! Use a mapper! This shit ain't gonna fly
    cpu->registers.pc = 0x8000;
}
