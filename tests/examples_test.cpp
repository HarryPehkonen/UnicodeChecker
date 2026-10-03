#include <gtest/gtest.h>

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "report.h"

#ifndef UC_EXAMPLES_DIR
#error "UC_EXAMPLES_DIR must be defined by CMake (the examples/ directory of this repo)"
#endif

using uc::analyze;
using uc::BomType;
using uc::CodepointKind;
using uc::NonCharKind;
using uc::NulInterleave;
using uc::Report;

namespace {

using Bytes = std::vector<std::uint8_t>;

// The examples are the tool's demo set: hand-written files the reader (or a
// delegate) opens in an editor and runs `unicode_checker <file>` over. They are
// checked here, so the documented behaviour of each one cannot drift away from
// what the tool actually reports -- an example that stops demonstrating what it
// claims is a broken example, and this is where that shows up.
bool load(const std::string& name, Bytes& bytes) {
    std::ifstream file(std::string(UC_EXAMPLES_DIR) + "/" + name, std::ios::binary);
    if (!file) {
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !file.bad();
}

const std::string& missing_message(const std::string& name, std::string& text) {
    text = "cannot read " + std::string(UC_EXAMPLES_DIR) + "/" + name;
    return text;
}

#define UC_LOAD(name, bytes)                                                                       \
    Bytes bytes;                                                                                   \
    std::string failure;                                                                           \
    ASSERT_TRUE(load(name, bytes)) << missing_message(name, failure)

bool has_codepoint(const std::vector<std::uint32_t>& values, std::uint32_t wanted) {
    for (const std::uint32_t value : values) {
        if (value == wanted) {
            return true;
        }
    }
    return false;
}

std::vector<std::uint32_t> invisible_codepoints(const Report& report) {
    std::vector<std::uint32_t> values;
    for (const auto& finding : report.invisible) {
        values.push_back(finding.codepoint);
    }
    return values;
}

std::vector<std::uint32_t> nonchar_codepoints(const Report& report) {
    std::vector<std::uint32_t> values;
    for (const auto& finding : report.noncharacters) {
        values.push_back(finding.codepoint);
    }
    return values;
}

bool has_nonchar_kind(const Report& report, NonCharKind kind) {
    for (const auto& finding : report.noncharacters) {
        if (finding.kind == kind) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST(ExamplesTest, PlainAsciiIsSilent) {
    UC_LOAD("01-plain-ascii.txt", bytes);
    const Report report = analyze("01-plain-ascii.txt", bytes);
    EXPECT_TRUE(report.utf8.valid);
    EXPECT_EQ(report.bom.type, BomType::None);
    EXPECT_TRUE(report.invisible.empty());
    EXPECT_EQ(report.tags.characters, 0u);
    EXPECT_TRUE(report.noncharacters.empty());
    EXPECT_EQ(report.nul.nul_count, 0u);
    EXPECT_FALSE(report.nul.binary_like);
}

TEST(ExamplesTest, Utf8BomIsReportedOnceAndNotAsAnInvisibleCharacter) {
    UC_LOAD("02-bom-utf8.txt", bytes);
    const Report report = analyze("02-bom-utf8.txt", bytes);
    EXPECT_EQ(report.bom.type, BomType::Utf8);
    EXPECT_EQ(report.bom_hex, "EF BB BF");
    EXPECT_TRUE(report.utf8.valid);
    // U+FEFF at offset 0 is the byte order mark, so the invisible detector must
    // stay silent about it -- the rule the README states.
    EXPECT_TRUE(report.invisible.empty());
}

TEST(ExamplesTest, Utf8DefectsAreAllCounted) {
    UC_LOAD("03-utf8-defects.txt", bytes);
    const Report report = analyze("03-utf8-defects.txt", bytes);
    EXPECT_FALSE(report.utf8.valid);
    EXPECT_GE(report.utf8.overlong_sequences, 1u);
    EXPECT_GE(report.utf8.lone_continuation_bytes, 1u);
    EXPECT_GE(report.utf8.invalid_start_bytes, 1u);
    EXPECT_GE(report.utf8.invalid_continuation_bytes, 1u);
    EXPECT_GE(report.utf8.surrogate_codepoints, 1u);
    EXPECT_GE(report.utf8.out_of_range_codepoints, 1u);
    EXPECT_GE(report.utf8.truncated_sequences, 1u);
}

TEST(ExamplesTest, InvisibleCharactersAreReportedAwayFromOffsetZero) {
    UC_LOAD("04-invisible.txt", bytes);
    const Report report = analyze("04-invisible.txt", bytes);
    EXPECT_TRUE(report.utf8.valid);
    EXPECT_EQ(report.bom.type, BomType::None) << "the U+FEFF here sits mid-file, not at offset 0";
    const std::vector<std::uint32_t> found = invisible_codepoints(report);
    EXPECT_TRUE(has_codepoint(found, 0x200Bu)) << "zero width space";
    EXPECT_TRUE(has_codepoint(found, 0x00ADu)) << "soft hyphen";
    EXPECT_TRUE(has_codepoint(found, 0x2060u)) << "word joiner";
    EXPECT_TRUE(has_codepoint(found, 0x202Eu)) << "right-to-left override";
    EXPECT_TRUE(has_codepoint(found, 0xFEFFu)) << "U+FEFF away from offset 0";
    for (const auto& finding : report.invisible) {
        EXPECT_GT(finding.offset, 0u);
    }
}

TEST(ExamplesTest, TheKeywordSplittingCaseIsDecodedBackToItsWord) {
    UC_LOAD("05-tags-injection.txt", bytes);
    const Report report = analyze("05-tags-injection.txt", bytes);
    ASSERT_EQ(report.tags.runs.size(), 2u);
    EXPECT_EQ(report.tags.flag_characters, 0u);
    EXPECT_EQ(report.tags.runs[0].payload, "a") << "\"fun\" + U+E0061 + \"ding\" reads as funding";
    EXPECT_EQ(report.tags.runs[1].payload, "ignore previous instructions");
    EXPECT_EQ(report.tags.hidden_characters,
              1u + std::string("ignore previous instructions").size());
    EXPECT_EQ(report.tags.characters, report.tags.hidden_characters);
}

TEST(ExamplesTest, ASubdivisionFlagAndTheSameLettersAsBareTagsDiffer) {
    UC_LOAD("06-tags-flag-vs-hidden.txt", bytes);
    const Report report = analyze("06-tags-flag-vs-hidden.txt", bytes);
    ASSERT_EQ(report.tags.runs.size(), 2u);
    EXPECT_TRUE(report.tags.runs[0].flag_sequence)
        << "U+1F3F4 + letters + U+E007F is an emoji flag";
    EXPECT_FALSE(report.tags.runs[1].flag_sequence) << "the same letters with no black flag";
    EXPECT_EQ(report.tags.runs[0].payload, "gbeng");
    EXPECT_EQ(report.tags.runs[1].payload, "gbeng");
    EXPECT_EQ(report.tags.flag_characters, 6u);
    EXPECT_EQ(report.tags.hidden_characters, 5u);
}

TEST(ExamplesTest, NoncharactersSurrogatesAndC1ControlsAreAllReported) {
    UC_LOAD("07-noncharacters.txt", bytes);
    const Report report = analyze("07-noncharacters.txt", bytes);
    EXPECT_TRUE(has_nonchar_kind(report, NonCharKind::Noncharacter));
    EXPECT_TRUE(has_nonchar_kind(report, NonCharKind::LoneSurrogate));
    EXPECT_TRUE(has_nonchar_kind(report, NonCharKind::C1Control));
    const std::vector<std::uint32_t> found = nonchar_codepoints(report);
    EXPECT_TRUE(has_codepoint(found, 0xFDD0u));
    EXPECT_TRUE(has_codepoint(found, 0xFFFFu));
    EXPECT_TRUE(has_codepoint(found, 0x0093u));
    EXPECT_TRUE(has_codepoint(found, 0xD800u));
}

TEST(ExamplesTest, Utf16StyleNulsAreRecognisedAsInterleaved) {
    UC_LOAD("08-nul-interleaved.bin", bytes);
    const Report report = analyze("08-nul-interleaved.bin", bytes);
    EXPECT_EQ(report.nul.interleaved, NulInterleave::Utf16LELike);
    EXPECT_GT(report.nul.nul_count, 0u);
    EXPECT_TRUE(report.nul.binary_like);
}

TEST(ExamplesTest, StrayNulsInTextAreCountedButNotCalledBinary) {
    UC_LOAD("09-nul-few.bin", bytes);
    const Report report = analyze("09-nul-few.bin", bytes);
    EXPECT_EQ(report.nul.nul_count, 2u);
    EXPECT_EQ(report.nul.interleaved, NulInterleave::None);
    EXPECT_FALSE(report.nul.binary_like) << "two NULs in a long text file is not a binary file";
    EXPECT_TRUE(report.utf8.valid);
}

TEST(ExamplesTest, TheKitchenSinkFileLightsUpEverySection) {
    UC_LOAD("10-everything-at-once.bin", bytes);
    const Report report = analyze("10-everything-at-once.bin", bytes);
    EXPECT_EQ(report.bom.type, BomType::Utf8);
    EXPECT_FALSE(report.utf8.valid);
    EXPECT_FALSE(report.invisible.empty());
    EXPECT_GT(report.tags.characters, 0u);
    EXPECT_FALSE(report.noncharacters.empty());
    EXPECT_GT(report.nul.nul_count, 0u);
}
