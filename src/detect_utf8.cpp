#include "detect_utf8.h"

namespace uc {
namespace {

// The state machine has six states: one for the start of a sequence, three
// for "still expecting N continuation bytes", plus the two terminal outcomes
// a sequence can reach.
enum class State {
    Start,
    Tail1,
    Tail2,
    Tail3,
    Accept,
    Reject,
};

// Byte classes, exactly the ranges UTF-8 assigns meaning to.
enum class ByteClass {
    Ascii,         // 00..7F
    Continuation,  // 80..BF
    Lead2,         // C0..DF
    Lead3,         // E0..EF
    Lead4,         // F0..F7
    Invalid,       // F8..FF
};

ByteClass classify(std::uint8_t byte) {
    if (byte < 0x80) return ByteClass::Ascii;
    if (byte < 0xC0) return ByteClass::Continuation;
    if (byte < 0xE0) return ByteClass::Lead2;
    if (byte < 0xF0) return ByteClass::Lead3;
    if (byte < 0xF8) return ByteClass::Lead4;
    return ByteClass::Invalid;
}

std::uint32_t minimum_value(std::size_t sequence_length) {
    switch (sequence_length) {
        case 2:
            return 0x80;
        case 3:
            return 0x800;
        default:
            return 0x10000;
    }
}

CodepointKind classify_codepoint(std::uint32_t value, std::size_t sequence_length) {
    if (value < minimum_value(sequence_length)) return CodepointKind::Overlong;
    if (value >= 0xD800 && value <= 0xDFFF) return CodepointKind::Surrogate;
    if (value > 0x10FFFF) return CodepointKind::OutOfRange;
    return CodepointKind::Valid;
}

char hex_digit(std::uint32_t nibble) {
    return static_cast<char>(nibble < 10 ? '0' + nibble : 'A' + (nibble - 10));
}

}  // namespace

Utf8Result decode_utf8(const std::vector<std::uint8_t>& bytes) {
    Utf8Result result;

    State state = State::Start;
    std::uint32_t value = 0;
    std::size_t sequence_start = 0;
    std::size_t sequence_length = 0;

    std::size_t i = 0;
    while (i < bytes.size()) {
        const std::uint8_t byte = bytes[i];
        const ByteClass byte_class = classify(byte);

        if (state == State::Start) {
            switch (byte_class) {
                case ByteClass::Ascii:
                    result.codepoints.push_back({byte, i, 1, CodepointKind::Valid});
                    break;
                case ByteClass::Continuation:
                    // A continuation byte with no lead byte in front of it.
                    ++result.lone_continuation_bytes;
                    result.valid = false;
                    break;
                case ByteClass::Lead2:
                    value = byte & 0x1Fu;
                    state = State::Tail1;
                    break;
                case ByteClass::Lead3:
                    value = byte & 0x0Fu;
                    state = State::Tail2;
                    break;
                case ByteClass::Lead4:
                    value = byte & 0x07u;
                    state = State::Tail3;
                    break;
                case ByteClass::Invalid:
                    ++result.invalid_start_bytes;
                    result.valid = false;
                    break;
            }
            if (state != State::Start) {
                sequence_start = i;
                sequence_length = state == State::Tail1 ? 2 : (state == State::Tail2 ? 3 : 4);
            }
            ++i;
            continue;
        }

        if (byte_class != ByteClass::Continuation) {
            ++result.invalid_continuation_bytes;
            result.valid = false;
            state = State::Reject;
        }

        if (state == State::Reject) {
            // Abandon the sequence and re-examine this byte as a lead byte,
            // which is the usual resynchronisation rule.
            state = State::Start;
            continue;
        }

        value = (value << 6) | (byte & 0x3Fu);
        state = state == State::Tail3 ? State::Tail2
                                      : (state == State::Tail2 ? State::Tail1 : State::Accept);
        ++i;

        if (state == State::Accept) {
            const CodepointKind kind = classify_codepoint(value, sequence_length);
            result.codepoints.push_back({value, sequence_start, sequence_length, kind});
            switch (kind) {
                case CodepointKind::Overlong:
                    ++result.overlong_sequences;
                    break;
                case CodepointKind::Surrogate:
                    ++result.surrogate_codepoints;
                    break;
                case CodepointKind::OutOfRange:
                    ++result.out_of_range_codepoints;
                    break;
                case CodepointKind::Valid:
                    break;
            }
            if (kind != CodepointKind::Valid) {
                result.valid = false;
            }
            state = State::Start;
        }
    }

    if (state != State::Start) {
        ++result.truncated_sequences;
        result.valid = false;
    }
    return result;
}

std::string format_codepoint(std::uint32_t value) {
    std::string digits;
    do {
        digits.insert(digits.begin(), hex_digit(value & 0xFu));
        value >>= 4;
    } while (value != 0);
    while (digits.size() < 4) {
        digits.insert(digits.begin(), '0');
    }
    return "U+" + digits;
}

}  // namespace uc
