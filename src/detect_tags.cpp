#include "detect_tags.h"

namespace uc {
namespace {

bool is_printable_ascii(std::uint32_t value) {
    return value >= 0x20u && value <= 0x7Eu;
}

// The block mirrors ASCII directly: U+E0000 carries 0x00, U+E0041 carries 'A'.
std::uint32_t ascii_value(std::uint32_t codepoint) {
    return codepoint - kTagAsciiOffset;
}

}  // namespace

bool is_tag_character(std::uint32_t codepoint) {
    return codepoint >= kTagBlockFirst && codepoint <= kTagBlockLast;
}

std::string tag_payload_character(std::uint32_t codepoint) {
    if (!is_tag_character(codepoint)) {
        return std::string();
    }
    const std::uint32_t value = ascii_value(codepoint);
    if (is_printable_ascii(value)) {
        return std::string(1, static_cast<char>(value));
    }
    if (codepoint == 0xE0001u) {
        return "<LANGUAGE TAG>";
    }
    if (codepoint == kCancelTag) {
        return "<CANCEL TAG>";
    }
    // U+E0000, the reserved U+E0002..U+E001F, and anything else that mirrors no
    // printable character: name the codepoint rather than writing a control byte.
    return "<" + format_codepoint(codepoint) + ">";
}

TagsResult detect_tags(const std::vector<DecodedCodepoint>& codepoints) {
    TagsResult result;

    std::size_t index = 0;
    while (index < codepoints.size()) {
        if (!is_tag_character(codepoints[index].value)) {
            ++index;
            continue;
        }

        // A run is as long as the tag characters are contiguous. Contiguity is the
        // decoder's codepoint order; nothing else has to sit between them.
        const std::size_t first = index;
        while (index < codepoints.size() && is_tag_character(codepoints[index].value)) {
            TagFinding finding;
            finding.codepoint = codepoints[index].value;
            finding.offset = codepoints[index].offset;
            const std::uint32_t value = ascii_value(finding.codepoint);
            finding.ascii = is_printable_ascii(value) ? static_cast<char>(value) : '\0';
            finding.rendering = tag_payload_character(finding.codepoint);
            result.findings.push_back(finding);
            ++index;
        }
        const std::size_t last = index - 1;

        TagRun run;
        run.offset = codepoints[first].offset;
        run.count = last - first + 1;
        // A subdivision flag emoji is a tag run directly after U+1F3F4 BLACK FLAG and
        // closed by U+E007F CANCEL TAG (England is U+1F3F4 "gbeng" U+E007F). Either
        // half missing and the run is hidden text.
        run.flag_sequence = first > 0 && codepoints[first - 1].value == kBlackFlag &&
                            codepoints[last].value == kCancelTag;
        // A hidden run carries every character it holds, the terminator included; a
        // flag carries only the letters between the black flag and its terminator.
        const std::size_t payload_end = run.flag_sequence ? last : index;
        for (std::size_t position = first; position < payload_end; ++position) {
            run.payload += tag_payload_character(codepoints[position].value);
        }
        result.runs.push_back(run);

        result.characters += run.count;
        if (run.flag_sequence) {
            result.flag_characters += run.count;
        } else {
            result.hidden_characters += run.count;
        }
    }

    return result;
}

TagsResult detect_tags(const std::vector<std::uint8_t>& bytes) {
    return detect_tags(decode_utf8(bytes).codepoints);
}

}  // namespace uc
