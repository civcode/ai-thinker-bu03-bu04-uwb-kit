// Harness smoke tests.
//
// These exist only to prove that the Catch2 + FetchContent + CTest wiring works
// and that the test targets link against the production sources. They are not
// the real suite: the actual cases are enumerated in
// docs/testing/catch2-unit-test-spec.md (UT-SER-*, UT-CFG-*, UT-DH-*, UT-CLI-*).

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <string>

#include "configuration/serialization.hpp"
#include "fakes/fake_uart.hpp"
#include "fixtures/hil_fixture.hpp"
#include "protocol/device_configuration.hpp"
#include "protocol/device_handler.hpp"

TEST_CASE("[Serialization] DeviceParameters to_json/from_json round-trip", "[unit][serialization]")
{
    // Placeholder for UT-SER-010 / UT-SER-020.
    const DeviceParameters in{1, 2, 5, 0};

    const json j = in;
    CHECK(j.at("command").get<std::string>() == "AT+GETCFG");
    CHECK(j.at("id").get<int>() == 1);
    CHECK(j.at("role").get<int>() == 2);
    CHECK(j.at("channel").get<int>() == 5);
    CHECK(j.at("rate").get<int>() == 0);

    const DeviceParameters out = j.get<DeviceParameters>();
    CHECK(out.id == in.id);
    CHECK(out.role == in.role);
    CHECK(out.channel == in.channel);
    CHECK(out.rate == in.rate);
}

TEST_CASE("[IUart] readText clears, resizes and forwards timeouts", "[unit][iuart]")
{
    // Placeholders for UT-IUART-001 / UT-IUART-002 / UT-IUART-003.
    FakeUart uart;
    uart.queueRead("OK\r\n");

    std::string text = "stale content that must be replaced";
    const std::size_t bytesRead = uart.readText(
        text, 64, std::chrono::milliseconds(250), std::chrono::milliseconds(25));

    CHECK(bytesRead == 4);
    CHECK(text == "OK\r\n");
    CHECK(uart.lastFirstByteTimeout() == std::chrono::milliseconds(250));
    CHECK(uart.lastInterByteTimeout() == std::chrono::milliseconds(25));
}

TEST_CASE("[IUart] writeText delegates to writeSlow with 5 ms delay", "[unit][iuart]")
{
    // Placeholder for UT-IUART-004 / UT-IUART-005.
    FakeUart uart;

    CHECK(uart.writeText("AT\r\n"));
    CHECK(uart.lastCommand() == "AT\r\n");
    CHECK(uart.lastWriteDelay() == std::chrono::milliseconds(5));
}

TEST_CASE("[DeviceHandler] GetAt writes AT and reports success", "[unit][device-handler]")
{
    // Placeholder for UT-DH-100 / UT-DH-030.
    FakeUart uart;
    uart.queueRead("OK\r\n");

    DeviceHandler handler(uart);
    std::string response;

    CHECK(handler.GetAt(response) == DeviceHandler::EResult::kSuccess);
    CHECK(uart.lastCommand() == "AT\r\n");
}

TEST_CASE("[DeviceHandler] empty scripted read yields timeout", "[unit][device-handler][error-path]")
{
    // Placeholder for UT-DH-031.
    FakeUart uart;

    DeviceHandler handler(uart);
    std::string response;

    CHECK(handler.GetAt(response) == DeviceHandler::EResult::kTimeout);
}

TEST_CASE("[HIL helper] unsolicited device output is drained before the first command", "[unit][hil-helper]")
{
    // Guards the workaround for the stale-RX-buffer failure seen on real hardware:
    // HandleComm() never purges the RX buffer, so a device that is already streaming
    // text gets read as the response to the first command.
    // Observed on real hardware (GBK): "加入网络超时或者接收错误" - a PDOA
    // join-network error the device prints on its own.
    const std::string unsolicited = "unsolicited device output\n";

    FakeUart uart;
    uart.queueRead(unsolicited);

    const std::size_t discarded = DrainUnsolicitedOutput(uart);
    CHECK(discarded == unsolicited.size());
    CHECK(uart.readCalls() == 2);  // second read returns 0 bytes and stops the drain

    // After draining, the scripted response is read as normal.
    uart.queueRead("OK\r\n");
    DeviceHandler handler(uart);
    std::string response;
    CHECK(handler.GetAt(response) == DeviceHandler::EResult::kSuccess);
}
