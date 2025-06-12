
#include <random>
#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "succinct/bit_vector.hpp"

namespace dint { namespace test {

static std::vector<bool> random_bit_vector(size_t n = 10000,
                                           double density = 0.5)
{
  std::random_device gen;
  std::default_random_engine dre(gen());
  std::bernoulli_distribution dist(density);

  std::vector<bool> bits;
  bits.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    bits.push_back(dist(dre));
  }

  return bits;
}

TEST_CASE("bit vectors from random bool vectors", "[succinct]") {
  std::vector<bool> bitvec = random_bit_vector();

  SECTION("bits mapped from bool std::vector") {
    succinct::bit_vector_builder builder;
    for (auto const& bit : bitvec) {
      builder.push_back(bit);
    }
    succinct::bit_vector bitmap(&builder);

    REQUIRE(bitvec.size() == bitmap.size());
    for (size_t i = 0; i < bitvec.size(); ++i) {
      REQUIRE(bitvec[i] == bitmap[i]);
    }
  }
}

} }
