#include "detect_bom.h"

namespace uc {
namespace {

bool starts_with(const std::vector<std::uint8_t>& bytes,
                 std::initializer_list<std::uint8_t> prefix) {
    if (bytes.size() < prefix.size()) {
        return false;
    }
    std::size_t i = 0;
    for (const std::uint8_t expected : prefix) {
        if (bytes[i++] != expected) {
            return false;
        }
    }
    return true;
}

char hex_digit(std::uint8_t nibble) {
    return static_cast<char>(nibble < 10 ? '0' + nibble : 'A' + (nibble - 10));
}

}  // namespace

BomResult detect_bom(const std::vector<std::uint8_t>& bytes) {
    // UTF-32BE must be tested before anything shorter that could shadow it,
    // and UTF-32LE before UTF-16LE: FF FE 00 00 starts with the UTF-16LE BOM.
    if (starts_with(bytes, {0x00, 0x00, 0xFE, 0xFF})) {
        return {BomType::Utf32BE, 4};
    }
    if (starts_with(bytes, {0xFF, 0xFE, 0x00, 0x00})) {
        return {BomType::Utf32LE, 4};
    }
    if (starts_with(bytes, {0xEF, 0xBB, 0xBF})) {
        return {BomType::Utf8, 3};
    }
    if (starts_with(bytes, {0xFE, 0xFF})) {
        return {BomType::Utf16BE, 2};
    }
    if (starts_with(bytes, {0xFF, 0xFE})) {
        return {BomType::Utf16LE, 2};
    }
    if (starts_with(bytes, {0x2B, 0x2F, 0x76})) {
        const std::uint8_t fourth = bytes.size() > 3 ? bytes[3] : 0x00;
        if (fourth == 0x38 || fourth == 0x39 || fourth == 0x2B || fourth == 0x2F) {
            return {BomType::Utf7, 4};
        }
    }
    return {BomType::None, 0};
}

const char* bom_name(BomType type) {
    switch (type) {
        case BomType::Utf8:
            return "UTF-8";
        case BomType::Utf16BE:
            return "UTF-16BE";
        case BomType::Utf16LE:
            return "UTF-16LE";
        case BomType::Utf32BE:
            return "UTF-32BE";
        case BomType::Utf32LE:
            return "UTF-32LE";
        case BomType::Utf7:
            return "UTF-7";
        case BomType::None:
            break;
    }
    return "none";
}

std::string bom_hex(const std::vector<std::uint8_t>& bytes, const BomResult& bom) {
    std::string out;
    for (std::size_t i = 0; i < bom.length && i < bytes.size(); ++i) {
        if (!out.empty()) {
            out += ' ';
        }
        out += hex_digit(static_cast<std::uint8_t>(bytes[i] >> 4));
        out += hex_digit(static_cast<std::uint8_t>(bytes[i] & 0x0F));
    }
    return out;
}

}  // namespace uc
