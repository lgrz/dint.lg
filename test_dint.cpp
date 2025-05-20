#include <iostream>
#include <catch2/catch_test_macros.hpp>

#include "io.h"
#include "postings_format.h"

TEST_CASE("", "[mmap]") {
  MappedFile f("test_collection.docs");
  REQUIRE(13763312 == f.size()); // test_collection.docs
}

TEST_CASE("PostingsFormat", "[hello]") {
  MappedFile f("test_collection.docs");
  PostingsFormat p(f.data(), f.size());

  REQUIRE((f.size() / sizeof(uint32_t)) == p.size()); // test_collection.docs
}
