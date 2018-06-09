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
 * File:   frame_type.hpp
 * Author: alex
 *
 * Created on June 7, 2018, 2:05 PM
 */

#ifndef STATICLIB_WEBSOCKET_FRAME_TYPE_HPP
#define STATICLIB_WEBSOCKET_FRAME_TYPE_HPP

#include <cstdint>

namespace staticlib {
namespace websocket {

enum class frame_type {
    invalid,
    continuation = 0x0,
    text = 0x1,
    binary = 0x2,
    close = 0x8,
    ping = 0x9,
    pong = 0xa
};

template<typename IntType>
frame_type make_frame_type(IntType val) {
    switch(val) {
    case static_cast<IntType>(frame_type::continuation): return frame_type::continuation;
    case static_cast<IntType>(frame_type::text): return frame_type::text;
    case static_cast<IntType>(frame_type::binary): return frame_type::binary;
    case static_cast<IntType>(frame_type::close): return frame_type::close;
    case static_cast<IntType>(frame_type::ping): return frame_type::ping;
    case static_cast<IntType>(frame_type::pong): return frame_type::pong;
    default: return frame_type::invalid;
    }
}

} // namespace
}

#endif /* STATICLIB_WEBSOCKET_FRAME_TYPE_HPP */

