// Copyright (C) 2022-2024 Elkeid-me
//
// This file is part of Proxy Lab.
//
// Proxy Lab is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Proxy Lab is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Proxy Lab.  If not, see <http://www.gnu.org/licenses/>.

#include "cache.hpp"
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <shared_mutex>

namespace cache
{
    void cache::delete_lru()
    {
        using item_t = std::pair<std::string, cache_block>;
        if (auto it{std::min_element(
                map.begin(), map.end(), [](const item_t &lhs, const item_t &rhs)
                { return lhs.second.stamp < rhs.second.stamp; })};
            it != map.end())
        {
            cache_size -= MAX_OBJECT_SIZE;
            map.erase(it);
        }
    }

    void cache::add_cache(const std::string &uri, const char *buf,
                          std::size_t size)
    {
        auto ptr{std::make_shared<char[]>(size)};
        std::memcpy(ptr.get(), buf, size);
        std::unique_lock lock(mtx);

        if (cache_size + size > MAX_CACHE_SIZE)
            delete_lru();
        map.emplace(
            std::make_pair(std::move(uri), cache_block{ptr, size, ++clock}));
        cache_size += MAX_OBJECT_SIZE;
    }

    std::pair<std::shared_ptr<char[]>, std::size_t>
    cache::find_cache(const std::string &uri)
    {
        std::shared_lock lock(mtx);
        if (auto it{map.find(uri)}; it != map.end())
        {
            auto [ptr, size, _]{it->second};
            it->second.stamp = ++clock;
            return {ptr, size};
        }
        return {nullptr, 0};
    }
}
