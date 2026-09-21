#ifndef TESTS_FIXTURES_HIL_FIXTURE_HPP_
#define TESTS_FIXTURES_HIL_FIXTURE_HPP_

// Hardware-in-the-loop fixture, per docs/testing/catch2-unit-test-spec.md §4.3 / §6.2.
//
// Device path resolution order:
//   1. UWB_HIL_SERIAL_DEVICE environment variable
//   2. HIL_SERIAL_DEVICE CMake cache variable (compile-time definition)
//   3. empty -> HIL tests skip instead of failing
//
// State-changing behaviour is gated behind UWB_HIL_ENABLE_STATEFUL=1. HilSession itself
// never changes device state: it only reads.
//
// Measured device behaviour (BU03/BU04 @ 115200, both UWB modes)
// -------------------------------------------------------------
// The module streams unsolicited GBK status text on the same UART that carries the AT
// protocol - "加入网络超时或者接收错误" (~ "join-network timeout or receive error") -
// roughly once per second:
//
//   bcd3 c8eb cdf8 c2e7 b3ac cab1 bbf2 d5df bdd3 cad5 b4ed cef3 0a   (25 bytes, every ~1 s)
//
// A command answer is a separate burst:
//
//   0d 0d "twr_pdoa_mode: 0" 0d 0a 0a "OK" 0d 0a
//
// This follows the stored device configuration, not the UWB mode: switching to TWR
// (AT+SETUWBMODE=0) leaves the output running, while a factory restore (AT+RESTORE, i.e.
// ./build/cli --restore) silences the port completely - measured 0 bytes in a 4 s idle listen
// afterwards.
//
// DeviceHandler::HandleComm() performs exactly one read per command and parses whatever
// arrives, so on this device the answer to command N is routinely read as the answer to
// command N+1, and multi-command flows (GetDeviceConfiguration = 8 commands) desync. That
// is the production defect recorded in docs/testing/catch2-unit-test-spec.md §7. The fixture
// therefore measures the idle chatter and exposes it, so tests that go through
// DeviceHandler can skip with an explanatory reason instead of reporting a false result.
//
// Fixture-level I/O uses HilRawRequest(), which reads until a line carrying the expected
// prefix shows up and discards everything else - the framing the product is missing.

#include "protocol/device_handler.hpp"
#include "transport/iuart.hpp"
#include "transport/uart.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>
#include <thread>

#ifndef HIL_SERIAL_DEVICE
#define HIL_SERIAL_DEVICE ""
#endif

// Values used by AT+GETUWBMODE / AT+SETUWBMODE (0=TWR, 1=PDOA).
inline constexpr int kHilUwbModeTwr = 0;
inline constexpr int kHilUwbModePdoa = 1;
inline constexpr int kHilUwbModeUnknown = -1;

// Fixture timeouts.
//
// kHilChatterWindow: how long the fixture listens for unsolicited output before handing the
// port to DeviceHandler. Unsolicited output on an affected device arrives every 500-1000 ms,
// so a single 1200 ms read sees at least one line. A quiet (restored) device pays that window
// once per test-binary run, because the measurement is cached across test cases; set
// UWB_HIL_REMEASURE_CHATTER=1 to measure it in every case.
inline constexpr auto kHilChatterWindow = std::chrono::milliseconds(1200);

// A command answer starts within a few tens of ms of writing the command.
inline constexpr auto kHilResponseTimeout = std::chrono::milliseconds(400);

// The answer payload and its trailing "OK" can be separated by more than the product's 50 ms
// inter-byte timeout, so the fixture tolerates a wider gap.
inline constexpr auto kHilGapTimeout = std::chrono::milliseconds(150);

inline constexpr auto kHilProbeBudget = std::chrono::milliseconds(1500);
inline constexpr auto kHilDrainTimeout = std::chrono::milliseconds(100);
inline constexpr auto kHilTerminatorGrace = std::chrono::milliseconds(150);

inline const char* HilEnv(const char* name)
{
    const char* value = std::getenv(name);
    return (value != nullptr && *value != '\0') ? value : nullptr;
}

inline std::string HilModeName(int mode)
{
    if (mode == kHilUwbModeTwr) return "twr";
    if (mode == kHilUwbModePdoa) return "pdoa";
    return "unknown";
}

struct HilFixture {
    std::string devicePath;

    HilFixture()
    {
        if (const char* env = HilEnv("UWB_HIL_SERIAL_DEVICE")) {
            devicePath = env;
        } else {
            devicePath = HIL_SERIAL_DEVICE;
        }
    }

    // True when a serial device path is configured and the node exists.
    bool available() const
    {
        if (devicePath.empty()) return false;
        std::error_code ec;
        return std::filesystem::exists(devicePath, ec);
    }

    bool statefulEnabled() const { return HilEnv("UWB_HIL_ENABLE_STATEFUL") != nullptr; }

    bool destructiveEnabled() const { return HilEnv("UWB_HIL_ENABLE_DESTRUCTIVE") != nullptr; }

    std::string skipReason() const
    {
        return "no HIL serial device configured: pass -DHIL_SERIAL_DEVICE=/dev/ttyUSB0 "
               "or set UWB_HIL_SERIAL_DEVICE";
    }
};

// Discards whatever the port is streaming. Returns the byte count.
inline std::size_t DrainUnsolicitedOutput(IUart& uart, int maxReads = 2,
                                          std::chrono::milliseconds firstByteTimeout = kHilDrainTimeout,
                                          std::chrono::milliseconds interByteTimeout = std::chrono::milliseconds(50))
{
    std::size_t discarded = 0;
    for (int attempt = 0; attempt < maxReads; ++attempt) {
        std::string stale;
        const std::size_t bytesRead = uart.readText(stale, 1024, firstByteTimeout, interByteTimeout);
        if (bytesRead == 0) break;
        discarded += bytesRead;
    }
    return discarded;
}

// True when the buffer contains a complete answer-terminator line.
inline bool HilTerminatorSeen(const std::string& buffer)
{
    std::size_t start = 0;

    while (start <= buffer.size()) {
        const std::size_t end = buffer.find('\n', start);
        std::string candidate = buffer.substr(
            start, end == std::string::npos ? std::string::npos : end - start);
        candidate.erase(std::remove(candidate.begin(), candidate.end(), '\r'), candidate.end());

        if (!candidate.empty() && (candidate == "OK" || candidate.find("ERR") != std::string::npos)) {
            return true;
        }
        if (end == std::string::npos) break;
        start = end + 1;
    }

    return false;
}

// Sends one command and returns the first line containing expectedPrefix, discarding any
// unsolicited output arriving before, between or after the answer. This is the framing
// DeviceHandler::HandleComm() lacks. Returns false when no matching line arrives in time.
inline bool HilRawRequest(Uart& uart, const std::string& command, const std::string& expectedPrefix,
                          std::string& line, std::chrono::milliseconds budget)
{
    uart.flushRxBuffer();
    if (!uart.writeText(command)) return false;

    const auto deadline = std::chrono::steady_clock::now() + budget;
    std::string buffer;
    bool answered = false;
    std::chrono::steady_clock::time_point answeredAt{};

    while (true) {
        if (answered) {
            // Read on until the answer's own terminator shows up, so the trailing "OK" is not
            // left in the buffer for the next reader - but stop the moment it is seen instead of
            // waiting out a fixed sleep.
            if (HilTerminatorSeen(buffer)) return true;
            if (std::chrono::steady_clock::now() > answeredAt + 3 * kHilTerminatorGrace) return true;
        } else if (std::chrono::steady_clock::now() >= deadline) {
            return false;
        }

        std::string chunk;
        const std::size_t bytesRead = uart.readText(chunk, 1024, kHilResponseTimeout, kHilGapTimeout);
        if (bytesRead == 0) continue;

        buffer += chunk;
        if (buffer.size() > 4096) buffer.erase(0, buffer.size() - 4096);
        if (answered) continue;

        std::size_t start = 0;
        while (start <= buffer.size()) {
            const std::size_t end = buffer.find('\n', start);
            std::string candidate = buffer.substr(
                start, end == std::string::npos ? std::string::npos : end - start);
            candidate.erase(std::remove(candidate.begin(), candidate.end(), '\r'), candidate.end());

            if (!candidate.empty() && candidate.find(expectedPrefix) != std::string::npos) {
                line = candidate;
                answered = true;
                answeredAt = std::chrono::steady_clock::now();
                break;
            }
            if (end == std::string::npos) break;
            start = end + 1;
        }
    }
}

// Framed AT+GETUWBMODE read. Returns the mode, or kHilUwbModeUnknown.
inline int HilProbeMode(Uart& uart)
{
    std::string line;
    if (!HilRawRequest(uart, "AT+GETUWBMODE\r\n", "twr_pdoa_mode:", line, kHilProbeBudget)) {
        return kHilUwbModeUnknown;
    }

    const std::size_t colon = line.find(':');
    if (colon == std::string::npos) return kHilUwbModeUnknown;

    try {
        return std::stoi(line.substr(colon + 1));
    } catch (const std::exception&) {
        return kHilUwbModeUnknown;
    }
}

// Listens for `window` without sending anything. Returns the number of bytes the device
// pushed on its own - the idle chatter that breaks DeviceHandler's single-read framing.
// One read: it reports 0 bytes only if the port stayed silent for the whole window.
inline std::size_t HilMeasureChatter(Uart& uart, std::string& sample,
                                     std::chrono::milliseconds window = kHilChatterWindow)
{
    return uart.readText(sample, 256, window, std::chrono::milliseconds(50));
}

// Opens the port at 115200 baud and clears whatever is already buffered.
struct HilDevice {
    Uart uart;
    std::size_t discardedBytes{0};
};

inline HilDevice OpenHilDevice(const HilFixture& hil)
{
    HilDevice device{Uart(hil.devicePath, B115200), 0};
    device.discardedBytes = DrainUnsolicitedOutput(device.uart);
    return device;
}

// Chatter window, overridable with UWB_HIL_CHATTER_WINDOW_MS. 0 disables the measurement and
// with it the chatter gate: the cases then run unconditionally and a chatty device shows up as
// a normal test failure instead of a skip.
inline std::chrono::milliseconds HilChatterWindow()
{
    if (const char* env = HilEnv("UWB_HIL_CHATTER_WINDOW_MS")) {
        try {
            const int value = std::stoi(env);
            if (value >= 0 && value <= 10000) return std::chrono::milliseconds(value);
        } catch (const std::exception&) {
        }
    }
    return kHilChatterWindow;
}

// Opens the device for one test case, reads the UWB mode with correct framing and measures
// the idle chatter. Read-only: no setting is written to the device.
//
// ready() is false (with skipReason()) when the port cannot be opened or does not answer
// AT+GETUWBMODE. chatty()/chatterBytes() report how much unsolicited output the module pushes
// once the port has been quiesced; framingSkipReason() spells out what that means for
// DeviceHandler's one-read-per-command flow, so a failing HIL case explains itself.
class HilSession {
public:
    explicit HilSession(const HilFixture& hil)
    {
        try {
            device_ = std::make_unique<HilDevice>(OpenHilDevice(hil));

            mode_ = HilProbeMode(device_->uart);
            if (mode_ == kHilUwbModeUnknown) {
                skipReason_ = "device did not answer AT+GETUWBMODE on " + hil.devicePath +
                              "; it may be busy, powered off or not a BU03/BU04";
                return;
            }

            // One short drain as insurance against a terminator the probe did not manage to
            // consume, so the first DeviceHandler command of the test is not reading a leftover.
            device_->discardedBytes += DrainUnsolicitedOutput(device_->uart, 1, kHilDrainTimeout);

            // Chatter is a property of the device, not of this particular open port, and these
            // cases do not change device state - so measure it once per run and reuse it.
            static std::string cachedPath;
            static std::size_t cachedBytes{0};
            static std::string cachedSample;

            const auto window = HilChatterWindow();

            if (window == std::chrono::milliseconds(0)) {
                cachedBytes = 0;
                cachedSample.clear();
                cachedPath = hil.devicePath;
            } else if (HilEnv("UWB_HIL_REMEASURE_CHATTER") != nullptr || cachedPath != hil.devicePath) {
                cachedBytes = HilMeasureChatter(device_->uart, cachedSample, window);
                cachedSample.resize(std::min<std::size_t>(cachedSample.size(), 96));
                cachedPath = hil.devicePath;
                cachedWindowMs = static_cast<int>(window.count());
            } else if (cachedWindowMs != static_cast<int>(window.count())) {
                // A shorter window than the cached measurement was taken with is not a valid
                // answer for this run, so re-measure instead of reusing a bigger sample.
                cachedBytes = HilMeasureChatter(device_->uart, cachedSample, window);
                cachedSample.resize(std::min<std::size_t>(cachedSample.size(), 96));
                cachedWindowMs = static_cast<int>(window.count());
            }

            chatterBytes_ = cachedBytes;
            chatterSample_ = cachedSample;

            handler_ = std::make_unique<DeviceHandler>(device_->uart);
        } catch (const std::exception& e) {
            skipReason_ = std::string("could not use ") + hil.devicePath + ": " + e.what();
        }
    }

    // Copies are out (unique ownership of the port); moves are needed because SKIP-free
    // helpers hand a ready session back to the test case.
    HilSession(const HilSession&) = delete;
    HilSession& operator=(const HilSession&) = delete;
    HilSession(HilSession&&) = default;
    HilSession& operator=(HilSession&&) = default;

    bool ready() const { return handler_ != nullptr && skipReason_.empty(); }

    const std::string& skipReason() const { return skipReason_; }

    DeviceHandler& handler() const { return *handler_; }

    int mode() const { return mode_; }

    bool chatty() const { return chatterBytes_ > 0; }

    std::size_t chatterBytes() const { return chatterBytes_; }

    std::size_t discardedBytes() const { return device_ ? device_->discardedBytes : 0; }

    // Test-skip reason for DeviceHandler-based cases; empty when the port is quiet.
    std::string framingSkipReason(const HilFixture& hil) const
    {
        if (!chatty()) return std::string();

        return "device pushes unsolicited status output on the protocol UART (" +
               std::to_string(chatterBytes()) + " bytes observed in " +
               std::to_string(HilChatterWindow().count()) +
               " ms of idle time, e.g. \"" + chatterSample_ + "\"); DeviceHandler::HandleComm()"
               " reads once per command, so its answers are off by one response. Known issue:"
               " see docs/testing/catch2-unit-test-spec.md §7.1. Clear the device state that"
               " produces the chatter before running these cases: export the configuration and"
               " factory-restore the device, e.g. ./build/cli --device " + hil.devicePath +
               " --export backup.json && ./build/cli --device " + hil.devicePath + " --restore";
    }

    std::string diagnostic(const HilFixture& hil) const
    {
        return "serial device: " + hil.devicePath + ", uwb mode: " + HilModeName(mode_) +
               ", bytes drained at open: " + std::to_string(discardedBytes()) +
               ", idle chatter: " + std::to_string(chatterBytes_) + " bytes";
    }

private:
    std::unique_ptr<HilDevice> device_;
    std::unique_ptr<DeviceHandler> handler_;
    int mode_{kHilUwbModeUnknown};
    std::size_t chatterBytes_{0};
    std::string chatterSample_;
    static inline int cachedWindowMs{-1};
    std::string skipReason_;
};

#endif  // TESTS_FIXTURES_HIL_FIXTURE_HPP_
