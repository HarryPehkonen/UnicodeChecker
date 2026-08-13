#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "detect_nul.h"

using uc::detect_nul;
using uc::NulInterleave;
using Bytes = std::vector<std::uint8_t>;

TEST(NulTest, PlainAsciiHasNoNuls) {
    const Bytes bytes{'H', 'e', 'l', 'l', 'o'};
    const auto result = detect_nul(bytes);
    EXPECT_EQ(result.nul_count, 0u);
    EXPECT_EQ(result.interleaved, NulInterleave::None);
    EXPECT_FALSE(result.binary_like);
    EXPECT_EQ(result.leading_nuls, 0u);
    EXPECT_DOUBLE_EQ(result.nul_ratio, 0.0);
}

TEST(NulTest, EmptyFileHasNoNuls) {
    const auto result = detect_nul(Bytes{});
    EXPECT_EQ(result.total_bytes, 0u);
    EXPECT_EQ(result.nul_count, 0u);
    EXPECT_EQ(result.interleaved, NulInterleave::None);
    EXPECT_FALSE(result.binary_like);
    EXPECT_DOUBLE_EQ(result.nul_ratio, 0.0);
}

TEST(NulTest, TrailingNulsAreInterleavedLittleEndian) {
    const Bytes bytes{'H', 0, 'e', 0, 'l', 0, 'l', 0, 'o', 0};
    const auto result = detect_nul(bytes);
    EXPECT_EQ(result.nul_count, 5u);
    EXPECT_EQ(result.interleaved, NulInterleave::Utf16LELike);
    EXPECT_EQ(result.leading_nuls, 0u);
}

TEST(NulTest, LeadingNulsAreInterleavedBigEndian) {
    const Bytes bytes{0, 'H', 0, 'e', 0, 'l', 0, 'l', 0, 'o'};
    const auto result = detect_nul(bytes);
    EXPECT_EQ(result.nul_count, 5u);
    EXPECT_EQ(result.interleaved, NulInterleave::Utf16BELike);
}

TEST(NulTest, ShortInterleavedRunIsNotEnoughEvidence) {
    const Bytes bytes{'H', 0};
    EXPECT_EQ(detect_nul(bytes).interleaved, NulInterleave::None);
}

TEST(NulTest, IrregularNulsAreNotInterleaved) {
    const Bytes bytes{'H', 0, 0, 'e', 'l', 0, 'l', 'o'};
    EXPECT_EQ(detect_nul(bytes).interleaved, NulInterleave::None);
}

TEST(NulTest, TenPercentNulsIsBinaryLike) {
    Bytes bytes(100, 'a');
    for (std::size_t i = 0; i < 10; ++i) {
        bytes[i * 7 + 3] = 0;
    }
    const auto result = detect_nul(bytes);
    EXPECT_EQ(result.nul_count, 10u);
    EXPECT_TRUE(result.binary_like);
    EXPECT_NEAR(result.nul_ratio, 0.10, 1e-9);
    EXPECT_EQ(result.interleaved, NulInterleave::None);
}

TEST(NulTest, SingleNulInALargeFileIsNotBinaryLike) {
    Bytes bytes(1000, 'a');
    bytes[500] = 0;
    const auto result = detect_nul(bytes);
    EXPECT_EQ(result.nul_count, 1u);
    EXPECT_FALSE(result.binary_like);
}

TEST(NulTest, LeadingNulsAreCounted) {
    const Bytes bytes{0, 0, 0, 'a', 'b', 'c'};
    const auto result = detect_nul(bytes);
    EXPECT_EQ(result.leading_nuls, 3u);
    EXPECT_EQ(result.nul_count, 3u);
    EXPECT_TRUE(result.binary_like);
}

TEST(NulTest, AllNulFileIsBinaryLike) {
    const Bytes bytes(16, 0);
    const auto result = detect_nul(bytes);
    EXPECT_EQ(result.nul_count, 16u);
    EXPECT_EQ(result.leading_nuls, 16u);
    EXPECT_TRUE(result.binary_like);
    EXPECT_EQ(result.interleaved, NulInterleave::None);
}
