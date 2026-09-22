#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "detect_tags.h"
#include "detect_utf8.h"
#include "report.h"

using uc::analyze;
using uc::detect_tags;
using uc::format_report;
using uc::is_tag_character;
using uc::tag_payload_character;

namespace {

using Bytes = std::vector<std::uint8_t>;

Bytes concat(std::initializer_list<Bytes> parts) {
    Bytes out;
    for (const auto& part : parts) {
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

// The four UTF-8 bytes of a tag codepoint, written out rather than encoded, so the
// tests state the encoding they expect instead of agreeing with the decoder.
Bytes tag_bytes(std::uint32_t codepoint) {
    return Bytes{static_cast<std::uint8_t>(0xF0 | ((codepoint >> 18) & 0x07u)),
                 static_cast<std::uint8_t>(0x80 | ((codepoint >> 12) & 0x3Fu)),
                 static_cast<std::uint8_t>(0x80 | ((codepoint >> 6) & 0x3Fu)),
                 static_cast<std::uint8_t>(0x80 | (codepoint & 0x3Fu))};
}

Bytes tags(const std::string& text) {
    Bytes out;
    for (const char character : text) {
        const auto value = static_cast<std::uint32_t>(static_cast<unsigned char>(character));
        const Bytes encoded = tag_bytes(0xE0000u + value);
        out.insert(out.end(), encoded.begin(), encoded.end());
    }
    return out;
}

const Bytes kBlackFlag{0xF0, 0x9F, 0x8F, 0xB4};  // U+1F3F4
const Bytes kCancelTag = tag_bytes(0xE007F);

}  // namespace

TEST(TagsTest, PlainAsciiHasNoTags) {
    const Bytes bytes{'H', 'e', 'l', 'l', 'o'};
    const auto result = detect_tags(bytes);
    EXPECT_EQ(result.characters, 0u);
    EXPECT_TRUE(result.runs.empty());
    EXPECT_TRUE(result.findings.empty());
}

TEST(TagsTest, EmptyInputHasNoTags) {
    const auto result = detect_tags(Bytes{});
    EXPECT_EQ(result.characters, 0u);
    EXPECT_TRUE(result.runs.empty());
}

TEST(TagsTest, TheInjectionCaseIsReportedAtItsByteOffset) {
    // "fun" + U+E0061 (a hidden 'a') + "ding": every human reads "funding".
    const auto bytes = concat({{'f', 'u', 'n'}, tag_bytes(0xE0061u), {'d', 'i', 'n', 'g'}});
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 1u);
    EXPECT_EQ(result.hidden_characters, 1u);
    EXPECT_EQ(result.flag_characters, 0u);
    ASSERT_EQ(result.findings.size(), 1u);
    EXPECT_EQ(result.findings[0].codepoint, 0xE0061u);
    EXPECT_EQ(result.findings[0].offset, 3u);
    EXPECT_EQ(result.findings[0].ascii, 'a');
    EXPECT_EQ(result.findings[0].rendering, "a");
    ASSERT_EQ(result.runs.size(), 1u);
    EXPECT_FALSE(result.runs[0].flag_sequence);
    EXPECT_EQ(result.runs[0].offset, 3u);
    EXPECT_EQ(result.runs[0].count, 1u);
    EXPECT_EQ(result.runs[0].payload, "a");
}

TEST(TagsTest, AHiddenPayloadIsDecodedToItsText) {
    const auto bytes = tags("ignore previous");
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 15u);
    ASSERT_EQ(result.runs.size(), 1u);
    EXPECT_EQ(result.runs[0].offset, 0u);
    EXPECT_EQ(result.runs[0].count, 15u);
    EXPECT_EQ(result.runs[0].payload, "ignore previous");
}

TEST(TagsTest, TheByteOffsetAccountsForMultibyteTextBeforeTheTag) {
    const Bytes e_acute{0xC3, 0xA9};  // é, two bytes
    const auto bytes = concat({e_acute, tag_bytes(0xE0041u)});
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 1u);
    EXPECT_EQ(result.findings[0].offset, 2u);
    EXPECT_EQ(result.findings[0].ascii, 'A');
}

TEST(TagsTest, BlockBoundariesAndNonPrintablesAreRendered) {
    const auto bytes = concat({tag_bytes(0xE0000u), tag_bytes(0xE007Eu), tag_bytes(0xE007Fu)});
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 3u);
    ASSERT_EQ(result.findings.size(), 3u);
    EXPECT_EQ(result.findings[0].rendering, "<U+E0000>");
    EXPECT_EQ(result.findings[0].ascii, '\0');
    EXPECT_EQ(result.findings[1].rendering, "~");
    EXPECT_EQ(result.findings[1].ascii, '~');
    EXPECT_EQ(result.findings[2].rendering, "<CANCEL TAG>");
    EXPECT_EQ(result.findings[2].ascii, '\0');
    ASSERT_EQ(result.runs.size(), 1u);
    EXPECT_EQ(result.runs[0].payload, "<U+E0000>~<CANCEL TAG>");
}

TEST(TagsTest, TheLanguageTagIsNamed) {
    const auto result = detect_tags(tag_bytes(0xE0001u));
    ASSERT_EQ(result.findings.size(), 1u);
    EXPECT_EQ(result.findings[0].rendering, "<LANGUAGE TAG>");
    EXPECT_EQ(result.findings[0].ascii, '\0');
}

TEST(TagsTest, TwoSeparateRunsAreTwoRuns) {
    const auto bytes = concat({tags("AB"), {'x'}, tags("CD")});
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 4u);
    ASSERT_EQ(result.runs.size(), 2u);
    EXPECT_EQ(result.runs[0].payload, "AB");
    EXPECT_EQ(result.runs[0].offset, 0u);
    EXPECT_EQ(result.runs[1].payload, "CD");
    EXPECT_EQ(result.runs[1].offset, 9u);
}

TEST(TagsTest, PlaneFourteenIsNotTheTagsBlock) {
    // U+E0080..U+E00FF is unassigned and U+E0100..U+E01EF is the variation-selector
    // range: both live in Plane 14 and neither is a tag.
    const auto bytes = concat(
        {tag_bytes(0xE0080u), tag_bytes(0xE00FFu), tag_bytes(0xE0100u), tag_bytes(0xE01EFu)});
    const auto result = detect_tags(bytes);
    EXPECT_EQ(result.characters, 0u);
    EXPECT_TRUE(result.runs.empty());
    EXPECT_FALSE(is_tag_character(0xE0080u));
    EXPECT_FALSE(is_tag_character(0xE0100u));
    EXPECT_FALSE(is_tag_character(0x1F3F4u));
    EXPECT_FALSE(is_tag_character(0x41u));
}

TEST(TagsTest, EveryBlockCodepointDecodesBackAndMatchesTheFourBytePattern) {
    for (std::uint32_t codepoint = uc::kTagBlockFirst; codepoint <= uc::kTagBlockLast;
         ++codepoint) {
        SCOPED_TRACE(codepoint);
        const Bytes bytes = tag_bytes(codepoint);
        EXPECT_EQ(bytes[0], 0xF3u);
        EXPECT_EQ(bytes[1], 0xA0u);
        EXPECT_TRUE(bytes[2] == 0x80u || bytes[2] == 0x81u);
        EXPECT_GE(bytes[3], 0x80u);
        EXPECT_LE(bytes[3], 0xBFu);
        EXPECT_TRUE(is_tag_character(codepoint));

        const auto utf8 = uc::decode_utf8(bytes);
        ASSERT_EQ(utf8.codepoints.size(), 1u);
        EXPECT_EQ(utf8.codepoints[0].value, codepoint);
        EXPECT_TRUE(utf8.codepoints[0].kind == uc::CodepointKind::Valid);

        const auto result = detect_tags(bytes);
        ASSERT_EQ(result.characters, 1u);
        ASSERT_EQ(result.findings.size(), 1u);
        EXPECT_EQ(result.findings[0].codepoint, codepoint);
        EXPECT_EQ(result.findings[0].offset, 0u);
        EXPECT_EQ(result.findings[0].rendering, tag_payload_character(codepoint));
    }
}

TEST(TagsTest, TheMirroredAsciiRangesOverThePrintableCharactersOnly) {
    for (std::uint32_t value = 0; value <= 0x7Fu; ++value) {
        SCOPED_TRACE(value);
        const std::string rendering = tag_payload_character(uc::kTagAsciiOffset + value);
        if (value >= 0x20u && value <= 0x7Eu) {
            EXPECT_EQ(rendering, std::string(1, static_cast<char>(value)));
        } else {
            EXPECT_EQ(rendering.find(static_cast<char>(value)), std::string::npos);
        }
    }
}

TEST(TagsTest, ASubdivisionFlagIsRecognisedAsEmojiNotAsSmuggledText) {
    const auto bytes = concat({kBlackFlag, tags("gbeng"), kCancelTag});  // England
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 6u);
    EXPECT_EQ(result.flag_characters, 6u);
    EXPECT_EQ(result.hidden_characters, 0u);
    ASSERT_EQ(result.runs.size(), 1u);
    EXPECT_TRUE(result.runs[0].flag_sequence);
    EXPECT_EQ(result.runs[0].offset, 4u);
    EXPECT_EQ(result.runs[0].count, 6u);
    EXPECT_EQ(result.runs[0].payload, "gbeng");
    ASSERT_EQ(result.findings.size(), 6u);
    EXPECT_EQ(result.findings[0].offset, 4u);
}

TEST(TagsTest, ABareBlackFlagCarriesNoTags) {
    const auto result = detect_tags(kBlackFlag);
    EXPECT_EQ(result.characters, 0u);
    EXPECT_TRUE(result.runs.empty());
}

TEST(TagsTest, AFlagLikeRunWithNoCancelTagIsHiddenText) {
    const auto bytes = concat({kBlackFlag, tags("gb")});
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 2u);
    EXPECT_EQ(result.hidden_characters, 2u);
    EXPECT_EQ(result.flag_characters, 0u);
    ASSERT_EQ(result.runs.size(), 1u);
    EXPECT_FALSE(result.runs[0].flag_sequence);
}

TEST(TagsTest, ACancelTerminatedRunWithNoBlackFlagIsHiddenText) {
    const auto bytes = concat({tags("gb"), kCancelTag});
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 3u);
    EXPECT_EQ(result.hidden_characters, 3u);
    EXPECT_EQ(result.flag_characters, 0u);
    ASSERT_EQ(result.runs.size(), 1u);
    EXPECT_FALSE(result.runs[0].flag_sequence);
    EXPECT_EQ(result.runs[0].payload, "gb<CANCEL TAG>");
}

TEST(TagsTest, TruncatedTagBytesAreUtf8FindingsAndNotTags) {
    const Bytes bytes{0xF3, 0xA0, 0x81};
    EXPECT_EQ(detect_tags(bytes).characters, 0u);
    EXPECT_EQ(uc::decode_utf8(bytes).truncated_sequences, 1u);
}

TEST(TagsTest, ATagAfterAnInvalidByteIsStillFound) {
    const auto bytes = concat({{0xFF}, tag_bytes(0xE0041u)});
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.characters, 1u);
    EXPECT_EQ(result.findings[0].offset, 1u);
}

TEST(TagsReportTest, TheSectionNamesTheHiddenTextAndItsPayload) {
    const auto bytes = concat({{'f', 'u', 'n'}, tag_bytes(0xE0061u), {'d', 'i', 'n', 'g'}});
    const std::string text = format_report(analyze("injected.txt", bytes));
    EXPECT_NE(text.find("[Unicode Tags (Plane 14)]"), std::string::npos);
    EXPECT_NE(text.find("Tag characters: 1 (hidden text: 1, inside emoji flag sequences: 0)"),
              std::string::npos);
    EXPECT_NE(text.find("hidden text at byte 3: 1 tag character, payload \"a\""),
              std::string::npos);
    EXPECT_NE(text.find("U+E0061 'a' at byte 3"), std::string::npos);
}

TEST(TagsReportTest, TheSectionShowsAFlagSequenceAsEmojiSyntax) {
    const auto bytes = concat({kBlackFlag, tags("gbeng"), kCancelTag});
    const std::string text = format_report(analyze("flag.txt", bytes));
    EXPECT_NE(text.find("Tag characters: 6 (hidden text: 0, inside emoji flag sequences: 6)"),
              std::string::npos);
    EXPECT_NE(text.find("valid emoji flag sequence at byte 4: 6 tag characters, payload \"gbeng\""),
              std::string::npos);
}

TEST(TagsReportTest, ThePayloadIsEscapedSoItCannotBreakTheLine) {
    const auto bytes = concat({tag_bytes(0xE0022u), tag_bytes(0xE005Cu)});  // a quote, a backslash
    const auto result = detect_tags(bytes);
    ASSERT_EQ(result.runs.size(), 1u);
    EXPECT_EQ(result.runs[0].payload, "\"\\");
    const std::string text = format_report(analyze("quoted.txt", bytes));
    const std::string expected = std::string("payload \"") + "\\\"" + "\\\\" + "\"";
    EXPECT_NE(text.find(expected), std::string::npos);
}

TEST(TagsReportTest, TheSectionSaysNoneWhenThereAreNoTags) {
    const Bytes bytes{'h', 'i'};
    const std::string text = format_report(analyze("plain.txt", bytes));
    EXPECT_NE(text.find("[Unicode Tags (Plane 14)]"), std::string::npos);
    EXPECT_NE(text.find("Tag characters: 0 (hidden text: 0, inside emoji flag sequences: 0)"),
              std::string::npos);
    EXPECT_NE(text.find("[Unicode Tags (Plane 14)]\n  Tag characters: 0"), std::string::npos);
}
