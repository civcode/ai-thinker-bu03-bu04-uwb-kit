#ifndef SERIALIZATION_HPP_
#define SERIALIZATION_HPP_

#include <iconv.h>
#include <stdexcept>
#include <ctime>
#include <chrono>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

#include "protocol/device_configuration.hpp"

using json = nlohmann::ordered_json;


#include <iconv.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

inline std::string gbk_to_utf8(const std::string& input)
{
    // std::cout << "Converting GBK to UTF-8: " << input << std::endl;

    iconv_t cd = iconv_open("UTF-8", "GBK");
    if (cd == (iconv_t)-1)
        throw std::runtime_error("iconv_open failed");

    size_t in_bytes = input.size();
    size_t out_bytes = input.size() * 4 + 1;

    std::vector<char> output(out_bytes);

    char* in_ptr = const_cast<char*>(input.data());
    char* out_ptr = output.data();

    size_t result = iconv(
        cd,
        &in_ptr,
        &in_bytes,
        &out_ptr,
        &out_bytes
    );

    iconv_close(cd);

    if (result == (size_t)-1)
        throw std::runtime_error("GBK to UTF-8 conversion failed");

    return std::string(output.data(), out_ptr - output.data());
}

inline void to_json(json& j, const WorkMode &workMode) {
    j = json{
        {"command", "AT+GETWORKMODE"},
        // {"description", "0=Normal mode, 1=Test mode"},
        {"workMode", workMode.mode}
    };
}

inline void from_json(const json& j, WorkMode &workMode) {
    j.at("workMode").get_to(workMode.mode);
}

inline void to_json(json& j, const UwbMode &uwbMode) {
    j = json{
        {"command", "AT+GETUWBMODE"},
        // {"description", "0=TWR, 1=PDOA"},
        {"uwbMode", uwbMode.mode}
    };
}

inline void from_json(const json& j, UwbMode &uwbMode) {
    j.at("uwbMode").get_to(uwbMode.mode);
}

inline void to_json(json& j, const DeviceParameters& p) {
    // static constexpr const char* command = "AT+GETCFG";
    j = json{
        {"command", "AT+GETCFG"},
        {"id", p.id},
        {"role", p.role},
        {"channel", p.channel},
        {"rate", p.rate}
    };
}

inline void from_json(const json& j, DeviceParameters& p) {
    j.at("id").get_to(p.id);
    j.at("role").get_to(p.role);
    j.at("channel").get_to(p.channel);
    j.at("rate").get_to(p.rate);
}

inline void to_json(json& j, const SensorData& s) {
    j = json{
        {"command", "AT+GETSENSOR"},
        {"accX", s.accX},
        {"accY", s.accY},
        {"accZ", s.accZ},
        {"angle", s.angle}
    };
}

inline void from_json(const json& j, SensorData& s) {
    j.at("accX").get_to(s.accX);
    j.at("accY").get_to(s.accY);
    j.at("accZ").get_to(s.accZ);
    j.at("angle").get_to(s.angle);
}

inline void to_json(json& j, const TwrParameters& t) {
    j = json{
        {"command", "AT+GETDEV"},
        {"tagCapacity", t.tagCapacity},
        {"antennaDelay", t.antennaDelay},
        {"isKalmanFilterEnabled", t.isKalmanFilterEnabled},
        {"kalmanQ", t.kalmanQ},
        {"kalmanR", t.kalmanR},
        {"correctionParameterA", t.correctionParameterA},
        {"correctionParameterB", t.correctionParameterB},
        {"isPositioningEnabled", t.isPositioningEnabled},
        {"positioningDimension", t.positioningDimension}
    };
}

inline void from_json(const json& j, TwrParameters& t) {
    j.at("tagCapacity").get_to(t.tagCapacity);
    j.at("antennaDelay").get_to(t.antennaDelay);
    j.at("isKalmanFilterEnabled").get_to(t.isKalmanFilterEnabled);
    j.at("kalmanQ").get_to(t.kalmanQ);
    j.at("kalmanR").get_to(t.kalmanR);
    j.at("correctionParameterA").get_to(t.correctionParameterA);
    j.at("correctionParameterB").get_to(t.correctionParameterB);
    j.at("isPositioningEnabled").get_to(t.isPositioningEnabled);
    j.at("positioningDimension").get_to(t.positioningDimension);
}

inline void to_json(json& j, const PdoaParameters& p) {
    j = json{
        {"command", "AT+PDOAGETCFG"},
        {"dlist", p.dlist},
        {"klist", p.klist},
        {"net", p.net},
        {"anchId", p.anchId},
        {"rate", p.rate},
        {"isFilterEnabled", p.isFilterEnabled},
        {"userCmd", p.userCmd},
        {"pdoaOffset", p.pdoaOffset},
        {"rngOffset", p.rngOffset}
    };
}

inline void from_json(const json& j, PdoaParameters& p) {
    j.at("dlist").get_to(p.dlist);
    j.at("klist").get_to(p.klist);
    j.at("net").get_to(p.net);
    j.at("anchId").get_to(p.anchId);
    j.at("rate").get_to(p.rate);
    j.at("isFilterEnabled").get_to(p.isFilterEnabled);
    j.at("userCmd").get_to(p.userCmd);
    j.at("pdoaOffset").get_to(p.pdoaOffset);
    j.at("rngOffset").get_to(p.rngOffset);
}

inline void to_json(json& j, const TagParameters& t) {
    j = json{
        {"a64", t.a64},
        {"a16", t.a16},
        {"F", t.F},
        {"S", t.S},
        {"M", t.M}
    };
}

inline void from_json(const json& j, TagParameters& t) {
    j.at("a64").get_to(t.a64);
    j.at("a16").get_to(t.a16);
    j.at("F").get_to(t.F);
    j.at("S").get_to(t.S);
    j.at("M").get_to(t.M);
}

inline void to_json(json& j, const PdoaMisc& p) {
    j = json{
        // {"fw_version", p.version},
        {"fw_version", gbk_to_utf8(p.version)},
        {"dlist", gbk_to_utf8(p.dlist)},
        {"klist", gbk_to_utf8(p.klist)}
        // {"dlist", p.dlist},
        // {"klist", p.klist}
    };
}

inline void from_json(const json& j, PdoaMisc& p) {
    j.at("fw_version").get_to(p.version);
    j.at("dlist").get_to(p.dlist);
    j.at("klist").get_to(p.klist);
}

inline void to_json(json& j, const DeviceConfiguration& d) {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm* local = std::localtime(&time);
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", local);

    std::string timestamp(buffer);

    j = json{
        {"version", d.version},
        {"timestamp", timestamp},
        {"deviceParam", d.deviceParam},
        {"twrParam", d.twrParam},
        {"pdoaParam", d.pdoaParam},
        {"workMode", d.workMode},
        {"uwbMode", d.uwbMode},
        {"pdoaMisc", d.pdoaMisc}
    };
}

inline void from_json(const json& j, DeviceConfiguration& d) {
    j.at("version").get_to(d.version);
    j.at("deviceParam").get_to(d.deviceParam);
    j.at("twrParam").get_to(d.twrParam);
    j.at("pdoaParam").get_to(d.pdoaParam);
    j.at("workMode").get_to(d.workMode);
    j.at("uwbMode").get_to(d.uwbMode);
    j.at("pdoaMisc").get_to(d.pdoaMisc);
}

inline void to_json(json& j, const DeviceConfigurationPatch& d) {
    j = json::object();

    if (d.deviceParam.has_value()) {
        j["deviceParam"] = d.deviceParam.value();
    }
    if (d.twrParam.has_value()) {
        j["twrParam"] = d.twrParam.value();
    }
    // if (d.pdoaParam.has_value()) {
    //     j["pdoaParam"] = d.pdoaParam.value();
    // }
}

inline void from_json(const json& j, DeviceConfigurationPatch& d) {
    if (j.contains("deviceParam") && !j.at("deviceParam").is_null()) {
        d.deviceParam = j.at("deviceParam").get<DeviceParameters>();
    } else {
        d.deviceParam.reset();    
    }

    if (j.contains("twrParam") && !j.at("twrParam").is_null()) {
        d.twrParam = j.at("twrParam").get<TwrParameters>();
    } else {
        d.twrParam.reset();
    }
    // if (j.contains("pdoaParam")) {
    //     d.pdoaParam = j.at("pdoaParam").get<PdoaParameters>();
    // }
}

#endif // SERIALIZATION_HPP_