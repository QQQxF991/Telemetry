#include "binary_message.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <vector>
#include <regex>
#include <sstream>
#include <iomanip>
#include <algorithm>

void BynaryServer() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return;
    }
    
    binary_listen_socket = server_fd;
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Ошибка setsockopt" << std::endl;
        close(server_fd);
        return;
    }
    
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(BINARY_PORT);
    
    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        close(server_fd);
        return;
    }
    
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Ошибка listen" << std::endl;
        close(server_fd);
        return;
    }
    
    std::cout << "Бинарный сервер запущен на порту " << BINARY_PORT << std::endl;
    
    while (running) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_fd, (sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Ошибка accept" << std::endl;
            }
            break;
        }
        
        std::thread([client_socket]() {
            std::vector<uint8_t> buffer;
            buffer.reserve(1024);
            
            while (running) {
                uint8_t temp[1024];
                ssize_t bytes_read = read(client_socket, temp, sizeof(temp));
                
                if (bytes_read <= 0) {
                    break;  
                }
                
                buffer.insert(buffer.end(), temp, temp + bytes_read);
                
                while (buffer.size() >= 14) {
                    uint8_t message[14];
                    std::copy_n(buffer.begin(), 14, message);
                    process_message(message);
                    buffer.erase(buffer.begin(), buffer.begin() + 14);
                }
            }
            
            close(client_socket);
        }).detach();
    }
    
    close(server_fd);
    binary_listen_socket = -1;
}

void HTTP_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Ошибка создания сокета HTTP" << std::endl;
        return;
    }
    
    http_listen_socket = server_fd;
    
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Ошибка setsockopt HTTP" << std::endl;
        close(server_fd);
        return;
    }
    
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(HTTP_PORT);
    
    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Ошибка привязки сокета HTTP" << std::endl;
        close(server_fd);
        return;
    }
    
    if (listen(server_fd, 10) < 0) {
        std::cerr << "Ошибка listen HTTP" << std::endl;
        close(server_fd);
        return;
    }
    
    std::cout << "HTTP сервер запущен на порту " << HTTP_PORT << std::endl;
    
    std::regex re_latest(R"(^/device/(\d{1,3})/latest$)");
    std::regex re_stats(R"(^/device/(\d{1,3})/stats$)");
    
    while (running) {
        int client_socket = accept(server_fd, nullptr, nullptr);
        if (client_socket < 0) {
            if (running) {
                std::cerr << "Ошибка accept HTTP" << std::endl;
            }
            break;
        }
        
        std::thread([client_socket, re_latest, re_stats]() {
            char request[4096];
            ssize_t bytes_read = read(client_socket, request, sizeof(request) - 1);
            
            if (bytes_read <= 0) {
                close(client_socket);
                return;
            }
            
            request[bytes_read] = '\0';
            
            std::istringstream req_stream(request);
            std::string method, path, version;
            req_stream >> method >> path >> version;
            
            std::string response;
            std::smatch match;
            
            if (method != "GET") {
                response = "HTTP/1.1 405 Method Not Allowed\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: 0\r\n\r\n";
                write(client_socket, response.c_str(), response.size());
                close(client_socket);
                return;
            }

            std::lock_guard<std::mutex> lock(devices_mutex);
            
            if (std::regex_match(path, match, re_latest)) {
                int device_id = std::stoi(match[1].str());
                
                auto it = devices.find(device_id);
                if (it == devices.end() || it->second.count == 0) {
                    std::string body = "{\"error\": \"No data available for device " + 
                                       std::to_string(device_id) + "\"}";
                    response = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
                } else {
                    const DeviceData& device = it->second;
                    const Sample& latest = device.latest;
                    
                    std::ostringstream json;
                    json << std::fixed << std::setprecision(6)
                         << "{\"device_id\": " << device_id
                         << ", \"value\": " << latest.value
                         << ", \"timestamp\": " << latest.timestamp << "}";
                    
                    std::string body = json.str();
                    response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
                }
            } else if (std::regex_match(path, match, re_stats)) {
                int device_id = std::stoi(match[1].str());
                
                auto it = devices.find(device_id);
                if (it == devices.end() || it->second.count == 0) {
                    std::string body = "{\"error\": \"No data available for device " + 
                                       std::to_string(device_id) + "\"}";
                    response = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
                } else {
                    const DeviceData& device = it->second;
                    double min_val, max_val, avg;
                    
                    if (device.get_stats(min_val, max_val, avg)) {
                        std::ostringstream json;
                        json << std::fixed << std::setprecision(6)
                             << "{\"device_id\": " << device_id
                             << ", \"min\": " << min_val
                             << ", \"max\": " << max_val
                             << ", \"average\": " << avg
                             << ", \"count\": " << device.count << "}";
                        
                        std::string body = json.str();
                        response = "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
                    } else {
                        std::string body = "{\"error\": \"Failed to calculate statistics\"}";
                        response = "HTTP/1.1 500 Internal Server Error\r\n"
                                  "Content-Type: application/json\r\n"
                                  "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
                    }
                }
            } else {
                std::string body = "{\"error\": \"Not Found\", \"message\": \"Use /device/{id}/latest or /device/{id}/stats\"}";
                response = "HTTP/1.1 404 Not Found\r\n"
                          "Content-Type: application/json\r\n"
                          "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
            }
            
            write(client_socket, response.c_str(), response.size());
            close(client_socket);
        }).detach();
    }
    
    close(server_fd);
    http_listen_socket = -1;
}