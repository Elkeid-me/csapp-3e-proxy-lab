#ifndef ERROR_PROCESS_HPP
#define ERROR_PROCESS_HPP
namespace lab
{
    void unix_error(const char *msg);
    void posix_error(int code, const char *msg);
    void gai_error(int code, const char *msg);
}
#endif
