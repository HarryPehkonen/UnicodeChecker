#ifndef UNICODE_CHECKER_DETECT_NONCHAR_H
#define UNICODE_CHECKER_DETECT_NONCHAR_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "detect_utf8.h"

namespace uc {

enum class NonCharKind {
    Noncharacter,   // U+FDD0..U+FDEF and U+xFFFE / U+xFFFF in every plane
    LoneSurrogate,  // U+D800..U+DFFF
    C1Control,      // U+0080..U+009F
};

struct NonCharFinding {
    NonCharKind kind = NonCharKind::Noncharacter;
    std::uint32_t codepoint = 0;
    std::size_t offset = 0;  // byte offset of the encoded character
};

std::vector<NonCharFinding> detect_noncharacters(const std::vector<DecodedCodepoint>& codepoints);

// Convenience overload: decodes the bytes first.
std::vector<NonCharFinding> detect_noncharacters(const std::vector<std::uint8_t>& bytes);

const char* nonchar_kind_name(NonCharKind kind);

bool is_noncharacter(std::uint32_t codepoint);
bool is_surrogate(std::uint32_t codepoint);
bool is_c1_control(std::uint32_t codepoint);

}  // namespace uc

#endif  // UNICODE_CHECKER_DETECT_NONCHAR_H
