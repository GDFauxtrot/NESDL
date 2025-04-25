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

    template <typename T>
    void WriteValue(string section, string key, T value);

    template <typename T>
    T ReadValue(string section, string key, T defVal);

    void WriteValue(string section, string key, bool value);
    void WriteValue(string section, string key, int value);
    void WriteValue(string section, string key, string value);
    bool ReadBool(string section, string key, bool defVal = false);
    int ReadInt(string section, string key, int defVal = 0);
    string ReadString(string section, string key, string defVal = "");
private:
    NESDL_Core* core;

    // Currently-loaded configuration data, organized into sections
    // (all data must live in a section!)
    unordered_map<string, unordered_map<string, string>> data;

    // Handle to the currently loaded config file
    fstream file;

    void BeginWrite(string filename);
    void ReadFileAndClose(string filename);
    void ReadFromFile();
    void WriteToFileAndClose();

    string ParseSectionName(string in)
    {
        size_t lBraceIndex = in.find_first_of('[');
        size_t rBraceIndex = in.find_first_of(']');
        // Continue only if both braces present and right brace is after left brace
        if (lBraceIndex < 0 || rBraceIndex < 0 || rBraceIndex < lBraceIndex)
        {
            return in;
        }
        return in.substr(lBraceIndex + 1, rBraceIndex - lBraceIndex - 1);
    }

    pair<string, string> GetKeyValuePair(string in, string delim = "=")
    {
        size_t i = in.find_first_of(delim);
        if (i > 0)
        {
            return pair<string, string>(in.substr(0, i), in.substr(i, in.length() - i - 1));
        }
        else
        {
            return pair<string, string>(in, "");
        }
    }

	// https://stackoverflow.com/a/25385766
    string& TrimWhitespace(string& in)
    {
        static const char* whitespace = " \t\n\r\f\v";
        in.erase(in.find_last_not_of(whitespace) + 1);
        in.erase(0, in.find_last_not_of(whitespace));
        return in;
    }
};