#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <memory>
#include <functional>
#include "message_parser.hpp"

class BinaryServer {
private:
    std::atomic<bool> running{false};
    std::thread server_thread;
    int server_fd{-1};
    int port;
    
    class ClientHandler {
    private:
        int client_fd;
        std::array<uint8_t, MessageParser::MESSAGE_SIZE> incomplete_msg{};
        
    public:
        ClientHandler(int fd) : client_fd(fd) {}
        
        void run() {
            std::vector<uint8_t> buffer;
            buffer.reserve(4096);
            
            while (true) {
                uint8_t temp[1024];
                ssize_t bytes = read(client_fd, temp, sizeof(temp));
                
                if (bytes <= 0) {
                    break;
                }
                
                buffer.insert(buffer.end(), temp, temp + bytes);
                
                size_t processed = MessageParser::process_buffer(
                    buffer.data(), buffer.size(), incomplete_msg);
                
                size_t to_remove = processed * MessageParser::MESSAGE_SIZE;
                if (to_remove < buffer.size()) {
                    std::memmove(buffer.data(), 
                                buffer.data() + to_remove, 
                                buffer.size() - to_remove);
                    buffer.resize(buffer.size() - to_remove);
                } else {
                    buffer.clear();
                }
            }
            
            close(client_fd);
        }
    };
    
public:
    BinaryServer(int port_num) : port(port_num) {}
    
    ~BinaryServer() {
        stop();
    }
    
    void start() {
        if (running) return;
        running = true;
        server_thread = std::thread(&BinaryServer::run, this);
    }
    
    void stop() {
        if (!running) return;
        
        running = false;
        
        if (server_fd >= 0) {
            shutdown(server_fd, SHUT_RDWR);
            close(server_fd);
            server_fd = -1;
        }
        
        if (server_thread.joinable()) {
            server_thread.join();
        }
    }
    
private:
    void run() {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("socket");
            return;
        }
        
     
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
    
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
       
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(server_fd);
            return;
        }
        
        if (listen(server_fd, 10) < 0) {
            perror("listen");
            close(server_fd);
            return;
        }
        
        std::cout << "Binary server listening on port " << port << std::endl;
        
        while (running) {
            struct sockaddr_in client_addr{};
            socklen_t client_len = sizeof(client_addr);
            
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd < 0) {
                if (errno == EWOULDBLOCK || errno == EAGAIN) {
                    
                    continue;
                }
                if (running) {
                    perror("accept");
                }
                break;
            }
            
           
            std::thread([client_fd]() {
                ClientHandler handler(client_fd);
                handler.run();
            }).detach();
        }
        
        if (server_fd >= 0) {
            close(server_fd);
            server_fd = -1;
        }
    }
};