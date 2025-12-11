#pragma once
#include <cstdint>
#include <cstring>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace utils {

    inline uint32_t swap_uint32(uint32_t val) {
        return ((val & 0x000000FF) << 24) |
               ((val & 0x0000FF00) << 8) |
               ((val & 0x00FF0000) >> 8) |
               ((val & 0xFF000000) >> 24);
    }
    
    inline uint64_t swap_uint64(uint64_t val) {
        return ((val & 0x00000000000000FFULL) << 56) |
               ((val & 0x000000000000FF00ULL) << 40) |
               ((val & 0x0000000000FF0000ULL) << 24) |
               ((val & 0x00000000FF000000ULL) << 8) |
               ((val & 0x000000FF00000000ULL) >> 8) |
               ((val & 0x0000FF0000000000ULL) >> 24) |
               ((val & 0x00FF000000000000ULL) >> 40) |
               ((val & 0xFF00000000000000ULL) >> 56);
    }
    
    constexpr bool is_little_endian() {
        const uint32_t test = 0x12345678;
        return reinterpret_cast<const uint8_t*>(&test)[0] == 0x78;
    }
    
    inline float be32tofloat(uint32_t be) {
        uint32_t native = is_little_endian() ? swap_uint32(be) : be;
        float result;
        std::memcpy(&result, &native, sizeof(float));
        return result;
    }
    
    inline uint64_t be64tohost(uint64_t be) {
        return is_little_endian() ? swap_uint64(be) : be;
    }
    
    inline uint64_t current_time_millis() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }
  
    inline std::string json_escape(const std::string& s) {
        std::ostringstream o;
        for (auto c : s) {
            switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    } else {
                        o << c;
                    }
            }
        }
        return o.str();
    }
    
    constexpr bool is_valid_device_id(uint8_t id) {
        return id >= 0 && id <= 255;
    }
    
    inline uint8_t crc8_xor(const uint8_t* data, size_t len) {
        uint8_t crc = 0;
        for (size_t i = 0; i < len; ++i) {
            crc ^= data[i];
        }
        return crc;
    }
}