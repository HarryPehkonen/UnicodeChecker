#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

#include "read_file.h"
#include "report.h"

#ifndef UC_EXAMPLES_DIR
#error "UC_EXAMPLES_DIR must be defined by CMake (the examples/ directory of this repo)"
#endif

using uc::read_file;

namespace {

using Bytes = std::vector<std::uint8_t>;

const std::string kExamplesDir = UC_EXAMPLES_DIR;

}  // namespace

TEST(ReadFileTest, AMissingPathIsReportedAndNotThrown) {
    Bytes bytes;
    EXPECT_FALSE(read_file(kExamplesDir + "/definitely-not-here.txt", bytes));
}

// The regression this file exists for. `unicode_checker .` used to die with
// "terminate called after throwing an instance of 'std::__ios_failure'" and SIGABRT,
// because an ifstream opens a directory on Linux and the failure only surfaces later,
// out of the stream buffer, inside std::istreambuf_iterator.
TEST(ReadFileTest, ADirectoryIsReportedAndNotThrown) {
    Bytes bytes;
    EXPECT_FALSE(read_file(kExamplesDir, bytes)) << "reading a directory must return false";
    EXPECT_FALSE(read_file(".", bytes)) << "the working directory is a directory too";
}

TEST(ReadFileTest, ARealFileIsReadWholeInBinaryMode) {
    Bytes bytes;
    ASSERT_TRUE(read_file(kExamplesDir + "/01-plain-ascii.txt", bytes));
    EXPECT_FALSE(bytes.empty());
    EXPECT_EQ(bytes.front(), 'P') << "the first byte of that example is 'P' (Plain ASCII...)";
    EXPECT_EQ(bytes.back(), '\n');
}

TEST(ReadFileTest, NulBytesSurviveTheRead) {
    Bytes bytes;
    ASSERT_TRUE(read_file(kExamplesDir + "/08-nul-interleaved.bin", bytes));
    std::size_t nuls = 0;
    for (const std::uint8_t byte : bytes) {
        if (byte == 0) {
            ++nuls;
        }
    }
    EXPECT_GT(nuls, 0u) << "opened in text mode, the NULs would be gone or the read would stop";
    EXPECT_EQ(bytes.size(), 18u);
}

TEST(ReadFileTest, WhatItReadsIsWhatThePipelineReports) {
    // The seams have to agree: bytes in, the same bytes analyzed.
    Bytes bytes;
    ASSERT_TRUE(read_file(kExamplesDir + "/03-utf8-defects.txt", bytes));
    const auto report = uc::analyze("03-utf8-defects.txt", bytes);
    EXPECT_FALSE(report.utf8.valid);
    EXPECT_EQ(report.size, bytes.size());
}
