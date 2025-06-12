
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <cstdint>
#include <random>
#include <ranges>
#include <vector>
#include "compact_elias_fano.h"
#include "succinct/bit_vector.hpp"

namespace dint { namespace test {

static std::vector<uint64_t> random_sequence(size_t universe, size_t n, bool strict = true)
{
  std::random_device gen;
  std::default_random_engine dre(gen());
  std::uniform_int_distribution<uint64_t> dist(0, universe);

  if (strict) {
    universe -= n;
  }

  std::vector<uint64_t> seq(0, n);
  for (size_t i = 0; i < n; ++i) {
    seq.push_back(dist(dre));
  }
  std::sort(seq.begin(), seq.end());

  if (strict) {
    for (size_t i = 0; i < n; ++i) {
      seq[i] += i;
    }
  }

  return seq;
}

TEST_CASE("compact elias fano sequence", "[eliasfano]") {
  // sequence parameters
  const size_t base = 0;
  const size_t n = 100'000;
  const size_t universe = n * 1024;
  // ds2i: high granularity to test corner cases
  const uint8_t log_sampling0 = 4;
  const uint8_t log_sampling1 = 5;

  // short sequence
  auto gen_vals = GENERATE(values({0, 1}));

  SECTION("`move_next` random access and enumeration") {
    std::vector<uint64_t> sequence;
    sequence.push_back(gen_vals);

    // setup CompactEliasFano sequence
    succinct::bit_vector_builder builder;
    CompactEliasFano::write(builder, sequence.begin(), universe,
        sequence.size(), log_sampling0, log_sampling1);
    // bit vector and enumerator
    succinct::bit_vector bv(&builder);
    CompactEliasFano::Enumerator r(bv, base, universe, sequence.size(),
        log_sampling0, log_sampling1);

    // tests...
    REQUIRE(sequence.size() == r.size());
    // TODO: An empty sequence generates a floating point exception in the
    //       above setup steps.
    if (sequence.empty()) {
      REQUIRE(sequence.size() == r.move(sequence.size()).first);
    }

    CompactEliasFano::Enumerator::value_type seq_val;
    // test random access
    INFO("random");
    for (uint64_t i = 0; i < sequence.size(); ++i) {
      seq_val = r.move(i);

      INFO("i = " << i);
      REQUIRE(i == seq_val.first);

      INFO("sequence[i] = " << sequence[i]);
      INFO("seq_val.second = " << seq_val.second);
      REQUIRE(sequence[i] == seq_val.second);
    }
    r.move(sequence.size());
    REQUIRE(sequence.back() == r.prev_value());

    seq_val = r.move(0);
    // test enumerator
    INFO("enumerator");
    for (uint64_t i = 0; i < sequence.size(); ++i) {
      INFO("i = " << i);
      REQUIRE(sequence[i] == seq_val.second);

      if (0 == i) {
        INFO("i = " << i);
        REQUIRE(0 == r.prev_value());
      } else {
        INFO("i = " << i);
        REQUIRE(sequence[i - 1] == r.prev_value());
      }

      seq_val = r.next();
    }
    REQUIRE(r.size() == seq_val.first);
    REQUIRE(sequence.back() == r.prev_value());
  }
}

} }
