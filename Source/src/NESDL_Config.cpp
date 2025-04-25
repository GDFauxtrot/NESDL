#include "NESDL.h"

void NESDL_Config::Init(NESDL_Core* c)
{
    core = c;
}

template <typename T>
void NESDL_Config::WriteValue(string section, string key, T value)
{

}

template <typename T>
T NESDL_Config::ReadValue(string section, string key, T defVal)
{
	return defVal;
}

void NESDL_Config::WriteValue(string section, string key, bool value)
{
    //config_file cfg(FILENAME, config_file::WRITE);
    //cfg.put<bool>(key, value);
    //cfg.write_changes_unique();
    //cfg.close();
}

void NESDL_Config::WriteValue(string section, string key, int value)
{
    //config_file cfg(FILENAME, config_file::WRITE);
    //cfg.put<int>(key, value);
    //cfg.write_changes_unique();
    //cfg.close();
}

void NESDL_Config::WriteValue(string section, string key, string value)
{
    //config_file cfg(FILENAME, config_file::WRITE);
    //cfg.put<string>(key, value);
    //cfg.write_changes_unique();
    //cfg.close();
}

bool NESDL_Config::ReadBool(string section, string key, bool defVal)
{
    //config_file cfg(FILENAME, config_file::READ);
    //bool result = cfg.get(key, defVal);
    //cfg.close();
    //return result;
}

int NESDL_Config::ReadInt(string section, string key, int defVal)
{
    //config_file cfg(FILENAME, config_file::READ);
    //int result = cfg.get(key, defVal);
    //cfg.close();
    //return result;
}

string NESDL_Config::ReadString(string section, string key, string defVal)
{
    //config_file cfg(FILENAME, config_file::READ);
    //string result = cfg.get(key, defVal);
    //cfg.close();
    //return result;
}

void NESDL_Config::BeginWrite(string filename)
{
    // Open file in fstream::in and read existing values
    file.open(filename, std::fstream::in);
    ReadFromFile();
    file.close();

    // Open file again in fstream::out, to be ready for writes
    file.open(filename, std::fstream::out);

    // Log an error if we couldn't open the file for whatever reason
    if (!file.is_open())
    {
        //if it isn't, throw an exception
        printf("CONFIG FILE NOT OPEN - something bad happened.\n");
    }
}

void NESDL_Config::ReadFileAndClose(string filename)
{
    //touch file to create it if non-existent
    file.open(filename, std::fstream::out | std::fstream::app);
    file.close();
    
    // Open file again in read mode
    file.open(filename, std::fstream::in);

    // Log an error if we couldn't open the file for whatever reason
    if (!file.is_open())
    {
        printf("CONFIG FILE NOT OPEN - something bad happened.\n");
    }
    else
    {
        // Read the values from the file
        ReadFromFile();
        // Close file once we're done (no need to keep it open)
        file.close();
    }
}

void NESDL_Config::ReadFromFile()
{
    // Do nothing if we have no file currently open
    if (!file.is_open())
    {
        return;
    }

    // Clear currently-held data values before loading new values
    data.clear();

    // Go through the currently-loaded file and load each line
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
        if (currentLine[0] == '[')
        {
            currentSection = ParseSectionName(currentLine);
        }
        else
        {
            pair<string, string> keyVal = GetKeyValuePair(currentLine);
            data[currentSection][keyVal.first] = keyVal.second;
        }
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
        file << "[" << sectionKV.first << "]\n";
        for (pair<string, string> kv : sectionKV.second)
        {
            file << kv.first << "=" << kv.second << "\n";
        }
    }
    file.close();
}
