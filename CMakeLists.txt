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
    src/read_file.cpp
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

# ---------------------------------------------------------------- install
include(GNUInstallDirs)

# `cmake --install build` defaults to the user's own bin directory, because this is a
# tool one person runs, and a system-wide copy is the deliberate choice rather than the
# accident. Two ways to say otherwise, both honoured:
#
#   cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local     # at configure time
#   cmake --install build --prefix /usr/local                 # at install time, no reconfigure
#
# The gate's `install` stage asserts this default, so it cannot quietly become /usr/local.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix (default: the user's ~/.local)" FORCE)
endif()

# One binary and nothing else: the detectors are a library for this program's own use,
# not a public one, and the examples are read from the source tree.
install(TARGETS unicode_checker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# ---------------------------------------------------------------- other targets
# CMake already provides `all`, `clean`, `test` (ctest, from enable_testing()) and
# `install`. These three are the ones a person actually reaches for, and each one is a
# thin call into something that already exists rather than a second implementation:

# check — the repo's gate, through its one script. Same verdict as the git hooks print,
# so "make check" and "git push" cannot disagree about whether this tree is good.
add_custom_target(check
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gate.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL
    COMMENT "running scripts/gate.sh (the full gate)"
)

# format — clang-format over the whole tree. The gate's format stage checks only the
# files a branch touches (so pre-existing drift cannot block you); the two share
# .clang-format, so they cannot disagree about style. CONFIGURE_DEPENDS keeps this list
# current as files are added, which a hand-written list would not.
file(GLOB_RECURSE UC_FORMAT_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.h
)
add_custom_target(format
    COMMAND clang-format -i ${UC_FORMAT_SOURCES}
    COMMENT "clang-format -i over src/ and tests/"
)

# examples — run the checker over every demo file in examples/. The point of that
# directory is to be run over, and this saves remembering the loop.
add_custom_target(examples
    COMMAND ${CMAKE_COMMAND} -E echo "running unicode_checker over examples/"
    COMMAND ${CMAKE_COMMAND} -E env sh -c
            "for f in ${CMAKE_CURRENT_SOURCE_DIR}/examples/*; do case \"$$f\" in *README*) continue;; esac; echo \"== $$f\"; ${CMAKE_BINARY_DIR}/unicode_checker \"$$f\"; done"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS unicode_checker
    USES_TERMINAL
)
