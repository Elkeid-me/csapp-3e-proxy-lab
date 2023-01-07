#include "error_process.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <netdb.h>

namespace lab
{
    void unix_error(const char *msg)
    {
        std::fprintf(stderr, "%s: %s\n", msg, std::strerror(errno));
    }

    void posix_error(int code, const char *msg)
    {
        std::fprintf(stderr, "%s: %s\n", msg, std::strerror(code));
    }

    void gai_error(int code, const char *msg)
    {
        std::fprintf(stderr, "%s: %s\n", msg, gai_strerror(code));
    }
}
