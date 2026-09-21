#ifndef TESTS_FAKES_FAKE_UART_HPP_
#define TESTS_FAKES_FAKE_UART_HPP_

// Test double for IUart, per docs/testing/catch2-unit-test-spec.md §4.2.
//
// Capabilities:
//   - queue scripted read responses (byte payload; empty queue models a timeout)
//   - capture write() / writeSlow() payloads in order
//   - force write failures
//   - expose last command and full command history
//   - record the timeout values handed to read() and the delay handed to writeSlow()

#include "transport/iuart.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

class FakeUart final : public IUart {
public:
    // ---- scripted input ---------------------------------------------------
    void queueRead(std::string data) { pendingReads_.push_back(std::move(data)); }
    void clearReads() { pendingReads_.clear(); }

    // ---- failure injection ------------------------------------------------
    void failNextWrite(bool fail) { failWrite_ = fail; }

    // ---- observation ------------------------------------------------------
    const std::vector<std::string>& writeHistory() const { return writes_; }
    std::string lastCommand() const { return writes_.empty() ? std::string() : writes_.back(); }
    std::string allWritten() const {
        std::string all;
        for (const std::string& w : writes_) all += w;
        return all;
    }
    std::size_t readCalls() const { return readCalls_; }
    std::size_t flushTxCalls() const { return flushTxCalls_; }
    std::size_t flushRxCalls() const { return flushRxCalls_; }
    std::chrono::milliseconds lastFirstByteTimeout() const { return lastFirstByteTimeout_; }
    std::chrono::milliseconds lastInterByteTimeout() const { return lastInterByteTimeout_; }
    std::chrono::milliseconds lastWriteDelay() const { return lastWriteDelay_; }

    // ---- IUart ------------------------------------------------------------
    std::size_t read(std::uint8_t* buffer, std::size_t capacity,
                     std::chrono::milliseconds firstByteTimeout,
                     std::chrono::milliseconds interByteTimeout) override
    {
        ++readCalls_;
        lastFirstByteTimeout_ = firstByteTimeout;
        lastInterByteTimeout_ = interByteTimeout;

        if (pendingReads_.empty()) return 0;  // no scripted data -> read timeout

        std::string data = std::move(pendingReads_.front());
        pendingReads_.pop_front();

        const std::size_t n = std::min(capacity, data.size());
        for (std::size_t i = 0; i < n; ++i) {
            buffer[i] = static_cast<std::uint8_t>(data[i]);
        }
        return n;
    }

    bool write(const std::uint8_t* data, std::size_t size) override
    {
        writes_.emplace_back(reinterpret_cast<const char*>(data), size);
        lastWriteDelay_ = std::chrono::milliseconds::zero();
        if (failWrite_) {
            failWrite_ = false;
            return false;
        }
        return true;
    }

    bool writeSlow(const std::uint8_t* data, std::size_t size, std::chrono::milliseconds delay) override
    {
        writes_.emplace_back(reinterpret_cast<const char*>(data), size);
        lastWriteDelay_ = delay;
        if (failWrite_) {
            failWrite_ = false;
            return false;
        }
        return true;
    }

    void flushTxBuffer() override { ++flushTxCalls_; }
    void flushRxBuffer() override { ++flushRxCalls_; }

private:
    std::deque<std::string> pendingReads_;
    std::vector<std::string> writes_;
    bool failWrite_ = false;
    std::size_t readCalls_ = 0;
    std::size_t flushTxCalls_ = 0;
    std::size_t flushRxCalls_ = 0;
    std::chrono::milliseconds lastFirstByteTimeout_{};
    std::chrono::milliseconds lastInterByteTimeout_{};
    std::chrono::milliseconds lastWriteDelay_{};
};

#endif  // TESTS_FAKES_FAKE_UART_HPP_
