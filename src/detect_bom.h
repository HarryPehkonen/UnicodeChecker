#ifndef UNICODE_CHECKER_DETECT_BOM_H
#define UNICODE_CHECKER_DETECT_BOM_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace uc {

enum class BomType {
    None,
    Utf8,
    Utf16BE,
    Utf16LE,
    Utf32BE,
    Utf32LE,
    Utf7,
};

struct BomResult {
    BomType type = BomType::None;
    std::size_t length = 0;  // number of bytes the BOM occupies
};

// Examines the first bytes of the buffer against the known BOM table.
// A BOM only counts at offset 0; the same bytes elsewhere are plain data.
BomResult detect_bom(const std::vector<std::uint8_t>& bytes);

// Human-readable label, e.g. "UTF-16LE"; "none" for BomType::None.
const char* bom_name(BomType type);

// Uppercase hex of the BOM bytes, e.g. "EF BB BF"; empty when there is none.
std::string bom_hex(const std::vector<std::uint8_t>& bytes, const BomResult& bom);

}  // namespace uc

#endif  // UNICODE_CHECKER_DETECT_BOM_H
