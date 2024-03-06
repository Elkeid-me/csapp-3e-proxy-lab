#ifndef CACHE_HPP
#define CACHE_HPP

#include <atomic>
#include <cstddef>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace cache
{
    constexpr std::size_t MAX_OBJECT_SIZE{102400}; // 100 KiB
    constexpr std::size_t MAX_CACHE_SIZE{1048576}; // 1 MiB

    struct cache_block
    {
        std::shared_ptr<char[]> ptr;
        std::size_t size;
        std::size_t stamp;
    };

    class cache
    {
    private:
        std::unordered_map<std::string, cache_block> map;
        std::shared_mutex mtx;

        std::size_t cache_size{0};
        std::atomic_size_t clock{0};

        void delete_lru();

    public:
        void add_cache(const std::string &uri, const char *buf,
                       std::size_t size);

        std::pair<std::shared_ptr<char[]>, std::size_t>
        find_cache(const std::string &uri);
    };
}
#endif
