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

#ifndef FILE_PROCESS_HPP
#define FILE_PROCESS_HPP

#include <cstddef>
#include <unistd.h>
namespace file
{
    [[nodiscard]] ssize_t robust_read(int fd, char *buf, std::size_t size);
    [[nodiscard]] ssize_t robust_write(int fd, const char *buf,
                                       std::size_t size);

    class fd_wrapper
    {
    private:
        int m_fd{-1};

    public:
        fd_wrapper() = default;
        fd_wrapper(int fd);
        ~fd_wrapper();
        fd_wrapper(const fd_wrapper &) = delete;
        fd_wrapper(fd_wrapper &&other);
        [[nodiscard]] ssize_t robust_read(char *buf, std::size_t size);
        [[nodiscard]] ssize_t robust_write(const char *buf, std::size_t size);
        bool valid() const;
        int get_fd();
    };
}
#endif
