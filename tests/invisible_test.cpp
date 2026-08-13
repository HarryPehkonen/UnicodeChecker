#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "detect_invisible.h"

using uc::detect_invisible;
using Bytes = std::vector<std::uint8_t>;

namespace {

Bytes concat(std::initializer_list<Bytes> parts) {
    Bytes out;
    for (const auto& part : parts) {
        out.insert(out.end(), part.begin(), part.end());
    }
    return out;
}

const Bytes kZwsp{0xE2, 0x80, 0x8B};        // U+200B
const Bytes kZwnj{0xE2, 0x80, 0x8C};        // U+200C
const Bytes kWordJoiner{0xE2, 0x81, 0xA0};  // U+2060
const Bytes kLre{0xE2, 0x80, 0xAA};         // U+202A
const Bytes kRlo{0xE2, 0x80, 0xAE};         // U+202E
const Bytes kSoftHyphen{0xC2, 0xAD};        // U+00AD
const Bytes kFeff{0xEF, 0xBB, 0xBF};        // U+FEFF
const Bytes kLrm{0xE2, 0x80, 0x8E};         // U+200E

}  // namespace

TEST(InvisibleTest, PlainAsciiHasNoFindings) {
    const Bytes bytes{'H', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
    EXPECT_TRUE(detect_invisible(bytes).empty());
}

TEST(InvisibleTest, ZeroWidthSpaceIsReportedWithPosition) {
    const auto bytes = concat({{'a', 'b'}, kZwsp, {'c'}});
    const auto findings = detect_invisible(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0x200Bu);
    EXPECT_EQ(findings[0].offset, 2u);
    EXPECT_EQ(std::string(findings[0].name), "Zero Width Space");
}

TEST(InvisibleTest, ZeroWidthNonJoinerIsReported) {
    const auto findings = detect_invisible(kZwnj);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0x200Cu);
}

TEST(InvisibleTest, WordJoinerIsReported) {
    const auto findings = detect_invisible(kWordJoiner);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0x2060u);
}

TEST(InvisibleTest, InvisibleOperatorsAreReported) {
    const Bytes bytes{0xE2, 0x81, 0xA1, 0xE2, 0x81, 0xA4};  // U+2061, U+2064
    const auto findings = detect_invisible(bytes);
    ASSERT_EQ(findings.size(), 2u);
    EXPECT_EQ(findings[0].codepoint, 0x2061u);
    EXPECT_EQ(findings[1].codepoint, 0x2064u);
}

TEST(InvisibleTest, BidiOverrideIsReported) {
    const auto bytes = concat({{'a'}, kLre, {'b'}, kRlo});
    const auto findings = detect_invisible(bytes);
    ASSERT_EQ(findings.size(), 2u);
    EXPECT_EQ(findings[0].codepoint, 0x202Au);
    EXPECT_EQ(findings[0].offset, 1u);
    EXPECT_EQ(findings[1].codepoint, 0x202Eu);
    EXPECT_EQ(findings[1].offset, 5u);
    EXPECT_EQ(std::string(findings[1].name), "Right-to-Left Override");
}

TEST(InvisibleTest, DirectionalMarksAreReported) {
    const auto findings = detect_invisible(kLrm);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0x200Eu);
    EXPECT_EQ(std::string(findings[0].name), "Left-to-Right Mark");
}

TEST(InvisibleTest, SoftHyphenIsReported) {
    const auto bytes = concat({{'c', 'o'}, kSoftHyphen, {'o', 'p'}});
    const auto findings = detect_invisible(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0x00ADu);
    EXPECT_EQ(findings[0].offset, 2u);
    EXPECT_EQ(std::string(findings[0].name), "Soft Hyphen");
}

TEST(InvisibleTest, FeffAtStartIsTreatedAsBomAndNotReported) {
    const auto bytes = concat({kFeff, {'h', 'i'}});
    EXPECT_TRUE(detect_invisible(bytes).empty());
}

TEST(InvisibleTest, FeffAwayFromStartIsReported) {
    const auto bytes = concat({{'h', 'i'}, kFeff, {'!'}});
    const auto findings = detect_invisible(bytes);
    ASSERT_EQ(findings.size(), 1u);
    EXPECT_EQ(findings[0].codepoint, 0xFEFFu);
    EXPECT_EQ(findings[0].offset, 2u);
    EXPECT_EQ(std::string(findings[0].name), "Zero Width No-Break Space");
}

TEST(InvisibleTest, MultipleInvisibleCharactersAreAllReported) {
    const auto bytes = concat({kZwsp, {'a'}, kSoftHyphen, {'b'}, kRlo, kFeff});
    const auto findings = detect_invisible(bytes);
    ASSERT_EQ(findings.size(), 4u);
    EXPECT_EQ(findings[0].codepoint, 0x200Bu);
    EXPECT_EQ(findings[1].codepoint, 0x00ADu);
    EXPECT_EQ(findings[2].codepoint, 0x202Eu);
    EXPECT_EQ(findings[3].codepoint, 0xFEFFu);
    EXPECT_EQ(findings[3].offset, 10u);
}

TEST(InvisibleTest, VisibleMultibyteTextIsNotReported) {
    const Bytes bytes{0xE2, 0x82, 0xAC, 0xC3, 0xA9};  // € é
    EXPECT_TRUE(detect_invisible(bytes).empty());
}
