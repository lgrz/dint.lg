
#pragma once

#include <iostream>
#include <iterator>
#include <cstdint>

#include "platform.h"
#include "io.h"

class PostingsFormat {
  public:
    using posting_type = uint32_t;

    struct Sequence {
      constexpr Sequence() = default;

      constexpr Sequence(posting_type const* begin, posting_type const* end)
        : m_begin(begin)
        , m_end(end)
      {
      }

      posting_type const* m_begin = nullptr;
      posting_type const* m_end = nullptr;
    };

    struct Iterator {
      using value_type = Sequence;
      using iterator_category = std::forward_iterator_tag;

      constexpr bool operator==(Iterator const& other) const
      {
        return m_index == other.m_index;
      }

      constexpr bool operator!=(Iterator const& other) const = default;

      constexpr Iterator& operator++()
      {
        m_index = m_next;
        next();
        return *this;
      }

      ALWAYS_INLINE constexpr value_type const* operator->() const
      {
        return &m_entry;
      }

      ALWAYS_INLINE constexpr value_type const& operator*() const
      {
        return m_entry;
      }

      constexpr Iterator() = default;

      constexpr Iterator(PostingsFormat const* container, size_t index)
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
        
        posting_type const* begin = &m_container->m_data[index];
        posting_type const* end = begin + len;

        m_next = m_index + len;
        m_entry = Sequence();
      }

      PostingsFormat const* m_container = nullptr;
      size_t m_index = 0;
      size_t m_next = 0;
      Sequence m_entry;
    };

    constexpr Iterator begin() const
    {
      return Iterator(this, 0);
    }

    constexpr Iterator end() const
    {
      return Iterator(this, m_size);
    }

    constexpr size_t size() const { return m_size; }

    PostingsFormat(uint8_t const* data, size_t length) 
    {
      m_data = (posting_type const*)data;
      m_size = length / sizeof(posting_type);
    }

private:
    posting_type const* m_data = nullptr;
    size_t m_size = 0;
};

