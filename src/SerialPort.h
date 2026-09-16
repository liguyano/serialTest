#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace serial {

enum class Parity {
    None,
    Odd,
    Even,
};

enum class StopBits {
    One,
    Two,
};

enum class FlowControl {
    None,
    Hardware,
    Software,
};

struct SerialSettings {
    std::wstring port;
    std::uint32_t baudRate = 115200;
    std::uint8_t dataBits = 8;
    Parity parity = Parity::None;
    StopBits stopBits = StopBits::One;
    FlowControl flowControl = FlowControl::None;
};

class SerialPort {
public:
    using ReceiveCallback = std::function<void(std::vector<std::uint8_t>)>;
    using ErrorCallback = std::function<void(std::string)>;

    SerialPort() = default;
    ~SerialPort();

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    static std::vector<std::wstring> EnumeratePorts();

    bool Open(const SerialSettings& settings,
              ReceiveCallback receiveCallback,
              ErrorCallback errorCallback);
    void Close();
    bool Write(const std::vector<std::uint8_t>& bytes, std::string& error);
    bool IsOpen() const;

private:
    void ReaderLoop();
    void ReportError(const std::string& message) const;

    mutable std::mutex handleMutex_;
    void* handle_ = nullptr;
    std::atomic<bool> running_{false};
    std::thread readerThread_;
    ReceiveCallback receiveCallback_;
    ErrorCallback errorCallback_;
};

}  // namespace serial
