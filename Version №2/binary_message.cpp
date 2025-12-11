#include "binary_message.hpp"
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <iomanip>
#include <sstream>
#include <chrono>


std::atomic<bool> running{true};
std::unordered_map<uint8_t, DeviceData> devices;
std::mutex devices_mutex;
int binary_listen_socket = -1;
int http_listen_socket = -1;


uint8_t calculate_crc8(const uint8_t* data, size_t length) {
    uint8_t crc = 0;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
    }
    return crc;
}

bool parse_binary_message(const uint8_t* data, uint8_t& device_id, 
                         float& value, uint64_t& timestamp) {
   
    uint8_t calculated_crc = calculate_crc8(data, 13);  
    if (calculated_crc != data[13]) {
        std::cerr << "Ошибка CRC: ожидалось " << (int)calculated_crc 
                  << ", получено " << (int)data[13] << std::endl;
        return false;
    }
    device_id = data[0];
    uint32_t temp;
    std::memcpy(&temp, &data[1], 4);
    temp = ntohl(temp);
    std::memcpy(&value, &temp, 4);
    uint64_t ts_temp = 0;
    for (int i = 0; i < 8; i++) {
        ts_temp = (ts_temp << 8) | data[5 + i];
    }
    timestamp = ts_temp;
    return true;
}


void process_message(const uint8_t* msg) {
    uint8_t device_id;
    float value;
    uint64_t timestamp;
    
    if (!parse_binary_message(msg, device_id, value, timestamp)) {
        return;  
    }
    
    std::lock_guard<std::mutex> lock(devices_mutex);
    
    devices[device_id].add_sample(static_cast<double>(value), timestamp);
    
   
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::cout << std::put_time(std::localtime(&now_time_t), "%H:%M:%S")
              << " Обработано: device=" << (int)device_id
              << ", value=" << std::fixed << std::setprecision(6) << value
              << ", timestamp=" << timestamp
              << ", буфер: " << devices[device_id].count << "/" << RING_SIZE 
              << std::endl;
}