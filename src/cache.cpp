#include "cache.hpp"
#include <algorithm>
#include <cstring>
#include <memory>

namespace lab
{
    std::atomic<std::size_t> clock{0};

    void cache::delete_lru()
    {
        using item_t = std::pair<std::string, cache_block>;
        if (auto it{std::min_element(map.begin(), map.end(),
                                     [](const item_t &lhs, const item_t &rhs) {
                                         return lhs.second.stamp <
                                                rhs.second.stamp;
                                     })};
            it != map.end())
        {
            cache_size -= MAX_OBJECT_SIZE;
            map.erase(it);
        }
    }

    void cache::add_cache(const std::string &uri, const char *buf,
                          std::size_t size)
    {
        std::shared_ptr<char[]> ptr{new char[size]};
        std::memcpy(ptr.get(), buf, size);
        mtx.lock();

        if (cache_size + size > MAX_CACHE_SIZE)
            delete_lru();
        map.emplace(
            std::make_pair(std::move(uri), cache_block{ptr, size, ++clock}));
        mtx.unlock();
        cache_size += MAX_OBJECT_SIZE;
    }

    std::pair<std::shared_ptr<char[]>, std::size_t>
    cache::find_cache(const std::string &uri)
    {
        mtx.lock_shared();
        if (auto it{map.find(uri)}; it != map.end())
        {
            auto [ptr, size, _]{it->second};
            it->second.stamp = ++clock;
            mtx.unlock_shared();
            return {ptr, size};
        }

        mtx.unlock_shared();
        return {nullptr, 0};
    }
}
