# Google Test 1.16.0 is installed system-wide; no FetchContent needed.
find_package(GTest REQUIRED)

add_executable(unicode_checker_test
    main.cpp
    bom_test.cpp
    utf8_test.cpp
    invisible_test.cpp
    nonchar_test.cpp
    nul_test.cpp
    tags_test.cpp
    examples_test.cpp
)
target_link_libraries(unicode_checker_test PRIVATE unicode_checker_core GTest::gtest)

# The example files are part of the test contract: examples_test.cpp opens each
# one from the source tree and asserts what it demonstrates. git archive HEAD
# carries them into the pristine stage, so the path is correct there too.
target_compile_definitions(unicode_checker_test
    PRIVATE UC_EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../examples"
)

add_test(NAME unicode_checker_test COMMAND unicode_checker_test)
