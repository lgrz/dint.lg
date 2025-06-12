
#pragma once

#include <bit>
#include <cassert>
#include <cstdint>
#include <utility>
#include "platform.h"
#include "succinct/bit_vector.hpp"
#include "succinct/broadword.hpp"
#include "succinct/util.hpp"

namespace dint {

// TODO: Update internal naming, for example `_offset` to `_base`.
struct Offset
{
  Offset() = default;

  Offset(uint64_t base,
         uint64_t universe,
         uint64_t n,
         uint8_t log_sampling0,
         uint8_t log_sampling1)
    : base(base)
    , universe(universe)
    , n(n)
    , log_sampling0(log_sampling0)
    , log_sampling1(log_sampling1)
  {
    assert(n > 0);

    low_bits = 0;
    if (universe > n) {
      low_bits = succinct::broadword::msb(universe / n);
    }

    mask = (uint64_t(-1) << low_bits) - 1;

    // add sentinels to both sides (zero padding)
    high_bits_length = (n + (universe >> low_bits) + 2);

    pointer_size = std::bit_ceil(high_bits_length);
    pointer0 = (high_bits_length - n) >> log_sampling0;
    pointer1 = n >> log_sampling1;

    pointer0_offset = base;
    pointer1_offset = pointer0_offset + pointer0 * pointer_size;
    high_bits_offset = pointer1_offset + pointer1 * pointer_size;
    low_bits_offset = high_bits_offset + high_bits_length;
    end = low_bits_offset + n * low_bits;
  }

  uint64_t base = 0;
  uint64_t universe = 0;
  uint64_t n = 0;

  uint8_t log_sampling0 = 0;
  uint8_t log_sampling1 = 0;

  uint64_t low_bits = 0;
  uint64_t mask = 0;
  uint64_t high_bits_length = 0;
  uint64_t pointer_size = 0;
  uint64_t pointer0 = 0;
  uint64_t pointer1 = 0;

  uint64_t pointer0_offset = 0;
  uint64_t pointer1_offset = 0;
  uint64_t high_bits_offset = 0;
  uint64_t low_bits_offset = 0;
  uint64_t end = 0;
};

struct CompactEliasFano
{
  class Enumerator;

  // Note the parameter ordering, it follows the `Offset` constructor
  // above (and is different to the `bitsize` in ds2i).
  static DINT_FLATTEN uint64_t bitsize(uint64_t universe,
                          uint64_t n,
                          uint8_t log_sampling0,
                          uint8_t log_sampling1)
  {
    return Offset(0, universe, n, log_sampling0, log_sampling1).end;
  }

  template<typename Iterator>
  static void write(succinct::bit_vector_builder& bvb,
                    Iterator begin,
                    uint64_t universe,
                    uint64_t n,
                    uint8_t log_sampling0,
                    uint8_t log_sampling1)
  {
    using succinct::util::ceil_div;

    uint64_t base = bvb.size();
    Offset ef_offset(base, universe, n, log_sampling0, log_sampling1);
    // zero initialize all bits
    bvb.zero_extend(ef_offset.end - base);

    uint64_t offset;
    // helper to set to set "0" pointers in the for loop below
    auto fn_set_pointer0 = [&ef_offset, &offset, &bvb]
                           (uint64_t begin, // TODO: name clash with parent scope
                            uint64_t end,
                            uint64_t rank_end) {
      // TODO: This function captures `offset` from the parent scope and
      //       changes its state and so on. But, in the loop below the `offset`
      //       variable is set prior to this function getting called.
      uint64_t zeros_begin = begin - rank_end;
      uint64_t zeros_end = end - rank_end;

      uint64_t ptr0 = ceil_div(zeros_begin,
                               uint64_t(1) << ef_offset.log_sampling0);
      for (/* ptr0 */; (ptr0 << ef_offset.log_sampling0) < zeros_end; ++ptr0) {
        if (!ptr0) {
          continue;
        }
        offset = ef_offset.pointer0_offset + (ptr0 - 1) * ef_offset.pointer_size;
        assert(offset + ef_offset.pointer_size <= ef_offset.pointer1_offset);
        bvb.set_bits(offset,
                     (ptr0 << ef_offset.log_sampling0) + rank_end,
                     ef_offset.pointer_size);
      }
    };

    uint64_t mask_sampling1 = (uint64_t(1) << ef_offset.log_sampling1) - 1;

    uint64_t last = 0;
    uint64_t last_high = 0;
    Iterator it = begin; // copy ??
    for (size_t i = 0; i < n; ++i) {
      uint64_t v = *it++;
      if (i > 0 && v < last) {
        // TODO: Push this responsibility elsewhere?
        throw std::runtime_error("sequence  is not sorted");
      }
      assert(v < universe);
      uint64_t high = (v >> ef_offset.low_bits) + i + 1;
      uint64_t low = v & ef_offset.mask;

      bvb.set(ef_offset.high_bits_offset + high, 1);
      // TODO: `offset` related to `fn_set_pointer0`
      offset = ef_offset.low_bits_offset + i * ef_offset.low_bits;
      assert(offset + ef_offset.low_bits <= ef_offset.end);
      // TODO: `bvb` related to `fn_set_pointer0`
      bvb.set_bits(offset, low, ef_offset.low_bits);

      if (i > 0 && 0 == (i & mask_sampling1)) {
        uint64_t ptr1 = i >> ef_offset.log_sampling1;
        assert(ptr1 > 0);
        // TODO: `offset` related to `fn_set_pointer0`
        offset = ef_offset.pointer1_offset + (ptr1 - 1) * ef_offset.pointer_size;
        assert(offset + ef_offset.pointer_size <= ef_offset.high_bits_offset);
        // TODO: `bvb` related to `fn_set_pointer0`
        bvb.set_bits(offset, high, ef_offset.pointer_size);
      }

      // write pointers for the run of zeros in [last, high) to `bvb`.
      // TODO: `offset`, `bvb` modified by `fn_set_pointer0`
      fn_set_pointer0(last_high + 1, high, i);
      last_high = high;
      last = v;
    }

    // write pointers to zeros after the last 1
    // TODO: `offset`, `bvb` modified by `fn_set_pointer0`
    fn_set_pointer0(last_high + 1, ef_offset.high_bits_length, n);
  }
};

class CompactEliasFano::Enumerator
{
public:
  // tuple of (position, value)
  using value_type = std::pair<uint64_t, uint64_t>;

  constexpr value_type move(uint64_t position)
  {
    assert(position <= m_offset.n);

    if (m_position == position) {
      return get();
    }

    uint64_t skip = position - m_position;
    // optimize small forward skips
    if (position > m_position && skip <= k_linear_scan_threshold) [[likely]] {
      m_position = position;
      if (m_position == size()) [[unlikely]] {
        m_value = m_offset.universe;
      } else {
        succinct::bit_vector::unary_enumerator he = m_high_enumerator;
        for (size_t i = 0; i < skip; ++i) {
          he.next();
        }
        m_value = (he.position() - m_offset.high_bits_offset - m_position - 1)
                  << m_offset.low_bits
                  | read_low();
        m_high_enumerator = he;
      }
      return get();
    }

    return slow_move(position);
  }

  constexpr value_type next()
  {
    m_position += 1;
    // TODO: note the less-than or equal check
    assert(m_position <= size());
    if (m_position < size()) [[likely]] {
      m_value = read_next();
    } else {
      m_value = m_offset.universe;
    }

    return get();
  }

  constexpr value_type next_geq(uint64_t lower_bound)
  {
    if (lower_bound == m_value) {
      return get();
    }

    uint64_t high_lower_bound = lower_bound >> m_offset.low_bits;
    uint64_t high_curr = m_value >> m_offset.low_bits;
    uint64_t high_diff = high_lower_bound - high_curr;

    if (lower_bound > m_value && high_diff <= k_linear_scan_threshold) [[likely]] {
      // optimize small skips
      Reader next_value(*this, m_position + 1);
      uint64_t val;
      do {
        ++m_position;
        if (m_position < size()) [[likely]] {
          val = next_value();
        } else {
          val = m_offset.universe;
          break;
        }
      } while(val < lower_bound);
      m_value = val;
      return get();
    }

    return slow_next_geq(lower_bound);
  }

  constexpr uint64_t prev_value() const
  {
    if (0 == m_position) {
      return 0;
    }

    uint64_t prev_high = 0;
    if (m_position < size()) [[likely]] {
      prev_high = m_bv->predecessor1(m_high_enumerator.position() - 1);
    } else {
      prev_high = m_bv->predecessor1(m_offset.low_bits_offset - 1);
    }
    prev_high -= m_offset.high_bits_offset;

    uint64_t prev_pos = m_position - 1;
    // not quite the same as `read_low()` because of `prev_pos`.
    uint64_t prev_low = m_bv->get_word56(m_offset.low_bits_offset +
                                         prev_pos * m_offset.low_bits) &
                        m_offset.mask;
    return ((prev_high - prev_pos - 1) << m_offset.low_bits) | prev_low;
  }

  constexpr uint64_t position() const
  {
    return m_position;
  }

  constexpr uint64_t size() const
  {
    return m_offset.n;
  }

  constexpr Enumerator() = default;

  constexpr Enumerator(succinct::bit_vector const &bv,
                       uint64_t base, // `offset` ds2i
                       uint64_t universe,
                       uint64_t n,
                       uint8_t log_sampling0,
                       uint8_t log_sampling1)
    : m_bv(&bv)
    , m_offset(base, universe, n, log_sampling0, log_sampling1)
    , m_position(size())
    , m_value(m_offset.universe)
  {}

private:
  value_type DINT_NEVER_INLINE slow_move(uint64_t position)
  {
    if (position == size()) [[unlikely]] {
      m_position = position;
      m_value = m_offset.universe;
      return get();
    }

    uint64_t skip = position - m_position;
    uint64_t to_skip;
    if (position > m_position && 0 == (skip >> m_offset.log_sampling1)) {
      to_skip = skip - 1;
    } else {
      uint64_t ptr = position >> m_offset.log_sampling1;
      uint64_t high_pos = pointer1(ptr);
      uint64_t high_rank = ptr << m_offset.log_sampling1;
      m_high_enumerator = succinct::bit_vector::unary_enumerator
                            (*m_bv, m_offset.high_bits_offset + high_pos);
      to_skip = position - high_rank;
    }

    m_high_enumerator.skip(to_skip);
    m_position = position;
    m_value = read_next();

    return get();
  }

  constexpr value_type slow_next_geq(uint64_t lower_bound)
  {
    if (lower_bound >= m_offset.universe) [[unlikely]] {
      return move(size());
    }

    uint64_t high_lower_bound = lower_bound >> m_offset.low_bits;
    uint64_t high_curr = m_value >> m_offset.low_bits;
    uint64_t high_diff = high_lower_bound - high_curr;

    // TODO: ds2i remark, bounds checking.
    uint64_t to_skip;
    if (lower_bound > m_value && 0 == (high_diff >> m_offset.log_sampling0)) {
      // ds2i remark.
      //
      // Note: at the current position in the bitvector there
      //       should be a 1, but since we already consumed it, it
      //       is 0 in the enumerator, so we need to skip it
      to_skip = high_diff;
    } else {
      uint64_t ptr = high_lower_bound >> m_offset.log_sampling0;
      uint64_t high_pos = pointer0(ptr);
      uint64_t high_rank0 = ptr << m_offset.log_sampling0;
      m_high_enumerator = succinct::bit_vector::unary_enumerator
                            (*m_bv, m_offset.high_bits_offset + high_pos);
      to_skip = high_lower_bound - high_rank0;
    }

    m_high_enumerator.skip0(to_skip);
    m_position = m_high_enumerator.position() -
                 m_offset.high_bits_offset - high_lower_bound;
    Reader read_value(*this, m_position);
    while (true) {
      if (m_position == size()) [[unlikely]] {
        m_value = m_offset.universe;
        return value();
      }
      auto val = read_value();
      if (val >= lower_bound) {
        m_value = val;
        return value();
      }
      ++m_position;
    }

    return value();
  }

  constexpr DINT_ALWAYS_INLINE uint64_t read_low()
  {
    return
      m_bv->get_word56(m_offset.low_bits_offset +
                       m_position * m_offset.low_bits) &
      m_offset.mask;
  }

  constexpr DINT_ALWAYS_INLINE uint64_t read_next()
  {
    assert(m_position < size());
    uint64_t high = m_high_enumerator.next() - m_offset.high_bits_offset;
    return ((high - m_position - 1) << m_offset.low_bits) | read_low();
  }

  constexpr DINT_ALWAYS_INLINE uint64_t pointer(uint64_t base, uint64_t i) const
  {
    if (0 == i) {
      return 0;
    }

    return
      m_bv->get_word56(base + (i - 1) * m_offset.pointer_size) &
      ((uint64_t(1) << m_offset.pointer_size) - 1);
  }

  constexpr DINT_ALWAYS_INLINE uint64_t pointer0(uint64_t i) const
  {
    return pointer(m_offset.pointer0_offset, i);
  }

  constexpr DINT_ALWAYS_INLINE uint64_t pointer1(uint64_t i) const
  {
    return pointer(m_offset.pointer1_offset, i);
  }

  // was `value()` in ds2i
  constexpr value_type get() const
  {
    return value_type(m_position, m_value);
  }

  /**
   * TODO: document nested data-structures adding structure to the code overall
   *       with nested stuff going on.
   */
  struct Reader {
    uint64_t operator()()
    {
      uint64_t high = high_enumerator.next() - base_high;
      uint64_t low = bv.get_word56(base_low) & mask;
      base_high += 1;
      base_low += low_bits;
      return high << low_bits | low;
    }

    constexpr Reader(Enumerator &e, uint64_t position)
      : e(e)
      , bv(*e.m_bv)
      , high_enumerator(e.m_high_enumerator)
      , mask(e.m_offset.mask)
      , low_bits(e.m_offset.low_bits)
      , base_low(e.m_offset.low_bits_offset + (position * low_bits))
      , base_high(e.m_offset.high_bits_offset + position + 1)
    {
    }

    constexpr ~Reader()
    {
      e.m_high_enumerator = high_enumerator;
    }

    Enumerator& e;
    succinct::bit_vector const& bv;
    succinct::bit_vector::unary_enumerator high_enumerator;
    uint64_t mask = 0;
    uint64_t low_bits = 0;
    uint64_t base_low = 0;
    uint64_t base_high = 0;
  };

  static const uint64_t k_linear_scan_threshold = 8;
  succinct::bit_vector const* m_bv;
  Offset m_offset;
  uint64_t m_position;
  uint64_t m_value;
  succinct::bit_vector::unary_enumerator m_high_enumerator;
};

}
