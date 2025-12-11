#pragma once
#include <cstdint>

static constexpr int RING_SIZE = 50;
static constexpr int BINARY_PORT = 9001;
static constexpr int HTTP_PORT = 8080;

struct Sample {
    double value{};
    uint64_t timestamp; 
};

struct DeviceData {
    Sample buffer[RING_SIZE];
    int head = 0;
    int count = 0;
    Sample latest;
};