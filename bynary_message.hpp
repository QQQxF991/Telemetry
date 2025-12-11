#include <atomic>
#include <string>
#include <unordered_map>
#include <mutex>
#include <thread>
#include "structs.cpp"

#include <atomic>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#if __has_include(<endian.h>)
    #include <endian.h>
    #define HAVE_ENDIAN_H 1
#endif

class Message final {
    public:
        Message () {};

        void process_message(const uint8_t msg[15]) {
            const uint8_t CRC = msg[14];
            if (CRC8_XOR(msg,14) != CRC) {
                throw std::runtime_error("Invalid CRC!"); 
                return;
            }
            
            uint8_t device_id = msg[0];
            uint32_t BigEndian_Float32 = (uint32_t(msg[1]) << 24) |
                                        (uint32_t(msg[2])<< 16) |
                                        (uint32_t(msg[3])<<8) |
                                        uint8_t(msg[4]);
            uint32_t Host_Float32 = ntohl(BigEndian_Float32);
            float F;
            static_assert(sizeof(float) == 4, "Float must be 4 bytes");
            std::memcpy(&F,&Host_Float32,4);

            uint64_t BigEndian_TimeStamp = {0};
            for (int i = 0; i < 8; ++i) {
                BigEndian_TimeStamp = (BigEndian_TimeStamp << 8) | msg[5+i];
            }
            uint64_t Time_Stamp = host_u64(BigEndian_TimeStamp);
            std::lock_guard<std::mutex> lock_ (_devices_mutex_);
            DeviceData &D = devices[device_id];
            D.buffer[D.head].value =(double)F;
            D.buffer[D.head].timestamp = Time_Stamp;
            D.head = (D.head + 1) % RING_SIZE;
            if (D.count < RING_SIZE) ++D.count;
        }

        ~Message () {
            running = {false};
        }
    private:
        std::atomic<bool> running = {true};
        std::unordered_map<uint8_t,DeviceData> devices;
        std::mutex _devices_mutex_ ; 

        uint8_t CRC8_XOR (const uint8_t *data,size_t len){
            uint8_t CRC = {0};
            for (size_t i = 0; i < len; ++i) {
                CRC ^= data[i];
            }
            return CRC;
        };      
        
    uint64_t host_u64(uint64_t be) {
    #ifdef HAVE_ENDIAN_H
        return be64toh(be);  
    #elif defined(__GNUC__) || defined(__clang__)
        #if __BYTE_ORDER == __LITTLE_ENDIAN
            return __builtin_bswap64(be);
        #else
            return be;
        #endif
    #else
        return ((be & 0xFF00000000000000ULL) >> 56) |
            ((be & 0x00FF000000000000ULL) >> 40) |
            ((be & 0x0000FF0000000000ULL) >> 24) |
            ((be & 0x000000FF00000000ULL) >> 8)  |
            ((be & 0x00000000FF000000ULL) << 8)  |
            ((be & 0x0000000000FF0000ULL) << 24) |
            ((be & 0x000000000000FF00ULL) << 40) |
            ((be & 0x00000000000000FFULL) << 56);
    #endif
    }
};