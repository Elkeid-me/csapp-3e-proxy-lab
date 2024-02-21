#include "socket.hpp"
#include "error_process.hpp"
#include "file_process.hpp"
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace lab
{
    static constexpr int LISTENQ{1024};
    static void Set_socket_option(int s, int level, int optname, const void *optval,
                           int optlen)
    {
        int rc;

        if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
            unix_error("Setsockopt error");
    }

    static addrinfo *Get_addr_info(const char *host, const char *service,
                                   const addrinfo *hints)
    {
        addrinfo *result;

        int ecode{getaddrinfo(host, service, hints, &result)};
        if (ecode == 0)
            return result;

        gai_error(ecode, "Function `getaddrinfo' error");
        return nullptr;
    }

    static int open_listen_fd(const char *port)
    {
        addrinfo hints;

        int listen_fd;

        std::memset(&hints, 0, sizeof(addrinfo));

        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG | AI_NUMERICSERV;

        addrinfo *list_head{Get_addr_info(nullptr, port, &hints)};

        addrinfo *ptr{list_head};
        while (ptr)
        {
            if ((listen_fd = socket(ptr->ai_family, ptr->ai_socktype,
                                    ptr->ai_protocol)) >= 0)
            {
                Set_socket_option(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval,
                                  sizeof(int));

                if (bind(listen_fd, ptr->ai_addr, ptr->ai_addrlen) == 0)
                    break;

                Close(listen_fd);
            }
            ptr = ptr->ai_next;
        }

        freeaddrinfo(list_head);
        if (!ptr)
            return -1;

        if (listen(listen_fd, LISTENQ) < 0)
        {
            Close(listen_fd);
            return -1;
        }

        return listen_fd;
    }

    static int open_clientfd(const char *hostname, const char *port)
    {
        int clientfd;
        struct addrinfo hints, *listp, *p;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_NUMERICSERV;
        hints.ai_flags |= AI_ADDRCONFIG;
        if (getaddrinfo(hostname, port, &hints, &listp) < 0)
            return -1;

        for (p = listp; p; p = p->ai_next)
        {
            if ((clientfd =
                     socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
                continue;

            if (connect(clientfd, p->ai_addr, p->ai_addrlen) != -1)
                break;
            Close(clientfd);
        }

        freeaddrinfo(listp);
        if (!p)
            return -1;
        else
            return clientfd;
    }

    int Open_listen_fd(const char *port)
    {
        int rc;

        if ((rc = open_listen_fd(port)) < 0)
            unix_error("Open_listenfd error");
        return rc;
    }

    int Open_clientfd(const char *hostname, const char *port)
    {
        int rc;

        if ((rc = open_clientfd(hostname, port)) < 0)
            unix_error("Open_clientfd error");
        return rc;
    }

    int Accept(int s, sockaddr *addr, socklen_t *addrlen)
    {
        int rc;

        if ((rc = accept(s, addr, addrlen)) < 0)
            unix_error("Accept error");
        return rc;
    }
}
