#pragma once

// String consts relevant to our config file
class ConfigKey
{
public:
    constexpr static const char* LASTROM		= "last_rom";
    constexpr static const char* LASTLOG		= "last_log";

    constexpr static const char* INPUT_UP		= "input_up";
    constexpr static const char* INPUT_DOWN		= "input_down";
    constexpr static const char* INPUT_LEFT		= "input_left";
    constexpr static const char* INPUT_RIGHT	= "input_right";
    constexpr static const char* INPUT_A		= "input_a";
    constexpr static const char* INPUT_B		= "input_b";
    constexpr static const char* INPUT_SELECT	= "input_select";
    constexpr static const char* INPUT_START	= "input_start";
};

class NESDL_Config
{
public:
    constexpr static const char* FILENAME = "nesdl.cfg";

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