#ifndef SOCKET_HPP
#define SOCKET_HPP

namespace net
{
    int open_listen_fd(const char *);
    int open_client_fd(const char *hostname, const char *port);
}
#endif
