#include "detect_nonchar.h"

namespace uc {

bool is_noncharacter(std::uint32_t codepoint) {
    if (codepoint >= 0xFDD0 && codepoint <= 0xFDEF) {
        return true;
    }
    // The last two codepoints of every plane, U+FFFE/U+FFFF through
    // U+10FFFE/U+10FFFF.
    return codepoint <= 0x10FFFF && (codepoint & 0xFFFEu) == 0xFFFEu;
}

bool is_surrogate(std::uint32_t codepoint) {
    return codepoint >= 0xD800 && codepoint <= 0xDFFF;
}

bool is_c1_control(std::uint32_t codepoint) {
    return codepoint >= 0x0080 && codepoint <= 0x009F;
}

std::vector<NonCharFinding> detect_noncharacters(const std::vector<DecodedCodepoint>& codepoints) {
    std::vector<NonCharFinding> findings;
    for (const DecodedCodepoint& codepoint : codepoints) {
        if (is_surrogate(codepoint.value)) {
            findings.push_back({NonCharKind::LoneSurrogate, codepoint.value, codepoint.offset});
        } else if (is_noncharacter(codepoint.value)) {
            findings.push_back({NonCharKind::Noncharacter, codepoint.value, codepoint.offset});
        } else if (is_c1_control(codepoint.value)) {
            findings.push_back({NonCharKind::C1Control, codepoint.value, codepoint.offset});
        }
    }
    return findings;
}

std::vector<NonCharFinding> detect_noncharacters(const std::vector<std::uint8_t>& bytes) {
    return detect_noncharacters(decode_utf8(bytes).codepoints);
}

const char* nonchar_kind_name(NonCharKind kind) {
    switch (kind) {
        case NonCharKind::LoneSurrogate:
            return "lone surrogate";
        case NonCharKind::C1Control:
            return "C1 control";
        case NonCharKind::Noncharacter:
            break;
    }
    return "noncharacter";
}

}  // namespace uc
