#include "binary_message.hpp"
#include <iostream>
#include <thread>
#include <csignal>
#include <chrono>
#include <sys/socket.h>  
#include <unistd.h>      

void signal_handler(int signal) {
    std::cout << "\nПолучен сигнал " << signal << ", завершение работы..." << std::endl;
    running = false;
    
    
    if (binary_listen_socket >= 0) {
        shutdown(binary_listen_socket, SHUT_RDWR);
        close(binary_listen_socket);
        binary_listen_socket = -1;
    }
    
    if (http_listen_socket >= 0) {
        shutdown(http_listen_socket, SHUT_RDWR);
        close(http_listen_socket);
        http_listen_socket = -1;
    }
}

int main() {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    
    try {
        std::cout << "==========================================" << std::endl;
        std::cout << "Сервис телеметрии запускается" << std::endl;
        std::cout << "Бинарный порт: " << BINARY_PORT << std::endl;
        std::cout << "HTTP порт: " << HTTP_PORT << std::endl;
        std::cout << "==========================================" << std::endl;
        std::cout << std::endl;
        
        std::thread binary_thread(BynaryServer);
        std::thread http_thread(HTTP_server);
        
        std::cout << "Серверы запущены. Используйте Ctrl+C для остановки." << std::endl;
        std::cout << std::endl;
        std::cout << "Доступные HTTP эндпоинты:" << std::endl;
        std::cout << "  GET /device/{id}/latest  - последнее значение устройства" << std::endl;
        std::cout << "  GET /device/{id}/stats   - статистика по устройству" << std::endl;
        std::cout << std::endl;

        binary_thread.join();
        http_thread.join();
        
        std::cout << "Сервис телеметрии завершил работу." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}