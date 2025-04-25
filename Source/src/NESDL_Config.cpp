#include "NESDL.h"
#include "..\src\SCL\SCL.hpp"
using namespace scl;

void NESDL_Config::Init(NESDL_Core* c)
{
	core = c;
}

void NESDL_Config::WriteValue(string key, bool value)
{
	config_file cfg(NESDL_CONFIG, config_file::WRITE);
	cfg.put<bool>(key, value);
	cfg.write_changes_unique();
	cfg.close();
}

void NESDL_Config::WriteValue(string key, int value)
{
	config_file cfg(NESDL_CONFIG, config_file::WRITE);
	cfg.put<int>(key, value);
	cfg.write_changes_unique();
	cfg.close();
}

void NESDL_Config::WriteValue(string key, string value)
{
	config_file cfg(NESDL_CONFIG, config_file::WRITE);
	cfg.put<string>(key, value);
	cfg.write_changes_unique();
	cfg.close();
}

bool NESDL_Config::ReadBool(const char* key, bool defVal)
{
	config_file cfg(NESDL_CONFIG, config_file::READ);
	bool result = cfg.get(key, defVal);
	cfg.close();
	return result;
}

int NESDL_Config::ReadInt(const char* key, int defVal)
{
	config_file cfg(NESDL_CONFIG, config_file::READ);
	int result = cfg.get(key, defVal);
	cfg.close();
	return result;
}

string NESDL_Config::ReadString(const char* key, string defVal)
{
	config_file cfg(NESDL_CONFIG, config_file::READ);
	string result = cfg.get(key, defVal);
	cfg.close();
	return result;
}