#ifndef UNICODE_CHECKER_DETECT_TAGS_H
#define UNICODE_CHECKER_DETECT_TAGS_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "detect_utf8.h"

namespace uc {

// The Unicode Tags block, U+E0000..U+E007F: the deprecated in-band language tags.
// Every one of them is default-ignorable -- no glyph, no advance width -- and each
// mirrors an ASCII character (U+E00XX carries 0xXX), so a run of them is a second
// text that a reader never sees and a parser reads in full.
//
// The block is also legitimate: U+1F3F4 (black flag) followed by tag letters and
// closed by U+E007F is a regional-subdivision flag emoji (England, Scotland, ...),
// not smuggled text.
inline constexpr std::uint32_t kTagBlockFirst = 0xE0000;
inline constexpr std::uint32_t kTagBlockLast = 0xE007F;
inline constexpr std::uint32_t kTagAsciiOffset = 0xE0000;  // codepoint - offset is the ASCII value
inline constexpr std::uint32_t kBlackFlag = 0x1F3F4;       // the base of a subdivision flag
inline constexpr std::uint32_t kCancelTag = 0xE007F;       // closes a subdivision flag

struct TagFinding {
    std::uint32_t codepoint = 0;
    std::size_t offset = 0;  // byte offset of the tag character's lead byte
    char ascii = '\0';       // the mirrored ASCII character, '\0' when it mirrors none printable
    std::string rendering;   // how to show it: "a", "<LANGUAGE TAG>", "<CANCEL TAG>", "<U+E0012>"
};

// One contiguous run of tag characters: hidden text, or one flag sequence.
struct TagRun {
    std::size_t offset = 0;      // byte offset of the run's first tag character
    std::size_t count = 0;       // tag characters in the run
    bool flag_sequence = false;  // true when U+1F3F4 ... U+E007F makes it a subdivision flag
    // The text the run carries. A hidden run mirrors every tag character it holds
    // ("funding", with "<CANCEL TAG>" where one appears); a flag sequence carries the
    // subdivision letters only ("gbeng"), because its closing U+E007F terminates the
    // sequence rather than adding a letter to it.
    std::string payload;
};

struct TagsResult {
    std::size_t characters = 0;         // every tag character found
    std::size_t hidden_characters = 0;  // the ones that are not part of a flag sequence
    std::size_t flag_characters = 0;    // the ones that are
    std::vector<TagRun> runs;
    std::vector<TagFinding> findings;
};

// Reports the block's characters: every one, plus each run and the text it carries.
TagsResult detect_tags(const std::vector<DecodedCodepoint>& codepoints);

// Convenience overload: decodes the bytes first.
TagsResult detect_tags(const std::vector<std::uint8_t>& bytes);

// U+E0000..U+E007F, and nothing else in Plane 14: U+E0100..U+E01EF is the
// variation-selector range and U+E0080..U+E00FF is unassigned.
bool is_tag_character(std::uint32_t codepoint);

// A tag codepoint as the text it carries: the mirrored character when it is
// printable, otherwise "<LANGUAGE TAG>", "<CANCEL TAG>", or "<U+Exxxx>".
std::string tag_payload_character(std::uint32_t codepoint);

}  // namespace uc

#endif  // UNICODE_CHECKER_DETECT_TAGS_H
