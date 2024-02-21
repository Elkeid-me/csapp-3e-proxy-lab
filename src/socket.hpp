#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <netdb.h>

namespace lab
{
    constexpr int optval{1};
    int Open_listen_fd(const char *);
    int Open_clientfd(const char *hostname, const char *port);
    int Accept(int s, sockaddr *addr, socklen_t *addrlen);
}
#endif
