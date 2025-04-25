#pragma once

class NESDL_Config
{
public:
	void Init(NESDL_Core* c);
	void WriteValue(string key, bool value);
	void WriteValue(string key, int value);
	void WriteValue(string key, string value);
	bool ReadBool(const char* key, bool defVal = false);
	int ReadInt(const char* key, int defVal = 0);
	string ReadString(const char* key, string defVal = "");
private:
	NESDL_Core* core;
};