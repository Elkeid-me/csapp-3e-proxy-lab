#include "rio.hpp"
#include "error_process.hpp"
#include <cerrno>
#include <cstring>
#include <unistd.h>
namespace lab::rio
{
    ssize_t rio_readn(int fd, void *usrbuf, std::size_t n)
    {
        std::size_t nleft = n;
        ssize_t nread;
        char *bufp = reinterpret_cast<char *>(usrbuf);

        while (nleft > 0)
        {
            if ((nread = read(fd, bufp, nleft)) < 0)
            {
                if (errno == EINTR)
                    nread = 0;
                else
                    return -1;
            }
            else if (nread == 0)
                break;
            nleft -= nread;
            bufp += nread;
        }
        return n - nleft;
    }

    ssize_t rio_writen(int fd, const void *usrbuf, std::size_t n)
    {
        std::size_t nleft = n;
        ssize_t nwritten;
        const char *bufp = reinterpret_cast<const char *>(usrbuf);

        while (nleft > 0)
        {
            if ((nwritten = write(fd, bufp, nleft)) <= 0)
            {
                if (errno == EINTR)
                    nwritten = 0;
                else
                    return -1;
            }
            nleft -= nwritten;
            bufp += nwritten;
        }
        return n;
    }

    ssize_t Rio_readn(int fd, void *ptr, std::size_t nbytes)
    {
        ssize_t n;

        if ((n = rio_readn(fd, ptr, nbytes)) < 0)
            unix_error("Rio_readn error");
        return n;
    }

    void Rio_writen(int fd, const void *usrbuf, std::size_t n)
    {
        if (rio_writen(fd, usrbuf, n) != static_cast<ssize_t>(n))
            unix_error("Rio_writen error");
    }
}

namespace lab::rio
{
    ssize_t rio_t::rio_read(char *user_buf, std::size_t n)
    {
        int cnt;

        while (rio_cnt <= 0)
        {
            rio_cnt = read(rio_fd, rio_buf, sizeof(rio_buf));
            if (rio_cnt < 0)
            {
                if (errno != EINTR)
                    return -1;
            }
            else if (rio_cnt == 0)
                return 0;
            else
                rio_bufptr = rio_buf;
        }

        cnt = n;
        if (rio_cnt < static_cast<int>(n))
            cnt = rio_cnt;
        std::memcpy(user_buf, rio_bufptr, cnt);
        rio_bufptr += cnt;
        rio_cnt -= cnt;
        return cnt;
    }

    ssize_t rio_t::rio_readnb(void *user_buf, std::size_t n)
    {
        std::size_t nleft = n;
        ssize_t nread;
        char *bufp = reinterpret_cast<char *>(user_buf);

        while (nleft > 0)
        {
            if ((nread = rio_read(bufp, nleft)) < 0)
                return -1;
            else if (nread == 0)
                break;
            nleft -= nread;
            bufp += nread;
        }
        return n - nleft;
    }

    ssize_t rio_t::rio_readlineb(void *user_buf, std::size_t maxlen)
    {
        int n, rc;
        char c, *bufp = reinterpret_cast<char *>(user_buf);

        for (n = 1; n < static_cast<int>(maxlen); n++)
        {
            if ((rc = rio_read(&c, 1)) == 1)
            {
                *bufp++ = c;
                if (c == '\n')
                {
                    n++;
                    break;
                }
            }
            else if (rc == 0)
            {
                if (n == 1)
                    return 0;
                else
                    break;
            }
            else
                return -1;
        }
        *bufp = 0;
        return n - 1;
    }

    ssize_t rio_t::Rio_readnb(void *user_buf, std::size_t n)
    {
        ssize_t rc{rio_readnb(user_buf, n)};

        if (rc < 0)
            unix_error("Rio_readnb error");
        return rc;
    }

    ssize_t rio_t::Rio_readlineb(void *user_buf, std::size_t maxlen)
    {
        ssize_t rc{rio_readlineb(user_buf, maxlen)};

        if (rc < 0)
            unix_error("Rio_readlineb error");
        return rc;
    }
}
