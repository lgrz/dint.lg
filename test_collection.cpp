
#include <iostream>
#include <ranges>
#include <string_view>
#include <catch2/catch_test_macros.hpp>

#include "collection.h"
#include "postings_format.h"

namespace dint {
namespace test {

static constexpr auto s_testcollection_path = "test_collection";
static constexpr auto s_testcollection_docs = "test_collection/sequence.docs";
static constexpr auto s_testcollection_freqs = "test_collection/sequence.freqs";
static constexpr auto s_testcollection_docsizes = "test_collection/docsizes";

TEST_CASE("mmap inverted index files", "[MappedFile]") {
  MappedFile docs(s_testcollection_docs);
  REQUIRE(13763312 == docs.size());

  MappedFile freqs(s_testcollection_freqs);
  REQUIRE(13763304 == freqs.size());

  MappedFile doclens(s_testcollection_docsizes);
  REQUIRE(40004 == doclens.size());
}

TEST_CASE("postings format and header ", "[PostingsFormat]") {
  MappedFile mmfile(s_testcollection_docs);
  PostingsFormat pfmt(mmfile.data(), mmfile.size());
  auto first = *pfmt.begin();

  REQUIRE((mmfile.size() / sizeof(PostingsFormat::posting_type)) == pfmt.size());
  REQUIRE(1 == first.size());
  REQUIRE(10000 == *first.begin());
}

TEST_CASE("integer sequence iterator", "[PostingsFormat]") {
  MappedFile docs(s_testcollection_docs);
  PostingsFormat pfmt(docs.data(), docs.size());

  PostingsFormat::posting_type sum = 0;
  std::size_t zero_count = 0;
  std::size_t record_count = 0;

  for (auto const& record : pfmt) {
    ++record_count;
    sum += record.size();
    if (0 == record.size()) {
      ++zero_count;
    }
  }

  REQUIRE(113307 == record_count);
  REQUIRE(3327521 == sum);
  REQUIRE(0 == zero_count);
}

TEST_CASE("collection from the index directory", "[Collection]") {
  dint::FrequencyCollection freqcoll(s_testcollection_path);

  REQUIRE(10000 == freqcoll.num_docs());
}

}
}
