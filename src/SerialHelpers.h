#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace serial {

enum class LineEnding {
    None,
    CR,
    LF,
    CRLF,
};

bool ParseHex(std::string_view text, std::vector<std::uint8_t>& output, std::string& error);
std::string FormatHex(const std::vector<std::uint8_t>& bytes);
void AppendLineEnding(std::vector<std::uint8_t>& bytes, LineEnding ending);

}  // namespace serial
