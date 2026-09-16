#include "SerialHelpers.h"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace serial {
namespace {

int HexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

}  // namespace

bool ParseHex(std::string_view text, std::vector<std::uint8_t>& output, std::string& error) {
    output.clear();
    error.clear();

    std::string digits;
    digits.reserve(text.size());
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        if (HexValue(c) < 0) {
            error = "HEX input contains a non-hex character.";
            return false;
        }
        digits.push_back(c);
    }

    if (digits.size() % 2 != 0) {
        error = "HEX input must contain an even number of digits.";
        return false;
    }

    output.reserve(digits.size() / 2);
    for (std::size_t i = 0; i < digits.size(); i += 2) {
        const int high = HexValue(digits[i]);
        const int low = HexValue(digits[i + 1]);
        output.push_back(static_cast<std::uint8_t>((high << 4) | low));
    }
    return true;
}

std::string FormatHex(const std::vector<std::uint8_t>& bytes) {
    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i != 0) {
            stream << ' ';
        }
        stream << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    }
    return stream.str();
}

void AppendLineEnding(std::vector<std::uint8_t>& bytes, LineEnding ending) {
    switch (ending) {
        case LineEnding::None:
            break;
        case LineEnding::CR:
            bytes.push_back('\r');
            break;
        case LineEnding::LF:
            bytes.push_back('\n');
            break;
        case LineEnding::CRLF:
            bytes.push_back('\r');
            bytes.push_back('\n');
            break;
    }
}

}  // namespace serial
