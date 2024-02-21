#ifndef RIO_HPP
#define RIO_HPP

#include <cstddef>
#include <sys/types.h>

namespace lab::rio
{
    constexpr std::size_t MAXLINE{8192};
    constexpr std::size_t RIO_BUFSIZE{8192};
    struct rio_t
    {
    private:
        int rio_fd;
        int rio_cnt;
        char *rio_bufptr;
        char rio_buf[RIO_BUFSIZE];
        ssize_t rio_read(char *user_buf, std::size_t n);

    public:
        rio_t(int fd)
        {
            rio_fd = fd;
            rio_cnt = 0;
            rio_bufptr = rio_buf;
        }

        ssize_t rio_readnb(void *user_buf, std::size_t n);
        ssize_t rio_readlineb(void *user_buf, std::size_t maxlen);
        ssize_t Rio_readnb(void *user_buf, std::size_t n);
        ssize_t Rio_readlineb(void *user_buf, std::size_t maxlen);
    };

    ssize_t rio_readn(int fd, void *usrbuf, std::size_t n);
    ssize_t rio_writen(int fd, const void *usrbuf, std::size_t n);

    ssize_t Rio_readn(int fd, void *usrbuf, std::size_t n);
    void Rio_writen(int fd, const void *usrbuf, std::size_t n);

}
#endif
