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

#ifndef SOCKET_HPP
#define SOCKET_HPP

namespace net
{
    int open_listen_fd(const char *);
    int open_client_fd(const char *hostname, const char *port);
}
#endif
