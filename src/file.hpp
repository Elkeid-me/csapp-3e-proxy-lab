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
