
#pragma once

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <span>
#include <cstdint>
#include <vector>

#include "platform.h"
#include "io.hpp"

// InvertedList
// InvertedDictionary
// ...
class PackedPostings {
  public:
    using posting_type = uint32_t;

    PackedPostings(uint8_t const* data, size_t length) 
    {
      m_data = (posting_type const*)data;
      m_size = length / sizeof(posting_type);
    }

private:
    posting_type const* m_data = nullptr;
    size_t m_size = 0;
};

class PostingsCursor {
public:
  class PackedPostingsIterator;
  friend PackedPostingsIterator;

  using posting_type = PackedPostings::posting_type;

  constexpr PostingsCursor() = default;

private:
  constexpr PostingsCursor(posting_type const* begin, posting_type const* end)
    : m_begin(begin)
    , m_end(end)
  {
  }

  posting_type const* m_begin = nullptr;
  posting_type const* m_end = nullptr;
};


template<typename Container, typename ValueType>
class PackedPostingsIterator {
public:
  friend Container;

  using value_type = ValueType;
  using iterator_category = std::forward_iterator_tag;

  constexpr bool operator==(PackedPostingsIterator const& other) const = default;
  constexpr bool operator!=(PackedPostingsIterator const& other) const = default;

  constexpr PackedPostingsIterator& operator++()
  {
    m_index = m_next;
    next();
    return *this;
  }

  ALWAYS_INLINE constexpr ValueType const* operator->() const
  {
    return &m_entry;
  }

  ALWAYS_INLINE constexpr ValueType const& operator*() const
  {
    return &m_entry;
  }

  constexpr PackedPostingsIterator() = default;

private:

  constexpr PackedPostingsIterator(Container& container, size_t index)
    : m_container(container)
    , m_index(index)
  {
    next();
  }

  constexpr void next()
  {
    if (m_index == m_container->m_size) {
      return;
    }

    size_t len = 0;
    size_t index = m_index;

    len = m_container->m_data[index++];
    if (0 == len) {
      std::cerr << "TODO: handle zero length entries...\n";
      std::exit(EXIT_FAILURE);
    }

    if (len > size_t(m_container->m_size - index)) {
      std::cerr << "TODO: handle truncated files...\n";
      std::exit(EXIT_FAILURE);
    }
    
    value_type const* begin = &m_container->m_data[index];
    value_type const* end = begin + len;

    m_next = m_index + len;
    m_entry = PostingsCursor();
  }

  Container const* m_container = nullptr;
  size_t m_index = 0;
  size_t m_next = 0;
  PostingsCursor m_entry;
};

