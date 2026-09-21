#include "configuration/configuration_file.hpp"

#include <fstream>
#include <stdexcept>

#include "nlohmann/json.hpp"

#include "configuration/configuration_file.hpp"
#include "configuration/serialization.hpp"

using json = nlohmann::ordered_json;

DeviceConfiguration ConfigurationFile::Load(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Failed to open configuration file: " + filePath.string());
    }

    try {
        json document;
        file >> document;
        return document.get<DeviceConfiguration>();
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse JSON configuration: " + std::string(e.what()));
    }
}

DeviceConfigurationPatch ConfigurationFile::LoadPatch(const std::filesystem::path& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open configuration patch file: " + filePath.string());
    }   

    try {
        json document;
        file >> document;
        return document.get<DeviceConfigurationPatch>();
    } catch (const json::exception& e) {
        throw std::runtime_error("Failed to parse JSON configuration patch: " + std::string(e.what()));
    }
}

void ConfigurationFile::Save(const std::filesystem::path& filePath, const DeviceConfiguration& config)
{
    std::ofstream file(filePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open configuration file for writing: " + filePath.string());
    }

    file << ConfigurationToJsonString(config);
    if (!file) {
        throw std::runtime_error("Failed to write configuration to file: " + filePath.string());
    }
}

std::string ConfigurationFile::GetJsonString(const DeviceConfiguration& config)
{
    return ConfigurationToJsonString(config);
}

std::string ConfigurationFile::ConfigurationToJsonString(const DeviceConfiguration& config)
{
    const json document = config;
    return document.dump(4);
}
