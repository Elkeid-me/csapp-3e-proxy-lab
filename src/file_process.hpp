#ifndef FILE_PROCESS_HPP
#define FILE_PROCESS_HPP

#include <cstddef>

namespace lab
{
    void Close(int fd);

    void Write(int fd, const char *buf, std::size_t size);
}
#endif
