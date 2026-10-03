#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

#include "read_file.h"
#include "report.h"
#include "version.hpp"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: unicode_checker <filepath>\n";
        std::cerr << "       unicode_checker --version\n";
        return 2;
    }

    const std::string path = argv[1];
    if (path == "--version" || path == "-V") {
        std::cout << "unicode_checker " << UNICODE_CHECKER_VERSION << '\n';
        return 0;
    }

    std::vector<std::uint8_t> bytes;
    if (!uc::read_file(path, bytes)) {
        std::cerr << "unicode_checker: cannot read '" << path << "'\n";
        return 1;
    }

    std::cout << uc::format_report(uc::analyze(path, bytes));
    return 0;
}
