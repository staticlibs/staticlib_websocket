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

#include "staticlib/websocket/frame.hpp"

#include <array>
#include <iostream>
#include <string>

#include "staticlib/config/assert.hpp"
#include "staticlib/io.hpp"

const std::string empty = sl::io::string_from_hex("8180be8b6908");
const std::string hi = sl::io::string_from_hex("81821875fdc8701c");
const std::string lorem_128 = sl::io::string_from_hex("81fe0080a2272042ee485227cf074932d1524d62c6484c2dd007532bd607412fc7530c62c1484e31c7445427d6525262c3434932cb54432bcc400027ce4e546e8254452682434f62c74e5531cf484462d6424d32cd55002bcc444926cb43552cd6075536824b4120cd554562c7530026cd4b4f30c7074d23c5494162c34b4933d7460e62f7530027");
const std::string lorem_128_plain = std::string("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut e");

std::string create_lorem_65664() {
    auto header = std::string("81ff00000000000100802d5e9603");
    auto lorem = std::string("6131e466407eff735e2bfb234931fa6c5f7ee56a597ef76e482aba234e31f870483de266592be4234c3aff73442df56a4339b6664137e22f0d2df3670d3af9234837e3704031f223593bfb73422cb66a433dff67443ae36d597ee3770d32f761422cf323482ab6674232f971487efb624a30f7234c32ff72583fb823782ab666");
    auto payload = std::string();
    for (size_t i = 0; i <= (1<<9); i++) {
        payload += lorem;
    }
    slassert(65664 == payload.length()/2);
    auto msg_hex = header + payload;
    return sl::io::string_from_hex(msg_hex);
}

void test_empty() {
    auto frame = sl::websocket::frame(empty);
    slassert(frame.is_well_formed());
    slassert(frame.is_complete());
    slassert(frame.is_final());
    slassert(frame.is_masked());
    slassert(sl::websocket::frame_type::text == frame.type());
    slassert(0xbe8b6908 == frame.mask_value());
    slassert(0 == frame.payload_length());
    slassert(6 == frame.size_bytes());
}

void test_payload_7() {
    auto frame = sl::websocket::frame(hi);
    slassert(frame.is_well_formed());
    slassert(frame.is_complete());
    slassert(frame.is_final());
    slassert(frame.is_masked());
    slassert(0x1875fdc8 == frame.mask_value());
    slassert(2 == frame.payload_length());
    slassert(8 == frame.size_bytes());
    slassert(2 == frame.payload_plain().size());
    auto src = frame.payload_unmasked();
    auto sink = sl::io::string_sink();
    sl::io::copy_all(src, sink);
    slassert("hi" == sink.get_string());
}

void test_payload_16() {
    auto frame = sl::websocket::frame(lorem_128);
    slassert(frame.is_well_formed());
    slassert(frame.is_complete());
    slassert(frame.is_final());
    slassert(frame.is_masked());
    slassert(0xa2272042 == frame.mask_value());
    slassert(128 == frame.payload_length());
    slassert(136 == frame.size_bytes());
    slassert(128 == frame.payload_plain().size());
    auto src = frame.payload_unmasked();
    auto sink = sl::io::string_sink();
    auto buf = std::array<char,2>();
    sl::io::copy_all(src, sink, buf);
    slassert(lorem_128_plain == sink.get_string());
}

void test_payload_64() {
    auto lorem = create_lorem_65664();
    auto frame = sl::websocket::frame(lorem);
    slassert(frame.is_well_formed());
    slassert(frame.is_complete());
    slassert(frame.is_final());
    slassert(frame.is_masked());
    slassert(0x2d5e9603 == frame.mask_value());
    slassert(65664 == frame.payload_length());
    slassert(65664 + 14 == frame.size_bytes());
    slassert(65664 == frame.payload_plain().size());
    auto src = frame.payload_unmasked();
    auto buf = std::array<char, 128>();
    for (size_t i = 0; i <= (1<<9); i++) {
        src.read(buf);
        slassert(lorem_128_plain == std::string(buf.data(), buf.size()));
    }
}

void check_incomplete(const std::string& hex) {
    auto st = sl::io::string_from_hex(hex);
    auto frame = sl::websocket::frame(st);
    slassert(frame.is_well_formed())
    slassert(!frame.is_complete())
}

void test_incomplete() {
    // empty
    check_incomplete("");
    check_incomplete("81");
    check_incomplete("8180");
    check_incomplete("8180be");
    check_incomplete("8180be8b");
    check_incomplete("8180be8b69");
    // payload_7
    check_incomplete("818218");
    check_incomplete("81821875");
    check_incomplete("81821875fd");
    check_incomplete("81821875fdc8");
    check_incomplete("81821875fdc870");
    // payload_16
    check_incomplete("81fe0080");
    check_incomplete("81fe0080a2");
    check_incomplete("81fe0080a227");
    check_incomplete("81fe0080a22720");
    check_incomplete("81fe0080a2272042");
    check_incomplete("81fe0080a2272042ee");
}

void check_not_well_formed(const std::string& hex) {
    auto st = sl::io::string_from_hex(hex);
    auto frame = sl::websocket::frame(st);
    slassert(!frame.is_well_formed())
}

void test_not_well_formed() {
    check_not_well_formed("8080be8b6908");
    check_not_well_formed("8380be8b6908");
    check_not_well_formed("8b80be8b6908");
    check_not_well_formed("818000000000");
}

int main() {
    try {
        test_empty();
        test_payload_7();
        test_payload_16();
        test_payload_64();
        test_incomplete();
        test_not_well_formed();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return 1;
    }
    return 0;
}
