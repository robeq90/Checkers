#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
public:
    // Config constructor automatically loads settings when the object is created
    Config()
    {
        reload();
    }

    // The reload() function
    // Loads settings from the JSON file "settings.json"
    // Opens the file, reads its content into the json object, then closes the file
    void reload()
    {
        std::ifstream fin(project_path + "settings.json");
        fin >> config;
        fin.close();
    }

    // Overloaded operator ()
    // Allows using the Config object like a function
    // Takes two parameters: a settings section and a setting name within that section
    // Returns the value of the setting from the loaded JSON configuration
    auto operator()(const std::string& setting_dir, const std::string& setting_name) const
    {
        return config[setting_dir][setting_name];
    }

private:
    json config; // Object that stores the loaded configuration from JSON
};