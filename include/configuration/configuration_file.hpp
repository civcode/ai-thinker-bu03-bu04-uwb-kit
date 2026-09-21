#ifndef CONFIGURATION_FILE_HPP_
#define CONFIGURATION_FILE_HPP_

#include <filesystem>
#include <string>


#include "protocol/device_configuration.hpp"

class ConfigurationFile {
public:
    static DeviceConfiguration Load(const std::filesystem::path& filePath);
    static DeviceConfigurationPatch LoadPatch(const std::filesystem::path& filePath);
    static void Save(const std::filesystem::path& filePath, const DeviceConfiguration& config);
    static std::string GetJsonString(const DeviceConfiguration& config); 
private:
    static std::string ConfigurationToJsonString(const DeviceConfiguration& config);
};

#endif // CONFIGURATION_FILE_HPP_