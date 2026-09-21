#ifndef DEVICE_CONFIG_HPP_
#define DEVICE_CONFIG_HPP_

#include <optional>
#include <string>

struct WorkMode {
    int mode;
};

struct UwbMode {
    int mode;
};

struct DeviceParameters {
    int id;
    int role;
    int channel;
    int rate;
};

struct SensorData {
    float accX;
    float accY;
    float accZ;
    float angle;
};

struct TwrParameters {
    int tagCapacity;
    int antennaDelay;
    bool isKalmanFilterEnabled;
    double kalmanQ;
    double kalmanR;
    double correctionParameterA;
    double correctionParameterB;
    bool isPositioningEnabled;
    int positioningDimension;
};

struct PdoaParameters {
    int dlist;
    int klist;
    int net;
    int anchId;
    int rate;
    bool isFilterEnabled;
    int userCmd;
    int pdoaOffset;
    int rngOffset;
};

struct PdoaMisc {
    std::string version;
    std::string dlist;
    std::string klist;
};

struct TagParameters {
    std::string a64;
    std::string a16;
    // int a16;
    int F;
    int S;
    int M;
};

struct DeviceConfiguration {

    static constexpr int kCurrentVersion = 1;

    int version{kCurrentVersion};
    DeviceParameters deviceParam;
    TwrParameters twrParam;
    PdoaParameters pdoaParam;
    WorkMode workMode;
    UwbMode uwbMode;
    PdoaMisc pdoaMisc;
};

struct DeviceConfigurationPatch {
    std::optional<DeviceParameters> deviceParam;
    std::optional<TwrParameters> twrParam;
    // std::optional<PdoaParameters> pdoaParam;

};

#endif // DEVICE_CONFIG_HPP_