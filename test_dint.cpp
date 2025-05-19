#include <iostream>
#include <catch2/catch_test_macros.hpp>

#include "collection.hpp"

TEST_CASE("", "[mmap]") {
  MappedFile f("test_collection.docs");
  REQUIRE(13763312 == f.size()); // test_collection.docs
}
