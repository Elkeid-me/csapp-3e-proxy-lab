#include "socket.hpp"
#include <memory>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace net
{
    static constexpr int LISTENQ{1024};
    static addrinfo *get_addr_info(const char *host, const char *service,
                                   const addrinfo *hints)
    {
        addrinfo *result;
        if (getaddrinfo(host, service, hints, &result) < 0)
            return nullptr;
        return result;
    }

    int open_listen_fd(const char *port)
    {
        constexpr int SO_REUSEADDR_OPTVAL{SO_REUSEADDR};
        addrinfo hint{AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV,
                      AF_UNSPEC,
                      SOCK_STREAM,
                      IPPROTO_IP,
                      0,
                      nullptr,
                      nullptr,
                      nullptr};
        std::unique_ptr<addrinfo, void (*)(addrinfo *)> addr_list_head(
            get_addr_info(nullptr, port, &hint), freeaddrinfo);

        if (addr_list_head == nullptr)
            return -1;

        for (addrinfo *ptr{addr_list_head.get()}; ptr != nullptr;
             ptr = ptr->ai_next)
        {
            int listen_fd{
                socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)};
            if (listen_fd < 0)
                continue;
            setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR,
                       &SO_REUSEADDR_OPTVAL, sizeof(int));
            if (bind(listen_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
            {
                if (listen(listen_fd, LISTENQ) < 0)
                {
                    close(listen_fd);
                    return -1;
                }
                return listen_fd;
            }
            close(listen_fd);
        }
        return -1;
    }

    int open_client_fd(const char *hostname, const char *port)
    {
        addrinfo hint{AI_NUMERICSERV | AI_ADDRCONFIG,
                      AF_UNSPEC,
                      SOCK_STREAM,
                      IPPROTO_IP,
                      0,
                      nullptr,
                      nullptr,
                      nullptr};

        std::unique_ptr<addrinfo, void (*)(addrinfo *)> addr_list_head(
            get_addr_info(hostname, port, &hint), freeaddrinfo);

        if (addr_list_head == nullptr)
            return -1;

        for (addrinfo *ptr{addr_list_head.get()}; ptr != nullptr;
             ptr = ptr->ai_next)
        {
            int client_fd{
                socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol)};
            if (client_fd < 0)
                continue;
            if (connect(client_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
                return client_fd;
            close(client_fd);
        }
        return -1;
    }
}
