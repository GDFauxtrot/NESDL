#include "NESDL.h"

void NESDL_Config::Init(NESDL_Core* c)
{
    core = c;

    // On init, read values from our config file
    ReadFileAndClose(FILENAME);

    // On first launch/missing cfg/etc, populate config file with defaults
    if (data.empty())
    {
        BeginWrite(FILENAME);
        data = GetConfigDefaults();
        WriteToFileAndClose();
    }
}


void NESDL_Config::WriteToFileAndClose()
{
    if (!file.is_open())
    {
        return;
    }

    for (pair<string, unordered_map<string, string>> sectionKV : data)
    {
        // Write section, then all entries in the section
        file << "[" << sectionKV.first << "]\n";
        for (pair<string, string> kv : sectionKV.second)
        {
            file << kv.first << "=" << kv.second << "\n";
        }
        file << "\n";
    }
    file.close();
}

unordered_map<string, unordered_map<string, string>> NESDL_Config::GetConfigDefaults()
{
    return
    {
        { ConfigSection::GENERAL,
            {
                { ConfigKey::LASTROM, "" },
                { ConfigKey::LASTLOG, "" }
            }
        },
        {
            ConfigSection::PLAYER1,
            {
                { ConfigKey::INPUT_UP, SDL_GetKeyName(SDLK_w) },
                { ConfigKey::INPUT_DOWN, SDL_GetKeyName(SDLK_s) },
                { ConfigKey::INPUT_LEFT, SDL_GetKeyName(SDLK_a) },
                { ConfigKey::INPUT_RIGHT, SDL_GetKeyName(SDLK_d) },
                { ConfigKey::INPUT_A, SDL_GetKeyName(SDLK_m) },
                { ConfigKey::INPUT_B, SDL_GetKeyName(SDLK_n) },
                { ConfigKey::INPUT_SELECT, SDL_GetKeyName(SDLK_COMMA) },
                { ConfigKey::INPUT_START, SDL_GetKeyName(SDLK_PERIOD) }
            }
        }
    };
}

void NESDL_Config::BeginWrite(string filename)
{
    // Open file in fstream::in and read existing values
    file.open(filename, std::fstream::in);
    ReadFromFile();
    file.close();

    file.open(filename, std::fstream::out);
    if (!file.is_open())
    {
        printf("CONFIG FILE NOT OPEN - something bad happened.\n");
    }
}

void NESDL_Config::ReadFileAndClose(string filename)
{
    // Touch file to create it if non-existent
    file.open(filename, std::fstream::out | std::fstream::app);
    file.close();

    file.open(filename, std::fstream::in);
    if (!file.is_open())
    {
        printf("CONFIG FILE NOT OPEN - something bad happened.\n");
    }
    else
    {
        ReadFromFile();
        file.close(); // Close file once we're done (no need to keep it open)
    }
}

void NESDL_Config::ReadFromFile()
{
    if (!file.is_open())
    {
        return;
    }
    data.clear();

    string currentSection = "";
    string currentLine;
    while (getline(file, currentLine))
    {
        currentLine = TrimWhitespace(currentLine);

        // Ignore line if it starts with a comment or is empty
        if (currentLine.empty() || currentLine[0] == '#')
        {
            continue;
        }
        // Section
        if (currentLine[0] == '[')
        {
            currentSection = ParseSectionName(currentLine);
        }
        // Normal entry
        else
        {
            pair<string, string> keyVal = GetKeyValuePair(currentLine);
            data[currentSection][keyVal.first] = keyVal.second;
        }
    }
}

string NESDL_Config::ParseSectionName(string in)
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

pair<string, string> NESDL_Config::GetKeyValuePair(string in, string delim)
{
    size_t i = in.find_first_of(delim);
    if (i > 0)
    {
        return pair<string, string>(in.substr(0, i), in.substr(i + 1, in.length() - i));
    }
    else
    {
        return pair<string, string>(in, "");
    }
}

// https://stackoverflow.com/a/25385766
string& NESDL_Config::TrimWhitespace(string& in)
{
    static const char* whitespace = " \t\n\r\f\v";
    in.erase(in.find_last_not_of(whitespace) + 1); // Trim right
    in.erase(0, in.find_first_not_of(whitespace)); // Trim left
    return in;
}
