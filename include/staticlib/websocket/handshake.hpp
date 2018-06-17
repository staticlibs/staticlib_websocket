/*
 * Copyright 2018, alex at staticlibs.net
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * File:   handshake.hpp
 * Author: alex
 *
 * Created on June 7, 2018, 2:23 PM
 */

#ifndef STATICLIB_WEBSOCKET_HANDSHAKE_HPP
#define STATICLIB_WEBSOCKET_HANDSHAKE_HPP

#include <string>
#include <utility>
#include <vector>

#include "staticlib/crypto.hpp"
#include "staticlib/io.hpp"
#include "staticlib/support.hpp"

namespace staticlib {
namespace websocket {
namespace handshake {

inline std::string make_request_line(const std::string& path) {
    return std::string("GET") + path + " HTTP/1.1";
}

inline std::vector<std::pair<std::string, std::string>> make_request_headers(
        const std::string& host, uint16_t port, const std::string& key) {
    auto src = sl::io::array_source(key.data(), key.length());
    auto key_base64 = sl::io::string_sink();
    {
        auto sink = sl::crypto::make_base64_sink(key_base64);
        sl::io::copy_all(src, sink);
    }
    auto vec = std::vector<std::pair<std::string, std::string>>();
    vec.emplace_back("Host", host + ":" + sl::support::to_string(port));
    vec.emplace_back("Connection", "Upgrade");
    vec.emplace_back("Pragma", "no-cache");
    vec.emplace_back("Cache-Control", "no-cache");
    vec.emplace_back("Upgrade", "websocket");
    vec.emplace_back("Sec-WebSocket-Version", "13");
    vec.emplace_back("Sec-WebSocket-Key", std::move(key_base64.get_string()));
    return vec;
}

inline std::string make_response_line() {
    return "HTTP/1.1 101 Switching Protocols";
}

inline std::vector<std::pair<std::string, std::string>> make_response_headers(
        const std::string& key) {
    auto concatted = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    auto src = sl::io::array_source(concatted.data(), concatted.length());
    auto sha1_sink = sl::crypto::make_sha1_sink(sl::io::null_sink());
    sl::io::copy_all(src, sha1_sink);
    auto& accept_sha1 = sha1_sink.get_hash();
    auto accept_sha1_hex_src = sl::io::array_source(accept_sha1.data(), accept_sha1.length());
    auto accept_sha1_src = sl::io::make_hex_source(accept_sha1_hex_src);
    auto accept_base64 = sl::io::string_sink();
    {
        auto base64_sink = sl::crypto::make_base64_sink(accept_base64);
        sl::io::copy_all(accept_sha1_src, base64_sink);
    }
    auto vec = std::vector<std::pair<std::string, std::string>>();
    vec.emplace_back("Upgrade", "websocket");
    vec.emplace_back("Connection", "Upgrade");
    vec.emplace_back("Sec-WebSocket-Accept", std::move(accept_base64.get_string()));
    return vec;
}

} // namespace
}
}

#endif /* STATICLIB_WEBSOCKET_HANDSHAKE_HPP */
