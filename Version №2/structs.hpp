#pragma once
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>


static constexpr int RING_SIZE = 50;
static constexpr int BINARY_PORT = 9001;
static constexpr int HTTP_PORT = 8080;


struct Sample {
    double value{};
    uint64_t timestamp;
};


#pragma pack(push, 1)
struct TelemetryMessage {
    uint8_t device_id{};
    float value{};
    uint64_t timestamp{};
    uint8_t crc{};
};
#pragma pack(pop)


struct DeviceData {
    Sample buffer[RING_SIZE];  
    int head = 0;              
    int count = 0;             
    Sample latest;             
    
    
    void add_sample(double value, uint64_t timestamp) {
        Sample sample;
        sample.value = value;
        sample.timestamp = timestamp;
        
        buffer[head] = sample;
        head = (head + 1) % RING_SIZE;
        if (count < RING_SIZE) {
            count++;
        }
        latest = sample;
    }
    
    
    bool get_stats(double& min_val, double& max_val, double& average) const {
        if (count == 0) return false;
        
        min_val = max_val = buffer[0].value;
        double sum = 0;
        
        for (int i = 0; i < count; i++) {
            double val = buffer[i].value;
            if (val < min_val) min_val = val;
            if (val > max_val) max_val = val;
            sum += val;
        }
        
        average = sum / count;
        return true;
    }
};