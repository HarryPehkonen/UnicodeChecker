#ifndef UNICODE_CHECKER_DETECT_UTF8_H
#define UNICODE_CHECKER_DETECT_UTF8_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace uc {

// How a decoded codepoint relates to the UTF-8 well-formedness rules.
enum class CodepointKind {
    Valid,
    Overlong,    // encoded in more bytes than necessary (e.g. C0 AF for '/')
    Surrogate,   // U+D800..U+DFFF, never valid in UTF-8
    OutOfRange,  // above U+10FFFF
};

struct DecodedCodepoint {
    std::uint32_t value = 0;
    std::size_t offset = 0;  // byte offset of the sequence's lead byte
    std::size_t length = 0;  // number of bytes the sequence occupies
    CodepointKind kind = CodepointKind::Valid;
};

struct Utf8Result {
    bool valid = true;
    std::size_t overlong_sequences = 0;
    std::size_t surrogate_codepoints = 0;
    std::size_t out_of_range_codepoints = 0;
    std::size_t invalid_continuation_bytes = 0;
    std::size_t lone_continuation_bytes = 0;
    std::size_t invalid_start_bytes = 0;   // F8..FF, which lead nothing
    std::size_t truncated_sequences = 0;   // input ended mid-sequence
    // Every sequence whose shape decoded, including the ill-formed ones, so
    // that later detectors can inspect what an attacker tried to smuggle.
    std::vector<DecodedCodepoint> codepoints;
};

// Runs the UTF-8 state machine over the buffer, decoding codepoints and
// counting each class of defect. Never throws; always consumes all input.
Utf8Result decode_utf8(const std::vector<std::uint8_t>& bytes);

// Formats a codepoint the usual way, e.g. "U+200B".
std::string format_codepoint(std::uint32_t value);

}  // namespace uc

#endif  // UNICODE_CHECKER_DETECT_UTF8_H
