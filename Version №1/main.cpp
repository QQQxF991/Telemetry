#include "config.hpp"
#include "binary_server.hpp"
#include "http_server.hpp"
#include <iostream>
#include <csignal>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdlib>

std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", shutting down..." << std::endl;
    shutdown_requested.store(true);
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n";
    std::cout << "Options:\n";
    std::cout << "  --binary-port=<port>     Binary server port (default: 9001)\n";
    std::cout << "  --http-port=<port>       HTTP server port (default: 8080)\n";
    std::cout << "  --ring-size=<size>       Ring buffer size (default: 50)\n";
    std::cout << "  --cleanup-age=<seconds>  Cleanup data older than seconds (0 = disabled)\n";
    std::cout << "  --help                   Show this help message\n";
    std::cout << "\nHTTP Endpoints:\n";
    std::cout << "  GET /device/{id}/latest   Latest value for device\n";
    std::cout << "  GET /device/{id}/stats    Statistics for device\n";
    std::cout << "  GET /devices              List active devices\n";
    std::cout << "  GET /health               Health check\n";
    std::cout << "  GET /metrics              Service metrics\n";
}

int main(int argc, char* argv[]) {

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    Config config;
    if (!config.parse_args(argc, argv)) {
        return 1;
    }
    
    if (config.has("help") || config.has("h")) {
        print_usage(argv[0]);
        return 0;
    }
    
    int binary_port = config.get_int("binary-port", BINARY_PORT);
    int http_port = config.get_int("http-port", HTTP_PORT);
    int cleanup_age = config.get_int("cleanup-age", 0);
    
    std::cout << "=== Telemetry Service ===\n";
    std::cout << "Binary port: " << binary_port << "\n";
    std::cout << "HTTP port: " << http_port << "\n";
    std::cout << "Ring buffer size: " << RING_SIZE << "\n";
    std::cout << "Cleanup age: " << (cleanup_age > 0 ? std::to_string(cleanup_age) + "s" : "disabled") << "\n";
    std::cout << "=========================\n";
    
    try {
        BinaryServer binary_server(binary_port);
        HttpServer http_server(http_port);
        
        binary_server.start();
        http_server.start();
        
        std::cout << "Service started. Press Ctrl+C to stop.\n";
        
        while (!shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            
            static int cleanup_counter = 0;
            if (cleanup_age > 0 && ++cleanup_counter >= 60) {
                DataStore::instance().cleanup_old(cleanup_age);
                cleanup_counter = 0;
                
                auto stats = DataStore::instance().get_global_stats();
                std::cout << "Cleanup completed. Active devices: " 
                          << stats.active_devices 
                          << ", Total samples: " << stats.total_samples << "\n";
            }
        }
        
        std::cout << "Shutting down servers...\n";
        http_server.stop();
        binary_server.stop();
        auto final_stats = DataStore::instance().get_global_stats();
        std::cout << "\n=== Final Statistics ===\n";
        std::cout << "Total updates processed: " << final_stats.total_updates << "\n";
        std::cout << "Total samples stored: " << final_stats.total_samples << "\n";
        std::cout << "Active devices: " << final_stats.active_devices << "\n";
        std::cout << "Total devices registered: " << final_stats.total_devices << "\n";
        std::cout << "=========================\n";
        std::cout << "Service stopped.\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}