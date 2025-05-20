
#include <iostream>
#include <catch2/catch_test_macros.hpp>

#include "io.h"
#include "postings_format.h"

TEST_CASE("mmap inverted index files", "[MappedFile]") {
  MappedFile docs("test_collection.docs");
  REQUIRE(13763312 == docs.size());

  MappedFile freqs("test_collection.freqs");
  REQUIRE(13763304 == freqs.size());

  MappedFile doclens("test_collection.sizes");
  REQUIRE(40004 == doclens.size());
}

TEST_CASE("postings format and header ", "[PostingsFormat]") {
  MappedFile f("test_collection.docs");
  PostingsFormat p(f.data(), f.size());
  auto first = *p.begin();

  REQUIRE((f.size() / sizeof(uint32_t)) == p.size());
  REQUIRE(1 == first.size());
  REQUIRE(10000 == *first.begin());
}

TEST_CASE("inverted lists iterator", "[PostingsFormat]") {
  MappedFile docs("test_collection.docs");
  PostingsFormat pf(docs.data(), docs.size());

  PostingsFormat::posting_type sum = 0;
  std::size_t zero_count = 0;
  for (auto const& record : pf) {
    sum += record.size();
    if (!record.size()) {
      ++zero_count;
    }
  }

  REQUIRE(3327521 == sum);
  REQUIRE(0 == zero_count);
}
