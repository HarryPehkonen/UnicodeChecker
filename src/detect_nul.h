#ifndef UNICODE_CHECKER_DETECT_NUL_H
#define UNICODE_CHECKER_DETECT_NUL_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace uc {

enum class NulInterleave {
    None,
    Utf16LELike,  // ASCII byte, NUL, ASCII byte, NUL, ...
    Utf16BELike,  // NUL, ASCII byte, NUL, ASCII byte, ...
};

struct NulResult {
    std::size_t total_bytes = 0;
    std::size_t nul_count = 0;
    double nul_ratio = 0.0;  // nul_count / total_bytes, 0 for an empty file
    bool binary_like = false;
    NulInterleave interleaved = NulInterleave::None;
    std::size_t leading_nuls = 0;
};

// A file is called binary-like when more than this fraction of it is NUL.
constexpr double kBinaryNulRatio = 0.005;

// Shortest run of byte pairs that counts as evidence of a UTF-16 pattern.
constexpr std::size_t kMinInterleavePairs = 2;

NulResult detect_nul(const std::vector<std::uint8_t>& bytes);

const char* nul_interleave_name(NulInterleave interleave);

}  // namespace uc

#endif  // UNICODE_CHECKER_DETECT_NUL_H
