#include <atomic>
#include <charconv>
#include <csignal>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <iostream>

#include <thread>

#include "transport/uart.hpp"
#include "protocol/device_handler.hpp"
#include "protocol/device_configuration.hpp"
#include "configuration/configuration_file.hpp"
#include "configuration/serialization.hpp"

#include "cxxopts.hpp"

namespace 
{
    std::atomic_flag stop_requested;

    extern "C" void handle_signal(int signal)
    {
        if (signal == SIGINT) {
            std::cout << "\nSIGINT received, stopping...\n";
            stop_requested.test_and_set();
        }
    }

}

bool SameDeviceParameters(const DeviceParameters& lhs, const DeviceParameters& rhs)
{
    return lhs.id == rhs.id && lhs.role == rhs.role && lhs.channel == rhs.channel && lhs.rate == rhs.rate;
}

bool SameTwrParameters(const TwrParameters& lhs, const TwrParameters& rhs)
{
    return lhs.tagCapacity == rhs.tagCapacity &&
           lhs.antennaDelay == rhs.antennaDelay &&
           lhs.isKalmanFilterEnabled == rhs.isKalmanFilterEnabled &&
           lhs.kalmanQ == rhs.kalmanQ &&
           lhs.kalmanR == rhs.kalmanR &&
           lhs.correctionParameterA == rhs.correctionParameterA &&
           lhs.correctionParameterB == rhs.correctionParameterB &&
           lhs.isPositioningEnabled == rhs.isPositioningEnabled &&
           lhs.positioningDimension == rhs.positioningDimension;
}

bool SamePdoaParameters(const PdoaParameters& lhs, const PdoaParameters& rhs)
{
    return lhs.dlist == rhs.dlist &&
           lhs.klist == rhs.klist &&
           lhs.net == rhs.net &&
           lhs.anchId == rhs.anchId &&
           lhs.rate == rhs.rate &&
           lhs.isFilterEnabled == rhs.isFilterEnabled &&
           lhs.userCmd == rhs.userCmd &&
           lhs.pdoaOffset == rhs.pdoaOffset &&
           lhs.rngOffset == rhs.rngOffset;
}

DeviceParameters ParseSetCfg(const std::string& response, const DeviceParameters& current)
{
    DeviceParameters deviceParams;
    std::vector<std::optional<std::string>> result;
    std::istringstream iss(response);
    std::string token;
    while (std::getline(iss, token, ',')) {
        if (token == "_") {
            result.push_back(std::nullopt);
            continue;
        }

        int number{};
        const char* begin = token.data();
        const char* end = begin + token.size();

        auto [ptr, ec] = std::from_chars(begin, end, number);

        if (ec != std::errc() || ptr != end) {
            std::cerr << "Unexpected response format: " + response << std::endl;
            std::cerr << "Failed to parse number: " << token << std::endl;
            throw std::runtime_error("Failed to parse number: " + token);
        }
        result.push_back(std::to_string(number));
    }
    if (result.size() != 4) {
        std::cerr << "Unexpected response format: " + response << std::endl;
        throw std::runtime_error("--set_cfg requires exactly 4 parameters");
    }

    std::vector<std::optional<int>> parsedValues;
    for (auto elem : result) {
        if (elem) {
            std::cout << "Parsed value: " << *elem << std::endl;
            parsedValues.push_back(std::stoi(*elem));
        } else {
            std::cout << "Parsed value: nullopt" << std::endl;
            parsedValues.push_back(std::nullopt);
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (parsedValues[i]) {
            switch (i) {
                case 0:
                    deviceParams.id = *parsedValues[i];
                    break;
                case 1:
                    deviceParams.role = *parsedValues[i];
                    break;
                case 2:
                    deviceParams.channel = *parsedValues[i];
                    break;
                case 3:
                    deviceParams.rate = *parsedValues[i];
                    break;
            }
        } else {
            switch (i) {
                case 0:
                    deviceParams.id = current.id;
                    break;
                case 1:
                    deviceParams.role = current.role;
                    break;
                case 2:
                    deviceParams.channel = current.channel;
                    break;
                case 3:
                    deviceParams.rate = current.rate;
                    break;
            }
        }
    }

    return deviceParams;
}

int main(int argc, const char* argv[])
{
    const std::string device = "/dev/ttyUSB0";

    std::signal(SIGINT, handle_signal);

    cxxopts::Options options(argv[0], "Command-line interface for BU04 device");

    options.add_options()
        ("device", "Serial device path", cxxopts::value<std::string>()->default_value(device))
        ("print", "Print device configuration", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
        ("export", "Export device configuration to JSON file", cxxopts::value<std::string>()
            ->default_value("device_config.json")
            ->implicit_value("device_config.json"))
        ("uwb_mode", "UWB Mode", cxxopts::value<int>(), "(int)")
        ("dev_id", "Device ID", cxxopts::value<int>(), "(int)")
        ("dev_role", "Device Role", cxxopts::value<int>(), "(int)")
        ("dev_channel", "Device Channel", cxxopts::value<int>(), "(int)")
        ("dev_rate", "Device Rate", cxxopts::value<int>(), "(int)")
        ("twr_tag_cap", "TWR Tag Capacity", cxxopts::value<int>(), "(int)")
        ("twr_antenna_delay", "TWR Antenna Delay", cxxopts::value<int>(), "(int)")
        ("twr_kalman_filter_enable", "TWR Kalman Filter Enable", cxxopts::value<int>(), "(int)")
        ("twr_kalman_Q", "TWR Kalman Filter Q", cxxopts::value<double>(), "(double)")
        ("twr_kalman_R", "TWR Kalman Filter R", cxxopts::value<double>(), "(double)")
        ("twr_correction_a", "TWR Correction parameter A", cxxopts::value<double>(), "(double)")
        ("twr_correction_b", "TWR Correction parameter B", cxxopts::value<double>(), "(double)")
        ("twr_positioning_enable", "TWR Positioning Enable", cxxopts::value<int>(), "(int)")
        ("twr_positioning_dimension", "TWR Positioning Dimension", cxxopts::value<int>(), "(int)")
        ("pdoa_d_list", "PDOA D List", cxxopts::value<int>(), "(int)")
        ("pdoa_k_list", "PDOA K List", cxxopts::value<int>(), "(int)")
        ("pdoa_net", "PDOA Network ID", cxxopts::value<int>(), "(int)")
        ("pdoa_anchor_id", "PDOA Anchor ID", cxxopts::value<int>(), "(int)")
        ("pdoa_uart_rate", "PDOA UART Rate", cxxopts::value<int>(), "(int)")
        ("pdoa_filter_enable", "PDOA Filter Enable", cxxopts::value<int>(), "(int)")
        ("pdoa_user_cmd", "PDOA User Command", cxxopts::value<int>(), "(int)")
        ("add_tag", "Add TWR Tag", cxxopts::value<std::string>(), "(string)")
        ("delete_tag", "Remove TWR Tag", cxxopts::value<std::string>(), "(string)")
        ("save", "Save device configuration", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
        ("restart", "Restart device", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
        ("restore", "Restore factory device settings", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))

        ("h,help", "Print help");
        
    auto result = options.parse(argc, argv);

    if (result.count("help") || result.arguments().empty()) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    std::cout << "Using device: " << result["device"].as<std::string>() << std::endl;

    try {
        auto uart = Uart(device, B115200);

        DeviceHandler handler(uart);

        DeviceHandler::EResult errorCode;
        std::string response;

        if (result["restore"].as<bool>()) {
            errorCode = handler.Restore(response);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5000));
                uart.flushRxBuffer();
                std::cout << "Device restored to factory settings successfully." << std::endl;
            } else {
                std::cerr << "Failed to restore device to factory settings." << std::endl;
                return 1;
            }
        }

        if (result.count("uwb_mode")) {
            int uwbMode = result["uwb_mode"].as<int>();
            errorCode = handler.SetUwbMode(uwbMode);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                int uwbModeBackup = uwbMode;
                errorCode = handler.GetUwbMode(response, uwbMode);
                if (errorCode == DeviceHandler::EResult::kSuccess && uwbMode == uwbModeBackup) {
                    std::cout << "UWB Mode verified successfully." << std::endl;
                } else {
                    std::cerr << "Failed to verify UWB Mode." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Failed to set UWB Mode." << std::endl;
                return 1;
            }

        }

        if (result.count("set_cfg")) {
            DeviceParameters current;
            errorCode = handler.GetCfg(response, current);
            if (errorCode != DeviceHandler::EResult::kSuccess) {
                std::cerr << "Failed to get current device configuration." << std::endl;
                return 1;
            }
            const std::string cfgArgs = result["set_cfg"].as<std::string>();
            DeviceParameters deviceParams;
            deviceParams = ParseSetCfg(cfgArgs, current);
            errorCode = handler.SetCfg(deviceParams);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                DeviceParameters deviceParamsBackup = deviceParams; 
                errorCode = handler.GetCfg(response, deviceParams);
                if (errorCode == DeviceHandler::EResult::kSuccess && SameDeviceParameters(deviceParams, deviceParamsBackup)) {
                    std::cout << "Device configuration verified successfully." << std::endl;
                } else {
                    std::cerr << "Failed to verify device configuration." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Failed to update device configuration." << std::endl;
                return 1;
            }
        }

        if (result.count("dev_id") || result.count("dev_role") || result.count("dev_channel") || result.count("dev_rate")) {
            DeviceParameters deviceParams;
            errorCode = handler.GetCfg(response, deviceParams);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                if (result.count("dev_id")) {
                    deviceParams.id = result["dev_id"].as<int>();
                }
                if (result.count("dev_role")) {
                    deviceParams.role = result["dev_role"].as<int>();
                }
                if (result.count("dev_channel")) {
                    deviceParams.channel = result["dev_channel"].as<int>();
                }
                if (result.count("dev_rate")) {
                    deviceParams.rate = result["dev_rate"].as<int>();
                }
                errorCode = handler.SetCfg(deviceParams);
                if (errorCode == DeviceHandler::EResult::kSuccess) {
                    DeviceParameters deviceParamsBackup = deviceParams; 
                    errorCode = handler.GetCfg(response, deviceParams);
                    if (errorCode == DeviceHandler::EResult::kSuccess && SameDeviceParameters(deviceParams, deviceParamsBackup)) {
                        std::cout << "Device configuration verified successfully." << std::endl;
                    } else {
                        std::cerr << "Failed to verify device configuration." << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Failed to update device configuration." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Failed to get current device configuration." << std::endl;
                return 1;
            }
        }

        if (result.count("twr_tag_cap") || result.count("twr_antenna_delay") || result.count("twr_kalman_filter_enable") ||
            result.count("twr_kalman_Q") || result.count("twr_kalman_R") || result.count("twr_correction_a") ||
            result.count("twr_correction_b") || result.count("twr_positioning_enable") || result.count("twr_positioning_dimension")) {
            TwrParameters twrParams;
            errorCode = handler.GetDev(response, twrParams);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                if (result.count("twr_tag_cap")) {
                    twrParams.tagCapacity = result["twr_tag_cap"].as<int>();
                }
                if (result.count("twr_antenna_delay")) {
                    twrParams.antennaDelay = result["twr_antenna_delay"].as<int>();
                }
                if (result.count("twr_kalman_filter_enable")) {
                    twrParams.isKalmanFilterEnabled = result["twr_kalman_filter_enable"].as<int>();
                }
                if (result.count("twr_kalman_Q")) {
                    twrParams.kalmanQ = result["twr_kalman_Q"].as<double>();
                }
                if (result.count("twr_kalman_R")) {
                    twrParams.kalmanR = result["twr_kalman_R"].as<double>();
                }
                if (result.count("twr_correction_a")) {
                    twrParams.correctionParameterA = result["twr_correction_a"].as<double>();
                    std::cout << "twr_correction_a: " << twrParams.correctionParameterA << std::endl;
                }
                if (result.count("twr_correction_b")) {
                    twrParams.correctionParameterB = result["twr_correction_b"].as<double>();
                    std::cout << "twr_correction_b: " << twrParams.correctionParameterB << std::endl;
                }
                if (result.count("twr_positioning_enable")) {
                    twrParams.isPositioningEnabled = result["twr_positioning_enable"].as<int>();
                }
                if (result.count("twr_positioning_dimension")) {
                    twrParams.positioningDimension = result["twr_positioning_dimension"].as<int>();
                }
                errorCode = handler.SetDev(twrParams);
                if (errorCode == DeviceHandler::EResult::kSuccess) {
                    TwrParameters twrParamsBackup = twrParams; 
                    errorCode = handler.GetDev(response, twrParams);
                    if (errorCode == DeviceHandler::EResult::kSuccess && SameTwrParameters(twrParams, twrParamsBackup)) {
                        std::cout << "TWR parameters verified successfully." << std::endl;
                    } else {
                        std::cerr << "Failed to verify TWR parameters." << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Failed to update TWR parameters." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Failed to get current TWR parameters." << std::endl;
                return 1;
            }
        }

        if (result.count("pdoa_d_list") || result.count("pdoa_k_list") || result.count("pdoa_net") ||
            result.count("pdoa_anchor_id") || result.count("pdoa_uart_rate") || result.count("pdoa_filter_enable") ||
            result.count("pdoa_user_cmd")) {
            PdoaParameters pdoaParams;
            errorCode = handler.GetPdoaCfg(response, pdoaParams);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                if (result.count("pdoa_d_list")) {
                    pdoaParams.dlist = result["pdoa_d_list"].as<int>();
                }
                if (result.count("pdoa_k_list")) {
                    pdoaParams.klist = result["pdoa_k_list"].as<int>();
                }
                if (result.count("pdoa_net")) {
                    pdoaParams.net = result["pdoa_net"].as<int>();
                }
                if (result.count("pdoa_anchor_id")) {
                    pdoaParams.anchId = result["pdoa_anchor_id"].as<int>();
                }
                if (result.count("pdoa_uart_rate")) {
                    pdoaParams.rate = result["pdoa_uart_rate"].as<int>();
                }
                if (result.count("pdoa_filter_enable")) {
                    pdoaParams.isFilterEnabled = result["pdoa_filter_enable"].as<int>();
                }
                if (result.count("pdoa_user_cmd")) {
                    pdoaParams.userCmd = result["pdoa_user_cmd"].as<int>();
                }
                errorCode = handler.SetPdoaCfg(pdoaParams);
                if (errorCode == DeviceHandler::EResult::kSuccess) {
                    PdoaParameters pdoaParamsBackup = pdoaParams; 
                    errorCode = handler.GetPdoaCfg(response, pdoaParams);
                    if (errorCode == DeviceHandler::EResult::kSuccess && SamePdoaParameters(pdoaParams, pdoaParamsBackup)) {
                        std::cout << "PDOA parameters verified successfully." << std::endl;
                    } else {
                        std::cerr << "Failed to verify PDOA parameters." << std::endl;
                        return 1;
                    }
                } else {
                    std::cerr << "Failed to update PDOA parameters." << std::endl;
                    return 1;
                }
            } else {
                std::cerr << "Failed to get current PDOA parameters." << std::endl;
                return 1;
            }
        }

        if (result.count("add_tag")) {
            std::string tagA64 = result["add_tag"].as<std::string>();
            TagParameters tagParams{
                .a64 = tagA64,
                .a16 = tagA64.substr(tagA64.size() - 4),
                .F = 64,
                .S = 64,
                .M = 0
            };
            std::cout << "a64: " << tagParams.a64 << ", a16: " << tagParams.a16 << ", F: " << tagParams.F << ", S: " << tagParams.S << ", M: " << tagParams.M << std::endl;
            errorCode = handler.AddTag(response, tagParams);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                std::cout << "TWR Tag added successfully." << std::endl;
            } else {
                std::cerr << "Failed to add TWR Tag." << std::endl;
                return 1;
            }
        }
        
        if (result.count("delete_tag")) {
            std::string tagA64 = result["delete_tag"].as<std::string>();
            errorCode = handler.DelTag(response, tagA64);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                std::cout << "TWR Tag removed successfully." << std::endl;
            } else {
                std::cerr << "Failed to remove TWR Tag." << std::endl;
                return 1;
            }
        }

        if (result.count("print") || result.count("export")) {
            DeviceConfiguration deviceConfig;
            errorCode = handler.GetDeviceConfiguration(deviceConfig);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                if (result.count("print")) {
                    std::cout << "Device Configuration (JSON):" << std::endl;
                    std::cout << ConfigurationFile::GetJsonString(deviceConfig) << std::endl;
                }
                if (result.count("export")) {
                    std::cout << "Exporting device configuration to: " << result["export"].as<std::string>() << std::endl;
                    ConfigurationFile::Save(result["export"].as<std::string>(), deviceConfig);
                }
            } else {
                std::cerr << "Failed to retrieve device configuration." << std::endl;
            }
        }

        if (result["save"].as<bool>()) {
            errorCode = handler.Save(response);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                uart.flushRxBuffer();
                std::cout << "Device configuration saved successfully." << std::endl;
            } else {
                std::cerr << "Failed to save device configuration." << std::endl;
                return 1;
            }
        }

        if (result["restart"].as<bool>()) {
            errorCode = handler.Restart(response);
            if (errorCode == DeviceHandler::EResult::kSuccess) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                std::cout << "Device restarted successfully." << std::endl;
            } else {
                std::cerr << "Failed to restart device." << std::endl;
                return 1;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
