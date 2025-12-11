#pragma once
#include "structs.hpp"
#include <atomic>
#include <memory>

extern std::atomic<bool> running;
extern std::unordered_map<uint8_t, DeviceData> devices;
extern std::mutex devices_mutex;
extern int binary_listen_socket;
extern int http_listen_socket;

uint8_t calculate_crc8(const uint8_t* data, size_t length);
bool parse_binary_message(const uint8_t* data, uint8_t& device_id, 
                         float& value, uint64_t& timestamp);
void process_message(const uint8_t* msg);
void BynaryServer();
void HTTP_server();
