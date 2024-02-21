#include "cache.hpp"
#include "file_process.hpp"
#include "rio.hpp"
#include "socket.hpp"
#include <csignal>
#include <cstddef>
#include <iostream>
#include <regex>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>

#define USE_CACHE

constexpr std::string_view USER_AGENT{
    "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 "
    "Firefox/10.0.3"};
constexpr std::string_view ESTABLISHED{
    "HTTP/1.0 200 Connection established\r\n\r\n"};
constexpr std::string_view DEFAULT_PORT{"80"};
constexpr std::string_view CONNECTION{"Connection: close\r\n"};
constexpr std::string_view PROXY_CONNECTION{"Proxy-Connection: close\r\n"};

const std::regex REGEX_HTTP_HEAD_WITH_PORT{
    R"(GET http://([^/]*):([0-9]*)/(.*?) HTTP/1\.[0-9]\r\n)"};
const std::regex REGEX_HTTP_HEAD_WITHOUT_PORT{
    R"(GET http://([^/]*)/(.*?) HTTP/1\.[0-9]\r\n)"};
const std::regex REGEX_HTTPS_HEAD_WITH_PORT{
    R"(CONNECT ([^/]*):([0-9]*) HTTP/1\.1\r\n)"};

enum class connection_type
{
    http,
    https,
    none
};

lab::cache global_cache;

std::tuple<connection_type, std::string_view, std::string_view,
           std::string_view>
parse_request_head(char *request_head, std::size_t length)
{
    std::cmatch m;

    const char *r_h_ptr{request_head};
    if (std::regex_match(r_h_ptr, r_h_ptr + length, m,
                         REGEX_HTTP_HEAD_WITH_PORT))
    {
        *(request_head + (m[1].first - request_head + m[1].length())) = '\0';
        *(request_head + (m[2].first - request_head + m[2].length())) = '\0';
        *(request_head + (m[3].first - request_head + m[3].length())) = '\0';
        return {connection_type::http,
                {m[1].first, static_cast<std::size_t>(m[1].length())},
                {m[2].first, static_cast<std::size_t>(m[2].length())},
                {m[3].first, static_cast<std::size_t>(m[3].length())}};
    }

    if (std::regex_match(r_h_ptr, r_h_ptr + length, m,
                         REGEX_HTTP_HEAD_WITHOUT_PORT))
    {
        *(request_head + (m[1].first - request_head + m[1].length())) = '\0';
        *(request_head + (m[2].first - request_head + m[2].length())) = '\0';
        return {connection_type::http,
                {m[1].first, static_cast<std::size_t>(m[1].length())},
                {DEFAULT_PORT.data(), DEFAULT_PORT.length()},
                {m[2].first, static_cast<std::size_t>(m[2].length())}};
    }

    if (std::regex_match(r_h_ptr, r_h_ptr + length, m,
                         REGEX_HTTPS_HEAD_WITH_PORT))
    {
        *(request_head + (m[1].first - request_head + m[1].length())) = '\0';
        *(request_head + (m[2].first - request_head + m[2].length())) = '\0';
        return {connection_type::https,
                {m[1].first, static_cast<std::size_t>(m[1].length())},
                {m[2].first, static_cast<std::size_t>(m[2].length())},
                {nullptr, 0}};
    }

    return {connection_type::none, {nullptr, 0}, {nullptr, 0}, {nullptr, 0}};
}

void https_proxy_help_thread(int fd_to_client, int fd_to_server)
{
    char buf[lab::MAX_OBJECT_SIZE];
    ssize_t n;
    while (true)
    {
        n = read(fd_to_client, buf, lab::MAX_OBJECT_SIZE);
        if (n < 0)
        {
            lab::Close(fd_to_server);
            return;
        }
        if (write(fd_to_server, buf, n) != n)
        {
            lab::Close(fd_to_server);
            return;
        }
    }
}

void do_https_proxy(int fd_to_client, char *buf, std::string_view host,
                    std::string_view port,
                    lab::rio::rio_t &rio)
{
    int fd_to_server{lab::Open_clientfd(host.data(), port.data())};

    ssize_t n;
    while (true)
    {
        n = rio.Rio_readlineb(buf, lab::MAX_OBJECT_SIZE);
        if (n < 0)
        {
            lab::Close(fd_to_server);
            return;
        }

        if (buf[0] == '\r')
            break;
    }

    lab::rio::Rio_writen(fd_to_client, ESTABLISHED.data(),
                         ESTABLISHED.length());

    std::thread help_thd(https_proxy_help_thread, fd_to_client, fd_to_server);
    help_thd.detach();
    while (true)
    {
        n = read(fd_to_server, buf, lab::MAX_OBJECT_SIZE);
        if (n < 0)
        {
            lab::Close(fd_to_server);
            return;
        }
        if (write(fd_to_client, buf, n) != n)
        {
            lab::Close(fd_to_server);
            return;
        }
    }
    lab::Close(fd_to_server);
}

void do_http_proxy(int fd_to_client, char *buf, std::string_view host,
                   std::string_view port, std::string_view resource,
                   lab::rio::rio_t &rio)
{
#ifdef USE_CACHE
    std::string uri{host.data(), host.length()};
    uri += ':';
    uri += port;
    uri += '/';
    uri += resource;

    auto [ptr, size]{global_cache.find_cache(uri)};

    if (ptr != nullptr)
    {
        lab::rio::Rio_writen(fd_to_client, ptr.get(), size);
        return;
    }
#endif

    int fd_to_server{lab::Open_clientfd(host.data(), port.data())};
    std::string tmp_head{"GET /"};
    tmp_head += resource;
    tmp_head += " HTTP/1.0\r\n";
    lab::rio::Rio_writen(fd_to_server, tmp_head.c_str(), tmp_head.length());

    tmp_head = "Host: ";
    tmp_head += host;
    tmp_head += ':';
    tmp_head += port;
    tmp_head += "\r\n";

    lab::rio::Rio_writen(fd_to_server, tmp_head.c_str(), tmp_head.length());

    lab::rio::Rio_writen(fd_to_server, USER_AGENT.data(), USER_AGENT.length());
    lab::rio::Rio_writen(fd_to_server, CONNECTION.data(), CONNECTION.length());
    lab::rio::Rio_writen(fd_to_server, PROXY_CONNECTION.data(),
                         PROXY_CONNECTION.length());

    ssize_t n;
    while (true)
    {
        n = rio.Rio_readlineb(buf, lab::MAX_OBJECT_SIZE);

        if (n < 0)
        {
            lab::Close(fd_to_server);
            return;
        }

        std::string_view sv{buf, static_cast<std::size_t>(n)};

        if (!(sv.starts_with("Connection: ") || sv.starts_with("Host: ") ||
              sv.starts_with("Proxy-Connection: ") ||
              sv.starts_with("User-Agent: ")))
            lab::rio::Rio_writen(fd_to_server, sv.data(), sv.length());

        if (sv[0] == '\r')
            break;
    }

    n = lab::rio::Rio_readn(fd_to_server, buf, lab::MAX_OBJECT_SIZE);
    if (n < 0)
    {
        lab::Close(fd_to_server);
        return;
    }
    lab::rio::Rio_writen(fd_to_client, buf, n);

#ifdef USE_CACHE
    if (n != lab::MAX_OBJECT_SIZE)
    {
        global_cache.add_cache(uri, buf, n);

        lab::Close(fd_to_server);
        return;
    }
#endif

    while (true)
    {
        n = lab::rio::Rio_readn(fd_to_server, buf, lab::MAX_OBJECT_SIZE);
        if (n < 0)
        {
            lab::Close(fd_to_server);
            return;
        }
        lab::rio::Rio_writen(fd_to_client, buf, n);

        if (n != lab::MAX_OBJECT_SIZE)
            break;
    }
    lab::Close(fd_to_server);
}

void proxy_thread_function(int fd_to_client)
{
    char buf[lab::MAX_OBJECT_SIZE];

    lab::rio::rio_t rio(fd_to_client);

    ssize_t n{rio.Rio_readlineb(buf, lab::MAX_OBJECT_SIZE)};

    if (n < 7 || n == lab::MAX_OBJECT_SIZE)
    {
        lab::Close(fd_to_client);
        return;
    }

    auto [type, host, port, resource]{parse_request_head(buf, n)};

    switch (type)
    {
    case connection_type::http:
        do_http_proxy(fd_to_client, buf, host, port, resource, rio);
        lab::Close(fd_to_client);
        return;
    case connection_type::https:
        do_https_proxy(fd_to_client, buf, host, port, rio);
        lab::Close(fd_to_client);
        return;
    case connection_type::none:
        lab::Close(fd_to_client);
        return;
    }
}

int main(int argc, char *argv[])
{
    std::signal(SIGPIPE, SIG_IGN);
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int listen_fd{lab::Open_listen_fd(argv[1])};
    while (true)
    {
        socklen_t client_len{sizeof(sockaddr_storage)};
        sockaddr_storage client_addr;
        int fd_to_client{lab::Accept(listen_fd,
                                     reinterpret_cast<sockaddr *>(&client_addr),
                                     &client_len)};

        std::thread new_thread(proxy_thread_function, fd_to_client);
        new_thread.detach();
    }
    return 0;
}
