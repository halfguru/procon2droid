include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.15.0
)

FetchContent_MakeAvailable(Catch2)

add_executable(procon2droid_tests
    test_init_sequence.cpp
    test_haptic_report.cpp
    ${CMAKE_SOURCE_DIR}/src/common/haptic_report.cpp
)

target_include_directories(procon2droid_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(procon2droid_tests PRIVATE
    Catch2::Catch2WithMain
)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(procon2droid_tests PRIVATE /W4)
else()
    target_compile_options(procon2droid_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(Catch)
catch_discover_tests(procon2droid_tests)
