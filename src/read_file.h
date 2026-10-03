#ifndef UNICODE_CHECKER_READ_FILE_H
#define UNICODE_CHECKER_READ_FILE_H

#include <cstdint>
#include <string>
#include <vector>

namespace uc {

// Reads the whole file into `bytes` in binary mode. Returns false when the path cannot be
// read as a file -- it does not exist, it is a directory, it is not readable, or the read
// fails part way -- and never throws.
//
// The directory case is the one that needs the guard: on Linux an ifstream opens a
// directory happily and the failure comes later, out of the stream buffer, as
// ios_base::failure. Letting that escape std::istreambuf_iterator turns
// `unicode_checker .` into std::terminate, which is a crash where the tool's whole job is
// to report what it thinks of its input.
bool read_file(const std::string& path, std::vector<std::uint8_t>& bytes);

}  // namespace uc

#endif  // UNICODE_CHECKER_READ_FILE_H
