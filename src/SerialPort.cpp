#include "SerialPort.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cwctype>
#include <memory>
#include <stdexcept>

namespace serial {
namespace {

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, text.c_str(),
                                             static_cast<int>(text.size()),
                                             nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return "Windows error";
    }
    std::string result(static_cast<std::size_t>(required), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                        result.data(), required, nullptr, nullptr);
    return result;
}

std::string Win32ErrorText(DWORD errorCode) {
    wchar_t* message = nullptr;
    const DWORD size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&message), 0, nullptr);

    if (size == 0 || message == nullptr) {
        return "Windows error " + std::to_string(errorCode);
    }

    std::wstring wide(message, size);
    LocalFree(message);
    while (!wide.empty() && (wide.back() == L'\r' || wide.back() == L'\n' || wide.back() == L' ')) {
        wide.pop_back();
    }
    return WideToUtf8(wide) + " (" + std::to_string(errorCode) + ")";
}

bool IsComName(const std::wstring& name) {
    if (name.size() <= 3) {
        return false;
    }
    if (towupper(name[0]) != L'C' || towupper(name[1]) != L'O' || towupper(name[2]) != L'M') {
        return false;
    }
    return std::all_of(name.begin() + 3, name.end(), [](wchar_t c) {
        return std::iswdigit(c) != 0;
    });
}

int ComNumber(const std::wstring& port) {
    try {
        return std::stoi(port.substr(3));
    } catch (...) {
        return 0;
    }
}

BYTE ToParity(Parity parity) {
    switch (parity) {
        case Parity::None: return NOPARITY;
        case Parity::Odd: return ODDPARITY;
        case Parity::Even: return EVENPARITY;
    }
    return NOPARITY;
}

BYTE ToStopBits(StopBits stopBits) {
    return stopBits == StopBits::Two ? TWOSTOPBITS : ONESTOPBIT;
}

bool ConfigureHandle(HANDLE handle, const SerialSettings& settings, std::string& error) {
    if (!SetupComm(handle, 64 * 1024, 64 * 1024)) {
        error = "SetupComm failed: " + Win32ErrorText(GetLastError());
        return false;
    }

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(handle, &dcb)) {
        error = "GetCommState failed: " + Win32ErrorText(GetLastError());
        return false;
    }

    dcb.BaudRate = settings.baudRate;
    dcb.ByteSize = settings.dataBits;
    dcb.Parity = ToParity(settings.parity);
    dcb.StopBits = ToStopBits(settings.stopBits);
    dcb.fBinary = TRUE;
    dcb.fParity = settings.parity == Parity::None ? FALSE : TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;

    if (settings.flowControl == FlowControl::Hardware) {
        dcb.fOutxCtsFlow = TRUE;
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
    } else if (settings.flowControl == FlowControl::Software) {
        dcb.fOutX = TRUE;
        dcb.fInX = TRUE;
    }

    if (!SetCommState(handle, &dcb)) {
        error = "SetCommState failed: " + Win32ErrorText(GetLastError());
        return false;
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;
    if (!SetCommTimeouts(handle, &timeouts)) {
        error = "SetCommTimeouts failed: " + Win32ErrorText(GetLastError());
        return false;
    }

    if (!PurgeComm(handle, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR)) {
        error = "PurgeComm failed: " + Win32ErrorText(GetLastError());
        return false;
    }

    return true;
}

}  // namespace

SerialPort::~SerialPort() {
    Close();
}

std::vector<std::wstring> SerialPort::EnumeratePorts() {
    std::vector<wchar_t> buffer(32768);
    DWORD chars = 0;

    for (;;) {
        chars = QueryDosDeviceW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (chars != 0) {
            break;
        }
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            return {};
        }
        buffer.resize(buffer.size() * 2);
    }

    std::vector<std::wstring> ports;
    const wchar_t* current = buffer.data();
    while (*current != L'\0') {
        std::wstring name(current);
        if (IsComName(name)) {
            ports.push_back(std::move(name));
        }
        current += std::wcslen(current) + 1;
    }

    std::sort(ports.begin(), ports.end(), [](const std::wstring& a, const std::wstring& b) {
        return ComNumber(a) < ComNumber(b);
    });
    ports.erase(std::unique(ports.begin(), ports.end()), ports.end());
    return ports;
}

bool SerialPort::Open(const SerialSettings& settings,
                      ReceiveCallback receiveCallback,
                      ErrorCallback errorCallback) {
    Close();
    receiveCallback_ = std::move(receiveCallback);
    errorCallback_ = std::move(errorCallback);

    if (settings.port.empty()) {
        ReportError("No COM port was selected.");
        return false;
    }

    const std::wstring devicePath = L"\\\\.\\" + settings.port;
    HANDLE handle = CreateFileW(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        ReportError("Could not open " + WideToUtf8(settings.port) + ": " +
                    Win32ErrorText(GetLastError()));
        return false;
    }

    std::string configurationError;
    if (!ConfigureHandle(handle, settings, configurationError)) {
        CloseHandle(handle);
        ReportError(configurationError);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(handleMutex_);
        handle_ = handle;
    }
    running_.store(true);

    try {
        readerThread_ = std::thread(&SerialPort::ReaderLoop, this);
    } catch (const std::exception& ex) {
        running_.store(false);
        {
            std::lock_guard<std::mutex> lock(handleMutex_);
            handle_ = nullptr;
        }
        CloseHandle(handle);
        ReportError(std::string("Could not start serial reader thread: ") + ex.what());
        return false;
    }

    return true;
}

void SerialPort::Close() {
    running_.store(false);

    HANDLE handle = nullptr;
    {
        std::lock_guard<std::mutex> lock(handleMutex_);
        handle = static_cast<HANDLE>(handle_);
    }

    if (handle != nullptr) {
        CancelIoEx(handle, nullptr);
    }

    if (readerThread_.joinable() && readerThread_.get_id() != std::this_thread::get_id()) {
        readerThread_.join();
    }

    {
        std::lock_guard<std::mutex> lock(handleMutex_);
        handle = static_cast<HANDLE>(handle_);
        handle_ = nullptr;
    }
    if (handle != nullptr) {
        CloseHandle(handle);
    }
}

bool SerialPort::Write(const std::vector<std::uint8_t>& bytes, std::string& error) {
    error.clear();
    if (bytes.empty()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(handleMutex_);
    HANDLE handle = static_cast<HANDLE>(handle_);
    if (!running_.load() || handle == nullptr) {
        error = "Serial port is not open.";
        return false;
    }

    DWORD written = 0;
    if (!WriteFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)) {
        error = "WriteFile failed: " + Win32ErrorText(GetLastError());
        return false;
    }
    if (written != bytes.size()) {
        error = "Serial write was incomplete.";
        return false;
    }
    return true;
}

bool SerialPort::IsOpen() const {
    if (!running_.load()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(handleMutex_);
    return handle_ != nullptr;
}

void SerialPort::ReaderLoop() {
    std::vector<std::uint8_t> buffer(4096);

    while (running_.load()) {
        HANDLE handle = nullptr;
        {
            std::lock_guard<std::mutex> lock(handleMutex_);
            handle = static_cast<HANDLE>(handle_);
        }
        if (handle == nullptr) {
            break;
        }

        DWORD bytesRead = 0;
        if (!ReadFile(handle, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr)) {
            const DWORD error = GetLastError();
            if (!running_.load() && (error == ERROR_OPERATION_ABORTED || error == ERROR_INVALID_HANDLE)) {
                break;
            }
            running_.store(false);
            ReportError("Serial read failed: " + Win32ErrorText(error));
            break;
        }

        if (bytesRead > 0 && receiveCallback_) {
            try {
                receiveCallback_(std::vector<std::uint8_t>(buffer.begin(), buffer.begin() + bytesRead));
            } catch (...) {
                running_.store(false);
                ReportError("Receive callback failed unexpectedly.");
                break;
            }
        }
    }
}

void SerialPort::ReportError(const std::string& message) const {
    if (errorCallback_) {
        errorCallback_(message);
    }
}

}  // namespace serial
