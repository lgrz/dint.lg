
#pragma once

#include <string>

#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class MappedFile {
public:
  MappedFile(const std::string& pathname)
  {
    int fd;
    struct stat sb;

    fd = open(pathname.data(), O_RDONLY);
    if (-1 == fd) {
      handle_error("open");
    }
    if (-1 == fstat(fd, &sb)) {
      handle_error("fstat");
    }

    m_size = sb.st_size;
    m_data = (uint8_t*)mmap(NULL, m_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == m_data) {
      handle_error("mmap");
    }
    if (-1 == madvise(m_data, m_size, POSIX_MADV_SEQUENTIAL)) {
      handle_error("madvise");
    }
    close(fd);
  }

  uint8_t const* data() const
  {
    return m_data;
  }

  size_t size() const
  {
    return m_size;
  }

private:
  uint8_t* m_data = nullptr;
  size_t m_size = 0;

  static void handle_error(const char* msg)
  {
    perror(msg);
    std::exit(EXIT_FAILURE);
  }
};
