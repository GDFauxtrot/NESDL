#pragma once

// String consts relevant to our config file
class ConfigSection
{
public:
    constexpr static const char* GENERAL		= "General";
    constexpr static const char* PLAYER1		= "Player1";
};

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
    constexpr static const char* FILENAME		= "nesdl.cfg";

    void Init(NESDL_Core* c);

    template <typename T>
    void WriteValue(const string& section, const string& key, T value)
    {
        BeginWrite(FILENAME);

        // Utilize stringstream to convert and store value
        // (idea borrowed from Simple Config Library)
        stringstream ss;
        ss << value;
        data[section][key] = ss.str();

        WriteToFileAndClose();
    }

    template <typename T>
    T ReadValue(const string& section, const string& key, T defVal)
    {
        bool hasSection = data.find(section) != data.end();
        bool hasKey = false;
        if (hasSection)
        {
            hasKey = data[section].find(key) == data[section].end();
        }
        if (hasSection && hasKey)
        {
            // Utilize stringstream to convert stored string back into value
            // (idea borrowed from Simple Config Library)
            stringstream ss(data[section][key]);

            T out;
            ss >> out;

            if (ss.fail())
            {
                return defVal;
            }
            else
            {
                return out;
            }
        }
        else
        {
            // Data not found - return default value instead
            return defVal;
        }
    }
    void WriteToFileAndClose();
    static unordered_map<string, unordered_map<string, string>> GetConfigDefaults();

private:
    NESDL_Core* core;
    unordered_map<string, unordered_map<string, string>> data;
    fstream file;

    // Internal I/O and parsing functions
    void BeginWrite(string filename);
    void ReadFileAndClose(string filename);
    void ReadFromFile();

    // Helper functions
    string ParseSectionName(string in);
    pair<string, string> GetKeyValuePair(string in, string delim = "=");
    string& TrimWhitespace(string& in);
};