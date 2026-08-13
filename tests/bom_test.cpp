#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "detect_bom.h"

using uc::BomType;
using uc::detect_bom;
using Bytes = std::vector<std::uint8_t>;

TEST(BomTest, Utf8BomIsDetected) {
    const Bytes bytes{0xEF, 0xBB, 0xBF, 'h', 'i'};
    const auto result = detect_bom(bytes);
    EXPECT_EQ(result.type, BomType::Utf8);
    EXPECT_EQ(result.length, 3u);
}

TEST(BomTest, Utf16BeBomIsDetected) {
    const Bytes bytes{0xFE, 0xFF, 0x00, 'h'};
    const auto result = detect_bom(bytes);
    EXPECT_EQ(result.type, BomType::Utf16BE);
    EXPECT_EQ(result.length, 2u);
}

TEST(BomTest, Utf16LeBomIsDetectedAndNotConfusedWithUtf32) {
    const Bytes bytes{0xFF, 0xFE, 'h', 0x00};
    const auto result = detect_bom(bytes);
    EXPECT_EQ(result.type, BomType::Utf16LE);
    EXPECT_EQ(result.length, 2u);
}

TEST(BomTest, Utf16LeBomWithOnlyTwoBytesIsUtf16Le) {
    const Bytes bytes{0xFF, 0xFE};
    EXPECT_EQ(detect_bom(bytes).type, BomType::Utf16LE);
}

TEST(BomTest, Utf32LeBomIsDetected) {
    const Bytes bytes{0xFF, 0xFE, 0x00, 0x00, 'h', 0x00, 0x00, 0x00};
    const auto result = detect_bom(bytes);
    EXPECT_EQ(result.type, BomType::Utf32LE);
    EXPECT_EQ(result.length, 4u);
}

TEST(BomTest, Utf32BeBomIsDetected) {
    const Bytes bytes{0x00, 0x00, 0xFE, 0xFF, 0x00, 0x00, 0x00, 'h'};
    const auto result = detect_bom(bytes);
    EXPECT_EQ(result.type, BomType::Utf32BE);
    EXPECT_EQ(result.length, 4u);
}

TEST(BomTest, Utf7BomVariantsAreDetected) {
    for (const std::uint8_t last : {0x38, 0x39, 0x2B, 0x2F}) {
        const Bytes bytes{0x2B, 0x2F, 0x76, last};
        const auto result = detect_bom(bytes);
        EXPECT_EQ(result.type, BomType::Utf7) << "last byte: " << int(last);
        EXPECT_EQ(result.length, 4u);
    }
}

TEST(BomTest, Utf7PrefixWithoutValidFinalByteIsNotABom) {
    const Bytes bytes{0x2B, 0x2F, 0x76, 0x41};
    EXPECT_EQ(detect_bom(bytes).type, BomType::None);
}

TEST(BomTest, EmptyFileHasNoBom) {
    const Bytes bytes{};
    const auto result = detect_bom(bytes);
    EXPECT_EQ(result.type, BomType::None);
    EXPECT_EQ(result.length, 0u);
}

TEST(BomTest, PlainAsciiHasNoBom) {
    const Bytes bytes{'H', 'e', 'l', 'l', 'o'};
    EXPECT_EQ(detect_bom(bytes).type, BomType::None);
}

TEST(BomTest, BomBytesAwayFromStartAreNotABom) {
    const Bytes bytes{'H', 'i', 0xFF, 0xFE, 'x'};
    EXPECT_EQ(detect_bom(bytes).type, BomType::None);
}

TEST(BomTest, TruncatedUtf8BomPrefixIsNotABom) {
    const Bytes bytes{0xEF, 0xBB};
    EXPECT_EQ(detect_bom(bytes).type, BomType::None);
}

TEST(BomTest, BomNamesAreHumanReadable) {
    EXPECT_STREQ(uc::bom_name(BomType::Utf8), "UTF-8");
    EXPECT_STREQ(uc::bom_name(BomType::Utf32LE), "UTF-32LE");
    EXPECT_STREQ(uc::bom_name(BomType::None), "none");
}
