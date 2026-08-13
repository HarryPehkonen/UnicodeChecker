#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "report.h"

namespace {

bool read_file(const std::string& path, std::vector<std::uint8_t>& bytes) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !file.bad();
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: unicode_checker <filepath>\n";
        return 2;
    }

    const std::string path = argv[1];
    std::vector<std::uint8_t> bytes;
    if (!read_file(path, bytes)) {
        std::cerr << "unicode_checker: cannot read '" << path << "'\n";
        return 1;
    }

    std::cout << uc::format_report(uc::analyze(path, bytes));
    return 0;
}
