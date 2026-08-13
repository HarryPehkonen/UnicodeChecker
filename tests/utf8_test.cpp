#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "detect_utf8.h"

using uc::CodepointKind;
using uc::decode_utf8;
using Bytes = std::vector<std::uint8_t>;

TEST(Utf8Test, EmptyInputIsTriviallyValid) {
    const auto result = decode_utf8(Bytes{});
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.codepoints.empty());
}

TEST(Utf8Test, AsciiIsValid) {
    const Bytes bytes{'H', 'e', 'l', 'l', 'o'};
    const auto result = decode_utf8(bytes);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.overlong_sequences, 0u);
    EXPECT_EQ(result.surrogate_codepoints, 0u);
    EXPECT_EQ(result.out_of_range_codepoints, 0u);
    EXPECT_EQ(result.invalid_continuation_bytes, 0u);
    EXPECT_EQ(result.lone_continuation_bytes, 0u);
    ASSERT_EQ(result.codepoints.size(), 5u);
    EXPECT_EQ(result.codepoints[0].value, 0x48u);
    EXPECT_EQ(result.codepoints[4].offset, 4u);
}

TEST(Utf8Test, TwoByteSequenceIsValid) {
    const Bytes bytes{0xC3, 0xA9};  // U+00E9 é
    const auto result = decode_utf8(bytes);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.codepoints.size(), 1u);
    EXPECT_EQ(result.codepoints[0].value, 0xE9u);
    EXPECT_EQ(result.codepoints[0].length, 2u);
    EXPECT_EQ(result.codepoints[0].kind, CodepointKind::Valid);
}

TEST(Utf8Test, ThreeByteSequenceIsValid) {
    const Bytes bytes{0xE2, 0x82, 0xAC};  // U+20AC €
    const auto result = decode_utf8(bytes);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.codepoints.size(), 1u);
    EXPECT_EQ(result.codepoints[0].value, 0x20ACu);
    EXPECT_EQ(result.codepoints[0].length, 3u);
}

TEST(Utf8Test, FourByteSequenceIsValid) {
    const Bytes bytes{0xF0, 0x9F, 0x92, 0xA9};  // U+1F4A9
    const auto result = decode_utf8(bytes);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.codepoints.size(), 1u);
    EXPECT_EQ(result.codepoints[0].value, 0x1F4A9u);
    EXPECT_EQ(result.codepoints[0].length, 4u);
}

TEST(Utf8Test, MixedMultibyteTextIsValid) {
    const Bytes bytes{'a', 0xC3, 0xA9, 'b', 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x92, 0xA9};
    const auto result = decode_utf8(bytes);
    EXPECT_TRUE(result.valid);
    ASSERT_EQ(result.codepoints.size(), 5u);
    EXPECT_EQ(result.codepoints[4].offset, 7u);
}

TEST(Utf8Test, OverlongSlashIsFlagged) {
    const Bytes bytes{0xC0, 0xAF};  // overlong '/'
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.overlong_sequences, 1u);
    ASSERT_EQ(result.codepoints.size(), 1u);
    EXPECT_EQ(result.codepoints[0].value, 0x2Fu);
    EXPECT_EQ(result.codepoints[0].kind, CodepointKind::Overlong);
}

TEST(Utf8Test, OverlongTwoByteC1IsFlagged) {
    const Bytes bytes{0xC1, 0xBF};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.overlong_sequences, 1u);
}

TEST(Utf8Test, OverlongThreeByteNulIsFlagged) {
    const Bytes bytes{0xE0, 0x80, 0x80};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.overlong_sequences, 1u);
    EXPECT_EQ(result.surrogate_codepoints, 0u);
}

TEST(Utf8Test, SurrogateEncodedAsUtf8IsFlagged) {
    const Bytes bytes{0xED, 0xA0, 0x80};  // U+D800
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.surrogate_codepoints, 1u);
    EXPECT_EQ(result.overlong_sequences, 0u);
    ASSERT_EQ(result.codepoints.size(), 1u);
    EXPECT_EQ(result.codepoints[0].value, 0xD800u);
    EXPECT_EQ(result.codepoints[0].kind, CodepointKind::Surrogate);
}

TEST(Utf8Test, ThreeByteEdSequenceBelowSurrogatesStaysValid) {
    const Bytes bytes{0xED, 0x9F, 0xBF};  // U+D7FF
    const auto result = decode_utf8(bytes);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.surrogate_codepoints, 0u);
}

TEST(Utf8Test, CodepointAboveMaxIsFlagged) {
    const Bytes bytes{0xF4, 0x90, 0x80, 0x80};  // U+110000
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.out_of_range_codepoints, 1u);
    ASSERT_EQ(result.codepoints.size(), 1u);
    EXPECT_EQ(result.codepoints[0].value, 0x110000u);
    EXPECT_EQ(result.codepoints[0].kind, CodepointKind::OutOfRange);
}

TEST(Utf8Test, LeadByteAboveF4IsOutOfRange) {
    const Bytes bytes{0xF5, 0x80, 0x80, 0x80};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.out_of_range_codepoints, 1u);
}

TEST(Utf8Test, MaxCodepointIsValid) {
    const Bytes bytes{0xF4, 0x8F, 0xBF, 0xBF};  // U+10FFFF
    const auto result = decode_utf8(bytes);
    EXPECT_TRUE(result.valid);
    EXPECT_EQ(result.out_of_range_codepoints, 0u);
}

TEST(Utf8Test, LoneContinuationByteIsFlagged) {
    const Bytes bytes{0x80};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.lone_continuation_bytes, 1u);
    EXPECT_TRUE(result.codepoints.empty());
}

TEST(Utf8Test, MissingContinuationIsTruncated) {
    const Bytes bytes{0xC3};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.truncated_sequences, 1u);
}

TEST(Utf8Test, TruncatedFourByteSequenceIsFlaggedOnce) {
    const Bytes bytes{0xF0, 0x9F, 0x92};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.truncated_sequences, 1u);
}

TEST(Utf8Test, InvalidContinuationByteIsFlagged) {
    const Bytes bytes{0xC3, 0x01};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.invalid_continuation_bytes, 1u);
    // The offending byte is re-examined as a start byte, so the ASCII
    // control character is still decoded.
    ASSERT_EQ(result.codepoints.size(), 1u);
    EXPECT_EQ(result.codepoints[0].value, 0x01u);
    EXPECT_EQ(result.codepoints[0].offset, 1u);
}

TEST(Utf8Test, InvalidStartByteIsFlagged) {
    const Bytes bytes{0xFE, 0xFF};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.invalid_start_bytes, 2u);
}

TEST(Utf8Test, DecodingResumesAfterAnError) {
    const Bytes bytes{0x80, 'o', 'k'};
    const auto result = decode_utf8(bytes);
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.lone_continuation_bytes, 1u);
    ASSERT_EQ(result.codepoints.size(), 2u);
    EXPECT_EQ(result.codepoints[0].value, 'o');
    EXPECT_EQ(result.codepoints[0].offset, 1u);
}
