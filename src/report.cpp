#include "report.h"

#include <iomanip>
#include <sstream>

namespace uc {
namespace {

std::string percent(double ratio) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(2) << ratio * 100.0 << '%';
    return out.str();
}

void append_count(std::ostringstream& out, const char* label, std::size_t count) {
    out << "  " << label << ": " << count << '\n';
}

// A payload is text smuggled inside tag characters, so it is quoted: escaping the
// quote and the backslash keeps one run on one line.
std::string quoted(const std::string& payload) {
    std::string out;
    for (const char character : payload) {
        if (character == '"' || character == '\\') {
            out += '\\';
        }
        out += character;
    }
    return out;
}

std::string tag_character_count(std::size_t count) {
    return std::to_string(count) + (count == 1 ? " tag character" : " tag characters");
}

}  // namespace

Report analyze(const std::string& filename, const std::vector<std::uint8_t>& bytes) {
    Report report;
    report.filename = filename;
    report.size = bytes.size();
    report.bom = detect_bom(bytes);
    report.bom_hex = bom_hex(bytes, report.bom);
    report.utf8 = decode_utf8(bytes);
    report.invisible = detect_invisible(report.utf8.codepoints);
    report.tags = detect_tags(report.utf8.codepoints);
    report.noncharacters = detect_noncharacters(report.utf8.codepoints);
    report.nul = detect_nul(bytes);
    return report;
}

std::string format_report(const Report& report) {
    std::ostringstream out;
    out << "file: " << report.filename << '\n';
    out << "size: " << report.size << " bytes\n";

    out << "\n[BOM]\n";
    if (report.bom.type == BomType::None) {
        out << "  No BOM detected\n";
    } else {
        out << "  " << bom_name(report.bom.type) << " BOM detected (" << report.bom_hex << ")\n";
    }

    out << "\n[UTF-8]\n";
    out << "  Valid UTF-8: " << (report.utf8.valid ? "yes" : "no") << '\n';
    append_count(out, "Overlong sequences", report.utf8.overlong_sequences);
    append_count(out, "Surrogate codepoints", report.utf8.surrogate_codepoints);
    append_count(out, "Codepoints > U+10FFFF", report.utf8.out_of_range_codepoints);
    append_count(out, "Invalid continuation bytes", report.utf8.invalid_continuation_bytes);
    append_count(out, "Lone continuation bytes", report.utf8.lone_continuation_bytes);
    append_count(out, "Invalid start bytes", report.utf8.invalid_start_bytes);
    append_count(out, "Truncated sequences", report.utf8.truncated_sequences);

    out << "\n[Invisible / Zero-Width / Bidi]\n";
    if (report.invisible.empty()) {
        out << "  none\n";
    }
    for (const InvisibleFinding& finding : report.invisible) {
        out << "  " << format_codepoint(finding.codepoint) << ' ' << finding.name << " at byte "
            << finding.offset << '\n';
    }

    out << "\n[Unicode Tags (Plane 14)]\n";
    out << "  Tag characters: " << report.tags.characters
        << " (hidden text: " << report.tags.hidden_characters
        << ", inside emoji flag sequences: " << report.tags.flag_characters << ")\n";
    if (report.tags.runs.empty()) {
        out << "  none\n";
    }
    for (const TagRun& run : report.tags.runs) {
        out << "  ";
        if (run.flag_sequence) {
            out << "valid emoji flag sequence at byte " << run.offset << ": "
                << tag_character_count(run.count);
        } else {
            out << "hidden text at byte " << run.offset << ": " << tag_character_count(run.count);
        }
        out << ", payload \"" << quoted(run.payload) << "\"\n";
    }
    for (const TagFinding& finding : report.tags.findings) {
        out << "  " << format_codepoint(finding.codepoint);
        if (finding.ascii != '\0') {
            out << " '" << finding.ascii << '\'';
        } else {
            out << ' ' << finding.rendering;
        }
        out << " at byte " << finding.offset << '\n';
    }

    out << "\n[Noncharacters / Controls]\n";
    if (report.noncharacters.empty()) {
        out << "  none\n";
    }
    for (const NonCharFinding& finding : report.noncharacters) {
        out << "  " << nonchar_kind_name(finding.kind) << ' ' << format_codepoint(finding.codepoint)
            << " at byte " << finding.offset << '\n';
    }

    out << "\n[NUL / Binary]\n";
    out << "  NUL bytes: " << report.nul.nul_count << " (" << percent(report.nul.nul_ratio)
        << ")\n";
    out << "  Interleaved NULs: " << nul_interleave_name(report.nul.interleaved) << '\n';
    out << "  Leading NUL bytes: " << report.nul.leading_nuls << '\n';
    out << "  Binary-like: " << (report.nul.binary_like ? "yes" : "no") << '\n';

    return out.str();
}

}  // namespace uc
