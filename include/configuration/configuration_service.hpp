#ifndef CONFIGURATION_SERVICE_HPP_
#define CONFIGURATION_SERVICE_HPP_


// #include "transport/iuart.hpp"
#include "protocol/device_configuration.hpp"
#include "protocol/device_handler.hpp"

class ConfigurationService {
public:
    ConfigurationService(DeviceHandler& handler) : handler_(handler) {}

    // DeviceParameters GetDeviceConfiguration();
    // bool SetDeviceConfiguration(const DeviceParameters& deviceConfig);


private:
    DeviceHandler& handler_;
};

#endif // CONFIGURATION_SERVICE_HPP_