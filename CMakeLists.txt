cmake_minimum_required(VERSION 3.21)

project(procon2droid
    VERSION 1.1.0
    DESCRIPTION "Magisk/KernelSU module for Nintendo Switch 2 Pro Controller"
    LANGUAGES CXX
)

# Generate module.prop from template so version stays in sync with CMake
math(EXPR MODULE_VERSION_CODE "${PROJECT_VERSION_MAJOR} * 10000 + ${PROJECT_VERSION_MINOR} * 100 + ${PROJECT_VERSION_PATCH}")
configure_file(
    "${CMAKE_SOURCE_DIR}/module.prop.in"
    "${CMAKE_SOURCE_DIR}/module.prop"
    @ONLY
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

add_subdirectory(src)

option(PROCON2DROID_BUILD_TESTS "Build test suite" ON)

if(PROCON2DROID_BUILD_TESTS AND NOT CMAKE_SYSTEM_NAME STREQUAL "Android")
    enable_testing()
    add_subdirectory(tests)
endif()
