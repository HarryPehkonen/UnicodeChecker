#include "detect_nul.h"

namespace uc {
namespace {

// True when every byte at `nul_index` of each pair is NUL and its partner is
// not, across all whole pairs in the buffer.
bool has_interleave(const std::vector<std::uint8_t>& bytes, std::size_t nul_index) {
    const std::size_t pairs = bytes.size() / 2;
    if (pairs < kMinInterleavePairs) {
        return false;
    }
    for (std::size_t pair = 0; pair < pairs; ++pair) {
        const std::uint8_t nul_byte = bytes[pair * 2 + nul_index];
        const std::uint8_t text_byte = bytes[pair * 2 + (1 - nul_index)];
        if (nul_byte != 0x00 || text_byte == 0x00) {
            return false;
        }
    }
    return true;
}

}  // namespace

NulResult detect_nul(const std::vector<std::uint8_t>& bytes) {
    NulResult result;
    result.total_bytes = bytes.size();

    for (const std::uint8_t byte : bytes) {
        if (byte == 0x00) {
            ++result.nul_count;
        }
    }
    while (result.leading_nuls < bytes.size() && bytes[result.leading_nuls] == 0x00) {
        ++result.leading_nuls;
    }

    if (!bytes.empty()) {
        result.nul_ratio =
            static_cast<double>(result.nul_count) / static_cast<double>(bytes.size());
    }
    result.binary_like = result.nul_ratio > kBinaryNulRatio;

    if (has_interleave(bytes, 1)) {
        result.interleaved = NulInterleave::Utf16LELike;
    } else if (has_interleave(bytes, 0)) {
        result.interleaved = NulInterleave::Utf16BELike;
    }
    return result;
}

const char* nul_interleave_name(NulInterleave interleave) {
    switch (interleave) {
        case NulInterleave::Utf16LELike:
            return "every other byte is NUL, consistent with UTF-16LE";
        case NulInterleave::Utf16BELike:
            return "NUL precedes every other byte, consistent with UTF-16BE";
        case NulInterleave::None:
            break;
    }
    return "none";
}

}  // namespace uc
