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
 * File:   masked_payload_source.hpp
 * Author: alex
 *
 * Created on June 7, 2018, 2:16 PM
 */

#ifndef STATICLIB_WEBSOCKET_MASKED_PAYLOAD_SOURCE_HPP
#define STATICLIB_WEBSOCKET_MASKED_PAYLOAD_SOURCE_HPP

#include <cstdint>

#include "staticlib/io.hpp"

namespace staticlib {
namespace websocket {

/**
 * `Source` implementation that can be used to unmask the payload
 * value in a streaming mode
 */
class masked_payload_source {
    /**
     * Input source
     */
    sl::io::span<const char> payload;
    /**
     * Mask
     */
    uint32_t mask;
    /**
     * Number of bytes read
     */
    size_t payload_idx = 0;

public:
    /**
     * Constructor
     * 
     * @param src input source
     */
    masked_payload_source(sl::io::span<const char> payload_view, uint32_t mask_val) :
    payload(payload_view),
    mask(mask_val) { }

    /**
     * Copy constructor
     * 
     * @param other instance
     */
    masked_payload_source(const masked_payload_source& other) :
    payload(other.payload),
    mask(other.mask),
    payload_idx(other.payload_idx) { }

    /**
     * Copy assignment operator
     * 
     * @param other instance
     * @return this instance 
     */
    masked_payload_source& operator=(const masked_payload_source& other) {
        payload = other.payload;
        mask = other.mask;
        payload_idx = other.payload_idx;
        return *this;
    }

    /**
     * Unmasking read implementation
     * 
     * @param span buffer span
     * @return number of bytes processed
     */
    std::streamsize read(sl::io::span<char> span) {
        size_t written = 0;
        for (size_t dest_idx = 0;
                written < span.size() && 
                payload_idx < payload.size();
                dest_idx++) {
            auto mask_idx = 3 - (payload_idx % 4);
            auto mask_byte = reinterpret_cast<uint8_t*>(std::addressof(mask))[mask_idx];
            char ch = payload.data()[payload_idx] ^ mask_byte;
            span[dest_idx] = ch;
            payload_idx += 1;
            written += 1;
        }
        if (written > 0 || payload_idx < payload.size()) {
            return static_cast<std::streamsize>(written);
        }
        return std::char_traits<char>::eof();
    }
};

} // namespace
}

#endif /* STATICLIB_WEBSOCKET_MASKED_PAYLOAD_SOURCE_HPP */

