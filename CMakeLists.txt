cmake_minimum_required(VERSION 3.25)

project(dint.lg
        VERSION 0.1.0
        LANGUAGES CXX C
        DESCRIPTION "Dictionary-based compression for inverted indexes"
        HOMEPAGE_URL ""
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DINT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${DINT_SOURCE_DIR}/cmake")

include(common)
include(CPM)
CPMAddPackage("gh:catchorg/Catch2@3.8.1")

add_executable(test_dint test_dint.cpp)
target_link_libraries(test_dint PRIVATE Catch2::Catch2WithMain)

enable_testing()
add_test(test_dint test_dint)
