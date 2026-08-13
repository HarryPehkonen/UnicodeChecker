# Google Test 1.16.0 is installed system-wide; no FetchContent needed.
find_package(GTest REQUIRED)

add_executable(unicode_checker_test
    main.cpp
    bom_test.cpp
    utf8_test.cpp
    invisible_test.cpp
    nonchar_test.cpp
    nul_test.cpp
)
target_link_libraries(unicode_checker_test PRIVATE unicode_checker_core GTest::gtest)

add_test(NAME unicode_checker_test COMMAND unicode_checker_test)
