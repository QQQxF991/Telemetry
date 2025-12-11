#include <cstdint>
#include "bynary_message.сpp"

static constexpr int RING_SIZE = {50};
static constexpr int BINARY_PORT = {9001};
static constexpr int HTTP_PORT = {8080};

struct Sample {
    double value {};
    uint64_t timestamp; 
};

struct TelemetryMessage{
    uint8_t device_id{};
    float big_endian{};
    uint64_t timestamp{};
    uint8_t CRC8 {};
};

struct DeviceData {
    Sample buffer[RING_SIZE];
    int head = {0};
    int count = {0};
    Sample latest;
};