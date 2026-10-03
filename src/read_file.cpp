#include "read_file.h"

#include <exception>
#include <fstream>
#include <iterator>

namespace uc {

bool read_file(const std::string& path, std::vector<std::uint8_t>& bytes) {
    bytes.clear();

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    try {
        bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    } catch (const std::exception&) {
        // The failure the guard is here for: a directory opens as an ifstream and only
        // fails when the read reaches the buffer, which throws ios_base::failure. Reading
        // is the whole of this function's job, and a read that fails is an answer -- "I
        // cannot read this" -- not a reason to unwind out of main().
        bytes.clear();
        return false;
    }

    return !file.bad();
}

}  // namespace uc
