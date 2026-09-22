#ifndef UNICODE_CHECKER_REPORT_H
#define UNICODE_CHECKER_REPORT_H

#include <cstdint>
#include <string>
#include <vector>

#include "detect_bom.h"
#include "detect_invisible.h"
#include "detect_nonchar.h"
#include "detect_nul.h"
#include "detect_tags.h"
#include "detect_utf8.h"

namespace uc {

// Everything the detectors found about one file. The tool reports evidence;
// it deliberately does not conclude what the file's encoding "really" is.
struct Report {
    std::string filename;
    std::size_t size = 0;
    BomResult bom;
    std::string bom_hex;
    Utf8Result utf8;
    std::vector<InvisibleFinding> invisible;
    TagsResult tags;
    std::vector<NonCharFinding> noncharacters;
    NulResult nul;
};

// Runs every detector over the buffer. The UTF-8 pass runs first because the
// invisible and noncharacter detectors work on decoded codepoints.
Report analyze(const std::string& filename, const std::vector<std::uint8_t>& bytes);

// Renders the report as plain text, one finding per line.
std::string format_report(const Report& report);

}  // namespace uc

#endif  // UNICODE_CHECKER_REPORT_H
