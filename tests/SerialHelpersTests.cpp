#include "SerialHelpers.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void ExpectTrue(bool condition, const std::string& name) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

template <typename T>
void ExpectEqual(const T& actual, const T& expected, const std::string& name) {
    if (!(actual == expected)) {
        ++failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

void TestParseSpacedHex() {
    std::vector<std::uint8_t> bytes;
    std::string error;
    const bool ok = serial::ParseHex("48 65 6c 6C 6F", bytes, error);
    ExpectTrue(ok, "ParseHex accepts whitespace-separated bytes");
    ExpectEqual(bytes, std::vector<std::uint8_t>({0x48, 0x65, 0x6C, 0x6C, 0x6F}),
                "ParseHex returns expected bytes for spaced input");
}

void TestParseContiguousHex() {
    std::vector<std::uint8_t> bytes;
    std::string error;
    const bool ok = serial::ParseHex("48656C6C6F", bytes, error);
    ExpectTrue(ok, "ParseHex accepts contiguous hex");
    ExpectEqual(bytes, std::vector<std::uint8_t>({0x48, 0x65, 0x6C, 0x6C, 0x6F}),
                "ParseHex returns expected bytes for contiguous input");
}

void TestRejectOddLengthHex() {
    std::vector<std::uint8_t> bytes{0xAA};
    std::string error;
    const bool ok = serial::ParseHex("ABC", bytes, error);
    ExpectTrue(!ok, "ParseHex rejects odd digit count");
    ExpectTrue(!error.empty(), "ParseHex explains odd digit count error");
    ExpectTrue(bytes.empty(), "ParseHex clears output on failure");
}

void TestRejectInvalidHex() {
    std::vector<std::uint8_t> bytes;
    std::string error;
    const bool ok = serial::ParseHex("GG", bytes, error);
    ExpectTrue(!ok, "ParseHex rejects non-hex characters");
    ExpectTrue(!error.empty(), "ParseHex explains invalid-character error");
}

void TestFormatHex() {
    const std::vector<std::uint8_t> bytes{0x00, 0x0A, 0x7F, 0xFF};
    ExpectEqual(serial::FormatHex(bytes), std::string("00 0A 7F FF"),
                "FormatHex uses uppercase two-digit spaced output");
}

void TestLineEndings() {
    {
        std::vector<std::uint8_t> bytes{'A'};
        serial::AppendLineEnding(bytes, serial::LineEnding::None);
        ExpectEqual(bytes, std::vector<std::uint8_t>({'A'}), "None adds no line ending");
    }
    {
        std::vector<std::uint8_t> bytes{'A'};
        serial::AppendLineEnding(bytes, serial::LineEnding::CR);
        ExpectEqual(bytes, std::vector<std::uint8_t>({'A', '\r'}), "CR appends carriage return");
    }
    {
        std::vector<std::uint8_t> bytes{'A'};
        serial::AppendLineEnding(bytes, serial::LineEnding::LF);
        ExpectEqual(bytes, std::vector<std::uint8_t>({'A', '\n'}), "LF appends line feed");
    }
    {
        std::vector<std::uint8_t> bytes{'A'};
        serial::AppendLineEnding(bytes, serial::LineEnding::CRLF);
        ExpectEqual(bytes, std::vector<std::uint8_t>({'A', '\r', '\n'}), "CRLF appends both bytes");
    }
}

}  // namespace

int main() {
    TestParseSpacedHex();
    TestParseContiguousHex();
    TestRejectOddLengthHex();
    TestRejectInvalidHex();
    TestFormatHex();
    TestLineEndings();

    if (failures == 0) {
        std::cout << "All SerialHelpers tests passed.\n";
        return 0;
    }

    std::cerr << failures << " test assertion(s) failed.\n";
    return 1;
}
