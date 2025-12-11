#pragma once
#include <atomic>
#include <thread>
#include <regex>
#include <functional>
#include <unordered_map>
#include "data_store.hpp"

class HttpServer {
private:
    std::atomic<bool> running{false};
    std::thread server_thread;
    int server_fd{-1};
    int port;
    
    class RequestHandler {
    private:
        int client_fd;
        
        enum class Status {
            OK = 200,
            BAD_REQUEST = 400,
            NOT_FOUND = 404,
            METHOD_NOT_ALLOWED = 405,
            TOO_MANY_REQUESTS = 429,
            INTERNAL_ERROR = 500
        };
        
        enum class Method {
            GET,
            POST,
            UNKNOWN
        };
        
        Method parse_method(const std::string& method_str) {
            if (method_str == "GET") return Method::GET;
            if (method_str == "POST") return Method::POST;
            return Method::UNKNOWN;
        }
        
        std::string status_to_string(Status status) {
            switch (status) {
                case Status::OK: return "200 OK";
                case Status::BAD_REQUEST: return "400 Bad Request";
                case Status::NOT_FOUND: return "404 Not Found";
                case Status::METHOD_NOT_ALLOWED: return "405 Method Not Allowed";
                case Status::TOO_MANY_REQUESTS: return "429 Too Many Requests";
                case Status::INTERNAL_ERROR: return "500 Internal Server Error";
                default: return "500 Internal Server Error";
            }
        }
        
        void send_response(Status status, const std::string& content_type, 
                          const std::string& body) {
            std::ostringstream response;
            response << "HTTP/1.1 " << status_to_string(status) << "\r\n";
            response << "Content-Type: " << content_type << "\r\n";
            response << "Content-Length: " << body.size() << "\r\n";
            response << "Connection: close\r\n";
            response << "\r\n";
            response << body;
            
            std::string response_str = response.str();
            write(client_fd, response_str.c_str(), response_str.size());
        }
        
        void send_json(Status status, const std::string& json) {
            send_response(status, "application/json", json);
        }
        
        std::string create_error_json(const std::string& error) {
            std::ostringstream json;
            json << "{\"error\": \"" << utils::json_escape(error) << "\"}";
            return json.str();
        }
        
    public:
        RequestHandler(int fd) : client_fd(fd) {}
        
        void run() {
            char buffer[8192];
            ssize_t bytes = read(client_fd, buffer, sizeof(buffer) - 1);
            
            if (bytes <= 0) {
                close(client_fd);
                return;
            }
            
            buffer[bytes] = '\0';
            std::string request(buffer);
            
            std::istringstream stream(request);
            std::string method_str, path, version;
            stream >> method_str >> path >> version;
            
            Method method = parse_method(method_str);
            if (method == Method::UNKNOWN) {
                send_json(Status::METHOD_NOT_ALLOWED, 
                         create_error_json("Method not allowed"));
                close(client_fd);
                return;
            }
            
            handle_path(method, path);
            close(client_fd);
        }
        
    private:
        void handle_path(Method method, const std::string& path) {
            static const std::regex latest_regex(R"(^/device/(\d+)/latest$)");
            static const std::regex stats_regex(R"(^/device/(\d+)/stats$)");
            static const std::regex devices_regex(R"(^/devices$)");
            static const std::regex health_regex(R"(^/health$)");
            static const std::regex metrics_regex(R"(^/metrics$)");
            
            std::smatch matches;
            
            if (method != Method::GET) {
                send_json(Status::METHOD_NOT_ALLOWED,
                         create_error_json("Only GET method is allowed"));
                return;
            }
            
            if (std::regex_match(path, matches, latest_regex)) {
                int device_id = std::stoi(matches[1].str());
                if (device_id < 0 || device_id > 255) {
                    send_json(Status::BAD_REQUEST,
                             create_error_json("Invalid device ID"));
                    return;
                }
                
                auto sample = DataStore::instance().get_latest(device_id);
                if (!sample) {
                    send_json(Status::NOT_FOUND,
                             create_error_json("No data for device"));
                    return;
                }
                
                std::ostringstream json;
                json << std::fixed << std::setprecision(6);
                json << "{";
                json << "\"device_id\": " << device_id << ", ";
                json << "\"value\": " << sample->value << ", ";
                json << "\"timestamp\": " << sample->timestamp;
                json << "}";
                
                send_json(Status::OK, json.str());
                return;
            }
            
            if (std::regex_match(path, matches, stats_regex)) {
                int device_id = std::stoi(matches[1].str());
                if (device_id < 0 || device_id > 255) {
                    send_json(Status::BAD_REQUEST,
                             create_error_json("Invalid device ID"));
                    return;
                }
                
                auto stats = DataStore::instance().get_stats(device_id);
                if (!stats) {
                    send_json(Status::NOT_FOUND,
                             create_error_json("No data for device"));
                    return;
                }
                
                std::ostringstream json;
                json << std::fixed << std::setprecision(6);
                json << "{";
                json << "\"device_id\": " << device_id << ", ";
                json << "\"min\": " << stats->min << ", ";
                json << "\"max\": " << stats->max << ", ";
                json << "\"average\": " << stats->average << ", ";
                json << "\"count\": " << stats->count << ", ";
                json << "\"oldest_timestamp\": " << stats->oldest_timestamp << ", ";
                json << "\"newest_timestamp\": " << stats->newest_timestamp;
                json << "}";
                
                send_json(Status::OK, json.str());
                return;
            }
            
            if (std::regex_match(path, matches, devices_regex)) {
                auto devices = DataStore::instance().get_active_devices();
                
                std::ostringstream json;
                json << "[";
                for (size_t i = 0; i < devices.size(); ++i) {
                    if (i > 0) json << ", ";
                    json << static_cast<int>(devices[i]);
                }
                json << "]";
                
                send_json(Status::OK, json.str());
                return;
            }
            
            if (std::regex_match(path, matches, health_regex)) {
                std::ostringstream json;
                json << "{";
                json << "\"status\": \"healthy\", ";
                json << "\"timestamp\": " << utils::current_time_millis();
                json << "}";
                
                send_json(Status::OK, json.str());
                return;
            }
            
            if (std::regex_match(path, matches, metrics_regex)) {
                auto global_stats = DataStore::instance().get_global_stats();
                
                std::ostringstream json;
                json << "{";
                json << "\"total_samples\": " << global_stats.total_samples << ", ";
                json << "\"total_updates\": " << global_stats.total_updates << ", ";
                json << "\"active_devices\": " << global_stats.active_devices << ", ";
                json << "\"total_devices\": " << global_stats.total_devices << ", ";
                json << "\"timestamp\": " << utils::current_time_millis();
                json << "}";
                
                send_json(Status::OK, json.str());
                return;
            }
          
            send_json(Status::NOT_FOUND,
                     create_error_json("Endpoint not found"));
        }
    };
    
public:
    HttpServer(int port_num) : port(port_num) {}
    
    ~HttpServer() {
        stop();
    }
    
    void start() {
        if (running) return;
        running = true;
        server_thread = std::thread(&HttpServer::run, this);
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
        
        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        
        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            close(server_fd);
            return;
        }
        
        if (listen(server_fd, 100) < 0) {
            perror("listen");
            close(server_fd);
            return;
        }
        
        std::cout << "HTTP server listening on port " << port << std::endl;
        
        while (running) {
            int client_fd = accept(server_fd, nullptr, nullptr);
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
                RequestHandler handler(client_fd);
                handler.run();
            }).detach();
        }
        
        if (server_fd >= 0) {
            close(server_fd);
            server_fd = -1;
        }
    }
};