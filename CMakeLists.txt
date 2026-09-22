cmake_minimum_required(VERSION 3.16)
project(unicode_checker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra)

# The second copy of the version number, generated so it can never drift from
# project(... VERSION ...) above.
configure_file(cmake/version.hpp.in ${CMAKE_BINARY_DIR}/generated/version.hpp @ONLY)

# The detectors live in a library so both the CLI and the tests can use them.
add_library(unicode_checker_core
    src/detect_bom.cpp
    src/detect_utf8.cpp
    src/detect_invisible.cpp
    src/detect_nonchar.cpp
    src/detect_nul.cpp
    src/detect_tags.cpp
    src/report.cpp
)
target_include_directories(unicode_checker_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_BINARY_DIR}/generated
)

add_executable(unicode_checker src/main.cpp)
target_link_libraries(unicode_checker PRIVATE unicode_checker_core)

enable_testing()
add_subdirectory(tests)
