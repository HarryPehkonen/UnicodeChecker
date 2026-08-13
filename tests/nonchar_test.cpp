#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "detect_nonchar.h"

using uc::detect_noncharacters;
using uc::NonCharKind;
using Bytes = std::vector<std::uint8_t>;

TEST(NonCharTest, PlainAsciiHasNoFindings) {
    const Bytes bytes{'H', 'e', 'l', 'l', 'o'};
    EXPECT_TRUE(detect_noncharacters(bytes).empty());
}

TEST(NonCharTest, ArabicPresentationFormsBlockNoncharacterIsReported) {
    const Bytes bytes{0xEF, 0xB7, 0x90};  // U+FDD0
    const auto findings = detect_noncharacters(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].kind, NonCharKind::Noncharacter);
    EXPECT_EQ(findings[0].codepoint, 0xFDD0u);
    EXPECT_EQ(findings[0].offset, 0u);
}

TEST(NonCharTest, FdefIsANoncharacterButFdf0IsNot) {
    const Bytes noncharacter{0xEF, 0xB7, 0xAF};  // U+FDEF
    EXPECT_EQ(detect_noncharacters(noncharacter).size(), 1u);
    const Bytes ordinary{0xEF, 0xB7, 0xB0};  // U+FDF0
    EXPECT_TRUE(detect_noncharacters(ordinary).empty());
}

TEST(NonCharTest, FffeIsReported) {
    const Bytes bytes{0xEF, 0xBF, 0xBE};  // U+FFFE
    const auto findings = detect_noncharacters(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].kind, NonCharKind::Noncharacter);
    EXPECT_EQ(findings[0].codepoint, 0xFFFEu);
}

TEST(NonCharTest, FfffIsReported) {
    const Bytes bytes{0xEF, 0xBF, 0xBF};  // U+FFFF
    const auto findings = detect_noncharacters(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0xFFFFu);
}

TEST(NonCharTest, PlaneOneNoncharacterIsReported) {
    const Bytes bytes{0xF0, 0x9F, 0xBF, 0xBF};  // U+1FFFF
    const auto findings = detect_noncharacters(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0x1FFFFu);
    EXPECT_EQ(findings[0].kind, NonCharKind::Noncharacter);
}

TEST(NonCharTest, LoneSurrogateIsReported) {
    const Bytes bytes{0xED, 0xA0, 0x80};  // U+D800
    const auto findings = detect_noncharacters(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].kind, NonCharKind::LoneSurrogate);
    EXPECT_EQ(findings[0].codepoint, 0xD800u);
}

TEST(NonCharTest, C1ControlIsReported) {
    const Bytes bytes{'a', 0xC2, 0x93};  // U+0093
    const auto findings = detect_noncharacters(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].kind, NonCharKind::C1Control);
    EXPECT_EQ(findings[0].codepoint, 0x0093u);
    EXPECT_EQ(findings[0].offset, 1u);
}

TEST(NonCharTest, C0ControlsAndLatin1LettersAreNotReported) {
    const Bytes bytes{0x07, 0x1B, 0xC3, 0xA9};  // BEL, ESC, é
    EXPECT_TRUE(detect_noncharacters(bytes).empty());
}

TEST(NonCharTest, MultipleFindingsAreAllReportedInOrder) {
    const Bytes bytes{
        0xEF, 0xBF, 0xBF,  // U+FFFF at 0
        'x',               // at 3
        0xC2, 0x93,        // U+0093 at 4
        0xED, 0xA0, 0x80,  // U+D800 at 6
        0xEF, 0xB7, 0x90,  // U+FDD0 at 9
    };
    const auto findings = detect_noncharacters(bytes);
    ASSERT_EQ(findings.size(), 4u);
    EXPECT_EQ(findings[0].offset, 0u);
    EXPECT_EQ(findings[0].kind, NonCharKind::Noncharacter);
    EXPECT_EQ(findings[1].offset, 4u);
    EXPECT_EQ(findings[1].kind, NonCharKind::C1Control);
    EXPECT_EQ(findings[2].offset, 6u);
    EXPECT_EQ(findings[2].kind, NonCharKind::LoneSurrogate);
    EXPECT_EQ(findings[3].offset, 9u);
    EXPECT_EQ(findings[3].kind, NonCharKind::Noncharacter);
}

TEST(NonCharTest, KindNamesAreHumanReadable) {
    EXPECT_STREQ(uc::nonchar_kind_name(NonCharKind::Noncharacter), "noncharacter");
    EXPECT_STREQ(uc::nonchar_kind_name(NonCharKind::LoneSurrogate), "lone surrogate");
    EXPECT_STREQ(uc::nonchar_kind_name(NonCharKind::C1Control), "C1 control");
}
