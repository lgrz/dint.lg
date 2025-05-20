
#pragma once

#include <iostream>
#include <iterator>
#include <cstdint>
#include <span>

#include "platform.h"
#include "io.h"

class PostingsFormat {
  public:
    using posting_type = uint32_t;

    struct Iterator {
      using value_type = std::span<const posting_type>;
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
        size_t offset = m_index;

        // TODO: remove zero-length check but only after testing all (test)
        //       inverted files for non-existence.
        do {
          len = m_container->m_data[offset++];
        } while(0 == len); // skip zero-length entries

        // TODO: remove file truncation check but only after testing all (test)
        //       inverted files for non-existence.
        len = std::min(len, size_t(m_container->m_size - offset));
        
        posting_type const* begin = &m_container->m_data[offset];

        m_next = offset + len;
        m_entry = value_type(begin, len);
      }

      PostingsFormat const* m_container = nullptr;
      size_t m_index = 0;
      size_t m_next = 0;
      value_type m_entry;
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

