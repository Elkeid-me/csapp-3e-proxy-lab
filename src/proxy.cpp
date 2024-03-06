#include "cache.hpp"
#include "file.hpp"
#include "socket.hpp"
#include <csignal>
#include <cstddef>
#include <iostream>
#include <ranges>
#include <regex>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <vector>

using std::literals::string_view_literals::operator""sv;

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

cache::cache global_cache;

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
    char buf[cache::MAX_OBJECT_SIZE];
    while (true)
    {
        ssize_t n{read(fd_to_client, buf, cache::MAX_OBJECT_SIZE)};
        if (n <= 0 || file::robust_write(fd_to_server, buf, n) != n)
            return;
    }
}

void do_https_proxy(file::fd_wrapper fd_to_client, char *buf,
                    std::string_view host, std::string_view port)
{
    file::fd_wrapper fd_to_server{
        net::open_client_fd(host.data(), port.data())};

    if (fd_to_client.robust_write(ESTABLISHED.data(), ESTABLISHED.length()) < 0)
        return;

    std::jthread help_thd(https_proxy_help_thread, fd_to_client.get_fd(),
                          fd_to_server.get_fd());
    while (true)
    {
        ssize_t n{read(fd_to_server.get_fd(), buf, cache::MAX_OBJECT_SIZE)};
        if (n <= 0 || fd_to_client.robust_write(buf, n) < 0)
            return;
    }
}

std::vector<std::string_view> split_sv(std::string_view sv)
{
    std::vector<std::string_view> v;
    for (std::string_view::size_type i_0{0}, i_1{sv.find('\n', i_0 + 1)};
         i_1 != std::string_view::npos; i_1 = sv.find('\n', i_0 + 1))
    {
        v.push_back({sv.data() + i_0, sv.data() + i_1 + 1});
        i_0 = i_1;
    }
    return v;
}

void do_http_proxy(file::fd_wrapper fd_to_client,
                   std::vector<std::string_view> sv_vec, char *buf,
                   std::string_view host, std::string_view port,
                   std::string_view resource)
{
    std::string uri{host.data(), host.length()};
    uri += ':';
    uri += port;
    uri += '/';
    uri += resource;

    auto [ptr, size]{global_cache.find_cache(uri)};

    if (ptr != nullptr)
    {
        ssize_t make_gcc_happy
            [[maybe_unused]]{fd_to_client.robust_write(ptr.get(), size)};
        return;
    }

    file::fd_wrapper fd_to_server{
        net::open_client_fd(host.data(), port.data())};
    std::string tmp_head{"GET /"sv};
    tmp_head += resource;
    tmp_head += " HTTP/1.0\r\n"sv;

    if (fd_to_server.robust_write(tmp_head.c_str(), tmp_head.length()) < 0)
        return;

    tmp_head = "Host: "sv;
    tmp_head += host;
    tmp_head += ':';
    tmp_head += port;
    tmp_head += "\r\n"sv;

    if (fd_to_server.robust_write(tmp_head.c_str(), tmp_head.length()) < 0 ||
        fd_to_server.robust_write(USER_AGENT.data(), USER_AGENT.length()) < 0 ||
        fd_to_server.robust_write(CONNECTION.data(), CONNECTION.length()) < 0 ||
        fd_to_server.robust_write(PROXY_CONNECTION.data(),
                                  PROXY_CONNECTION.length()) < 0)
        return;

    for (auto sv : std::views::drop(sv_vec, 1))
    {
        if (!(sv.starts_with("Connection: "sv) || sv.starts_with("Host: "sv) ||
              sv.starts_with("Proxy-Connection: "sv) ||
              sv.starts_with("User-Agent: "sv)))
        {
            if (fd_to_server.robust_write(sv.data(), sv.length()) < 0)
                return;
        }
    }

    ssize_t n{fd_to_server.robust_read(buf, cache::MAX_OBJECT_SIZE)};
    if (n < 0 || fd_to_client.robust_write(buf, n) < 0)
        return;
    if (n != cache::MAX_OBJECT_SIZE)
    {
        global_cache.add_cache(uri, buf, n);
        return;
    }

    while (true)
    {
        ssize_t n{fd_to_server.robust_read(buf, cache::MAX_OBJECT_SIZE)};
        if (n < 0 || fd_to_client.robust_write(buf, n) < 0)
            return;
        if (n != cache::MAX_OBJECT_SIZE)
            break;
    }
}

void proxy_thread_function(file::fd_wrapper fd_to_client)
{
    char buf[cache::MAX_OBJECT_SIZE];
    ssize_t n{read(fd_to_client.get_fd(), buf, cache::MAX_OBJECT_SIZE)};
    if (n < 0)
        return;
    std::vector<std::string_view> sv_vec{
        split_sv({buf, static_cast<std::size_t>(n)})};

    if (sv_vec.empty() || sv_vec.front().length() < 7)
        return;

    auto [type, host, port,
          resource]{parse_request_head(buf, sv_vec.front().length())};

    switch (type)
    {
    case connection_type::http:
        do_http_proxy(std::move(fd_to_client), std::move(sv_vec), buf, host,
                      port, resource);
        return;
    case connection_type::https:
        do_https_proxy(std::move(fd_to_client), buf, host, port);
        return;
    case connection_type::none:
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

    file::fd_wrapper listen_fd{net::open_listen_fd(argv[1])};
    while (true)
    {
        socklen_t client_len{sizeof(sockaddr_storage)};
        sockaddr_storage client_addr;
        file::fd_wrapper fd_to_client{
            accept(listen_fd.get_fd(),
                   reinterpret_cast<sockaddr *>(&client_addr), &client_len)};
        if (fd_to_client.valid())
        {
            std::thread new_thread(proxy_thread_function,
                                   std::move(fd_to_client));
            new_thread.detach();
        }
    }
    return 0;
}
