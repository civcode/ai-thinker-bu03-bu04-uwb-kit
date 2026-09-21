#ifndef UART_HPP_
#define UART_HPP_

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <stdexcept>
#include <string>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <span>
#include <vector>

#include "transport/iuart.hpp"

class Uart final : public IUart {
public:
    Uart(const std::string& device, speed_t baudRate)
        : fd_(-1)
    {
        fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (fd_ < 0) {
            throw std::runtime_error(
                "Failed to open " + device + ": " + std::strerror(errno));
        }

        try {
            configure(baudRate);
        } catch (...) {
            ::close(fd_);
            fd_ = -1;
            throw;
        }
    }

    ~Uart() override
    {
        if (fd_ >= 0) {
            ::close(fd_);
        }
    }

    Uart(const Uart&) = delete;
    Uart& operator=(const Uart&) = delete;

    Uart(Uart&&) noexcept = default;
    Uart& operator=(Uart&&) noexcept = default;

    void flushUartBuffers()
    {
        if (::tcflush(fd_, TCIOFLUSH) != 0) {
            throw std::runtime_error(
                "tcflush failed: " + std::string(std::strerror(errno)));
        }
    }

    void flushTxBuffer()
    {
        if (::tcflush(fd_, TCOFLUSH) != 0) {
            throw std::runtime_error(
                "tcflush failed: " + std::string(std::strerror(errno)));
        }
    }

    void flushRxBuffer()
    {
        if (::tcflush(fd_, TCIFLUSH) != 0) {
            throw std::runtime_error(
                "tcflush failed: " + std::string(std::strerror(errno)));
        }
    }

    bool write(const std::uint8_t* data, std::size_t size) override
    {
        // flushRxBuffer();

        std::size_t totalWritten = 0;

        while (totalWritten < size) {
            const ssize_t written = ::write(
                fd_,
                data + totalWritten,
                size - totalWritten);

            if (written > 0) {
                totalWritten += static_cast<std::size_t>(written);
            } else if (written < 0 && errno == EINTR) {
                continue;
            } else {
                return false;
            }
        }

        // Wait until all bytes have physically left the transmitter.
        if (::tcdrain(fd_) != 0) {
            return false;
        }

        return true;
    }

    bool writeSlow(const std::uint8_t* data, std::size_t size, std::chrono::milliseconds delay)
    {
        for (std::size_t i = 0; i < size; ++i) {

            while (true) {
                const ssize_t written = ::write(fd_, data + i, 1);

                if (written == 1) {
                    break;
                }

                if (written < 0 && errno == EINTR) {
                    continue;
                }

                return false;
            }

            // Ensure this byte has actually left the UART
            if (::tcdrain(fd_) != 0) {
                return false;
            }

            // Delay before sending the next byte
            if (i + 1 < size) {
                std::this_thread::sleep_for(delay);
            }
        }

        return true;
    }

    // std::span<std::uint8_t> readSome(std::size_t maxBytes = 256)
    // {
    //     std::vector<std::uint8_t> bufferInit(maxBytes, 0);
    //     std::span<std::uint8_t> buffer(bufferInit);

    //     const ssize_t count = ::read(fd_, buffer.data(), buffer.size());

    //     if (count > 0) {
    //         // buffer. resize(static_cast<std::size_t>(count));

    //         return buffer.first(count);
    //     }

    //     if (count == 0) {
    //         // Timeout occurred.
    //         return {};
    //     }

    //     if (errno == EINTR) {
    //         return {};
    //     }

    //     throw std::runtime_error(
    //         "UART read failed: " + std::string(std::strerror(errno)));
    // }
    
    std::size_t read(std::uint8_t* buffer, std::size_t capacity, 
                     std::chrono::milliseconds firstByteTimeout=std::chrono::milliseconds(1000),
                     std::chrono::milliseconds interByteTimeout=std::chrono::milliseconds(200)) override
    {
        // flushTxBuffer();
        // std::string result;
        char read_buffer[256];
        std::size_t totalRead = 0;

        int timeoutMs = firstByteTimeout.count();

        while (true) {
            pollfd descriptor{};
            descriptor.fd = fd_;
            descriptor.events = POLLIN;

            int pollResult;

            do {
                pollResult = ::poll(&descriptor, 1, timeoutMs);
            } while (pollResult < 0 && errno == EINTR);

            if (pollResult < 0) {
                throw std::runtime_error(
                    "UART poll failed: " +
                    std::string(std::strerror(errno)));
            }

            if (pollResult == 0) {
                // No data arrived before the timeout.
                break;
            }

            if (descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                throw std::runtime_error("UART poll reported an error");
            }

            if (descriptor.revents & POLLIN) {
                ssize_t count;

                do {
                    count = ::read(fd_, read_buffer, sizeof(read_buffer));
                } while (count < 0 && errno == EINTR);

                if (count > 0) {
                    // result.append(read_buffer, static_cast<std::size_t>(count));
                    const std::size_t available = capacity - totalRead;
                    if (available == 0) {
                        break;
                    }

                    const std::size_t toCopy = std::min(
                        static_cast<std::size_t>(count),
                        available);

                    std::memcpy(
                        buffer + totalRead,
                        read_buffer,
                        toCopy);
                    totalRead += toCopy;

                    if (totalRead >= capacity) {
                        break;
                    }

                    // After receiving the first bytes, use the shorter
                    // timeout to detect the end of the message.
                    timeoutMs = interByteTimeout.count();
                    continue;
                }

                if (count == 0) {
                    break;
                }

                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }

                throw std::runtime_error(
                    "UART read failed: " +
                    std::string(std::strerror(errno)));
            }
        }

        // flushUartBuffers();
        return totalRead;
    }

private:
    int fd_;

    void configure(speed_t baudRate)
    {
        termios tty{};

        if (::tcgetattr(fd_, &tty) != 0) {
            throw std::runtime_error(
                "tcgetattr failed: " + std::string(std::strerror(errno)));
        }

        // Set input and output baud rates.
        if (::cfsetispeed(&tty, baudRate) != 0 ||
            ::cfsetospeed(&tty, baudRate) != 0) {
            throw std::runtime_error(
                "Failed to set baud rate: " +
                std::string(std::strerror(errno)));
        }

        // 8 data bits.
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;

        // No parity.
        tty.c_cflag &= ~PARENB;

        // One stop bit.
        tty.c_cflag &= ~CSTOPB;

        // Disable hardware flow control.
        tty.c_cflag &= ~CRTSCTS;

        // Enable receiver and ignore modem-control lines.
        tty.c_cflag |= CREAD | CLOCAL;

        // Raw input mode.
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

        // Disable software flow control and special input processing.
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
                         ICRNL);

        // Raw output mode.
        tty.c_oflag &= ~(OPOST | ONLCR);

        /*
         * Read behavior:
         *
         * VMIN  = 0
         * VTIME = 10
         *
         * read() waits for up to 1 second.
         * VTIME is measured in tenths of a second.
         */
        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 10; // time in 100 ms => 10*0.1 = 1 s

        if (::tcsetattr(fd_, TCSANOW, &tty) != 0) {
            throw std::runtime_error(
                "tcsetattr failed: " + std::string(std::strerror(errno)));
        }

        // Discard stale data already present in the UART buffers.
        if (::tcflush(fd_, TCIOFLUSH) != 0) {
            throw std::runtime_error(
                "tcflush failed: " + std::string(std::strerror(errno)));
        }
    }
};
#endif // UART_HPP_