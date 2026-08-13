#include "detect_invisible.h"

namespace uc {

const char* invisible_name(std::uint32_t codepoint) {
    switch (codepoint) {
        case 0x00AD:
            return "Soft Hyphen";
        case 0x200B:
            return "Zero Width Space";
        case 0x200C:
            return "Zero Width Non-Joiner";
        case 0x200D:
            return "Zero Width Joiner";
        case 0x200E:
            return "Left-to-Right Mark";
        case 0x200F:
            return "Right-to-Left Mark";
        case 0x202A:
            return "Left-to-Right Embedding";
        case 0x202B:
            return "Right-to-Left Embedding";
        case 0x202C:
            return "Pop Directional Formatting";
        case 0x202D:
            return "Left-to-Right Override";
        case 0x202E:
            return "Right-to-Left Override";
        case 0x2060:
            return "Word Joiner";
        case 0x2061:
            return "Function Application";
        case 0x2062:
            return "Invisible Times";
        case 0x2063:
            return "Invisible Separator";
        case 0x2064:
            return "Invisible Plus";
        case 0xFEFF:
            return "Zero Width No-Break Space";
        default:
            return nullptr;
    }
}

std::vector<InvisibleFinding> detect_invisible(const std::vector<DecodedCodepoint>& codepoints) {
    std::vector<InvisibleFinding> findings;
    for (const DecodedCodepoint& codepoint : codepoints) {
        if (codepoint.value == 0xFEFF && codepoint.offset == 0) {
            continue;  // a byte order mark, reported by the BOM detector
        }
        if (const char* name = invisible_name(codepoint.value)) {
            findings.push_back({codepoint.value, codepoint.offset, name});
        }
    }
    return findings;
}

std::vector<InvisibleFinding> detect_invisible(const std::vector<std::uint8_t>& bytes) {
    return detect_invisible(decode_utf8(bytes).codepoints);
}

}  // namespace uc
