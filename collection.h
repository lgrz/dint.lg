
#pragma once

#include <iostream>
#include <iterator>
#include <cstdint>
#include <span>
#include <filesystem>
#include <stdexcept>

#include "io.h"
#include "platform.h"
#include "postings_format.h"

namespace dint {

namespace fs = std::filesystem;

using namespace std::literals;

// TODO: The document lengths stored in `docsizes` file is not used here, it is
//       used by WandData (and WandData does not exist yet).
static constexpr auto s_docs_filename = "sequence.docs"sv;
static constexpr auto s_freqs_filename = "sequence.freqs"sv;
static constexpr auto s_docsizes_filename = "docsizes"sv;

class FrequencyCollection {
public:

  FrequencyCollection(fs::path const& index_path)
  {
    MappedFile docs_buf = MappedFile(index_path / s_docs_filename);
    MappedFile freqs_buf = MappedFile(index_path / s_freqs_filename);
    m_docs = PostingsFormat(docs_buf.data(), docs_buf.size());
    m_freqs = PostingsFormat(freqs_buf.data(), freqs_buf.size());
    auto header = *m_docs.begin();
    if (1 != header.size()) {
      // TODO: replace with std::expected which pushes errors to the caller and
      //       is self documenting in unit tests.
      throw std::invalid_argument("error: header sequence");
    }
    m_num_docs = *header.begin();
  }

  struct SequencePair {
    std::span<const PostingsFormat::posting_type> docs;
    std::span<const PostingsFormat::posting_type> freqs;
  };

  struct Iterator {
      using value_type = SequencePair;
      using iterator_category = std::forward_iterator_tag;

      constexpr bool operator==(Iterator const& other) const
      {
        return m_it_docs == other.m_it_docs;
      }

      constexpr bool operator!=(Iterator const& other) const = default;

      constexpr Iterator& operator++()
      {
        m_entry.docs = *++m_it_docs;
        m_entry.freqs = *++m_it_freqs;
        return *this;
      }

      DINT_ALWAYS_INLINE constexpr value_type const* operator->() const
      {
        return &m_entry;
      }

      DINT_ALWAYS_INLINE constexpr value_type const& operator*() const
      {
        return m_entry;
      }

      constexpr Iterator(PostingsFormat::Iterator it_docs, PostingsFormat::Iterator it_freqs)
        : m_it_docs(it_docs)
        , m_it_freqs(it_freqs)
      {
        m_entry.docs = *m_it_docs;
        m_entry.freqs = *m_it_freqs;
      }

      constexpr Iterator() = default;

      PostingsFormat::Iterator m_it_docs;
      PostingsFormat::Iterator m_it_freqs;
      value_type m_entry;
  };

  constexpr Iterator begin() const
  {
    auto it_docs = ++m_docs.begin();
    return Iterator(it_docs, m_freqs.begin());
  }

  constexpr Iterator end() const
  {
    return Iterator(m_docs.end(), m_freqs.end());
  }

  constexpr uint64_t size() const
  {
    // It is the number of "posting_type" entries (and not the number of
    // "postings lists") which would be the vocabulary size anyway.
    // Subtract two slots for the header (a span/sequence of length one) in the
    // "sequence.docs" index file.
    // Note though that this is not a correct tally of "posting_type" elements
    // in the collection because each list has two fields (header, values). The
    // header is one slot and has the length of the values list. The correct
    // tally would need to factor out these header elements.
    return m_docs.size() + m_freqs.size() - 2;
  }

  constexpr uint64_t num_docs() const { return m_num_docs; }

private:
  // TODO: rename `PostingsFormat` to something like `SequenceFormat` as we
  //       will be using the term postings in abstraction layers above.
  PostingsFormat m_docs;
  PostingsFormat m_freqs;
  uint64_t m_num_docs = 0;
};

};
