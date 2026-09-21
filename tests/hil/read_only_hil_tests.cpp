// Read-only hardware-in-the-loop tests, per docs/testing/catch2-unit-test-spec.md §2.2 / §7.1.
//
// Runs only when a device is configured:
//   cmake -B build -DENABLE_HIL_TESTS=ON -DHIL_SERIAL_DEVICE=/dev/ttyUSB0
//   ctest --test-dir build -L hil
// or:  UWB_HIL_SERIAL_DEVICE=/dev/ttyUSB0 ./build/tests/hil_tests
//
// Nothing here writes to the device. The suite expects a quiet device, which the measured
// device state provides only after a factory restore:
//
//   ./build/cli --device /dev/ttyUSB0 --export backup.json   # keep the current config
//   ./build/cli --device /dev/ttyUSB0 --restore              # stops the unsolicited output
//
// If the module is still streaming status text, the fixture measures it and every case below
// skips with that measurement and the remedy above, instead of failing on a device state the
// tests are not allowed to change. See docs/testing/catch2-unit-test-spec.md §7.1.

#include "fixtures/hil_fixture.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

// Opens the port, reads the mode with correct framing and measures idle output. Skips the
// case when the device is unreachable, or when its unsolicited output would make
// DeviceHandler's one-read-per-command answers unreliable.
HilSession OpenQuietSession(const HilFixture& hil)
{
    HilSession session(hil);

    if (!session.ready()) SKIP(session.skipReason());
    if (!session.framingSkipReason(hil).empty()) SKIP(session.framingSkipReason(hil));

    return session;
}

}  // namespace

TEST_CASE("[HIL] configured device answers AT+GETUWBMODE", "[hil][read-only]")
{
    HilFixture hil;
    if (!hil.available()) SKIP(hil.skipReason());

    HilSession session(hil);
    if (!session.ready()) SKIP(session.skipReason());

    INFO(session.diagnostic(hil));

    // HilProbeMode() already issued and parsed AT+GETUWBMODE, so the device is reachable and
    // speaks the AT protocol. The value itself is firmware/configuration dependent.
    CHECK((session.mode() == kHilUwbModeTwr || session.mode() == kHilUwbModePdoa));
}

TEST_CASE("[HIL] consecutive commands on one open port stay in step", "[hil][read-only][framing]")
{
    HilFixture hil;
    if (!hil.available()) SKIP(hil.skipReason());

    HilSession session = OpenQuietSession(hil);

    std::string response;
    std::string version;
    int workMode = -1;

    // Six independent commands on one port. HandleComm() parses whatever its single read
    // returns, so a trailing "OK" left over from the previous answer, or any unsolicited
    // status line, is parsed as the answer to the next command. On a quiet, restored port all
    // six must be answered in step; this is what broke `cli --print` in the state described by
    // docs/testing/catch2-unit-test-spec.md §7.1 items 2-3.
    int total = 0;
    int successes = 0;
    std::string failures;

    for (int attempt = 1; attempt <= 6; ++attempt) {
        const bool useVer = (attempt % 2) == 1;
        const auto result = useVer ? session.handler().GetVer(response, version)
                                   : session.handler().GetWorkMode(response, workMode);
        ++total;
        if (result == DeviceHandler::EResult::kSuccess) {
            ++successes;
        } else {
            failures += "command " + std::to_string(attempt) + " -> result " +
                        std::to_string(static_cast<int>(result)) +
                        " response \"" + response + "\"\n";
        }
    }

    INFO(session.diagnostic(hil));
    INFO(std::to_string(successes) + "/" + std::to_string(total) + " commands answered in step");
    INFO(failures);

    CHECK(total == 6);
    CHECK(successes == total);
}

TEST_CASE("[HIL] GetVer returns a version string", "[hil][read-only]")
{
    HilFixture hil;
    if (!hil.available()) SKIP(hil.skipReason());

    HilSession session = OpenQuietSession(hil);

    std::string response;
    std::string version;
    const auto result = session.handler().GetVer(response, version);

    INFO(session.diagnostic(hil));
    INFO("raw response: \"" << response << "\"");

    CHECK(result == DeviceHandler::EResult::kSuccess);
    CHECK_FALSE(version.empty());
}

TEST_CASE("[HIL] GetWorkMode returns a work mode", "[hil][read-only]")
{
    HilFixture hil;
    if (!hil.available()) SKIP(hil.skipReason());

    HilSession session = OpenQuietSession(hil);

    std::string response;
    int workMode = -1;
    const auto result = session.handler().GetWorkMode(response, workMode);

    INFO(session.diagnostic(hil));
    INFO("raw response: \"" << response << "\"");

    CHECK(result == DeviceHandler::EResult::kSuccess);
    CHECK(workMode >= 0);
}

TEST_CASE("[HIL] GetDeviceConfiguration reads back a configuration", "[hil][read-only]")
{
    HilFixture hil;
    if (!hil.available()) SKIP(hil.skipReason());

    HilSession session = OpenQuietSession(hil);

    DeviceConfiguration readBack;
    const auto result = session.handler().GetDeviceConfiguration(readBack);

    INFO(session.diagnostic(hil));

    // Eight sequential commands on one port: the flow that desyncs first when answers are
    // misaligned. An incomplete configuration is reported as kError by design (Appendix A,
    // UT-DH-091), so a restored device must reach kSuccess here.
    CHECK(result == DeviceHandler::EResult::kSuccess);
}
