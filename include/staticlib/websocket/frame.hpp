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
 * File:   frame.hpp
 * Author: alex
 *
 * Created on June 7, 2018, 12:25 PM
 */

#ifndef STATICLIB_WEBSOCKET_FRAME_HPP
#define STATICLIB_WEBSOCKET_FRAME_HPP

#include <cstdint>
#include <array>
#include <limits>

#include "staticlib/support.hpp"
#include "staticlib/endian.hpp"
#include "staticlib/io.hpp"

#include "staticlib/websocket/frame_type.hpp"
#include "staticlib/websocket/masked_payload_source.hpp"

namespace staticlib {
namespace websocket {

/*
  0                   1                   2                   3
  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 +-+-+-+-+-------+-+-------------+-------------------------------+
 |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
 |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
 |N|V|V|V|       |S|             |   (if payload len==126/127)   |
 | |1|2|3|       |K|             |                               |
 +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
 |     Extended payload length continued, if payload len == 127  |
 + - - - - - - - - - - - - - - - +-------------------------------+
 |                               |Masking-key, if MASK set to 1  |
 +-------------------------------+-------------------------------+
 | Masking-key (continued)       |          Payload Data         |
 +-------------------------------- - - - - - - - - - - - - - - - +
 :                     Payload Data continued ...                :
 + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
 |                     Payload Data continued ...                |
 +---------------------------------------------------------------+
 */

/**
 * This class represents a WebSocket frame. It doesn't own the underlying buffer.
 */
class frame {
    /**
     * Span that points to the buffer, that contains one or
     * more WebSocket frames.
     */
    sl::io::span<const char> view;

    /**
     * Flag used during the parsing to represent the
     * parsing failure
     */
    bool parsing = true;
    /**
     * Flag shows whether the frame is invalid (corrupted),
     * incomplete frames are deemed well-formed unless invalid
     * frame structure is detected
     */
    bool well_formed = true;
    /**
     * Flag shows that the frame was successfully parsed
     * and can be used in the application
     */
    bool complete = false;

    /**
     * Flag shows whether `final` bit is set in frame
     */
    bool final = false;
    /**
     * Frame type
     */
    frame_type ftype = frame_type::invalid;
    /**
     * Length of the payload as specified in header,
     * only valid for values under 126 bytes
     */
    uint8_t payload_len_7 = 0;
    /**
     * Size of the extended payload length field (0, 2 or 8 bytes)
     */
    uint8_t ex_payload_len_field_size = 0;
    /**
     * Payload length
     */
    uint32_t payload_len = 0;
    /**
     * Flag shows whether this frame is masked (came from client)
     */
    bool masked = false;
    /**
     * Mask value
     */
    uint32_t mask = 0;

    /**
     * Size of the mandatory frame header
     */
    static const size_t prefix_len = 2;

public:
    /**
     * Constructor, takes a span that should point to a buffer
     * containing one or more frames. Buffer must remain valid during all
     * the frame use.
     * 
     * @param data_view span pointing to buffer with one or more frames
     */
    frame(sl::io::span<const char> data_view) :
    view(data_view) {
        check_min_len();
        parse_final();
        parse_opcode();
        parse_payload_7();
        parse_payload_16();
        parse_payload_64();
        parse_mask();
        check_complete();
    }

    /**
     * Flag shows whether the frame is invalid (corrupted),
     * incomplete frames are deemed well-formed unless invalid
     * frame structure is detected
     * 
     * @return `true` if frame is valid, `false` otherwise
     */
    bool is_well_formed() {
        return well_formed;
    }

    /**
     * Flag shows that the frame was successfully parsed
     * and can be used in the application
     * 
     * @return whether parsing completed successfully
     */
    bool is_complete() {
        return complete;
    }

    /**
     * Flag shows whether `final` bit is set in frame
     * 
     * @return whether `final` bit is set in frame
     */
    bool is_final() {
        return final;
    }

    /**
     * Frame type (opcode)
     * 
     * @return opcode
     */
    frame_type type() {
        return ftype;
    }

    /**
     * Flag shows whether this frame is masked (came from client)
     * 
     * @return `true`if masked, `false` otherwise
     */
    bool is_masked() {
        return masked;
    }

    /**
     * Pointer to the underlying buffer
     * 
     * @return pointer to the underlying buffer
     */
    const char* data() {
        return view.data();
    }

    /**
     * Size of this frame (header + payload)
     * 
     * @return frame size for complete frames, zero for incomplete ones
     */
    uint32_t size() {
        return static_cast<uint32_t>(complete ? payload_pos() + payload_len : 0);
    }

    /**
     * Mask value
     * 
     * @return mask value
     */
    uint32_t mask_value() {
        return mask;
    }

    /**
     * Payload length in bytes
     * 
     * @return payload length in bytes
     */
    uint32_t payload_length() {
        return payload_len;
    }

    /**
     * Span that points to the header, for incomplete
     * frames - points to the part of the header
     * that is available
     * 
     * @return span that points to the header
     */
    sl::io::span<const char> header() {
        auto ppos = payload_pos();
        auto len = ppos <= view.size() ? ppos : view.size();
        return sl::io::make_span(view.data(), len);
    }

    /**
     * Header in Hexadecimal format
     * 
     * @return header in Hexadecimal format
     */
    std::string header_hex() {
        auto head = header();
        auto st = std::string(head.data(), head.size());
        return sl::io::hex_from_string(st);
    }

    /**
     * Span that points to the (possibly masked) payload
     * 
     * @return payload span
     */
    sl::io::span<const char> payload() {
        if(well_formed && complete) {
            return sl::io::make_span(view.data() + payload_pos(), payload_len);
        } else {
            return sl::io::span<const char>(nullptr, 0);
        }
    }

    /**
     * Source, that allows to read the unmasked payload
     * 
     * @return unmasked payload source
     */
    masked_payload_source payload_unmasked() {
        auto span = complete ?
            sl::io::make_span(view.data() + payload_pos(), payload_len) :
            sl::io::span<const char>(nullptr, 0);
        return masked_payload_source(span, mask);
    }

    /**
     * Creates a frame header and writes it into the specified buffer
     * 
     * @param buf dest buffer
     * @param fr_type frame type
     * @param pl_len length of the payload that will be sent with this frame
     * @param masked whether payload will be masked
     * @param partial whether the `final` bit needs to be set to `0`
     * @return header span that points to dest buffer
     */
    static sl::io::span<char> make_header(std::array<char, 10>& buf, frame_type fr_type, size_t pl_len,
            bool masked = false, bool partial = false) {
        uint8_t mask_byte = !masked ? 0 : (1<<7);
        buf[0] = (!partial ? (1<<7) : 0) | static_cast<uint8_t>(fr_type);
        if (pl_len < (1<<7) - 2) {
            buf[1] = static_cast<char>(mask_byte | pl_len);
            return sl::io::make_span(buf.data(), 2);
        } else {
            auto sink = sl::io::memory_sink({buf.data() + 2, buf.size() - 2});
            if(pl_len < (1<<16)) {
                auto val = mask_byte | ((1<<7) - 2);
                buf[1] = static_cast<uint8_t>(val);
                sl::endian::write_16_be(sink, pl_len);
                return sl::io::make_span(buf.data(), 4);
            } else {
                auto val = mask_byte | ((1<<7) - 1);
                buf[1] = static_cast<uint8_t>(val);
                sl::endian::write_64_be(sink, pl_len);
                return sl::io::make_span(buf.data(), buf.size());
            }
        }
    }

private:

    void check_min_len() {
        if (view.size() < prefix_len) {
            this->parsing = false;
        }
    }
    
    void parse_final() {
        if (parsing) {
            this->final = 1 == ((view[0] >> 7) & 0x01);
        }
    }

    void parse_opcode() {
        if (parsing) {
            uint8_t opcode = view[0] & 0x0F;
            auto tp = make_frame_type(opcode);
            if (frame_type::invalid != tp) {
                this->ftype = tp;
            } else {
                this->parsing = false;
                this->well_formed = false;
            }
        }
    }

    void parse_payload_7() {
        if (parsing) {
            this->payload_len_7 = view[1] & 0x7F;
            if (payload_len_7 < 126) {
                this->payload_len = payload_len_7;
            }
        }
    }

    void parse_payload_16() {
        if (parsing && 126 == payload_len_7) {
            if (view.size() >= 4) {
                auto src = sl::io::array_source(view.data() + prefix_len, 2);
                this->payload_len = sl::endian::read_16_be<uint32_t>(src);
                this->ex_payload_len_field_size = 2;
            } else {
                this->parsing = false;
            }
        }
    }

    void parse_payload_64() {
        if (parsing && 127 == payload_len_7) {
            if (view.size() >= 10) {
                auto src = sl::io::array_source(view.data() + prefix_len, 8);
                auto p64 = sl::endian::read_64_be<uint64_t>(src);
                if (p64 < std::numeric_limits<int32_t>::max() -
                        (prefix_len + 8 + sizeof(mask))) {
                    this->payload_len = static_cast<uint32_t>(p64);
                    this->ex_payload_len_field_size = 8;
                } else {
                    this->parsing = false;
                    this->well_formed = false;
                }
            } else {
                this->parsing = false;
            }
        }
    }

    void parse_mask() {
        if (parsing) {
            this->masked = 1 == ((view[1] >> 7) & 0x01);
            if (masked && view.size() >= prefix_len + ex_payload_len_field_size + 4) {
                auto src = sl::io::array_source(view.data() + prefix_len + ex_payload_len_field_size, 4);
                this->mask = sl::endian::read_32_be<uint32_t>(src);
                if (mask > 0) {
                    // no-op
                } else {
                    this->parsing = false;
                    this->well_formed = false;
                }
            }
        }
    }

    void check_complete() {
        if (parsing && view.size() >= prefix_len + ex_payload_len_field_size + mask_length() + payload_len) {
            this->complete = true;
        }
    }

    uint8_t mask_length() {
        return static_cast<uint8_t>(masked ? 4 : 0);
    }

    size_t payload_pos() {
        return prefix_len + ex_payload_len_field_size + mask_length();
    }

};

} // namespace
}

#endif /* STATICLIB_WEBSOCKET_FRAME_HPP */

