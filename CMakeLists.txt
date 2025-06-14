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
add_subdirectory(extern/succinct)

add_executable(test_collection test_collection.cpp)
target_link_libraries(test_collection PRIVATE
    Catch2::Catch2WithMain
)

add_executable(test_sequence test_sequence.cpp)
target_include_directories(test_sequence PRIVATE
  $<BUILD_INTERFACE:${DINT_SOURCE_DIR}/extern>
  $<INSTALL_INTERFACE:include/dint/succinct>
)
target_link_libraries(test_sequence PRIVATE
    Catch2::Catch2WithMain
    succinct
)

add_executable(test_compact_elias_fano test_compact_elias_fano.cpp)
target_include_directories(test_compact_elias_fano PRIVATE
  $<BUILD_INTERFACE:${DINT_SOURCE_DIR}/extern>
  $<INSTALL_INTERFACE:include/dint/succinct>
)
target_link_libraries(test_compact_elias_fano PRIVATE
    Catch2::Catch2WithMain
    succinct
)
