#pragma once
#include "utils.hpp"
#include "data_store.hpp"
#include <cstdint>
#include <cstring>
#include <array>

class MessageParser {
private:
    static constexpr size_t MESSAGE_SIZE = 15;
    
public:
    struct ParsedMessage {
        uint8_t device_id;
        float value;
        uint64_t timestamp;
        bool valid;
    };
    
    static ParsedMessage parse(const uint8_t* data) {
        ParsedMessage msg{};
        msg.valid = false;
        
        uint8_t received_crc = data[14];
        uint8_t calculated_crc = utils::crc8_xor(data, 14);
        
        if (received_crc != calculated_crc) {
            return msg;
        }
        
        
        msg.device_id = data[0];
        

        uint32_t float_be = (static_cast<uint32_t>(data[1]) << 24) |
                           (static_cast<uint32_t>(data[2]) << 16) |
                           (static_cast<uint32_t>(data[3]) << 8) |
                           static_cast<uint32_t>(data[4]);
        msg.value = utils::be32tofloat(float_be);
     
        uint64_t timestamp_be = 0;
        for (int i = 0; i < 8; ++i) {
            timestamp_be = (timestamp_be << 8) | data[5 + i];
        }
        msg.timestamp = utils::be64tohost(timestamp_be);
        
        msg.valid = true;
        return msg;
    }
   
    static size_t process_buffer(const uint8_t* buffer, size_t size, 
                                std::array<uint8_t, MESSAGE_SIZE>& incomplete) {
        size_t processed = 0;
        size_t offset = 0;
        
  
        while (size - offset >= MESSAGE_SIZE) {
            ParsedMessage msg = parse(buffer + offset);
            if (msg.valid) {
                DataStore::instance().update(msg.device_id, msg.value, msg.timestamp);
                processed++;
            }
            offset += MESSAGE_SIZE;
        }
        
        if (offset < size) {
            size_t remaining = size - offset;
            std::memcpy(incomplete.data(), buffer + offset, remaining);
            std::memset(incomplete.data() + remaining, 0, MESSAGE_SIZE - remaining);
        } else {
            incomplete.fill(0);
        }
        
        return processed;
    }
};