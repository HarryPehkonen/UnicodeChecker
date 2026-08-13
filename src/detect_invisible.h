#ifndef UNICODE_CHECKER_DETECT_INVISIBLE_H
#define UNICODE_CHECKER_DETECT_INVISIBLE_H

#include <cstddef>
#include <cstdint>
#include <vector>

#include "detect_utf8.h"

namespace uc {

struct InvisibleFinding {
    std::uint32_t codepoint = 0;
    std::size_t offset = 0;   // byte offset of the encoded character
    const char* name = "";    // Unicode character name
};

// Reports zero-width, invisible and bidi-control characters.
// U+FEFF at offset 0 is a byte order mark, not a finding.
std::vector<InvisibleFinding> detect_invisible(const std::vector<DecodedCodepoint>& codepoints);

// Convenience overload: decodes the bytes first.
std::vector<InvisibleFinding> detect_invisible(const std::vector<std::uint8_t>& bytes);

// Name of an invisible codepoint, or nullptr if it is not one of them.
const char* invisible_name(std::uint32_t codepoint);

}  // namespace uc

#endif  // UNICODE_CHECKER_DETECT_INVISIBLE_H
