// Copyright (C) 2022-2024 Elkeid-me
//
// This file is part of Proxy Lab.
//
// Proxy Lab is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Proxy Lab is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Proxy Lab.  If not, see <http://www.gnu.org/licenses/>.

#include "file.hpp"
#include <cerrno>
#include <unistd.h>

namespace file
{
    fd_wrapper::fd_wrapper(int fd) : m_fd{fd} {}
    fd_wrapper::~fd_wrapper()
    {
        if (m_fd >= 0)
            close(m_fd);
    }
    fd_wrapper::fd_wrapper(fd_wrapper &&other) : m_fd{other.m_fd}
    {
        other.m_fd = -1;
    }
    bool fd_wrapper::valid() const { return m_fd >= 0; }
    int fd_wrapper::get_fd() { return m_fd; }

    [[nodiscard]] ssize_t robust_write(int fd, const char *buf,
                                       std::size_t size)
    {
        std::size_t n_written_bytes{0};
        while (n_written_bytes != size)
        {
            ssize_t n_written{
                write(fd, buf + n_written_bytes, size - n_written_bytes)};
            if (n_written < 0)
            {
                if (errno == EINTR)
                    n_written = 0;
                else
                    return n_written;
            }

            n_written_bytes += n_written;
        }
        return n_written_bytes;
    }

    [[nodiscard]] ssize_t robust_read(int fd, char *buf, const std::size_t size)
    {
        std::size_t n_read_bytes{0};
        while (n_read_bytes != size)
        {
            ssize_t n_read{read(fd, buf + n_read_bytes, size - n_read_bytes)};
            if (n_read < 0)
            {
                if (errno == EINTR)
                    n_read = 0;
                else
                    return n_read;
            }
            else if (n_read == 0)
                break;
            n_read_bytes += n_read;
        }
        return n_read_bytes;
    }

    [[nodiscard]] ssize_t fd_wrapper::robust_write(const char *buf,
                                                   std::size_t size)
    {
        return file::robust_write(m_fd, buf, size);
    }

    [[nodiscard]] ssize_t fd_wrapper::robust_read(char *buf,
                                                  const std::size_t size)
    {
        return file::robust_read(m_fd, buf, size);
    }
}
