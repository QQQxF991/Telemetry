#pragma once
#include "structs.hpp"
#include "utils.hpp"
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <vector>
#include <algorithm>
#include <limits>

class DataStore {
private:
    mutable std::shared_mutex mutex;
    std::unordered_map<uint8_t, DeviceData> devices;
    std::atomic<uint64_t> total_samples{0};
    std::atomic<uint64_t> total_updates{0};
    
    struct DeviceStats {
        uint8_t id;
        int sample_count;
        uint64_t last_update;
    };
    
public:
    static DataStore& instance() {
        static DataStore instance;
        return instance;
    }
    
    bool update(uint8_t device_id, float value, uint64_t timestamp) {
        if (!utils::is_valid_device_id(device_id)) {
            return false;
        }
        
        std::unique_lock lock(mutex);
        DeviceData& device = devices[device_id];
        
        uint64_t now = utils::current_time_millis();
        if (timestamp > now + 60000) { 
            return false;
        }
        
        int index = device.head;
        device.buffer[index].value = static_cast<double>(value);
        device.buffer[index].timestamp = timestamp;
        
       
        device.latest.value = static_cast<double>(value);
        device.latest.timestamp = timestamp;
        
        
        device.head = (device.head + 1) % RING_SIZE;
        if (device.count < RING_SIZE) {
            device.count++;
        }
        
        total_samples.fetch_add(1, std::memory_order_relaxed);
        total_updates.fetch_add(1, std::memory_order_relaxed);
        
        return true;
    }
    
    std::optional<Sample> get_latest(uint8_t device_id) const {
        if (!utils::is_valid_device_id(device_id)) {
            return std::nullopt;
        }
        
        std::shared_lock lock(mutex);
        auto it = devices.find(device_id);
        if (it == devices.end() || it->second.count == 0) {
            return std::nullopt;
        }
        return it->second.latest;
    }
    
    struct Statistics {
        double min;
        double max;
        double average;
        int count;
        uint64_t oldest_timestamp;
        uint64_t newest_timestamp;
    };
    
    std::optional<Statistics> get_stats(uint8_t device_id) const {
        if (!utils::is_valid_device_id(device_id)) {
            return std::nullopt;
        }
        
        std::shared_lock lock(mutex);
        auto it = devices.find(device_id);
        if (it == devices.end() || it->second.count == 0) {
            return std::nullopt;
        }
        
        const DeviceData& device = it->second;
        Statistics stats{};
        stats.count = device.count;
        
        if (stats.count == 0) {
            return stats;
        }
        
    
        int first_idx = (device.head - 1 + RING_SIZE) % RING_SIZE;
        double first_val = device.buffer[first_idx].value;
        stats.min = first_val;
        stats.max = first_val;
        double sum = first_val;
        
        stats.oldest_timestamp = device.buffer[first_idx].timestamp;
        stats.newest_timestamp = device.buffer[first_idx].timestamp;
        
 
        for (int i = 0; i < stats.count; ++i) {
            int idx = (device.head - 1 - i + RING_SIZE) % RING_SIZE;
            const Sample& sample = device.buffer[idx];
            
            if (sample.value < stats.min) stats.min = sample.value;
            if (sample.value > stats.max) stats.max = sample.value;
            sum += sample.value;
            
            if (sample.timestamp < stats.oldest_timestamp) {
                stats.oldest_timestamp = sample.timestamp;
            }
            if (sample.timestamp > stats.newest_timestamp) {
                stats.newest_timestamp = sample.timestamp;
            }
        }
        
        stats.average = sum / stats.count;
        return stats;
    }
    
 
    std::vector<uint8_t> get_active_devices() const {
        std::shared_lock lock(mutex);
        std::vector<uint8_t> result;
        result.reserve(devices.size());
        
        for (const auto& [id, data] : devices) {
            if (data.count > 0) {
                result.push_back(id);
            }
        }
        
        std::sort(result.begin(), result.end());
        return result;
    }
    
   
    struct GlobalStats {
        uint64_t total_samples;
        uint64_t total_updates;
        size_t active_devices;
        size_t total_devices;
    };
    
    GlobalStats get_global_stats() const {
        GlobalStats stats{};
        stats.total_samples = total_samples.load(std::memory_order_relaxed);
        stats.total_updates = total_updates.load(std::memory_order_relaxed);
        
        std::shared_lock lock(mutex);
        stats.total_devices = devices.size();
        stats.active_devices = std::count_if(devices.begin(), devices.end(),
            [](const auto& pair) { return pair.second.count > 0; });
        
        return stats;
    }
    
    void cleanup_old(uint64_t max_age_seconds) {
        if (max_age_seconds == 0) return;
        
        std::unique_lock lock(mutex);
        uint64_t now = utils::current_time_millis();
        uint64_t cutoff = now - (max_age_seconds * 1000);
        
        for (auto& [id, device] : devices) {
            int valid_count = 0;
            for (int i = 0; i < device.count; ++i) {
                int idx = (device.head - 1 - i + RING_SIZE) % RING_SIZE;
                if (device.buffer[idx].timestamp >= cutoff) {
                    valid_count++;
                } else {
                    break;
                }
            }
            device.count = valid_count;
            
            if (valid_count == 0) {
                device.head = 0;
            }
        }
    }
    
private:
    DataStore() = default;
    ~DataStore() = default;
    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;
};